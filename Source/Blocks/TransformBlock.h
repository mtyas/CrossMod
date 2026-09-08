#pragma once

#include "../Common/MidiBlock.h"
#include "../Common/ScaleTheory.h"
#include <unordered_map>

namespace MidiFlux
{

class TransformBlock : public MidiBlock
{
public:
    TransformBlock();

    juce::String getTypeId() const override { return "transform"; }
    juce::String getDisplayName() const override { return "Invert & Mirror"; }
    juce::String getCategory() const override { return "Utility"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xffec4899); } // Magenta

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

    juce::String getStatusDescription() const override;

private:
    float pitchInvert = 1.0f;     // 0: Off, 1: On
    float pivotNote = 60.0f;      // 0 - 127 (C4 = 60 default)
    float snapToScale = 1.0f;     // 0: Off, 1: On
    float invertVelocity = 0.0f;  // 0: Off, 1: On (127 - vel)
    float octaveWrap = 0.0f;      // 0: Off, 1: 1 Octave, 2: 2 Octaves, 3: 3 Octaves
    float gateScale = 1.0f;       // 0.25 to 2.0 (25% to 200%)

    // Track input note -> output transformed note
    std::unordered_map<int, int> noteMap;
};

} // namespace MidiFlux
