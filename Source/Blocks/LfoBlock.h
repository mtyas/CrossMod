#pragma once

#include "../Common/MidiBlock.h"

namespace MidiFlux
{

class LfoBlock : public MidiBlock
{
public:
    LfoBlock();

    juce::String getTypeId() const override { return "lfo"; }
    juce::String getDisplayName() const override { return "MIDI LFO"; }
    juce::String getCategory() const override { return "Utility"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xffd946ef); } // Electric Violet

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
    float targetCC = 0.0f;        // 0: CC 1, 1: CC 11, 2: CC 74, 3: CC 71, 4: CC 10, 5: CC 7, 6: Pitch Bend
    float syncMode = 0.0f;        // 0: Tempo Synced, 1: Free Rate (Hz)
    float syncRate = 4.0f;        // Default 1/4 (index 4)
    float freeRateHz = 1.0f;      // 0.05 to 20.0 Hz
    float waveform = 0.0f;        // 0: Sine, 1: Triangle, 2: Saw Up, 3: Saw Down, 4: Square, 5: Random Steps, 6: Smooth Random Walk
    float depth = 0.60f;          // 0.0 to 1.0 (0% to 100%)
    float centerOffset = 64.0f;   // 0 to 127
    float retriggerMode = 0.0f;   // 0: Free Run, 1: Retrigger on Note

    FastRandom rng;
    double currentSampleRate = 44100.0;
    double lfoPhase = 0.0;
    float currentRandomStep = 0.5f;
    float smoothRandomWalk = 0.5f;
    int lastSentVal = -1;
    int samplesSinceLastOutput = 0;

    double getSyncDivisionInBeats(int index) const;
    float computeLfoValue(double phase, int wave);
};

} // namespace MidiFlux
