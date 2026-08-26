#pragma once

#include "../Common/MidiBlock.h"
#include <vector>
#include <unordered_map>

namespace MidiFlux
{

class HumanizerBlock : public MidiBlock
{
public:
    HumanizerBlock();

    juce::String getTypeId() const override { return "humanizer"; }
    juce::String getDisplayName() const override { return "Humanizer"; }
    juce::String getCategory() const override { return "Timing"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xfff59e0b); } // Amber Glow

    void prepare(double sampleRate, int maxSamplesPerBlock) override;
    void reset() override;
    void allNotesOff(juce::MidiBuffer& outBuffer) override;

    void processBlock(const juce::MidiBuffer& inputMidi,
                      juce::MidiBuffer& outputMidi,
                      const BlockContext& ctx) override;

    int getNumParameters() const override;
    const ParameterDefinition& getParameterDef(int index) const override;
    float getParameterValue(int index) const override;
    void setParameterValue(int index, float value) override;

private:
    float timingJitterMs = 30.0f;  // 0 - 100 ms (Pronounced jitter)
    float pushPullMs = 0.0f;       // -50 ms to +50 ms
    float velocityJitter = 25.0f;  // 0 - 60
    float durationJitter = 0.35f;  // 0.0 - 1.0
    float grooveFeel = 0.0f;       // 0: Natural, 1: Loose/Drunk, 2: Laid-Back, 3: Rushing

    FastRandom rng;
    double currentSampleRate = 44100.0;

    struct DelayedEvent
    {
        juce::MidiMessage message;
        int samplesRemaining;
    };
    std::vector<DelayedEvent> delayedQueue;
    std::unordered_map<int, int> noteDelayMap;
};

} // namespace MidiFlux
