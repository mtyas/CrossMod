#pragma once

#include "../Common/MidiBlock.h"
#include <unordered_map>

namespace MidiFlux
{

class TransposeBlock : public MidiBlock
{
public:
    TransposeBlock();

    juce::String getTypeId() const override { return "transpose"; }
    juce::String getDisplayName() const override { return "Transpose"; }
    juce::String getCategory() const override { return "Harmonic"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff8b5cf6); } // Violet

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
    float chromaticShift = 0.0f;     // -36 to +36
    float diatonicShift = 0.0f;      // -14 to +14
    float octaveShift = 0.0f;        // -3 to +3
    float randomJumpChance = 0.0f;   // 0.0 to 0.5
    float randomIntervalMode = 0.0f; // 0: Octave (+/-12), 1: Fifth (+/-7), 2: Third (+/-4), 3: Random

    FastRandom rng;
    std::unordered_map<int, int> noteMap;
};

} // namespace MidiFlux
