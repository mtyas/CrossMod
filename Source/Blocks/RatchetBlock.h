#pragma once

#include "../Common/MidiBlock.h"
#include <vector>

namespace MidiFlux
{

class RatchetBlock : public MidiBlock
{
public:
    RatchetBlock();

    juce::String getTypeId() const override { return "ratchet"; }
    juce::String getDisplayName() const override { return "Ratchet & Roll"; }
    juce::String getCategory() const override { return "Generative"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xfff43f5e); } // Coral

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
    float ratchetChance = 0.50f;   // 0.0 - 1.0
    float subdivisions = 2.0f;     // 0: 2x, 1: 3x, 2: 4x, 3: 6x, 4: 8x, 5: Random
    float burstDivision = 1.0f;    // 0: 1/16, 1: 1/32, 2: 1/64
    float velocityDecay = -0.2f;   // -0.5 to +0.5

    FastRandom rng;
    double currentSampleRate = 44100.0;

    struct QueuedBurstNote
    {
        int channel;
        int noteNumber;
        uint8_t velocity;
        int sampleDelay;
        bool isNoteOn;
    };
    std::vector<QueuedBurstNote> queuedNotes;
};

} // namespace MidiFlux
