#include "ProbabilityBlock.h"

namespace MidiFlux
{

ProbabilityBlock::ProbabilityBlock()
{
    reset();
}

void ProbabilityBlock::prepare(double, int)
{
    reset();
}

void ProbabilityBlock::reset()
{
    noteCounter = 0;
    passedNotes.clear();
}

void ProbabilityBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    for (int key : passedNotes)
    {
        int ch = (key >> 8) & 0xff;
        int note = key & 0x7f;
        outBuffer.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
    }
    passedNotes.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int ProbabilityBlock::getNumParameters() const
{
    return 6;
}

const ParameterDefinition& ProbabilityBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 6> defs = {{
        { "gateProb", "Gate Prob", 0.0f, 1.0f, 0.85f, 0.01f, "%", {} },
        { "velMin", "Min Vel", 1.0f, 127.0f, 40.0f, 1.0f, "", {} },
        { "velMax", "Max Vel", 1.0f, 127.0f, 120.0f, 1.0f, "", {} },
        { "velRandomize", "Vel Random", 0.0f, 1.0f, 0.3f, 0.01f, "%", {} },
        { "ghostNoteProb", "Ghost Chance", 0.0f, 0.5f, 0.0f, 0.01f, "%", {} },
        { "conditionMode", "Condition", 0.0f, 4.0f, 0.0f, 1.0f, "", { "Always", "1 of 2", "2 of 4", "1 of 4", "3 of 4" } }
    }};
    return defs[juce::jlimit(0, 5, index)];
}

float ProbabilityBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return gateProb;
        case 1: return velMin;
        case 2: return velMax;
        case 3: return velRandomize;
        case 4: return ghostNoteProb;
        case 5: return conditionMode;
        default: return 0.0f;
    }
}

void ProbabilityBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: gateProb = juce::jlimit(0.0f, 1.0f, value); break;
        case 1: velMin = juce::jlimit(1.0f, 127.0f, value); if (velMin > velMax) velMax = velMin; break;
        case 2: velMax = juce::jlimit(1.0f, 127.0f, value); if (velMax < velMin) velMin = velMax; break;
        case 3: velRandomize = juce::jlimit(0.0f, 1.0f, value); break;
        case 4: ghostNoteProb = juce::jlimit(0.0f, 0.5f, value); break;
        case 5: conditionMode = juce::jlimit(0.0f, 4.0f, value); break;
    }
}

void ProbabilityBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                    juce::MidiBuffer& outputMidi,
                                    const BlockContext&)
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
            noteCounter++;
            bool passesCondition = true;
            int cond = static_cast<int>(std::round(conditionMode));

            if (cond == 1) // 1 of 2
                passesCondition = (noteCounter % 2 == 1);
            else if (cond == 2) // 2 of 4
                passesCondition = (noteCounter % 4 == 2);
            else if (cond == 3) // 1 of 4
                passesCondition = (noteCounter % 4 == 1);
            else if (cond == 4) // 3 of 4
                passesCondition = (noteCounter % 4 == 3);

            bool passesGate = passesCondition && rng.nextBool(gateProb);

            if (passesGate)
            {
                passedNotes.insert(key);

                // Velocity modification
                float baseVel = msg.getVelocity();
                if (velRandomize > 0.001f)
                {
                    float rVel = rng.nextFloat(velMin, velMax);
                    baseVel = juce::jlimit(1.0f, 127.0f, baseVel * (1.0f - velRandomize) + rVel * velRandomize);
                }

                auto outMsg = juce::MidiMessage::noteOn(ch, note, (uint8_t)std::round(baseVel));
                outputMidi.addEvent(outMsg, samplePos);
                triggerActivity();

                // Ghost note trigger
                if (ghostNoteProb > 0.001f && rng.nextBool(ghostNoteProb))
                {
                    int ghostSample = std::min(samplePos + 100, 500);
                    int ghostNote = juce::jlimit(0, 127, note + (rng.nextBool(0.5f) ? 12 : -12));
                    auto ghostMsg = juce::MidiMessage::noteOn(ch, ghostNote, (uint8_t)rng.nextInt(15, 35));
                    outputMidi.addEvent(ghostMsg, ghostSample);
                    // Add ghost note off
                    auto ghostOff = juce::MidiMessage::noteOff(ch, ghostNote);
                    outputMidi.addEvent(ghostOff, ghostSample + 150);
                }
            }
        }
        else if (msg.isNoteOff())
        {
            if (passedNotes.find(key) != passedNotes.end())
            {
                passedNotes.erase(key);
                outputMidi.addEvent(msg, samplePos);
            }
        }
        else
        {
            // Pass all CCs, pitch bends, etc.
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

juce::String ProbabilityBlock::getStatusDescription() const
{
    juce::String s = "Pass: " + juce::String(static_cast<int>(gateProb * 100.0f)) + "%";
    if (ghostNoteProb > 0.01f)
        s += " | Ghost: " + juce::String(static_cast<int>(ghostNoteProb * 100.0f)) + "%";
    return s;
}

} // namespace MidiFlux
