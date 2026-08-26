#pragma once

#include "../Common/MidiBlock.h"
#include <unordered_map>
#include <vector>

namespace MidiFlux
{

class ChordBlock : public MidiBlock
{
public:
    ChordBlock();

    juce::String getTypeId() const override { return "chords"; }
    juce::String getDisplayName() const override { return "Chord Generator"; }
    juce::String getCategory() const override { return "Harmonic"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xffa855f7); } // Electric Purple

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
    float chordType = 0.0f;     // 0: Diatonic Auto, 1: Maj, 2: Min, 3: Dom7, 4: Maj7, 5: Min7, 6: Sus2, 7: Sus4, 8: Dim, 9: 9th, 10: Power
    float inversion = 0.0f;     // 0: Root, 1: 1st, 2: 2nd, 3: 3rd, 4: Random
    float voicing = 0.0f;       // 0: Close, 1: Drop-2, 2: Drop-3, 3: Spread Open
    float strumSpeedMs = 20.0f; // 0 - 100 ms
    float strumDirection = 0.0f;// 0: Up, 1: Down, 2: Alternate, 3: Random
    float strumVelocityRamp = 0.0f; // -0.5 to +0.5
    float randomDropChance = 0.0f;  // 0.0 to 0.5

    FastRandom rng;
    double currentSampleRate = 44100.0;
    bool altDirection = false;

    // Track which generated chord notes belong to which input note
    // Key = (channel << 8) | rootNoteNumber
    struct GeneratedNote
    {
        int channel;
        int noteNumber;
    };
    std::unordered_map<int, std::vector<GeneratedNote>> activeChords;

    struct DelayedStrumNote
    {
        int channel;
        int noteNumber;
        uint8_t velocity;
        int samplesRemaining;
        int chordKey;
    };
    std::vector<DelayedStrumNote> delayedStrumNotes;
};

} // namespace MidiFlux
