#include "HarmonizerBlock.h"

namespace MidiFlux
{

HarmonizerBlock::HarmonizerBlock()
{
    reset();
}

void HarmonizerBlock::prepare(double, int)
{
    reset();
}

void HarmonizerBlock::reset()
{
    activeVoices.clear();
}

void HarmonizerBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    for (auto const& [key, notes] : activeVoices)
    {
        for (const auto& vn : notes)
        {
            outBuffer.addEvent(juce::MidiMessage::noteOff(vn.channel, vn.noteNumber), 0);
        }
    }
    activeVoices.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int HarmonizerBlock::getNumParameters() const
{
    return 5;
}

const ParameterDefinition& HarmonizerBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 5> defs = {{
        { "v1Interval", "Voice 1", 0.0f, 7.0f, 1.0f, 1.0f, "", { "Off", "Diat 3rd Up", "Diat 3rd Dn", "Diat 5th Up", "Diat 6th Up", "+1 Octave", "-1 Octave", "+7 Semitones" } },
        { "v2Interval", "Voice 2", 0.0f, 7.0f, 0.0f, 1.0f, "", { "Off", "Diat 3rd Up", "Diat 3rd Dn", "Diat 5th Up", "Diat 6th Up", "+1 Octave", "-1 Octave", "+7 Semitones" } },
        { "v1Level",    "V1 Level",0.1f, 1.5f, 0.85f, 0.01f, "%", {} },
        { "v2Level",    "V2 Level",0.1f, 1.5f, 0.70f, 0.01f, "%", {} },
        { "voiceProb",  "Prob",    0.0f, 1.0f, 1.0f, 0.01f, "%", {} }
    }};
    return defs[juce::jlimit(0, 4, index)];
}

float HarmonizerBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return voice1Interval;
        case 1: return voice2Interval;
        case 2: return voice1VelScale;
        case 3: return voice2VelScale;
        case 4: return voiceProb;
        default: return 0.0f;
    }
}

void HarmonizerBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: voice1Interval = juce::jlimit(0.0f, 7.0f, value); break;
        case 1: voice2Interval = juce::jlimit(0.0f, 7.0f, value); break;
        case 2: voice1VelScale = juce::jlimit(0.1f, 1.5f, value); break;
        case 3: voice2VelScale = juce::jlimit(0.1f, 1.5f, value); break;
        case 4: voiceProb = juce::jlimit(0.0f, 1.0f, value); break;
    }
}

int HarmonizerBlock::calcVoicePitch(int rootNote, int mode, const BlockContext& ctx) const
{
    switch (mode)
    {
        case 1: // Diatonic 3rd Up
            return ScaleTheory::transposeDiatonic(rootNote, 2, ctx.rootKey, ctx.scaleType);
        case 2: // Diatonic 3rd Down
            return ScaleTheory::transposeDiatonic(rootNote, -2, ctx.rootKey, ctx.scaleType);
        case 3: // Diatonic 5th Up
            return ScaleTheory::transposeDiatonic(rootNote, 4, ctx.rootKey, ctx.scaleType);
        case 4: // Diatonic 6th Up
            return ScaleTheory::transposeDiatonic(rootNote, 5, ctx.rootKey, ctx.scaleType);
        case 5: // +1 Octave
            return rootNote + 12;
        case 6: // -1 Octave
            return rootNote - 12;
        case 7: // +7 Semitones
            return rootNote + 7;
        default:
            return -1;
    }
}

void HarmonizerBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                   juce::MidiBuffer& outputMidi,
                                   const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    int v1Mode = static_cast<int>(std::round(voice1Interval));
    int v2Mode = static_cast<int>(std::round(voice2Interval));

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();
        int key = (ch << 8) | (note & 0x7f);

        if (msg.isNoteOn())
        {
            outputMidi.addEvent(msg, samplePos);
            triggerActivity();

            std::vector<HarmonyVoiceNote> voices;

            if (v1Mode > 0 && rng.nextBool(voiceProb))
            {
                int p1 = calcVoicePitch(note, v1Mode, ctx);
                if (p1 >= 0 && p1 <= 127)
                {
                    uint8_t vel = (uint8_t)juce::jlimit(1.0f, 127.0f, (float)msg.getVelocity() * voice1VelScale);
                    outputMidi.addEvent(juce::MidiMessage::noteOn(ch, p1, vel), samplePos);
                    voices.push_back({ ch, p1 });
                }
            }

            if (v2Mode > 0 && rng.nextBool(voiceProb))
            {
                int p2 = calcVoicePitch(note, v2Mode, ctx);
                if (p2 >= 0 && p2 <= 127)
                {
                    uint8_t vel = (uint8_t)juce::jlimit(1.0f, 127.0f, (float)msg.getVelocity() * voice2VelScale);
                    outputMidi.addEvent(juce::MidiMessage::noteOn(ch, p2, vel), samplePos);
                    voices.push_back({ ch, p2 });
                }
            }

            activeVoices[key] = voices;
        }
        else if (msg.isNoteOff())
        {
            outputMidi.addEvent(msg, samplePos);

            auto it = activeVoices.find(key);
            if (it != activeVoices.end())
            {
                for (const auto& vn : it->second)
                {
                    outputMidi.addEvent(juce::MidiMessage::noteOff(vn.channel, vn.noteNumber, msg.getVelocity()), samplePos);
                }
                activeVoices.erase(it);
            }
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

} // namespace MidiFlux
