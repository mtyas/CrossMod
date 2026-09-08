#pragma once

#include "../Common/MidiBlock.h"
#include <vector>

namespace MidiFlux
{

class EuclideanBlock : public MidiBlock
{
public:
    EuclideanBlock();

    juce::String getTypeId() const override { return "euclidean"; }
    juce::String getDisplayName() const override { return "Euclidean Rhythms"; }
    juce::String getCategory() const override { return "Generative"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff14b8a6); } // Teal

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

    bool requiresDawPlayback() const override { return true; }
    juce::String getStatusDescription() const override;
    int getVisualizerStep() const override { return currentStep; }
    bool isStepHit(int step) const { return isHit(step, (int)std::round(pulses), (int)std::round(steps), (int)std::round(rotation)); }

private:
    float pulses = 5.0f;       // 1 to 16
    float steps = 8.0f;        // 1 to 16
    float rotation = 0.0f;     // 0 to 15
    float rateDivision = 2.0f; // 0: 1/4, 1: 1/8, 2: 1/16, 3: 1/32
    float gateLength = 0.75f;  // 0.1 to 1.0

    double currentSampleRate = 44100.0;
    double phaseInQuarters = 0.0;
    int currentStep = 0;

    struct HeldNote
    {
        int channel;
        int noteNumber;
        uint8_t velocity;
        bool isHeld = true;
        int releaseGraceSamples = 0;
    };
    std::vector<HeldNote> heldNotes;

    struct ActiveNote
    {
        int channel;
        int noteNumber;
        int remainingSamples;
    };
    std::vector<ActiveNote> activeNotes;

    bool isHit(int step, int p, int s, int rot) const;
};

} // namespace MidiFlux
