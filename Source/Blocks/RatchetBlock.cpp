#include "RatchetBlock.h"

namespace MidiFlux
{

RatchetBlock::RatchetBlock()
{
    reset();
}

void RatchetBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
}

void RatchetBlock::reset()
{
    queuedNotes.clear();
}

void RatchetBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    for (const auto& qn : queuedNotes)
    {
        if (qn.isNoteOn)
            outBuffer.addEvent(juce::MidiMessage::noteOff(qn.channel, qn.noteNumber), 0);
    }
    queuedNotes.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int RatchetBlock::getNumParameters() const
{
    return 4;
}

const ParameterDefinition& RatchetBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 4> defs = {{
        { "ratchetChance", "Roll Chance", 0.0f, 1.0f, 0.50f, 0.01f, "%", {} },
        { "subdiv",        "Repeats",     0.0f, 5.0f, 2.0f, 1.0f, "", { "2x", "3x", "4x", "6x", "8x", "Random" } },
        { "burstRate",     "Speed",       0.0f, 2.0f, 1.0f, 1.0f, "", { "1/16", "1/32", "1/64" } },
        { "velDecay",      "Dynamics",    -0.5f, 0.5f, -0.2f, 0.01f, "%", {} }
    }};
    return defs[juce::jlimit(0, 3, index)];
}

float RatchetBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return ratchetChance;
        case 1: return subdivisions;
        case 2: return burstDivision;
        case 3: return velocityDecay;
        default: return 0.0f;
    }
}

void RatchetBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: ratchetChance = juce::jlimit(0.0f, 1.0f, value); break;
        case 1: subdivisions = juce::jlimit(0.0f, 5.0f, value); break;
        case 2: burstDivision = juce::jlimit(0.0f, 2.0f, value); break;
        case 3: velocityDecay = juce::jlimit(-0.5f, 0.5f, value); break;
    }
}

void RatchetBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                juce::MidiBuffer& outputMidi,
                                const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    // Process currently queued burst events
    for (auto it = queuedNotes.begin(); it != queuedNotes.end(); )
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
            it = queuedNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    double bpm = (ctx.bpm > 0.0) ? ctx.bpm : 120.0;
    double samplesPerBeat = (60.0 / bpm) * currentSampleRate;
    double burstFraction = 0.25; // 1/16
    int bRate = static_cast<int>(std::round(burstDivision));
    if (bRate == 1) burstFraction = 0.125;      // 1/32
    else if (bRate == 2) burstFraction = 0.0625; // 1/64

    int burstIntervalSamples = static_cast<int>(burstFraction * samplesPerBeat);
    if (burstIntervalSamples < 30)
        burstIntervalSamples = 30;

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();

        if (msg.isNoteOn())
        {
            // Always output the initial hit
            outputMidi.addEvent(msg, samplePos);
            triggerActivity();

            if (rng.nextBool(ratchetChance))
            {
                int count = 2;
                int sub = static_cast<int>(std::round(subdivisions));
                switch (sub)
                {
                    case 0: count = 2; break;
                    case 1: count = 3; break;
                    case 2: count = 4; break;
                    case 3: count = 6; break;
                    case 4: count = 8; break;
                    case 5: count = rng.nextInt(2, 6); break;
                }

                // Add noteOff for initial hit
                int noteDur = burstIntervalSamples / 2;
                queuedNotes.push_back({ ch, note, 0, samplePos + noteDur, false });

                // Queue remaining burst hits
                for (int i = 1; i < count; ++i)
                {
                    float factor = 1.0f + velocityDecay * ((float)i / (float)(count - 1));
                    uint8_t bVel = (uint8_t)juce::jlimit(1.0f, 127.0f, (float)msg.getVelocity() * factor);

                    int onDelay = samplePos + i * burstIntervalSamples;
                    int offDelay = onDelay + noteDur;

                    queuedNotes.push_back({ ch, note, bVel, onDelay, true });
                    queuedNotes.push_back({ ch, note, 0, offDelay, false });
                }
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

juce::String RatchetBlock::getStatusDescription() const
{
    static const char* divNames[] = { "1/16", "1/32", "1/64" };
    int divIdx = juce::jlimit(0, 2, static_cast<int>(std::round(burstDivision)));
    return "Rate: " + juce::String(divNames[divIdx]) + " | Chance: " + juce::String(static_cast<int>(ratchetChance * 100.0f)) + "%";
}

} // namespace MidiFlux
