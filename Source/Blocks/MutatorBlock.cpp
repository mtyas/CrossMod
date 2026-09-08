#include "MutatorBlock.h"

namespace MidiFlux
{

MutatorBlock::MutatorBlock()
{
    reset();
}

void MutatorBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
}

void MutatorBlock::reset()
{
    pitchMap.clear();
}

void MutatorBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    for (auto const& [inKey, outKey] : pitchMap)
    {
        int ch = (outKey >> 8) & 0xff;
        int note = outKey & 0x7f;
        outBuffer.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
    }
    pitchMap.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int MutatorBlock::getNumParameters() const
{
    return 6;
}

const ParameterDefinition& MutatorBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 6> defs = {{
        { "pitchMutate", "Mutate Pitch", 0.0f, 1.0f, 0.35f, 0.01f, "%", {} },
        { "pitchRange",  "Pitch Range",  1.0f, 12.0f, 5.0f, 1.0f, "st", {} },
        { "octaveJump",  "Octave Jump",  0.0f, 1.0f, 0.20f, 0.01f, "%", {} },
        { "rhythmSlip",  "Rhythm Slip",  0.0f, 1.0f, 0.20f, 0.01f, "%", {} },
        { "rhythmSlipMs","Slip Amount",  0.0f, 100.0f, 30.0f, 1.0f, "ms", {} },
        { "scaleSnap",   "Snap to Scale", 0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } }
    }};
    return defs[juce::jlimit(0, 5, index)];
}

float MutatorBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return pitchMutateChance;
        case 1: return pitchRange;
        case 2: return octaveJumpChance;
        case 3: return rhythmSlipChance;
        case 4: return rhythmSlipMs;
        case 5: return scaleSnap;
        default: return 0.0f;
    }
}

void MutatorBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: pitchMutateChance = juce::jlimit(0.0f, 1.0f, value); break;
        case 1: pitchRange = juce::jlimit(1.0f, 12.0f, value); break;
        case 2: octaveJumpChance = juce::jlimit(0.0f, 1.0f, value); break;
        case 3: rhythmSlipChance = juce::jlimit(0.0f, 1.0f, value); break;
        case 4: rhythmSlipMs = juce::jlimit(0.0f, 100.0f, value); break;
        case 5: scaleSnap = juce::jlimit(0.0f, 1.0f, value); break;
    }
}

void MutatorBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                juce::MidiBuffer& outputMidi,
                                const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();
        int key = (ch << 8) | (note & 0x7f);

        if (msg.isNoteOn())
        {
            int mutatedNote = note;

            // Pitch mutation
            if (rng.nextBool(pitchMutateChance))
            {
                int maxOffset = static_cast<int>(std::round(pitchRange));
                int offset = rng.nextInt(-maxOffset, maxOffset);
                mutatedNote += offset;

                // Scale snap
                if (scaleSnap > 0.5f)
                {
                    mutatedNote = ScaleTheory::quantizeToScale(mutatedNote, ctx.rootKey, ctx.scaleType);
                }
            }

            // Octave jump
            if (rng.nextBool(octaveJumpChance))
            {
                int oct = rng.nextBool(0.5f) ? 12 : -12;
                mutatedNote += oct;
            }

            mutatedNote = juce::jlimit(0, 127, mutatedNote);

            // Record mapping so noteOff matches
            int outKey = (ch << 8) | (mutatedNote & 0x7f);
            pitchMap[key] = outKey;

            // Rhythm slip
            int finalSamplePos = samplePos;
            if (rng.nextBool(rhythmSlipChance) && rhythmSlipMs > 0.0f)
            {
                int slipSamples = static_cast<int>((rhythmSlipMs * 0.001f * currentSampleRate) * rng.nextFloat(0.1f, 1.0f));
                finalSamplePos = std::min(ctx.numSamples - 1, samplePos + slipSamples);
            }

            auto outMsg = juce::MidiMessage::noteOn(ch, mutatedNote, msg.getVelocity());
            outputMidi.addEvent(outMsg, finalSamplePos);
            triggerActivity();
        }
        else if (msg.isNoteOff())
        {
            auto it = pitchMap.find(key);
            if (it != pitchMap.end())
            {
                int outKey = it->second;
                pitchMap.erase(it);
                int outNote = outKey & 0x7f;
                auto outMsg = juce::MidiMessage::noteOff(ch, outNote, msg.getVelocity());
                outputMidi.addEvent(outMsg, samplePos);
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

juce::String MutatorBlock::getStatusDescription() const
{
    return "Chance: " + juce::String(static_cast<int>(pitchMutateChance * 100.0f)) + "% | Range: ±" + juce::String(static_cast<int>(pitchRange)) + "st";
}

} // namespace MidiFlux
