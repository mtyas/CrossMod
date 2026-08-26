#pragma once

#include "../Common/MidiBlock.h"
#include <unordered_map>

namespace MidiFlux
{

class QuantizerBlock : public MidiBlock
{
public:
    QuantizerBlock();

    juce::String getTypeId() const override { return "quantizer"; }
    juce::String getDisplayName() const override { return "Scale & Time Quantize"; }
    juce::String getCategory() const override { return "Harmonic"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff6366f1); } // Indigo Glow

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
    float scaleSnapActive = 1.0f;  // 0: Off, 1: On
    float rootKey = 0.0f;          // 0: C ... 11: B
    float scaleType = 0.0f;        // ScaleType enum
    float snapDirection = 0.0f;    // 0: Closest, 1: Up, 2: Down
    float timeSnapActive = 0.0f;   // 0: Off, 1: On
    float timeGrid = 2.0f;         // 0: 1/4, 1: 1/8, 2: 1/16, 3: 1/32
    float snapStrength = 1.0f;     // 0.0 - 1.0

    FastRandom rng;
    double currentSampleRate = 44100.0;
    double internalBeatClock = 0.0;
    // Map input note key -> quantized note key
    std::unordered_map<int, int> noteMap;

    struct DelayedEvent
    {
        juce::MidiMessage message;
        int samplesRemaining;
    };
    std::vector<DelayedEvent> delayedQueue;
    std::unordered_map<int, int> noteDelayMap;
};

} // namespace MidiFlux
