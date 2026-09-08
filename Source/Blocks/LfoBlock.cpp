#include "LfoBlock.h"
#include <cmath>

namespace MidiFlux
{

LfoBlock::LfoBlock()
{
    reset();
}

void LfoBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
}

void LfoBlock::reset()
{
    lfoPhase = 0.0;
    currentRandomStep = 0.5f;
    smoothRandomWalk = 0.5f;
    lastSentVal = -1;
    samplesSinceLastOutput = 0;
}

void LfoBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    MidiBlock::allNotesOff(outBuffer);
}

int LfoBlock::getNumParameters() const
{
    return 8;
}

const ParameterDefinition& LfoBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 8> defs = {{
        { "target",    "Target",    0.0f, 6.0f, 0.0f, 1.0f, "", { "CC 1 (Mod)", "CC 11 (Exp)", "CC 74 (Cutoff)", "CC 71 (Reso)", "CC 10 (Pan)", "CC 7 (Vol)", "Pitch Bend" } },
        { "syncMode",  "Sync Mode", 0.0f, 1.0f, 0.0f, 1.0f, "", { "Tempo Synced", "Free Rate (Hz)" } },
        { "syncRate",  "Tempo Rate",0.0f, 10.0f, 4.0f, 1.0f, "", { "4 Bars", "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T", "1/8D", "1/16D" } },
        { "freeRate",  "Free Rate", 0.05f, 20.0f, 1.0f, 0.05f, "Hz", {} },
        { "waveform",  "Shape",     0.0f, 6.0f, 0.0f, 1.0f, "", { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random Steps", "Smooth Walk" } },
        { "depth",     "Depth",     0.0f, 1.0f, 0.65f, 0.01f, "%", {} },
        { "offset",    "Center",    0.0f, 127.0f, 64.0f, 1.0f, "", {} },
        { "retrigger", "Retrigger", 0.0f, 1.0f, 0.0f, 1.0f, "", { "Free Run", "On Note" } }
    }};
    return defs[juce::jlimit(0, 7, index)];
}

float LfoBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return targetCC;
        case 1: return syncMode;
        case 2: return syncRate;
        case 3: return freeRateHz;
        case 4: return waveform;
        case 5: return depth;
        case 6: return centerOffset;
        case 7: return retriggerMode;
        default: return 0.0f;
    }
}

void LfoBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: targetCC = juce::jlimit(0.0f, 6.0f, value); break;
        case 1: syncMode = juce::jlimit(0.0f, 1.0f, value); break;
        case 2: syncRate = juce::jlimit(0.0f, 10.0f, value); break;
        case 3: freeRateHz = juce::jlimit(0.05f, 20.0f, value); break;
        case 4: waveform = juce::jlimit(0.0f, 6.0f, value); break;
        case 5: depth = juce::jlimit(0.0f, 1.0f, value); break;
        case 6: centerOffset = juce::jlimit(0.0f, 127.0f, value); break;
        case 7: retriggerMode = juce::jlimit(0.0f, 1.0f, value); break;
    }
}

double LfoBlock::getSyncDivisionInBeats(int index) const
{
    switch (index)
    {
        case 0:  return 16.0;               // 4 Bars
        case 1:  return 8.0;                // 2 Bars
        case 2:  return 4.0;                // 1 Bar
        case 3:  return 2.0;                // 1/2
        case 4:  return 1.0;                // 1/4
        case 5:  return 0.5;                // 1/8
        case 6:  return 0.25;               // 1/16
        case 7:  return 0.5 * (2.0 / 3.0);  // 1/8T
        case 8:  return 0.25 * (2.0 / 3.0); // 1/16T
        case 9:  return 0.75;               // 1/8D
        case 10: return 0.375;              // 1/16D
        default: return 1.0;
    }
}

float LfoBlock::computeLfoValue(double phase, int wave)
{
    double p = phase - std::floor(phase);
    switch (wave)
    {
        case 0: // Sine
            return static_cast<float>(0.5 + 0.5 * std::sin(p * 2.0 * juce::MathConstants<double>::pi));
        case 1: // Triangle
            return static_cast<float>(1.0 - std::abs(2.0 * p - 1.0));
        case 2: // Saw Up
            return static_cast<float>(p);
        case 3: // Saw Down
            return static_cast<float>(1.0 - p);
        case 4: // Square
            return (p < 0.5) ? 1.0f : 0.0f;
        case 5: // Random Steps
            return currentRandomStep;
        case 6: // Smooth Random Walk
            return smoothRandomWalk;
        default:
            return 0.5f;
    }
}

