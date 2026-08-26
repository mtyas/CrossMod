#include "TransposeBlock.h"

namespace MidiFlux
{

TransposeBlock::TransposeBlock()
{
    reset();
}

void TransposeBlock::prepare(double, int)
{
    reset();
}

void TransposeBlock::reset()
{
    noteMap.clear();
}

void TransposeBlock::allNotesOff(juce::MidiBuffer& outBuffer)
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

int TransposeBlock::getNumParameters() const
{
    return 5;
}

const ParameterDefinition& TransposeBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 5> defs = {{
        { "chromatic",   "Chromatic", -36.0f, 36.0f, 0.0f, 1.0f, "st", {} },
        { "diatonic",    "Diatonic",  -14.0f, 14.0f, 0.0f, 1.0f, "deg", {} },
        { "octave",      "Octave",    -3.0f, 3.0f, 0.0f, 1.0f, "oct", {} },
        { "randomJump",  "Rand Jump", 0.0f, 0.5f, 0.0f, 0.01f, "%", {} },
        { "randInterval","Rand Type", 0.0f, 3.0f, 0.0f, 1.0f, "", { "Octave", "Fifth", "Third", "Random" } }
    }};
    return defs[juce::jlimit(0, 4, index)];
}

float TransposeBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return chromaticShift;
        case 1: return diatonicShift;
        case 2: return octaveShift;
        case 3: return randomJumpChance;
        case 4: return randomIntervalMode;
        default: return 0.0f;
    }
}

void TransposeBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: chromaticShift = juce::jlimit(-36.0f, 36.0f, value); break;
        case 1: diatonicShift = juce::jlimit(-14.0f, 14.0f, value); break;
        case 2: octaveShift = juce::jlimit(-3.0f, 3.0f, value); break;
        case 3: randomJumpChance = juce::jlimit(0.0f, 0.5f, value); break;
        case 4: randomIntervalMode = juce::jlimit(0.0f, 3.0f, value); break;
    }
}

void TransposeBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                  juce::MidiBuffer& outputMidi,
                                  const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    int chrom = static_cast<int>(std::round(chromaticShift));
    int diat = static_cast<int>(std::round(diatonicShift));
    int oct = static_cast<int>(std::round(octaveShift));
    int rMode = static_cast<int>(std::round(randomIntervalMode));

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

            // Diatonic shift first
            if (diat != 0)
            {
                finalNote = ScaleTheory::transposeDiatonic(finalNote, diat, ctx.rootKey, ctx.scaleType);
            }

            // Chromatic + octave
            finalNote += chrom + (oct * 12);

            // Stochastic jump
            if (randomJumpChance > 0.001f && rng.nextBool(randomJumpChance))
            {
                int rInterval = 12;
                if (rMode == 0) rInterval = 12;
                else if (rMode == 1) rInterval = 7;
                else if (rMode == 2) rInterval = 4;
                else if (rMode == 3) rInterval = rng.nextInt(1, 12);

                finalNote += rng.nextBool(0.5f) ? rInterval : -rInterval;
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
                outputMidi.addEvent(juce::MidiMessage::noteOff(ch, outKey & 0x7f, msg.getVelocity()), samplePos);
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
