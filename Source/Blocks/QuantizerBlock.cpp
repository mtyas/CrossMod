#include "QuantizerBlock.h"

namespace MidiFlux
{

QuantizerBlock::QuantizerBlock()
{
    reset();
}

void QuantizerBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
}

void QuantizerBlock::reset()
{
    noteMap.clear();
    delayedQueue.clear();
    noteDelayMap.clear();
    internalBeatClock = 0.0;
}

void QuantizerBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    delayedQueue.clear();
    noteDelayMap.clear();
    for (auto const& [inKey, outKey] : noteMap)
    {
        int ch = (outKey >> 8) & 0xff;
        int note = outKey & 0x7f;
        outBuffer.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
    }
    noteMap.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int QuantizerBlock::getNumParameters() const
{
    return 7;
}

const ParameterDefinition& QuantizerBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 7> defs = {{
        { "scaleSnap",   "Scale Snap", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } },
        { "rootKey",     "Key",        0.0f, 11.0f, 0.0f, 1.0f, "", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" } },
        { "scaleType",   "Scale",      0.0f, (float)(NumScales - 1), 0.0f, 1.0f, "", {} },
        { "snapDir",     "Snap Dir",   0.0f, 2.0f, 0.0f, 1.0f, "", { "Closest", "Up", "Down" } },
        { "timeSnap",    "Time Snap",  0.0f, 1.0f, 0.0f, 1.0f, "", { "Off", "On" } },
        { "timeGrid",    "Grid",       0.0f, 3.0f, 2.0f, 1.0f, "", { "1/4", "1/8", "1/16", "1/32" } },
        { "snapStrength","Strength",   0.0f, 1.0f, 1.0f, 0.01f, "%", {} }
    }};
    return defs[juce::jlimit(0, 6, index)];
}

float QuantizerBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return scaleSnapActive;
        case 1: return rootKey;
        case 2: return scaleType;
        case 3: return snapDirection;
        case 4: return timeSnapActive;
        case 5: return timeGrid;
        case 6: return snapStrength;
        default: return 0.0f;
    }
}

void QuantizerBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: scaleSnapActive = juce::jlimit(0.0f, 1.0f, value); break;
        case 1: rootKey = juce::jlimit(0.0f, 11.0f, value); break;
        case 2: scaleType = juce::jlimit(0.0f, (float)(NumScales - 1), value); break;
        case 3: snapDirection = juce::jlimit(0.0f, 2.0f, value); break;
        case 4: timeSnapActive = juce::jlimit(0.0f, 1.0f, value); break;
        case 5: timeGrid = juce::jlimit(0.0f, 3.0f, value); break;
        case 6: snapStrength = juce::jlimit(0.0f, 1.0f, value); break;
    }
}

void QuantizerBlock::processBlock(const juce::MidiBuffer& inputMidi,
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

    int rKey = static_cast<int>(std::round(rootKey));
    int sType = static_cast<int>(std::round(scaleType));
    bool snapUpward = (static_cast<int>(std::round(snapDirection)) == 1);

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();
        int key = (ch << 8) | (note & 0x7f);

        if (msg.isNoteOn())
        {
            int finalNote = note;

            // Pitch quantization
            if (scaleSnapActive > 0.5f)
            {
                int quantized = ScaleTheory::quantizeToScale(note, rKey, sType, snapUpward);
                if (rng.nextBool(snapStrength))
                    finalNote = quantized;
            }

            int outKey = (ch << 8) | (finalNote & 0x7f);
            noteMap[key] = outKey;

            // Time quantization
            int delaySamples = 0;
            if (timeSnapActive > 0.5f)
            {
                double gridDiv = 0.25; // 1/16 default
                int g = static_cast<int>(std::round(timeGrid));
                if (g == 0) gridDiv = 1.0;
                else if (g == 1) gridDiv = 0.5;
                else if (g == 2) gridDiv = 0.25;
                else if (g == 3) gridDiv = 0.125;

                double noteBeat = currentBlockBeat + samplePos * beatsPerSample;
                double nextGrid = std::ceil(noteBeat / gridDiv) * gridDiv;
                double prevGrid = std::floor(noteBeat / gridDiv) * gridDiv;

                double beatsToNext = nextGrid - noteBeat;
                double beatsFromPrev = noteBeat - prevGrid;

                if (beatsToNext < beatsFromPrev) // Rushing (closer to next beat)
                {
                    double samplesToNext = beatsToNext * (60.0 / bpm) * currentSampleRate;
                    delaySamples = static_cast<int>(samplesToNext * snapStrength);
                }
                else // Dragging (just passed beat)
                {
                    if (snapStrength >= 0.9f && beatsFromPrev > 0.05 * gridDiv)
                    {
                        double samplesToNext = beatsToNext * (60.0 / bpm) * currentSampleRate;
                        delaySamples = static_cast<int>(samplesToNext * snapStrength);
                    }
                }
            }

            noteDelayMap[key] = delaySamples;
            auto outMsg = juce::MidiMessage::noteOn(ch, finalNote, msg.getVelocity());
            int targetSample = samplePos + delaySamples;

            if (targetSample < ctx.numSamples)
            {
                outputMidi.addEvent(outMsg, targetSample);
                triggerActivity();
            }
            else
            {
                delayedQueue.push_back({ outMsg, targetSample - ctx.numSamples });
            }
        }
        else if (msg.isNoteOff())
        {
            int outNote = note;
            auto itMap = noteMap.find(key);
            if (itMap != noteMap.end())
            {
                outNote = itMap->second & 0x7f;
                noteMap.erase(itMap);
            }

            int delaySamples = 0;
            auto itDelay = noteDelayMap.find(key);
            if (itDelay != noteDelayMap.end())
            {
                delaySamples = itDelay->second;
                noteDelayMap.erase(itDelay);
            }

            auto offMsg = juce::MidiMessage::noteOff(ch, outNote, msg.getVelocity());
            int targetSample = samplePos + delaySamples;

            if (targetSample < ctx.numSamples)
            {
                outputMidi.addEvent(offMsg, targetSample);
            }
            else
            {
                delayedQueue.push_back({ offMsg, targetSample - ctx.numSamples });
            }
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

} // namespace MidiFlux
