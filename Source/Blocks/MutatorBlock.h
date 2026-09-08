#pragma once

#include "../Common/MidiBlock.h"
#include <unordered_map>

namespace MidiFlux
{

class MutatorBlock : public MidiBlock
{
public:
    MutatorBlock();

    juce::String getTypeId() const override { return "mutator"; }
    juce::String getDisplayName() const override { return "Mutator"; }
    juce::String getCategory() const override { return "Generative"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff10b981); } // Emerald

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

    juce::String getStatusDescription() const override;

private:
    float pitchMutateChance = 0.35f; // 0.0 - 1.0
    float pitchRange = 5.0f;         // 1 - 12 semitones
    float octaveJumpChance = 0.20f;  // 0.0 - 1.0
    float rhythmSlipChance = 0.20f;  // 0.0 - 1.0
    float rhythmSlipMs = 30.0f;      // 0 - 100 ms
    float scaleSnap = 1.0f;          // 0: Off, 1: On

    FastRandom rng;
    double currentSampleRate = 44100.0;
    // Map input note key -> output mutated note key so Note-Off matches!
    std::unordered_map<int, int> pitchMap;
};

} // namespace MidiFlux