void LfoBlock::processBlock(const juce::MidiBuffer& inputMidi,
                            juce::MidiBuffer& outputMidi,
                            const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    int defaultChannel = 1;
    bool hasIncomingNotes = false;

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        outputMidi.addEvent(msg, metadata.samplePosition);
        if (msg.isNoteOn())
        {
            hasIncomingNotes = true;
            defaultChannel = msg.getChannel();
            if (retriggerMode > 0.5f)
            {
                lfoPhase = 0.0;
            }
        }
    }

    // Determine target CC or Pitch Bend
    int targetMode = static_cast<int>(std::round(targetCC));
    int ccNum = 1;
    bool isPitchBend = (targetMode == 6);
    if (targetMode == 0) ccNum = 1;       // Mod Wheel
    else if (targetMode == 1) ccNum = 11; // Expression
    else if (targetMode == 2) ccNum = 74; // Filter Cutoff
    else if (targetMode == 3) ccNum = 71; // Resonance
    else if (targetMode == 4) ccNum = 10; // Pan
    else if (targetMode == 5) ccNum = 7;  // Volume

    // Calculate phase increment
    double phaseInc = 0.0;
    if (syncMode > 0.5f) // Free Rate (Hz)
    {
        phaseInc = freeRateHz / currentSampleRate;
    }
    else // Tempo Synced
    {
        double bpm = (ctx.bpm > 0.0) ? ctx.bpm : 120.0;
        double beats = getSyncDivisionInBeats(static_cast<int>(std::round(syncRate)));
        phaseInc = (bpm / 60.0) / (beats * currentSampleRate);
    }

    int wave = static_cast<int>(std::round(waveform));

    // Send CC / Pitch Bend updates across block
    int outputInterval = 64; // Every ~64 samples (1.4ms at 44.1k)
    for (int s = 0; s < ctx.numSamples; ++s)
    {
        double oldPhase = lfoPhase;
        lfoPhase += phaseInc;

        if (std::floor(lfoPhase) > std::floor(oldPhase))
        {
            currentRandomStep = rng.nextFloat();
        }

        if (wave == 6)
        {
            smoothRandomWalk = rng.nextBrownian(smoothRandomWalk, 0.02f, 0.95f, 0.5f);
        }

        samplesSinceLastOutput++;
        if (samplesSinceLastOutput >= outputInterval || (s == 0 && lastSentVal == -1))
        {
            samplesSinceLastOutput = 0;
            float rawLfo = computeLfoValue(lfoPhase, wave); // 0.0 to 1.0
            lastRawLfo = rawLfo;

            if (isPitchBend)
            {
                // Pitch Bend range 0 to 16383, center 8192
                int pbCenter = static_cast<int>((centerOffset / 127.0f) * 16383.0f);
                int pbRange = static_cast<int>(8191.0f * depth);
                int pbVal = juce::jlimit(0, 16383, pbCenter + static_cast<int>((rawLfo * 2.0f - 1.0f) * pbRange));

                if (pbVal != lastSentVal)
                {
                    lastSentVal = pbVal;
                    outputMidi.addEvent(juce::MidiMessage::pitchWheel(defaultChannel, pbVal), s);
                    triggerActivity();
                }
            }
            else
            {
                // CC range 0 to 127
                float c = centerOffset;
                float halfDepth = 63.5f * depth;
                int ccVal = juce::jlimit(0, 127, static_cast<int>(std::round(c + (rawLfo * 2.0f - 1.0f) * halfDepth)));

                if (ccVal != lastSentVal)
                {
                    lastSentVal = ccVal;
                    outputMidi.addEvent(juce::MidiMessage::controllerEvent(defaultChannel, ccNum, ccVal), s);
                    triggerActivity();
                }
            }
        }
    }
}

juce::String LfoBlock::getStatusDescription() const
{
    static const char* targetNames[] = { "CC 1 Mod", "CC 11 Exp", "CC 74 Cutoff", "CC 71 Reso", "CC 10 Pan", "CC 7 Vol", "Pitch Bend" };
    static const char* rateNames[] = { "4 Bars", "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T", "1/8D", "1/16D" };
    int t = juce::jlimit(0, 6, (int)std::round(targetCC));
    if (syncMode > 0.5f)
        return juce::String(targetNames[t]) + " • " + juce::String(freeRateHz, 2) + " Hz";
    int r = juce::jlimit(0, 10, (int)std::round(syncRate));
    return juce::String(targetNames[t]) + " • " + juce::String(rateNames[r]);
}

} // namespace MidiFlux
