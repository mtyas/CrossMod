#pragma once

#include "../Common/MidiBlock.h"
#include <vector>
#include <set>

namespace MidiFlux
{

class ArpeggiatorBlock : public MidiBlock
{
public:
    ArpeggiatorBlock();

    juce::String getTypeId() const override { return "arpeggiator"; }
    juce::String getDisplayName() const override { return "Arpeggiator"; }
    juce::String getCategory() const override { return "Generative"; }
    juce::Colour getAccentColor() const override { return juce::Colour(0xff38bdf8); } // Sky Blue

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

    bool isHoldActive() const { return holdMode > 0.5f; }
    void setHoldActive(bool h) { holdMode = h ? 1.0f : 0.0f; }

private:
    float arpMode = 0.0f;          // 0: Up, 1: Down, 2: Up/Down, 3: Converge, 4: Diverge, 5: Random, 6: Walk, 7: Chord
    float arpRate = 2.0f;          // 0: 1/4, 1: 1/8, 2: 1/16, 3: 1/32, 4: 1/8T, 5: 1/16T, 6: 1/8D, 7: 1/16D
    float octaveRange = 1.0f;      // 1 to 4
    float gateLength = 0.8f;       // 0.1 to 1.5
    float swing = 0.0f;            // 0.0 to 0.75
    float euclideanPulses = 16.0f; // 1 to 16
    float euclideanSteps = 16.0f;  // 1 to 16
    float skipChance = 0.0f;       // 0.0 to 0.8
    float mutateChance = 0.15f;    // 0.0 to 1.0
    float holdMode = 0.0f;         // 0: Off, 1: On (Latch)
    float syncMode = 0.0f;         // 0: Key / Always, 1: Host Transport Only

    FastRandom rng;
    double currentSampleRate = 44100.0;
    double phaseInQuarterNotes = 0.0;
    int currentStepIndex = 0;
    int walkIndex = 0;
    bool upDownDirection = true; // true = up, false = down
    bool sustainPedalDown = false;

    struct HeldNote
    {
        int channel;
        int noteNumber;
        uint8_t velocity;
    };

    // Notes physically held down
    std::vector<HeldNote> physicalNotes;
    // Notes currently in the arpeggiator playing pool (influenced by sustain / hold)
    std::vector<HeldNote> arpPool;

    struct ActiveArpNote
    {
        int channel;
        int noteNumber;
        int remainingSamples;
    };
    std::vector<ActiveArpNote> activeNotes;

    double getDivisionInQuarterNotes(int rateIndex) const;
    bool isEuclideanHit(int step, int pulses, int steps) const;
    std::vector<int> buildNotePool() const;
    void silenceActiveNotes(juce::MidiBuffer& outputMidi);
};

} // namespace MidiFlux
