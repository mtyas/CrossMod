#pragma once

#include "../Common/MidiBlock.h"
#include <vector>
#include <unordered_map>

namespace MidiFlux
{

class TimeQuantizeBlock : public MidiBlock
{
public:
    TimeQuantizeBlock();

    juce::String getTypeId() const override { return "time_quantize"; }
    juce::String getDisplayName() const override { return "Time Quantizer"; }
    juce::String getCategory() const override { return "Timing"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff06b6d4); } // Cyan

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

    bool requiresDawPlayback() const override { return snapStrength > 0.01f; }
    juce::String getStatusDescription() const override;

private:
    float gridDivision = 6.0f;     // Default 6 = 1/16
    float snapStrength = 1.0f;     // 0.0 = Off, 1.0 = 100% Snap
    float swing = 0.0f;            // 0.0 - 0.75
    float windowTolerance = 0.0f;  // 0: Full, 1: 25ms, 2: 50ms, 3: 100ms

    double currentSampleRate = 44100.0;
    double internalBeatClock = 0.0;

    struct DelayedEvent
    {
        juce::MidiMessage message;
        int samplesRemaining;
    };
    std::vector<DelayedEvent> delayedQueue;
    std::unordered_map<int, int> noteDelayMap;

    double getGridDivisionInBeats(int index) const;
};

} // namespace MidiFlux
