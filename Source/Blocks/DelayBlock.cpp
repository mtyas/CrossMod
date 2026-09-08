#include "DelayBlock.h"

namespace MidiFlux
{

DelayBlock::DelayBlock()
{
    reset();
}

void DelayBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
    activeEchoes.reserve(256);
}

void DelayBlock::reset()
{
    activeEchoes.clear();
}

void DelayBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    for (const auto& ee : activeEchoes)
    {
        if (ee.isNoteOn)
            outBuffer.addEvent(juce::MidiMessage::noteOff(ee.channel, ee.noteNumber), 0);
    }
    activeEchoes.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int DelayBlock::getNumParameters() const
{
    return 5;
}

const ParameterDefinition& DelayBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 5> defs = {{
        { "delayTime",  "Time",       0.0f, 6.0f, 2.0f, 1.0f, "", { "1/4", "1/8", "1/16", "1/8D", "1/16D", "1/8T", "1/16T" } },
        { "repeats",    "Repeats",    1.0f, 8.0f, 3.0f, 1.0f, "", {} },
        { "decay",      "Decay",      0.1f, 2.0f, 0.65f, 0.01f, "%", {} },
        { "pitchShift", "Shift / Tap",-12.0f, 12.0f, 0.0f, 1.0f, "st", {} },
        { "snapScale",  "Snap Scale", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } }
    }};
    return defs[juce::jlimit(0, 4, index)];
}

float DelayBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return delayTime;
        case 1: return repeatCount;
        case 2: return decayRate;
        case 3: return pitchShiftPerTap;
        case 4: return scaleSnap;
        default: return 0.0f;
    }
}

void DelayBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: delayTime = juce::jlimit(0.0f, 6.0f, value); break;
        case 1: repeatCount = juce::jlimit(1.0f, 8.0f, value); break;
        case 2: decayRate = juce::jlimit(0.1f, 2.0f, value); break;
        case 3: pitchShiftPerTap = juce::jlimit(-12.0f, 12.0f, value); break;
        case 4: scaleSnap = juce::jlimit(0.0f, 1.0f, value); break;
    }
}

void DelayBlock::processBlock(const juce::MidiBuffer& inputMidi,
                              juce::MidiBuffer& outputMidi,
                              const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    // Process queued echoes
    for (auto it = activeEchoes.begin(); it != activeEchoes.end(); )
    {
        it->sampleDelay -= ctx.numSamples;
        if (it->sampleDelay <= 0)
        {
            int triggerPos = std::max(0, it->sampleDelay + ctx.numSamples);
            triggerPos = std::min(ctx.numSamples - 1, triggerPos);

            if (it->isNoteOn)
            {
                outputMidi.addEvent(juce::MidiMessage::noteOn(it->channel, it->noteNumber, it->velocity), triggerPos);
                triggerActivity();
            }
            else
            {
                outputMidi.addEvent(juce::MidiMessage::noteOff(it->channel, it->noteNumber), triggerPos);
            }
            it = activeEchoes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    double bpm = (ctx.bpm > 0.0) ? ctx.bpm : 120.0;
    double samplesPerBeat = (60.0 / bpm) * currentSampleRate;
    double timeDiv = 0.25; // 1/16
    int dIdx = static_cast<int>(std::round(delayTime));
    switch (dIdx)
    {
        case 0: timeDiv = 1.0; break;             // 1/4
        case 1: timeDiv = 0.5; break;             // 1/8
        case 2: timeDiv = 0.25; break;            // 1/16
        case 3: timeDiv = 0.75; break;            // 1/8D
        case 4: timeDiv = 0.375; break;           // 1/16D
        case 5: timeDiv = 0.5 * (2.0/3.0); break; // 1/8T
        case 6: timeDiv = 0.25 * (2.0/3.0); break;// 1/16T
    }

    int delaySamples = static_cast<int>(timeDiv * samplesPerBeat);
    if (delaySamples < 50)
        delaySamples = 50;

    int repeats = static_cast<int>(std::round(repeatCount));
    int shift = static_cast<int>(std::round(pitchShiftPerTap));
    bool doSnap = scaleSnap > 0.5f;

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();

        if (msg.isNoteOn())
        {
            outputMidi.addEvent(msg, samplePos);
            triggerActivity();

            int noteDur = delaySamples / 2;
            float currentVel = (float)msg.getVelocity();
            int currentPitch = note;

            for (int r = 1; r <= repeats; ++r)
            {
                currentVel *= decayRate;
                if (decayRate <= 1.0f && currentVel < 5.0f)
                    break;
                if (currentVel > 127.0f)
                    currentVel = 127.0f;

                currentPitch += shift;
                int echoPitch = currentPitch;
                if (doSnap)
                    echoPitch = ScaleTheory::quantizeToScale(echoPitch, ctx.rootKey, ctx.scaleType);
                echoPitch = juce::jlimit(0, 127, echoPitch);

                int onDelay = samplePos + r * delaySamples;
                int offDelay = onDelay + noteDur;

                uint8_t outVel = static_cast<uint8_t>(juce::jlimit(1, 127, (int)std::round(currentVel)));
                activeEchoes.push_back({ ch, echoPitch, outVel, onDelay, true });
                activeEchoes.push_back({ ch, echoPitch, 0, offDelay, false });
            }
        }
        else if (msg.isNoteOff())
        {
            outputMidi.addEvent(msg, samplePos);
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

juce::String DelayBlock::getStatusDescription() const
{
    static const char* timeNames[] = { "1/4", "1/8", "1/16", "1/8D", "1/16D", "1/8T", "1/16T" };
    int tIdx = juce::jlimit(0, 6, static_cast<int>(std::round(delayTime)));
    return "Taps: " + juce::String(static_cast<int>(repeatCount)) + " @ " + juce::String(timeNames[tIdx]) + " (" + juce::String(static_cast<int>(decayRate * 100.0f)) + "%)";
}

} // namespace MidiFlux
