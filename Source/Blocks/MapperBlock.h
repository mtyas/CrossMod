#pragma once

#include "../Common/MidiBlock.h"

namespace MidiFlux
{

class MapperBlock : public MidiBlock
{
public:
    MapperBlock();

    juce::String getTypeId() const override { return "mapper"; }
    juce::String getDisplayName() const override { return "Mapper"; }
    juce::String getCategory() const override { return "Utility"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff0ea5e9); } // Blue Grey

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
    float velCurve = 0.0f;       // 0: Linear, 1: Compress, 2: Expand, 3: Log, 4: Exp, 5: Invert, 6: Random
    float velOutMin = 1.0f;      // 1 - 127
    float velOutMax = 127.0f;    // 1 - 127
    float ccRemapActive = 0.0f;  // 0: Off, 1: On
    float ccSrc = 1.0f;          // 0 - 127 (e.g. CC 1 Mod Wheel)
    float ccDst = 74.0f;         // 0 - 127 (e.g. CC 74 Brightness / Cutoff)
    float ccScale = 1.0f;        // -1.0 to +1.0

    FastRandom rng;
};

} // namespace MidiFlux
