#include "EuclideanBlock.h"
#include <algorithm>

namespace MidiFlux
{

EuclideanBlock::EuclideanBlock()
{
    reset();
}

void EuclideanBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
}

void EuclideanBlock::reset()
{
    phaseInQuarters = 0.0;
    currentStep = 0;
    heldNotes.clear();
    activeNotes.clear();
}

void EuclideanBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    for (const auto& an : activeNotes)
    {
        outBuffer.addEvent(juce::MidiMessage::noteOff(an.channel, an.noteNumber), 0);
    }
    activeNotes.clear();
    heldNotes.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int EuclideanBlock::getNumParameters() const
{
    return 5;
}

const ParameterDefinition& EuclideanBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 5> defs = {{
        { "pulses",   "Hits",       1.0f, 16.0f, 5.0f, 1.0f, "", {} },
        { "steps",    "Total Steps",1.0f, 16.0f, 8.0f, 1.0f, "", {} },
        { "rotation", "Shift / Rot",0.0f, 15.0f, 0.0f, 1.0f, "", {} },
        { "rate",     "Speed",      0.0f, 3.0f, 2.0f, 1.0f, "", { "1/4", "1/8", "1/16", "1/32" } },
        { "gate",     "Gate Length",0.1f, 1.0f, 0.75f, 0.01f, "%", {} }
    }};
    return defs[juce::jlimit(0, 4, index)];
}

float EuclideanBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return pulses;
        case 1: return steps;
        case 2: return rotation;
        case 3: return rateDivision;
        case 4: return gateLength;
        default: return 0.0f;
    }
}

void EuclideanBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: pulses = juce::jlimit(1.0f, 16.0f, value); if (pulses > steps) steps = pulses; break;
        case 1: steps = juce::jlimit(1.0f, 16.0f, value); if (steps < pulses) pulses = steps; break;
        case 2: rotation = juce::jlimit(0.0f, 15.0f, value); break;
        case 3: rateDivision = juce::jlimit(0.0f, 3.0f, value); break;
        case 4: gateLength = juce::jlimit(0.1f, 1.0f, value); break;
    }
}

bool EuclideanBlock::isHit(int step, int p, int s, int rot) const
{
    if (s <= 0) return false;
    if (p >= s) return true;
    if (p <= 0) return false;

    int shifted = (step + rot) % s;
    if (shifted < 0) shifted += s;

    return ((shifted * p) % s) < p;
}

void EuclideanBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                  juce::MidiBuffer& outputMidi,
                                  const BlockContext& ctx)
{
    if (isBypassed())
    {
        allNotesOff(outputMidi);
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    double div = 0.25; // 1/16
    int rIdx = static_cast<int>(std::round(rateDivision));
    if (rIdx == 0) div = 1.0;
    else if (rIdx == 1) div = 0.5;
    else if (rIdx == 2) div = 0.25;
    else if (rIdx == 3) div = 0.125;

    double bpm = (ctx.bpm > 0.0) ? ctx.bpm : 120.0;
    double quartersPerSample = (bpm / 60.0) / currentSampleRate;
    double samplesPerStep = div * (60.0 / bpm) * currentSampleRate;

    // Process incoming notes
    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();

        // Check for All Notes Off / All Sound Off / Reset
        if (msg.isAllNotesOff() || msg.isAllSoundOff() ||
            (msg.isController() && (msg.getControllerNumber() == 123 || msg.getControllerNumber() == 120)))
        {
            allNotesOff(outputMidi);
            continue;
        }

        if (msg.isNoteOn())
        {
            auto it = std::find_if(heldNotes.begin(), heldNotes.end(), [&](const HeldNote& hn) {
                return hn.channel == ch && hn.noteNumber == note;
            });
            if (it == heldNotes.end())
                heldNotes.push_back({ ch, note, msg.getVelocity(), true, 0 });
            else
            {
                it->velocity = msg.getVelocity();
                it->isHeld = true;
                it->releaseGraceSamples = 0;
            }
        }
        else if (msg.isNoteOff())
        {
            auto it = std::find_if(heldNotes.begin(), heldNotes.end(), [&](const HeldNote& hn) {
                return hn.channel == ch && hn.noteNumber == note;
            });
            if (it != heldNotes.end())
            {
                it->isHeld = false;
                // Give short echo notes / pulses a grace period so the euclidean sequencer hits them
                it->releaseGraceSamples = static_cast<int>(samplesPerStep * 2.0);
            }
        }
        else
        {
            outputMidi.addEvent(msg, metadata.samplePosition);
        }
    }

    // Decay releaseGraceSamples for unheld notes
    for (auto it = heldNotes.begin(); it != heldNotes.end(); )
    {
        if (!it->isHeld)
        {
            it->releaseGraceSamples -= ctx.numSamples;
            if (it->releaseGraceSamples <= 0)
            {
                it = heldNotes.erase(it);
                continue;
            }
        }
        ++it;
    }

    if (heldNotes.empty())
    {
        for (const auto& an : activeNotes)
            outputMidi.addEvent(juce::MidiMessage::noteOff(an.channel, an.noteNumber), 0);
        activeNotes.clear();
        return;
    }

    // Decay active notes
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

    int p = static_cast<int>(std::round(pulses));
    int s = static_cast<int>(std::round(steps));
    int rot = static_cast<int>(std::round(rotation));

    for (int smp = 0; smp < ctx.numSamples; ++smp)
    {
        double oldP = phaseInQuarters;
        phaseInQuarters += quartersPerSample;

        int oldStep = static_cast<int>(std::floor(oldP / div));
        int newStep = static_cast<int>(std::floor(phaseInQuarters / div));

        if (newStep > oldStep)
        {
            currentStep++;
            if (isHit(currentStep, p, s, rot))
            {
                int noteDur = static_cast<int>(samplesPerStep * gateLength);
                for (const auto& hn : heldNotes)
                {
                    outputMidi.addEvent(juce::MidiMessage::noteOn(hn.channel, hn.noteNumber, hn.velocity), smp);
                    activeNotes.push_back({ hn.channel, hn.noteNumber, noteDur });
                }
                triggerActivity();
            }
        }
    }
}

juce::String EuclideanBlock::getStatusDescription() const
{
    static const char* rateNames[] = { "1/4", "1/8", "1/16", "1/32" };
    int r = juce::jlimit(0, 3, (int)std::round(rateDivision));
    int p = (int)std::round(pulses);
    int s = (int)std::round(steps);
    return juce::String(p) + "/" + juce::String(s) + " PULSES • " + juce::String(rateNames[r]);
}

} // namespace MidiFlux
