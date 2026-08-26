#include "ScaleQuantizeBlock.h"

namespace MidiFlux
{

ScaleQuantizeBlock::ScaleQuantizeBlock()
{
    reset();
}

void ScaleQuantizeBlock::reset()
{
    noteMap.clear();
}

void ScaleQuantizeBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    for (auto const& [inKey, outKey] : noteMap)
    {
        int ch = (outKey >> 8) & 0xff;
        int note = outKey & 0x7f;
        outBuffer.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
    }
    noteMap.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int ScaleQuantizeBlock::getNumParameters() const
{
    return 6;
}

const ParameterDefinition& ScaleQuantizeBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 6> defs = {{
        { "snapAmount", "Snap Amount", 0.0f, 1.0f, 1.0f, 0.01f, "%", {} }, // 0% = Off, 100% = Full
        { "syncGlobal", "Sync Global", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Custom", "Global Scale" } },
        { "rootKey",    "Key",         0.0f, 11.0f, 0.0f, 1.0f, "", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" } },
        { "scaleType",  "Scale",       0.0f, (float)(NumScales - 1), 0.0f, 1.0f, "", {} },
        { "snapDir",    "Direction",   0.0f, 2.0f, 0.0f, 1.0f, "", { "Closest", "Up", "Down" } },
        { "shiftDegree","Degree Shift",-7.0f, 7.0f, 0.0f, 1.0f, "deg", {} }
    }};
    return defs[juce::jlimit(0, 5, index)];
}

float ScaleQuantizeBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return snapStrength;
        case 1: return useGlobalScale;
        case 2: return rootKey;
        case 3: return scaleType;
        case 4: return snapDirection;
        case 5: return degreeShift;
        default: return 0.0f;
    }
}

void ScaleQuantizeBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: snapStrength = juce::jlimit(0.0f, 1.0f, value); break;
        case 1: useGlobalScale = juce::jlimit(0.0f, 1.0f, value); break;
        case 2: rootKey = juce::jlimit(0.0f, 11.0f, value); break;
        case 3: scaleType = juce::jlimit(0.0f, (float)(NumScales - 1), value); break;
        case 4: snapDirection = juce::jlimit(0.0f, 2.0f, value); break;
        case 5: degreeShift = juce::jlimit(-7.0f, 7.0f, value); break;
    }
}

void ScaleQuantizeBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                      juce::MidiBuffer& outputMidi,
                                      const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    int activeKey = (useGlobalScale > 0.5f) ? ctx.rootKey : static_cast<int>(std::round(rootKey));
    int activeScale = (useGlobalScale > 0.5f) ? ctx.scaleType : static_cast<int>(std::round(scaleType));
    bool snapUp = (static_cast<int>(std::round(snapDirection)) == 1);
    int dShift = static_cast<int>(std::round(degreeShift));

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

            // 0.0 = No quantize (raw input), 1.0 = Full quantize
            if (snapStrength > 0.001f)
            {
                if (snapStrength >= 0.999f || rng.nextFloat() <= snapStrength)
                {
                    finalNote = ScaleTheory::quantizeToScale(note, activeKey, activeScale, snapUp);
                }
            }

            // Diatonic degree shift within scale
            if (dShift != 0)
            {
                finalNote = ScaleTheory::transposeDiatonic(finalNote, dShift, activeKey, activeScale);
            }

            finalNote = juce::jlimit(0, 127, finalNote);
            int outKey = (ch << 8) | (finalNote & 0x7f);
            noteMap[key] = outKey;

            outputMidi.addEvent(juce::MidiMessage::noteOn(ch, finalNote, msg.getVelocity()), samplePos);
            triggerActivity();
        }
        else if (msg.isNoteOff())
        {
            auto it = noteMap.find(key);
            if (it != noteMap.end())
            {
                int outKey = it->second;
                noteMap.erase(it);
                int outNote = outKey & 0x7f;
                outputMidi.addEvent(juce::MidiMessage::noteOff(ch, outNote, msg.getVelocity()), samplePos);
            }
            else
            {
                outputMidi.addEvent(msg, samplePos);
            }
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

} // namespace MidiFlux
