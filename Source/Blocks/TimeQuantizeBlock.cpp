#include "TimeQuantizeBlock.h"
#include <cmath>
#include <algorithm>

namespace MidiFlux
{

TimeQuantizeBlock::TimeQuantizeBlock()
{
    reset();
}

void TimeQuantizeBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
}

void TimeQuantizeBlock::reset()
{
    delayedQueue.clear();
    noteDelayMap.clear();
    internalBeatClock = 0.0;
}

void TimeQuantizeBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    delayedQueue.clear();
    noteDelayMap.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int TimeQuantizeBlock::getNumParameters() const
{
    return 4;
}

const ParameterDefinition& TimeQuantizeBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 4> defs = {{
        { "grid",     "Grid",      0.0f, 12.0f, 6.0f, 1.0f, "", {
            "1/4", "1/4T", "1/4D",
            "1/8", "1/8T", "1/8D",
            "1/16", "1/16T", "1/16D",
            "1/32", "1/32T", "1/32D",
            "1/64" } },
        { "strength", "Strength",  0.0f, 1.0f, 1.0f, 0.01f, "%", {} }, // 0% = Off, 100% = Full Snap
        { "swing",    "Swing",     0.0f, 0.75f, 0.0f, 0.01f, "%", {} },
        { "window",   "Tolerance", 0.0f, 3.0f, 0.0f, 1.0f, "", { "Full Grid", "Tight (25ms)", "Med (50ms)", "Loose (100ms)" } }
    }};
    return defs[juce::jlimit(0, 3, index)];
}

float TimeQuantizeBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return gridDivision;
        case 1: return snapStrength;
        case 2: return swing;
        case 3: return windowTolerance;
        default: return 0.0f;
    }
}

void TimeQuantizeBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: gridDivision = juce::jlimit(0.0f, 12.0f, value); break;
        case 1: snapStrength = juce::jlimit(0.0f, 1.0f, value); break;
        case 2: swing = juce::jlimit(0.0f, 0.75f, value); break;
        case 3: windowTolerance = juce::jlimit(0.0f, 3.0f, value); break;
    }
}

double TimeQuantizeBlock::getGridDivisionInBeats(int index) const
{
    switch (index)
    {
        case 0:  return 1.0;                 // 1/4
        case 1:  return 1.0 * (2.0 / 3.0);   // 1/4T
        case 2:  return 1.5;                 // 1/4D
        case 3:  return 0.5;                 // 1/8
        case 4:  return 0.5 * (2.0 / 3.0);   // 1/8T
        case 5:  return 0.75;                // 1/8D
        case 6:  return 0.25;                // 1/16
        case 7:  return 0.25 * (2.0 / 3.0);  // 1/16T
        case 8:  return 0.375;               // 1/16D
        case 9:  return 0.125;               // 1/32
        case 10: return 0.125 * (2.0 / 3.0); // 1/32T
        case 11: return 0.1875;              // 1/32D
        case 12: return 0.0625;              // 1/64
        default: return 0.25;
    }
}

void TimeQuantizeBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                     juce::MidiBuffer& outputMidi,
                                     const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    // 1. Drain delayed quantize events scheduled in previous buffers
    for (auto it = delayedQueue.begin(); it != delayedQueue.end(); )
    {
        it->samplesRemaining -= ctx.numSamples;
        if (it->samplesRemaining <= 0)
        {
            int trigPos = juce::jlimit(0, ctx.numSamples - 1, it->samplesRemaining + ctx.numSamples);
            outputMidi.addEvent(it->message, trigPos);
            if (it->message.isNoteOn())
                triggerActivity();
            it = delayedQueue.erase(it);
        }
        else
        {
            ++it;
        }
    }

    double bpm = (ctx.bpm > 0.0) ? ctx.bpm : 120.0;
    double beatsPerSample = (bpm / 60.0) / currentSampleRate;
    double currentBlockBeat = (ctx.ppqPosition > 0.0) ? ctx.ppqPosition : internalBeatClock;
    internalBeatClock += ctx.numSamples * beatsPerSample;

    double gridDiv = getGridDivisionInBeats(static_cast<int>(std::round(gridDivision)));
    int winMode = static_cast<int>(std::round(windowTolerance));
    double windowMsLimit = 9999.0;
    if (winMode == 1) windowMsLimit = 25.0;
    else if (winMode == 2) windowMsLimit = 50.0;
    else if (winMode == 3) windowMsLimit = 100.0;

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();
        int key = (ch << 8) | (note & 0x7f);

        if (msg.isNoteOn())
        {
            int delaySamples = 0;

            if (snapStrength > 0.001f)
            {
                double noteBeat = currentBlockBeat + samplePos * beatsPerSample;

                // Apply swing offset to alternate subdivisions
                double swingOffset = 0.0;
                int stepIndex = static_cast<int>(std::floor(noteBeat / gridDiv));
                if ((stepIndex % 2) == 1 && swing > 0.001f)
                {
                    swingOffset = gridDiv * swing * 0.5;
                }

                double effectiveBeat = noteBeat - swingOffset;
                double nextGrid = std::ceil(effectiveBeat / gridDiv) * gridDiv + swingOffset;
                double prevGrid = std::floor(effectiveBeat / gridDiv) * gridDiv + swingOffset;

                double beatsToNext = nextGrid - noteBeat;
                double beatsFromPrev = noteBeat - prevGrid;

                double distNextMs = beatsToNext * (60.0 / bpm) * 1000.0;
                double distPrevMs = beatsFromPrev * (60.0 / bpm) * 1000.0;

                if (distNextMs <= distPrevMs) // Rushing (closer to next grid)
                {
                    if (distNextMs <= windowMsLimit)
                    {
                        double samplesToNext = beatsToNext * (60.0 / bpm) * currentSampleRate;
                        delaySamples = static_cast<int>(samplesToNext * snapStrength);
                    }
                }
                else // Dragging (played just after beat)
                {
                    if (snapStrength >= 0.95f && distPrevMs <= windowMsLimit && beatsFromPrev > 0.02 * gridDiv)
                    {
                        double samplesToNext = beatsToNext * (60.0 / bpm) * currentSampleRate;
                        delaySamples = static_cast<int>(samplesToNext * snapStrength);
                    }
                }
            }

            noteDelayMap[key] = delaySamples;
            int targetSample = samplePos + delaySamples;

            if (targetSample < ctx.numSamples)
            {
                outputMidi.addEvent(msg, targetSample);
                triggerActivity();
            }
            else
            {
                delayedQueue.push_back({ msg, targetSample - ctx.numSamples });
            }
        }
        else if (msg.isNoteOff())
        {
            int delaySamples = 0;
            auto it = noteDelayMap.find(key);
            if (it != noteDelayMap.end())
            {
                delaySamples = it->second;
                noteDelayMap.erase(it);
            }

            int targetSample = samplePos + delaySamples;
            if (targetSample < ctx.numSamples)
            {
                outputMidi.addEvent(msg, targetSample);
            }
            else
            {
                delayedQueue.push_back({ msg, targetSample - ctx.numSamples });
            }
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

juce::String TimeQuantizeBlock::getStatusDescription() const
{
    static const char* gridNames[] = {
        "1/4", "1/4T", "1/4D",
        "1/8", "1/8T", "1/8D",
        "1/16", "1/16T", "1/16D",
        "1/32", "1/32T", "1/32D",
        "1/64"
    };
    int g = juce::jlimit(0, 12, (int)std::round(gridDivision));
    juce::String desc = "GRID: " + juce::String(gridNames[g]);
    if (swing > 0.01f)
        desc += " (SWING " + juce::String((int)(swing * 100)) + "%)";
    return desc;
}

} // namespace MidiFlux
