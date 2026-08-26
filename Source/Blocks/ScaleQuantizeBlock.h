#pragma once

#include "../Common/MidiBlock.h"
#include "../Common/ScaleTheory.h"
#include <unordered_map>

namespace MidiFlux
{

class ScaleQuantizeBlock : public MidiBlock
{
public:
    ScaleQuantizeBlock();

    juce::String getTypeId() const override { return "scale_quantize"; }
    juce::String getDisplayName() const override { return "Scale Quantizer"; }
    juce::String getCategory() const override { return "Harmonic"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff6366f1); } // Indigo Glow

    void prepare(double sampleRate, int maxSamplesPerBlock) override { (void)sampleRate; (void)maxSamplesPerBlock; reset(); }
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
    float snapStrength = 1.0f;     // 0.0 = No quantize (0%), 1.0 = Full quantize (100%)
    float rootKey = 0.0f;          // 0: C ... 11: B
    float scaleType = 0.0f;        // ScaleType enum (19 scales)
    float snapDirection = 0.0f;    // 0: Closest, 1: Snap Up, 2: Snap Down
    float useGlobalScale = 1.0f;   // 0: Custom, 1: Follow Global Scale
    float degreeShift = 0.0f;      // -7 to +7 diatonic steps

    FastRandom rng;
    // Map input note key -> output note key to guarantee clean Note-Off tracking
    // Key = (channel << 8) | inputNoteNumber
    std::unordered_map<int, int> noteMap;
};

} // namespace MidiFlux
