#include "ArpeggiatorBlock.h"
#include <algorithm>

namespace MidiFlux
{

ArpeggiatorBlock::ArpeggiatorBlock()
{
    reset();
}

void ArpeggiatorBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    physicalNotes.reserve(32);
    arpPool.reserve(32);
    activeNotes.reserve(64);
    cachedNotePool.reserve(128);
    reset();
}

void ArpeggiatorBlock::reset()
{
    phaseInQuarterNotes = 0.0;
    currentStepIndex = 0;
    walkIndex = 0;
    upDownDirection = true;
    sustainPedalDown = false;
    latched = false;
    physicalNotes.clear();
    arpPool.clear();
    activeNotes.clear();
    cachedNotePool.clear();
    notePoolDirty = true;
}

void ArpeggiatorBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    silenceActiveNotes(outBuffer);
    physicalNotes.clear();
    arpPool.clear();
    latched = false;
    sustainPedalDown = false;
    cachedNotePool.clear();
    notePoolDirty = true;
    MidiBlock::allNotesOff(outBuffer);
}

void ArpeggiatorBlock::silenceActiveNotes(juce::MidiBuffer& outBuffer)
{
    std::set<std::pair<int, int>> sentNoteOffs;
    for (const auto& an : activeNotes)
    {
        if (sentNoteOffs.insert({ an.channel, an.noteNumber }).second)
        {
            outBuffer.addEvent(juce::MidiMessage::noteOff(an.channel, an.noteNumber), 0);
        }
    }
    activeNotes.clear();
}

int ArpeggiatorBlock::getNumParameters() const
{
    return 11;
}

const ParameterDefinition& ArpeggiatorBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 11> defs = {{
        { "arpMode",       "Mode",       0.0f, 7.0f, 0.0f, 1.0f, "", { "Up", "Down", "Up/Down", "Converge", "Diverge", "Random", "Walk", "Chords" } },
        { "arpRate",       "Rate",       0.0f, 7.0f, 2.0f, 1.0f, "", { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T", "1/8D", "1/16D" } },
        { "octaveRange",   "Octaves",    1.0f, 4.0f, 1.0f, 1.0f, "oct", {} },
        { "gateLength",    "Gate",       0.1f, 1.5f, 0.8f, 0.05f, "%", {} },
        { "swing",         "Swing",      0.0f, 0.75f, 0.0f, 0.01f, "%", {} },
        { "euclideanPulses","Euclid Hits",1.0f, 16.0f, 16.0f, 1.0f, "", {} },
        { "euclideanSteps", "Euclid Steps",1.0f, 16.0f, 16.0f, 1.0f, "", {} },
        { "skipChance",    "Skip Prob",  0.0f, 0.8f, 0.0f, 0.01f, "%", {} },
        { "mutateChance",  "Mutate",     0.0f, 1.0f, 0.15f, 0.01f, "%", {} },
        { "holdMode",      "Hold / Latch", 0.0f, 1.0f, 0.0f, 1.0f, "", { "Off", "Hold" } },
        { "syncMode",      "Play Mode",  0.0f, 1.0f, 0.0f, 1.0f, "", { "Key Pressed", "DAW Sync" } }
    }};
    return defs[juce::jlimit(0, 10, index)];
}

float ArpeggiatorBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return arpMode;
        case 1: return arpRate;
        case 2: return octaveRange;
        case 3: return gateLength;
        case 4: return swing;
        case 5: return euclideanPulses;
        case 6: return euclideanSteps;
        case 7: return skipChance;
        case 8: return mutateChance;
        case 9: return holdMode;
        case 10: return syncMode;
        default: return 0.0f;
    }
}

void ArpeggiatorBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: arpMode = juce::jlimit(0.0f, 7.0f, value); break;
        case 1: arpRate = juce::jlimit(0.0f, 7.0f, value); break;
        case 2:
            octaveRange = juce::jlimit(1.0f, 4.0f, value);
            notePoolDirty = true;
            break;
        case 3: gateLength = juce::jlimit(0.1f, 1.5f, value); break;
        case 4: swing = juce::jlimit(0.0f, 0.75f, value); break;
        case 5: euclideanPulses = juce::jlimit(1.0f, 16.0f, value); break;
        case 6: euclideanSteps = juce::jlimit(1.0f, 16.0f, value); break;
        case 7: skipChance = juce::jlimit(0.0f, 0.8f, value); break;
        case 8: mutateChance = juce::jlimit(0.0f, 1.0f, value); break;
        case 9:
        {
            float prev = holdMode;
            holdMode = juce::jlimit(0.0f, 1.0f, value);
            if (prev > 0.5f && holdMode < 0.5f && !sustainPedalDown)
            {
                latched = false;
                arpPool.erase(std::remove_if(arpPool.begin(), arpPool.end(), [this](const HeldNote& an) {
                    return std::none_of(physicalNotes.begin(), physicalNotes.end(), [&](const HeldNote& pn) {
                        return pn.noteNumber == an.noteNumber;
                    });
                }), arpPool.end());
                notePoolDirty = true;
            }
            break;
        }
        case 10: syncMode = juce::jlimit(0.0f, 1.0f, value); break;
    }
}

