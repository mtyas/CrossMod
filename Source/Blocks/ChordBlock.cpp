#include "ChordBlock.h"
#include <algorithm>

namespace MidiFlux
{

ChordBlock::ChordBlock()
{
    reset();
}

void ChordBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
    activeChords.reserve(64);
    delayedStrumNotes.reserve(64);
}

void ChordBlock::reset()
{
    activeChords.clear();
    delayedStrumNotes.clear();
    altDirection = false;
}

void ChordBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    delayedStrumNotes.clear();
    for (auto const& [key, notes] : activeChords)
    {
        for (const auto& gn : notes)
        {
            outBuffer.addEvent(juce::MidiMessage::noteOff(gn.channel, gn.noteNumber), 0);
        }
    }
    activeChords.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int ChordBlock::getNumParameters() const
{
    return 7;
}

const ParameterDefinition& ChordBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 7> defs = {{
        { "chordType", "Type", 0.0f, 10.0f, 0.0f, 1.0f, "", { "Diatonic Auto", "Major", "Minor", "Dominant 7", "Major 7", "Minor 7", "Sus2", "Sus4", "Diminished", "9th", "Power" } },
        { "inversion", "Inversion", 0.0f, 4.0f, 0.0f, 1.0f, "", { "Root", "1st", "2nd", "3rd", "Random" } },
        { "voicing",   "Voicing", 0.0f, 3.0f, 0.0f, 1.0f, "", { "Close", "Drop-2", "Drop-3", "Spread Open" } },
        { "strumSpeed","Strum (ms)", 0.0f, 100.0f, 20.0f, 1.0f, "ms", {} },
        { "strumDir",  "Strum Dir", 0.0f, 3.0f, 0.0f, 1.0f, "", { "Up", "Down", "Alternate", "Random" } },
        { "velRamp",   "Vel Ramp", -0.5f, 0.5f, 0.0f, 0.01f, "%", {} },
        { "dropChance","Drop Note", 0.0f, 0.5f, 0.0f, 0.01f, "%", {} }
    }};
    return defs[juce::jlimit(0, 6, index)];
}

float ChordBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return chordType;
        case 1: return inversion;
        case 2: return voicing;
        case 3: return strumSpeedMs;
        case 4: return strumDirection;
        case 5: return strumVelocityRamp;
        case 6: return randomDropChance;
        default: return 0.0f;
    }
}

void ChordBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: chordType = juce::jlimit(0.0f, 10.0f, value); break;
        case 1: inversion = juce::jlimit(0.0f, 4.0f, value); break;
        case 2: voicing = juce::jlimit(0.0f, 3.0f, value); break;
        case 3: strumSpeedMs = juce::jlimit(0.0f, 100.0f, value); break;
        case 4: strumDirection = juce::jlimit(0.0f, 3.0f, value); break;
        case 5: strumVelocityRamp = juce::jlimit(-0.5f, 0.5f, value); break;
        case 6: randomDropChance = juce::jlimit(0.0f, 0.5f, value); break;
    }
}

void ChordBlock::processBlock(const juce::MidiBuffer& inputMidi,
                              juce::MidiBuffer& outputMidi,
                              const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    // 1. Drain delayed strum notes scheduled in previous buffers
    for (auto it = delayedStrumNotes.begin(); it != delayedStrumNotes.end(); )
    {
        it->samplesRemaining -= ctx.numSamples;
        if (it->samplesRemaining <= 0)
        {
            int trigPos = juce::jlimit(0, ctx.numSamples - 1, it->samplesRemaining + ctx.numSamples);
            outputMidi.addEvent(juce::MidiMessage::noteOn(it->channel, it->noteNumber, it->velocity), trigPos);
            triggerActivity();
            it = delayedStrumNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int rootNote = msg.getNoteNumber();
        int key = (ch << 8) | (rootNote & 0x7f);

        if (msg.isNoteOn())
        {
            // Determine chord type
            int typeIdx = static_cast<int>(std::round(chordType));
            ChordType ct = static_cast<ChordType>(typeIdx);

            static const char* typeNames[] = { "Diatonic", "Major", "Minor", "Dom7", "Maj7", "Min7", "Sus2", "Sus4", "Dim", "9th", "Power" };
            int tIdx = juce::jlimit(0, 10, typeIdx);
            lastVoicedChord = juce::String(ScaleTheory::getNoteName(rootNote % 12)) + " " + juce::String(typeNames[tIdx]);

            auto intervals = ScaleTheory::getChordIntervals(rootNote, ct, ctx.rootKey, ctx.scaleType);

            // Inversion
            int inv = static_cast<int>(std::round(inversion));
            if (inv == 4) // Random inversion
                inv = rng.nextInt(0, 3);

            int vc = static_cast<int>(std::round(voicing));
            auto voicedIntervals = ScaleTheory::applyInversion(intervals, inv, vc);

            // Strum direction
            int sDir = static_cast<int>(std::round(strumDirection));
            bool reverseOrder = false;
            if (sDir == 1) // Down
            {
                reverseOrder = true;
            }
            else if (sDir == 2) // Alternate
            {
                reverseOrder = altDirection;
                altDirection = !altDirection;
            }
            else if (sDir == 3) // Random
            {
                reverseOrder = rng.nextBool(0.5f);
            }

            if (reverseOrder)
                std::reverse(voicedIntervals.begin(), voicedIntervals.end());

            std::vector<GeneratedNote> genNotes;
            int numChordNotes = static_cast<int>(voicedIntervals.size());
            float strumIntervalSamples = (strumSpeedMs * 0.001f * (float)currentSampleRate);

            for (int i = 0; i < numChordNotes; ++i)
            {
                // Random note drop check (never drop the root if i==0)
                if (i > 0 && randomDropChance > 0.001f && rng.nextBool(randomDropChance))
                    continue;

                int pitch = rootNote + voicedIntervals[i];
                if (pitch < 0 || pitch > 127)
                    continue;

                // Velocity ramp across strum
                float velFactor = 1.0f;
                if (numChordNotes > 1)
                {
                    float progress = (float)i / (float)(numChordNotes - 1);
                    velFactor += strumVelocityRamp * (progress * 2.0f - 1.0f);
                }
                uint8_t noteVel = (uint8_t)juce::jlimit(1.0f, 127.0f, msg.getVelocity() * velFactor);

                genNotes.push_back({ ch, pitch });

                int targetSample = samplePos + static_cast<int>(i * strumIntervalSamples);
                if (targetSample < ctx.numSamples)
                {
                    outputMidi.addEvent(juce::MidiMessage::noteOn(ch, pitch, noteVel), targetSample);
                    triggerActivity();
                }
                else
                {
                    delayedStrumNotes.push_back({ ch, pitch, noteVel, targetSample - ctx.numSamples, key });
                }
            }

            activeChords[key] = genNotes;
            triggerActivity();
        }
        else if (msg.isNoteOff())
        {
            // Cancel any pending strum notes for this chord that haven't fired yet
            delayedStrumNotes.erase(std::remove_if(delayedStrumNotes.begin(), delayedStrumNotes.end(),
                [key](const DelayedStrumNote& dsn) { return dsn.chordKey == key; }), delayedStrumNotes.end());

            auto it = activeChords.find(key);
            if (it != activeChords.end())
            {
                for (const auto& gn : it->second)
                {
                    outputMidi.addEvent(juce::MidiMessage::noteOff(gn.channel, gn.noteNumber, msg.getVelocity()), samplePos);
                }
                activeChords.erase(it);
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
