#pragma once

#include "../Common/MidiBlock.h"
#include <unordered_set>

namespace MidiFlux
{

class ProbabilityBlock : public MidiBlock
{
public:
    ProbabilityBlock();

    juce::String getTypeId() const override { return "probability"; }
    juce::String getDisplayName() const override { return "Probability"; }
    juce::String getCategory() const override { return "Generative"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff00e5ff); } // Neon Cyan

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
    float gateProb = 0.85f;     // 0.0 - 1.0 (85% pass rate)
    float velMin = 40.0f;       // 1 - 127
    float velMax = 120.0f;      // 1 - 127
    float velRandomize = 0.3f;  // 0.0 - 1.0
    float ghostNoteProb = 0.0f; // 0.0 - 0.5
    float conditionMode = 0.0f; // 0: None, 1: 1 of 2, 2: 2 of 4, 3: 1 of 4, 4: 3 of 4

    FastRandom rng;
    int noteCounter = 0;
    std::unordered_set<int> passedNotes; // Key: (channel << 8) | noteNumber
};

} // namespace MidiFlux
