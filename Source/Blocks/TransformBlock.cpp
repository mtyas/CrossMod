#include "TransformBlock.h"

namespace MidiFlux
{

TransformBlock::TransformBlock()
{
    reset();
}

void TransformBlock::reset()
{
    noteMap.clear();
}

void TransformBlock::allNotesOff(juce::MidiBuffer& outBuffer)
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

int TransformBlock::getNumParameters() const
{
    return 6;
}

const ParameterDefinition& TransformBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 6> defs = {{
        { "invertPitch", "Invert Pitch", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } },
        { "pivotNote",   "Pivot Note",  24.0f, 96.0f, 60.0f, 1.0f, "", {} }, // Default 60 = C4
        { "snapScale",   "Scale Snap",  0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } },
        { "invertVel",   "Invert Vel",  0.0f, 1.0f, 0.0f, 1.0f, "", { "Off", "On" } },
        { "octaveWrap",  "Octave Wrap", 0.0f, 3.0f, 0.0f, 1.0f, "", { "Off", "1 Octave", "2 Octaves", "3 Octaves" } },
        { "gateScale",   "Gate Scale",  0.25f, 2.0f, 1.0f, 0.05f, "%", {} }
    }};
    return defs[juce::jlimit(0, 5, index)];
}

float TransformBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return pitchInvert;
        case 1: return pivotNote;
        case 2: return snapToScale;
        case 3: return invertVelocity;
        case 4: return octaveWrap;
        case 5: return gateScale;
        default: return 0.0f;
    }
}

void TransformBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: pitchInvert = juce::jlimit(0.0f, 1.0f, value); break;
        case 1: pivotNote = juce::jlimit(24.0f, 96.0f, value); break;
        case 2: snapToScale = juce::jlimit(0.0f, 1.0f, value); break;
        case 3: invertVelocity = juce::jlimit(0.0f, 1.0f, value); break;
        case 4: octaveWrap = juce::jlimit(0.0f, 3.0f, value); break;
        case 5: gateScale = juce::jlimit(0.25f, 2.0f, value); break;
    }
}

void TransformBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                  juce::MidiBuffer& outputMidi,
                                  const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    int pNote = static_cast<int>(std::round(pivotNote));
    int wrapMode = static_cast<int>(std::round(octaveWrap));
    int wrapSpan = (wrapMode == 1) ? 12 : (wrapMode == 2 ? 24 : (wrapMode == 3 ? 36 : 127));

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

            // Pitch Inversion around pivot
            if (pitchInvert > 0.5f)
            {
                finalNote = 2 * pNote - note;
            }

            // Octave wrap inside range
            if (wrapMode > 0)
            {
                while (finalNote < pNote - wrapSpan) finalNote += 12;
                while (finalNote > pNote + wrapSpan) finalNote -= 12;
            }

            // Scale snap
            if (snapToScale > 0.5f)
            {
                finalNote = ScaleTheory::quantizeToScale(finalNote, ctx.rootKey, ctx.scaleType);
            }
            finalNote = juce::jlimit(0, 127, finalNote);

            // Velocity inversion
            uint8_t finalVel = msg.getVelocity();
            if (invertVelocity > 0.5f)
            {
                finalVel = static_cast<uint8_t>(juce::jlimit(1, 127, 128 - (int)finalVel));
            }

            int outKey = (ch << 8) | (finalNote & 0x7f);
            noteMap[key] = outKey;

            outputMidi.addEvent(juce::MidiMessage::noteOn(ch, finalNote, finalVel), samplePos);
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
