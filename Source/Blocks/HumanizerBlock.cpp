#include "HumanizerBlock.h"
#include <algorithm>

namespace MidiFlux
{

HumanizerBlock::HumanizerBlock()
{
    reset();
}

void HumanizerBlock::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    reset();
}

void HumanizerBlock::reset()
{
    delayedQueue.clear();
    noteDelayMap.clear();
}

void HumanizerBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    delayedQueue.clear();
    noteDelayMap.clear();
    MidiBlock::allNotesOff(outBuffer);
}

int HumanizerBlock::getNumParameters() const
{
    return 5;
}

const ParameterDefinition& HumanizerBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 5> defs = {{
        { "timeJitter", "Time Jitter", 0.0f, 100.0f, 30.0f, 1.0f, "ms", {} },
        { "pushPull",   "Push / Pull", -50.0f, 50.0f, 0.0f, 1.0f, "ms", {} },
        { "velJitter",  "Vel Jitter",  0.0f, 60.0f, 25.0f, 1.0f, "", {} },
        { "durJitter",  "Gate Jitter", 0.0f, 1.0f, 0.35f, 0.01f, "%", {} },
        { "grooveFeel", "Groove Feel", 0.0f, 3.0f, 0.0f, 1.0f, "", { "Natural", "Loose / Drunk", "Laid-Back", "Rushing" } }
    }};
    return defs[juce::jlimit(0, 4, index)];
}

float HumanizerBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return timingJitterMs;
        case 1: return pushPullMs;
        case 2: return velocityJitter;
        case 3: return durationJitter;
        case 4: return grooveFeel;
        default: return 0.0f;
    }
}

void HumanizerBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: timingJitterMs = juce::jlimit(0.0f, 100.0f, value); break;
        case 1: pushPullMs = juce::jlimit(-50.0f, 50.0f, value); break;
        case 2: velocityJitter = juce::jlimit(0.0f, 60.0f, value); break;
        case 3: durationJitter = juce::jlimit(0.0f, 1.0f, value); break;
        case 4: grooveFeel = juce::jlimit(0.0f, 3.0f, value); break;
    }
}

void HumanizerBlock::processBlock(const juce::MidiBuffer& inputMidi,
                                  juce::MidiBuffer& outputMidi,
                                  const BlockContext& ctx)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    // 1. Drain delayed humanized events scheduled in previous buffers
    for (auto it = delayedQueue.begin(); it != delayedQueue.end(); )
    {
        it->samplesRemaining -= ctx.numSamples;
        if (it->samplesRemaining <= 0)
        {
            int trigPos = juce::jlimit(0, ctx.numSamples - 1, it->samplesRemaining + ctx.numSamples);
            outputMidi.addEvent(it->message, trigPos);
            if (it->message.isNoteOn())
                triggerActivity();
            it = delayedQueue.erase(it);
        }
        else
        {
            ++it;
        }
    }

    int feelMode = static_cast<int>(std::round(grooveFeel));

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();
        int note = msg.getNoteNumber();
        int key = (ch << 8) | (note & 0x7f);

        if (msg.isNoteOn())
        {
            // Pronounced microtiming delay calculation
            float totalMsOffset = pushPullMs;

            if (feelMode == 1) // Loose / Drunk
            {
                totalMsOffset += rng.nextGaussian(10.0f, (timingJitterMs + 15.0f) * 0.7f);
            }
            else if (feelMode == 2) // Laid-Back
            {
                totalMsOffset += 18.0f + std::abs(rng.nextGaussian(0.0f, timingJitterMs * 0.5f));
            }
            else if (feelMode == 3) // Rushing
            {
                totalMsOffset -= 12.0f;
                if (timingJitterMs > 0.001f)
                    totalMsOffset += rng.nextGaussian(0.0f, timingJitterMs * 0.4f);
            }
            else // Natural
            {
                if (timingJitterMs > 0.001f)
                    totalMsOffset += rng.nextGaussian(0.0f, timingJitterMs * 0.5f);
            }

            // Ensure non-negative delay for real-time queue
            float safeDelayMs = std::max(0.0f, totalMsOffset);
            int sampleOffset = static_cast<int>(safeDelayMs * 0.001f * (float)currentSampleRate);
            noteDelayMap[key] = sampleOffset;

            // Pronounced velocity dynamics jitter
            float vel = msg.getVelocity();
            if (velocityJitter > 0.0f)
            {
                vel += rng.nextGaussian(0.0f, velocityJitter * 0.6f);
            }
            uint8_t finalVel = (uint8_t)juce::jlimit(1.0f, 127.0f, vel);

            auto outMsg = juce::MidiMessage::noteOn(ch, note, finalVel);
            int targetSample = samplePos + sampleOffset;

            if (targetSample < ctx.numSamples)
            {
                outputMidi.addEvent(outMsg, targetSample);
                triggerActivity();
            }
            else
            {
                delayedQueue.push_back({ outMsg, targetSample - ctx.numSamples });
            }
        }
        else if (msg.isNoteOff())
        {
            int sampleOffset = 0;
            auto it = noteDelayMap.find(key);
            if (it != noteDelayMap.end())
            {
                sampleOffset = it->second;
                noteDelayMap.erase(it);
            }

            if (durationJitter > 0.01f)
            {
                float durOffsetMs = rng.nextGaussian(0.0f, 40.0f * durationJitter);
                sampleOffset = std::max(0, sampleOffset + static_cast<int>(durOffsetMs * 0.001f * (float)currentSampleRate));
            }

            int targetSample = samplePos + sampleOffset;
            if (targetSample < ctx.numSamples)
            {
                outputMidi.addEvent(msg, targetSample);
            }
            else
            {
                delayedQueue.push_back({ msg, targetSample - ctx.numSamples });
            }
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

juce::String HumanizerBlock::getStatusDescription() const
{
    return "Vel: ±" + juce::String(static_cast<int>(std::round(velocityJitter))) + " | Time: ±" + juce::String(static_cast<int>(std::round(timingJitterMs))) + "ms";
}

} // namespace MidiFlux
