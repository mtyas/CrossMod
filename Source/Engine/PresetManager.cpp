#include "PresetManager.h"

namespace MidiFlux
{

std::vector<Preset> PresetManager::getFactoryPresets()
{
    std::vector<Preset> presets;

    // Preset 1: Generative Ambient Arp
    {
        juce::ValueTree vt("MidiFluxState");
        vt.setProperty("rootKey", 0, nullptr);   // C
        vt.setProperty("scaleType", 0, nullptr); // Major
        vt.setProperty("masterBypassed", false, nullptr);

        juce::ValueTree blocks("Blocks");

        // Chords
        juce::ValueTree b1("Block");
        b1.setProperty("type", "chords", nullptr);
        b1.setProperty("bypassed", false, nullptr);
        b1.setProperty("chordType", 0.0f, nullptr); // Diatonic Auto
        b1.setProperty("voicing", 3.0f, nullptr);   // Spread Open
        b1.setProperty("strumSpeed", 30.0f, nullptr);
        blocks.addChild(b1, -1, nullptr);

        // Arp
        juce::ValueTree b2("Block");
        b2.setProperty("type", "arpeggiator", nullptr);
        b2.setProperty("bypassed", false, nullptr);
        b2.setProperty("arpMode", 6.0f, nullptr);     // Brownian Walk
        b2.setProperty("arpRate", 6.0f, nullptr);     // 1/8D
        b2.setProperty("octaveRange", 2.0f, nullptr);
        b2.setProperty("euclideanPulses", 5.0f, nullptr);
        b2.setProperty("euclideanSteps", 8.0f, nullptr);
        b2.setProperty("mutateChance", 0.25f, nullptr);
        blocks.addChild(b2, -1, nullptr);

        // Mutator
        juce::ValueTree b3("Block");
        b3.setProperty("type", "mutator", nullptr);
        b3.setProperty("bypassed", false, nullptr);
        b3.setProperty("pitchMutate", 0.25f, nullptr);
        b3.setProperty("octaveJump", 0.15f, nullptr);
        b3.setProperty("rhythmSlipMs", 20.0f, nullptr);
        blocks.addChild(b3, -1, nullptr);

        // Delay
        juce::ValueTree b4("Block");
        b4.setProperty("type", "delay", nullptr);
        b4.setProperty("bypassed", false, nullptr);
        b4.setProperty("delayTime", 3.0f, nullptr);  // 1/8D
        b4.setProperty("repeats", 4.0f, nullptr);
        b4.setProperty("decay", 0.60f, nullptr);
        b4.setProperty("pitchShift", 7.0f, nullptr); // Rising fifths
        blocks.addChild(b4, -1, nullptr);

        // Humanizer
        juce::ValueTree b5("Block");
        b5.setProperty("type", "humanizer", nullptr);
        b5.setProperty("bypassed", false, nullptr);
        b5.setProperty("timeJitter", 18.0f, nullptr);
        b5.setProperty("pitchDrift", 0.15f, nullptr);
        blocks.addChild(b5, -1, nullptr);

        vt.addChild(blocks, -1, nullptr);
        presets.push_back({ "Generative Ambient Arp", vt });
    }

    // Preset 2: Neo-Soul Strummer
    {
        juce::ValueTree vt("MidiFluxState");
        vt.setProperty("rootKey", 2, nullptr);   // D
        vt.setProperty("scaleType", 4, nullptr); // Dorian
        vt.setProperty("masterBypassed", false, nullptr);

        juce::ValueTree blocks("Blocks");

        juce::ValueTree b1("Block");
        b1.setProperty("type", "chords", nullptr);
        b1.setProperty("bypassed", false, nullptr);
        b1.setProperty("chordType", 0.0f, nullptr); // Diatonic Auto
        b1.setProperty("voicing", 1.0f, nullptr);   // Drop-2
        b1.setProperty("strumSpeed", 35.0f, nullptr);
        b1.setProperty("strumDir", 2.0f, nullptr);  // Alternate
        b1.setProperty("velRamp", -0.2f, nullptr);
        blocks.addChild(b1, -1, nullptr);

        juce::ValueTree b2("Block");
        b2.setProperty("type", "humanizer", nullptr);
        b2.setProperty("bypassed", false, nullptr);
        b2.setProperty("timeJitter", 22.0f, nullptr);
        b2.setProperty("pushPull", 8.0f, nullptr);   // Laid-back
        b2.setProperty("velJitter", 16.0f, nullptr);
        blocks.addChild(b2, -1, nullptr);

        vt.addChild(blocks, -1, nullptr);
        presets.push_back({ "Neo-Soul Strummer", vt });
    }

    // Preset 3: Glitch Mutator & Ratchet
    {
        juce::ValueTree vt("MidiFluxState");
        vt.setProperty("rootKey", 9, nullptr);   // A
        vt.setProperty("scaleType", 1, nullptr); // Minor
        vt.setProperty("masterBypassed", false, nullptr);

        juce::ValueTree blocks("Blocks");

        juce::ValueTree b1("Block");
        b1.setProperty("type", "mutator", nullptr);
        b1.setProperty("bypassed", false, nullptr);
        b1.setProperty("pitchMutate", 0.50f, nullptr);
        b1.setProperty("octaveJump", 0.30f, nullptr);
        b1.setProperty("rhythmSlip", 0.40f, nullptr);
        b1.setProperty("rhythmSlipMs", 45.0f, nullptr);
        blocks.addChild(b1, -1, nullptr);

        juce::ValueTree b2("Block");
        b2.setProperty("type", "ratchet", nullptr);
        b2.setProperty("bypassed", false, nullptr);
        b2.setProperty("ratchetChance", 0.60f, nullptr);
        b2.setProperty("subdiv", 5.0f, nullptr);    // Random
        b2.setProperty("burstRate", 1.0f, nullptr); // 1/32
        blocks.addChild(b2, -1, nullptr);

        juce::ValueTree b3("Block");
        b3.setProperty("type", "probability", nullptr);
        b3.setProperty("bypassed", false, nullptr);
        b3.setProperty("gateProb", 0.80f, nullptr);
        b3.setProperty("ghostNoteProb", 0.15f, nullptr);
        blocks.addChild(b3, -1, nullptr);

        vt.addChild(blocks, -1, nullptr);
        presets.push_back({ "Glitch Mutator & Ratchet", vt });
    }

    // Preset 4: Acid Cyber-Arp & CC LFO
    {
        juce::ValueTree vt("MidiFluxState");
        vt.setProperty("rootKey", 0, nullptr);
        vt.setProperty("scaleType", 1, nullptr); // Minor
        vt.setProperty("masterBypassed", false, nullptr);

        juce::ValueTree blocks("Blocks");

        juce::ValueTree b1("Block");
        b1.setProperty("type", "arpeggiator", nullptr);
        b1.setProperty("bypassed", false, nullptr);
        b1.setProperty("arpMode", 2.0f, nullptr);     // Up/Down
        b1.setProperty("arpRate", 2.0f, nullptr);     // 1/16
        b1.setProperty("octaveRange", 2.0f, nullptr);
        b1.setProperty("gateLength", 0.65f, nullptr);
        b1.setProperty("mutateChance", 0.20f, nullptr);
        blocks.addChild(b1, -1, nullptr);

        juce::ValueTree b2("Block");
        b2.setProperty("type", "lfo", nullptr);
        b2.setProperty("bypassed", false, nullptr);
        b2.setProperty("target", 2.0f, nullptr);     // Cutoff
        b2.setProperty("waveform", 0.0f, nullptr);   // Sine
        b2.setProperty("syncRate", 3.0f, nullptr);   // 1/2
        b2.setProperty("depth", 0.75f, nullptr);
        blocks.addChild(b2, -1, nullptr);

        vt.addChild(blocks, -1, nullptr);
        presets.push_back({ "Acid Cyber-Arp & LFO", vt });
    }

    // Preset 5: Euclidean Polyrhythms
    {
        juce::ValueTree vt("MidiFluxState");
        vt.setProperty("rootKey", 0, nullptr);
        vt.setProperty("scaleType", 0, nullptr);
        vt.setProperty("masterBypassed", false, nullptr);

        juce::ValueTree blocks("Blocks");

        juce::ValueTree b1("Block");
        b1.setProperty("type", "euclidean", nullptr);
        b1.setProperty("bypassed", false, nullptr);
        b1.setProperty("pulses", 5.0f, nullptr);
        b1.setProperty("steps", 8.0f, nullptr);
        b1.setProperty("rate", 2.0f, nullptr);
        blocks.addChild(b1, -1, nullptr);

        juce::ValueTree b2("Block");
        b2.setProperty("type", "harmonizer", nullptr);
        b2.setProperty("bypassed", false, nullptr);
        b2.setProperty("v1Interval", 3.0f, nullptr); // Diat 5th Up
        b2.setProperty("v2Interval", 6.0f, nullptr); // -1 Octave
        blocks.addChild(b2, -1, nullptr);

        vt.addChild(blocks, -1, nullptr);
        presets.push_back({ "Euclidean Polyrhythms", vt });
    }

    return presets;
}

void PresetManager::applyPreset(MidiChainProcessor& chain, int presetIndex)
{
    auto presets = getFactoryPresets();
    if (presetIndex >= 0 && presetIndex < static_cast<int>(presets.size()))
    {
        chain.setState(presets[presetIndex].state);
    }
}

} // namespace MidiFlux
