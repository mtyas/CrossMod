#pragma once

#include "../Common/MidiBlock.h"
#include <unordered_map>
#include <vector>

namespace MidiFlux
{

class HarmonizerBlock : public MidiBlock
{
public:
    HarmonizerBlock();

    juce::String getTypeId() const override { return "harmonizer"; }
    juce::String getDisplayName() const override { return "Harmonizer"; }
    juce::String getCategory() const override { return "Harmonic"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff9333ea); } // Purple

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
    float voice1Interval = 1.0f;   // 0: Off, 1: Diat 3rd Up, 2: Diat 3rd Down, 3: Diat 5th Up, 4: Diat 6th Up, 5: Octave Up, 6: Octave Down, 7: Fifth Up (+7)
    float voice2Interval = 0.0f;   // Same options
    float voice1VelScale = 0.85f;  // 0.1 to 1.5
    float voice2VelScale = 0.70f;  // 0.1 to 1.5
    float voiceProb = 1.0f;        // 0.0 to 1.0

    FastRandom rng;

    struct HarmonyVoiceNote
    {
        int channel;
        int noteNumber;
    };
    std::unordered_map<int, std::vector<HarmonyVoiceNote>> activeVoices;

    int calcVoicePitch(int rootNote, int mode, const BlockContext& ctx) const;
};

} // namespace MidiFlux