double ArpeggiatorBlock::getDivisionInQuarterNotes(int rateIndex) const
{
    switch (rateIndex)
    {
        case 0: return 1.0;                // 1/4
        case 1: return 0.5;                // 1/8
        case 2: return 0.25;               // 1/16
        case 3: return 0.125;              // 1/32
        case 4: return 0.5 * (2.0 / 3.0);  // 1/8T
        case 5: return 0.25 * (2.0 / 3.0); // 1/16T
        case 6: return 0.75;               // 1/8D
        case 7: return 0.375;              // 1/16D
        default: return 0.25;
    }
}

bool ArpeggiatorBlock::isEuclideanHit(int step, int pulses, int steps) const
{
    if (pulses >= steps) return true;
    if (pulses <= 0) return false;
    return ((step * pulses) % steps) < pulses;
}

const std::vector<int>& ArpeggiatorBlock::getNotePool() const
{
    if (!notePoolDirty)
        return cachedNotePool;

    cachedNotePool.clear();
    if (arpPool.empty())
    {
        notePoolDirty = false;
        return cachedNotePool;
    }

    std::vector<int> baseNotes;
    baseNotes.reserve(arpPool.size());
    for (const auto& hn : arpPool)
        baseNotes.push_back(hn.noteNumber);

    std::sort(baseNotes.begin(), baseNotes.end());

    int octCount = static_cast<int>(std::round(octaveRange));
    cachedNotePool.reserve(baseNotes.size() * octCount);
    for (int oct = 0; oct < octCount; ++oct)
    {
        for (int note : baseNotes)
        {
            int n = note + oct * 12;
            if (n <= 127)
                cachedNotePool.push_back(n);
        }
    }
    notePoolDirty = false;
    return cachedNotePool;
}

void ArpeggiatorBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                    juce::MidiBuffer& outputMidi,
                                    const BlockContext& ctx)
{
    if (isBypassed())
    {
        silenceActiveNotes(outputMidi);
        physicalNotes.clear();
        arpPool.clear();
        latched = false;
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    bool isHold = (holdMode > 0.5f);
    bool requireTransport = (syncMode > 0.5f);

    // If Hold and Sustain are both OFF: synchronize arpPool with physicalNotes
    // (Ensures that turning Hold off immediately stops notes if no keys are held!)
    if (!isHold && !sustainPedalDown)
    {
        latched = false;
        if (physicalNotes.empty())
        {
            if (!arpPool.empty())
            {
                arpPool.clear();
                notePoolDirty = true;
                silenceActiveNotes(outputMidi);
                phaseInQuarterNotes = 0.0;
                currentStepIndex = 0;
            }
        }
        else
        {
            size_t oldSz = arpPool.size();
            arpPool.erase(std::remove_if(arpPool.begin(), arpPool.end(), [this](const HeldNote& an) {
                return std::none_of(physicalNotes.begin(), physicalNotes.end(), [&](const HeldNote& pn) {
                    return pn.noteNumber == an.noteNumber;
                });
            }), arpPool.end());
            if (arpPool.size() != oldSz)
                notePoolDirty = true;
        }
    }

    // 1. Process incoming MIDI messages
    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();

        // Check for All Notes Off / All Sound Off / Reset
        if (msg.isAllNotesOff() || msg.isAllSoundOff() ||
            (msg.isController() && (msg.getControllerNumber() == 123 || msg.getControllerNumber() == 120)))
        {
            silenceActiveNotes(outputMidi);
            physicalNotes.clear();
            arpPool.clear();
            notePoolDirty = true;
            latched = false;
            sustainPedalDown = false;
            continue;
        }

        // Sustain Pedal (CC 64)
        if (msg.isController() && msg.getControllerNumber() == 64)
        {
            bool newSustain = (msg.getControllerValue() >= 64);
            if (sustainPedalDown && !newSustain) // Pedal released
            {
                if (!isHold)
                {
                    size_t oldSz = arpPool.size();
                    arpPool.erase(std::remove_if(arpPool.begin(), arpPool.end(), [this](const HeldNote& an) {
                        return std::none_of(physicalNotes.begin(), physicalNotes.end(), [&](const HeldNote& pn) {
                            return pn.noteNumber == an.noteNumber;
                        });
                    }), arpPool.end());
                    if (arpPool.size() != oldSz)
                        notePoolDirty = true;
                }
            }
            sustainPedalDown = newSustain;
            outputMidi.addEvent(msg, metadata.samplePosition);
            continue;
        }

        if (msg.isNoteOn())
        {
            // Update or add to physicalNotes
            auto itP = std::find_if(physicalNotes.begin(), physicalNotes.end(), [&](const HeldNote& hn) {
                return hn.noteNumber == note;
            });
            if (itP == physicalNotes.end())
                physicalNotes.push_back({ ch, note, msg.getVelocity() });
            else
            {
                itP->velocity = msg.getVelocity();
                itP->channel = ch;
            }

            if (isHold)
            {
                // If previous chord was latched (user had lifted all keys),
                // this new NoteOn begins a fresh chord!
                if (latched)
                {
                    arpPool.clear();
                    silenceActiveNotes(outputMidi);
                    latched = false;
                }

                auto itA = std::find_if(arpPool.begin(), arpPool.end(), [&](const HeldNote& hn) {
                    return hn.noteNumber == note;
                });
                if (itA == arpPool.end())
                {
                    arpPool.push_back({ ch, note, msg.getVelocity() });
                    notePoolDirty = true;
                }
                else
                    itA->velocity = msg.getVelocity();
            }
            else
            {
                latched = false;
                auto itA = std::find_if(arpPool.begin(), arpPool.end(), [&](const HeldNote& hn) {
                    return hn.noteNumber == note;
                });
                if (itA == arpPool.end())
                {
                    arpPool.push_back({ ch, note, msg.getVelocity() });
                    notePoolDirty = true;
                }
                else
                    itA->velocity = msg.getVelocity();
            }
        }
        else if (msg.isNoteOff())
        {
            // Remove from physical notes (match noteNumber)
            physicalNotes.erase(std::remove_if(physicalNotes.begin(), physicalNotes.end(), [&](const HeldNote& hn) {
                return hn.noteNumber == note;
            }), physicalNotes.end());

            if (isHold)
            {
                if (physicalNotes.empty())
                {
                    // All keys are now released -> Latch the current chord!
                    latched = true;
                }
                else if (!latched)
                {
                    // While keys are still actively held (e.g. legato chord change or lifting individual fingers):
                    // Remove released note so old chord notes don't stick into new chords!
                    size_t oldSz = arpPool.size();
                    arpPool.erase(std::remove_if(arpPool.begin(), arpPool.end(), [&](const HeldNote& hn) {
                        return hn.noteNumber == note;
                    }), arpPool.end());
                    if (arpPool.size() != oldSz)
                        notePoolDirty = true;
                }
            }
            else
            {
                if (!sustainPedalDown)
                {
                    size_t oldSz = arpPool.size();
                    arpPool.erase(std::remove_if(arpPool.begin(), arpPool.end(), [&](const HeldNote& hn) {
                        return hn.noteNumber == note;
                    }), arpPool.end());
                    if (arpPool.size() != oldSz)
                        notePoolDirty = true;
                }
            }
        }
        else
        {
            outputMidi.addEvent(msg, metadata.samplePosition);
        }
    }

    // If transport is required but host is stopped, stop playing
    if (requireTransport && !ctx.isPlaying)
    {
        silenceActiveNotes(outputMidi);
        return;
    }

    // If no notes in the arp pool, silence active notes immediately
    if (arpPool.empty())
    {
        silenceActiveNotes(outputMidi);
        phaseInQuarterNotes = 0.0;
        currentStepIndex = 0;
        return;
    }

    double division = getDivisionInQuarterNotes(static_cast<int>(std::round(arpRate)));
    double samplesPerQuarterNote = (ctx.bpm > 0.0 ? (60.0 / ctx.bpm) : 0.5) * currentSampleRate;
    double samplesPerStep = division * samplesPerQuarterNote;
    if (samplesPerStep < 10.0)
        samplesPerStep = 10.0;

    int defaultChannel = arpPool.front().channel;
    uint8_t defaultVelocity = arpPool.front().velocity;

    // Advance active notes and send note-offs
    for (auto it = activeNotes.begin(); it != activeNotes.end(); )
    {
        it->remainingSamples -= ctx.numSamples;
        if (it->remainingSamples <= 0)
        {
            outputMidi.addEvent(juce::MidiMessage::noteOff(it->channel, it->noteNumber), 0);
            it = activeNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Sample-accurate step trigger loop
    double quartersPerSample = (ctx.bpm > 0.0 ? (ctx.bpm / 60.0) : 2.0) / currentSampleRate;
    const auto& notePool = getNotePool();
    if (notePool.empty())
        return;

    int mode = static_cast<int>(std::round(arpMode));
    int ePulses = static_cast<int>(std::round(euclideanPulses));
    int eSteps = static_cast<int>(std::round(euclideanSteps));

    for (int s = 0; s < ctx.numSamples; ++s)
    {
        double oldPhase = phaseInQuarterNotes;
        phaseInQuarterNotes += quartersPerSample;

        // Check if we crossed a step boundary
        int oldStep = static_cast<int>(std::floor(oldPhase / division));
        int newStep = static_cast<int>(std::floor(phaseInQuarterNotes / division));

        if (newStep > oldStep)
        {
            currentStepIndex++;

            // Euclidean rhythm check
            if (!isEuclideanHit(currentStepIndex % eSteps, ePulses, eSteps))
                continue;

            // Random skip chance
            if (skipChance > 0.001f && rng.nextBool(skipChance))
                continue;

            // Select note(s) based on mode
            std::vector<int> notesToPlay;

            if (mode == 7) // Chord mode: trigger all held notes
            {
                notesToPlay = notePool;
            }
            else
            {
                int poolSize = static_cast<int>(notePool.size());
                int noteIndex = 0;

                switch (mode)
                {
                    case 0: // Up
                        noteIndex = currentStepIndex % poolSize;
                        break;
                    case 1: // Down
                        noteIndex = (poolSize - 1) - (currentStepIndex % poolSize);
                        break;
                    case 2: // Up/Down
                    {
                        int cycle = (poolSize > 1) ? (poolSize * 2 - 2) : 1;
                        int pos = currentStepIndex % cycle;
                        noteIndex = (pos < poolSize) ? pos : (cycle - pos);
                        break;
                    }
                    case 3: // Converge
                    {
                        int half = (currentStepIndex / 2) % poolSize;
                        noteIndex = (currentStepIndex % 2 == 0) ? half : (poolSize - 1 - half);
                        break;
                    }
                    case 4: // Diverge
                    {
                        int center = poolSize / 2;
                        int offset = (currentStepIndex / 2) % (poolSize / 2 + 1);
                        noteIndex = (currentStepIndex % 2 == 0) ? (center + offset) : (center - offset);
                        noteIndex = juce::jlimit(0, poolSize - 1, noteIndex);
                        break;
                    }
                    case 5: // Random
                        noteIndex = rng.nextInt(0, poolSize - 1);
                        break;
                    case 6: // Brownian Walk
                    {
                        int step = rng.nextInt(-1, 1);
                        walkIndex = juce::jlimit(0, poolSize - 1, walkIndex + step);
                        noteIndex = walkIndex;
                        break;
                    }
                }

                // Pattern mutation chance
                if (mutateChance > 0.001f && rng.nextBool(mutateChance))
                {
                    noteIndex = juce::jlimit(0, poolSize - 1, noteIndex + rng.nextInt(-2, 2));
                }

                notesToPlay.push_back(notePool[juce::jlimit(0, poolSize - 1, noteIndex)]);
            }

            int noteDurationSamples = static_cast<int>(samplesPerStep * gateLength);
            for (int noteToPlay : notesToPlay)
            {
                outputMidi.addEvent(juce::MidiMessage::noteOn(defaultChannel, noteToPlay, defaultVelocity), s);
                activeNotes.push_back({ defaultChannel, noteToPlay, noteDurationSamples });
            }
            triggerActivity();
        }
    }
}

juce::String ArpeggiatorBlock::getStatusDescription() const
{
    static const char* modeNames[] = { "Up", "Down", "Up/Down", "Converge", "Diverge", "Random", "Walk", "Chord" };
    static const char* rateNames[] = { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T", "1/8D", "1/16D" };
    int m = juce::jlimit(0, 7, (int)std::round(arpMode));
    int r = juce::jlimit(0, 7, (int)std::round(arpRate));
    return juce::String(modeNames[m]) + " • " + juce::String(rateNames[r]);
}

} // namespace MidiFlux
