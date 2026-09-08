#pragma once

#include "../Common/MidiBlock.h"
#include <vector>

namespace MidiFlux
{

class DelayBlock : public MidiBlock
{
public:
    DelayBlock();

    juce::String getTypeId() const override { return "delay"; }
    juce::String getDisplayName() const override { return "MIDI Delay & Cascade"; }
    juce::String getCategory() const override { return "Timing"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xffd97706); } // Gold Amber

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
    float delayTime = 2.0f;        // 0: 1/4, 1: 1/8, 2: 1/16, 3: 1/8D, 4: 1/16D, 5: 1/8T, 6: 1/16T
    float repeatCount = 3.0f;      // 1 to 8
    float decayRate = 0.65f;       // 0.1 to 2.0 (10% to 200%)
    float pitchShiftPerTap = 0.0f; // -12 to +12 semitones
    float scaleSnap = 1.0f;        // 0: Off, 1: On

    double currentSampleRate = 44100.0;

    struct EchoEvent
    {
        int channel;
        int noteNumber;
        uint8_t velocity;
        int sampleDelay;
        bool isNoteOn;
    };
    std::vector<EchoEvent> activeEchoes;
};

} // namespace MidiFlux
