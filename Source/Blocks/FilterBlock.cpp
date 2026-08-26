#include "FilterBlock.h"

namespace MidiFlux
{

FilterBlock::FilterBlock()
{
}

void FilterBlock::prepare(double, int)
{
}

void FilterBlock::reset()
{
}

void FilterBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    MidiBlock::allNotesOff(outBuffer);
}

int FilterBlock::getNumParameters() const
{
    return 7;
}

const ParameterDefinition& FilterBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 7> defs = {{
        { "channel",  "Channel",  0.0f, 16.0f, 0.0f, 1.0f, "", { "All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" } },
        { "lowNote",  "Low Note", 0.0f, 127.0f, 0.0f, 1.0f, "", {} },
        { "highNote", "High Note",0.0f, 127.0f, 127.0f, 1.0f, "", {} },
        { "minVel",   "Min Vel",  1.0f, 127.0f, 1.0f, 1.0f, "", {} },
        { "maxVel",   "Max Vel",  1.0f, 127.0f, 127.0f, 1.0f, "", {} },
        { "passCC",   "Pass CC",  0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } },
        { "passPB",   "Pass PB",  0.0f, 1.0f, 1.0f, 1.0f, "", { "Off", "On" } }
    }};
    return defs[juce::jlimit(0, 6, index)];
}

float FilterBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return channelFilter;
        case 1: return lowNote;
        case 2: return highNote;
        case 3: return minVel;
        case 4: return maxVel;
        case 5: return passCC;
        case 6: return passPB;
        default: return 0.0f;
    }
}

void FilterBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: channelFilter = juce::jlimit(0.0f, 16.0f, value); break;
        case 1: lowNote = juce::jlimit(0.0f, 127.0f, value); if (lowNote > highNote) highNote = lowNote; break;
        case 2: highNote = juce::jlimit(0.0f, 127.0f, value); if (highNote < lowNote) lowNote = highNote; break;
        case 3: minVel = juce::jlimit(1.0f, 127.0f, value); if (minVel > maxVel) maxVel = minVel; break;
        case 4: maxVel = juce::jlimit(1.0f, 127.0f, value); if (maxVel < minVel) minVel = maxVel; break;
        case 5: passCC = juce::jlimit(0.0f, 1.0f, value); break;
        case 6: passPB = juce::jlimit(0.0f, 1.0f, value); break;
    }
}

void FilterBlock::processBlock(const juce::MidiBuffer& inputMidi,
                               juce::MidiBuffer& outputMidi,
                               const BlockContext&)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    int chFilter = static_cast<int>(std::round(channelFilter));
    int lNote = static_cast<int>(std::round(lowNote));
    int hNote = static_cast<int>(std::round(highNote));
    int mnVel = static_cast<int>(std::round(minVel));
    int mxVel = static_cast<int>(std::round(maxVel));

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();

        if (chFilter > 0 && ch != chFilter)
            continue;

        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            int vel = msg.getVelocity();

            if (note < lNote || note > hNote)
                continue;

            if (vel < mnVel || vel > mxVel)
                continue;

            outputMidi.addEvent(msg, samplePos);
            triggerActivity();
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            if (note >= lNote && note <= hNote)
            {
                outputMidi.addEvent(msg, samplePos);
            }
        }
        else if (msg.isController())
        {
            if (passCC > 0.5f)
                outputMidi.addEvent(msg, samplePos);
        }
        else if (msg.isPitchWheel())
        {
            if (passPB > 0.5f)
                outputMidi.addEvent(msg, samplePos);
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

} // namespace MidiFlux
