#pragma once

#include "../Common/MidiBlock.h"

namespace MidiFlux
{

class FilterBlock : public MidiBlock
{
public:
    FilterBlock();

    juce::String getTypeId() const override { return "filter"; }
    juce::String getDisplayName() const override { return "Filter & Split"; }
    juce::String getCategory() const override { return "Filter"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff64748b); } // Slate Blue

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
    float channelFilter = 0.0f;  // 0: All, 1..16
    float lowNote = 0.0f;        // 0 - 127
    float highNote = 127.0f;     // 0 - 127
    float minVel = 1.0f;         // 1 - 127
    float maxVel = 127.0f;       // 1 - 127
    float passCC = 1.0f;         // 0: Off, 1: On
    float passPB = 1.0f;         // 0: Off, 1: On
};

} // namespace MidiFlux
