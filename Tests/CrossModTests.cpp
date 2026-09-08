#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <iostream>
#include <cassert>
#include <cmath>

#include "../Source/CrossMod/DSP/CrossModVoice.h"
#include "../Source/CrossMod/DSP/CrossModEngine.h"
#include "../Source/CrossMod/DSP/VintageFilter.h"
#include "../Source/CrossMod/DSP/ModMatrix.h"
#include "../Source/CrossMod/Presets/FactoryPresets.h"
#include "../Source/CrossMod/Presets/PresetManager.h"
#include "../Source/CrossMod/MIDI/MidiLearnManager.h"

static int g_testFailures = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            g_testFailures++; \
        } else { \
            std::cout << "PASS: " << msg << "\n"; \
        } \
    } while (0)

void testCrossModVoiceStability()
{
    std::cout << "\n=== Testing CrossModVoice Stability & Cross-Modulation ===\n";
    CrossModVoice voice;
    voice.setSampleRate(48000.0);
    voice.setEnvelopes(0.001f, 0.1f, 0.8f, 0.1f,
                       0.001f, 0.1f, 0.8f, 0.1f,
                       0.001f, 0.1f, 0.8f, 0.1f);

    TEST_ASSERT(!voice.isActive(), "Voice starts inactive");

    voice.noteOn(60, 0.9f, false);
    TEST_ASSERT(voice.isActive(), "Voice is active after noteOn");
    TEST_ASSERT(voice.getActiveMidiNote() == 60, "Voice records MIDI note 60");

    // Render with heavy bidirectional cross modulation and filter drive
    ModDestinationOffsets offsets;
    float outL = 0.0f, outR = 0.0f;
    bool hasNonZero = false;
    bool allFinite = true;

    for (int i = 0; i < 4800; ++i)
    {
        voice.renderNextSample(outL, outR, offsets,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.75f,
                               CrossModMode::FM,
                               OscWaveform::Sine, 0, 0.0f, 0.8f,
                               OscWaveform::Sine, 7, 0.0f, 0.8f,
                               OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                               0.85f, 0.9f, // Extreme cross-mod 1->2 and 2->1
                               0.0f,
                               2500.0f, 0.75f, FilterType::Lowpass24,
                               0.5f, 0.3f, 0.4f,
                               0.8f, 0.0f, 0.2f);

        if (!std::isfinite(outL) || !std::isfinite(outR))
        {
            allFinite = false;
            break;
        }

        if (std::abs(outL) > 0.0001f || std::abs(outR) > 0.0001f)
            hasNonZero = true;
    }

    TEST_ASSERT(allFinite, "All audio samples remain strictly finite under extreme cross-mod");
    TEST_ASSERT(hasNonZero, "Voice produces audio output signal");

    voice.noteOff();
    // Render until release completes
    for (int i = 0; i < 48000; ++i)
    {
        voice.renderNextSample(outL, outR, offsets,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.75f,
                               CrossModMode::FM,
                               OscWaveform::Sine, 0, 0.0f, 0.8f,
                               OscWaveform::Sine, 7, 0.0f, 0.8f,
                               OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                               0.0f, 0.0f, 0.0f,
                               10000.0f, 0.1f, FilterType::Lowpass24,
                               0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f);
        if (!voice.isActive())
            break;
    }
    TEST_ASSERT(!voice.isActive(), "Voice returns to idle after release");
}

void testVintageFilter()
{
    std::cout << "\n=== Testing VintageFilter Modes & Stability ===\n";
    VintageFilter filter;
    filter.setSampleRate(44100.0);

    // Test Lowpass 24
    filter.setParameters(1000.0f, 0.5f, FilterType::Lowpass24, 0.2f);
    bool stable = true;
    for (int i = 0; i < 1000; ++i)
    {
        float in = (i % 2 == 0) ? 0.5f : -0.5f;
        float out = filter.processSample(in);
        if (!std::isfinite(out) || std::abs(out) > 10.0f)
        {
            stable = false;
            break;
        }
    }
    TEST_ASSERT(stable, "Filter LP24 is stable");

    // Test Self-oscillation stability under extreme resonance
    filter.setParameters(440.0f, 0.98f, FilterType::Lowpass24, 0.5f);
    for (int i = 0; i < 1000; ++i)
    {
        float in = (i == 0) ? 1.0f : 0.0f; // impulse
        float out = filter.processSample(in);
        if (!std::isfinite(out))
        {
            stable = false;
            break;
        }
    }
    TEST_ASSERT(stable, "Filter remains stable under high resonance impulse");
}

void testModMatrixRouting()
{
    std::cout << "\n=== Testing Modulation Matrix Routing & Scaling ===\n";
    ModMatrix matrix;

    // Slot 0: LFO 1 -> Filter Cutoff with +0.5 amount
    matrix.setSlot(0, ModSource::LFO1, ModDestination::FilterCutoff, 0.5f);
    // Slot 1: Mod Env -> Osc 1 Pitch with +0.25 amount
    matrix.setSlot(1, ModSource::ModEnv, ModDestination::Osc1Pitch, 0.25f);
    // Slot 2: Velocity -> CrossMod 1->2 with +0.8 amount
    matrix.setSlot(2, ModSource::Velocity, ModDestination::CrossMod1to2, 0.8f);

    ModSourceValues sources;
    sources.lfo1 = 1.0f;
    sources.modEnv = 0.8f;
    sources.velocity = 0.5f;

    ModDestinationOffsets offsets;
    matrix.evaluate(sources, offsets);

    // LFO1 (1.0) * 0.5 * 5.0 octaves = 2.5 octaves
    TEST_ASSERT(std::abs(offsets.filterCutoffOctaves - 2.5f) < 0.001f,
                "Filter cutoff modulation scaled accurately");

    // ModEnv (0.8) * 0.25 * 24.0 st = 4.8 semitones
    TEST_ASSERT(std::abs(offsets.osc1PitchSemis - 4.8f) < 0.001f,
                "Osc 1 pitch modulation scaled accurately");

    // Velocity (0.5) * 0.8 = 0.4 cross-mod
    TEST_ASSERT(std::abs(offsets.crossMod1to2 - 0.4f) < 0.001f,
                "Cross-mod 1->2 modulation scaled accurately");

    // Test FX parameter modulation in matrix
    matrix.setSlot(3, ModSource::LFO2, ModDestination::DelayFxTime, 0.5f);
    matrix.setSlot(4, ModSource::ModWheel, ModDestination::ReverbFxDecay, 0.5f);
    sources.lfo2 = 1.0f;
    sources.modWheel = 1.0f;
    matrix.evaluate(sources, offsets);

    // LFO2 (1.0) * 0.5 * 0.8s = 0.4s delay time mod
    TEST_ASSERT(std::abs(offsets.delayFxTime - 0.4f) < 0.001f, "Delay time modulation scaled accurately");
    // ModWheel (1.0) * 0.5 * 4.0s = 2.0s reverb decay mod
    TEST_ASSERT(std::abs(offsets.reverbFxDecay - 2.0f) < 0.001f, "Reverb decay modulation scaled accurately");

    // Test Glide & Width modulation
    matrix.setSlot(5, ModSource::Velocity, ModDestination::Osc1Glide, 0.4f);
    matrix.setSlot(6, ModSource::KeyTrack, ModDestination::Osc2Glide, -0.3f);
    matrix.setSlot(7, ModSource::ModWheel, ModDestination::StereoWidth, 0.6f);
    sources.velocity = 0.5f;
    sources.keyTrack = 1.0f;
    sources.modWheel = 0.8f;
    matrix.evaluate(sources, offsets);
    TEST_ASSERT(std::abs(offsets.osc1Glide - 0.2f) < 0.001f, "Osc 1 glide modulation routed accurately");
    TEST_ASSERT(std::abs(offsets.osc2Glide - (-0.3f)) < 0.001f, "Osc 2 glide modulation routed accurately");
    TEST_ASSERT(std::abs(offsets.stereoWidth - 0.48f) < 0.001f, "Stereo width modulation routed accurately");
}

void testEnginePolyphonyModes()
{
    std::cout << "\n=== Testing Engine Voice Modes (Mono, Poly, Voice Cross-Mod) ===\n";
    CrossModEngine engine;
    engine.prepare(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 256);

    // Mode 1: Mono Mode note priority stack & legato
    engine.noteOn(60, 0.8f);
    TEST_ASSERT((engine.getActiveVoiceMask() & 1u) != 0, "Voice 0 active in mono mode for note 60");

    engine.noteOn(67, 0.8f); // Legato to note 67
    TEST_ASSERT((engine.getActiveVoiceMask() & 1u) != 0, "Voice 0 remains active for legato note 67");

    engine.noteOff(67); // Releasing 67 should fall back to 60
    TEST_ASSERT((engine.getActiveVoiceMask() & 1u) != 0, "Voice 0 returns to note 60 in note stack");

    engine.noteOff(60);
    // Render release
    buffer.clear();
    engine.processBlock(buffer, VoiceMode::Mono, CrossModMode::FM, 0.05f, 0.05f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

    // Mode 2: Poly Multi-Mono
    engine.reset();
    engine.setVoiceMode(VoiceMode::PolyMultiMono);
    engine.noteOn(60, 0.8f);
    engine.noteOn(64, 0.8f);
    engine.noteOn(67, 0.8f);
    uint32_t mask3 = engine.getActiveVoiceMask();
    int activeCount = 0;
    for (int v = 0; v < 16; ++v)
        if (mask3 & (1u << v)) activeCount++;
    TEST_ASSERT(activeCount == 3, "Poly Multi-Mono mode allocated 3 distinct voices");

    // Render Poly Multi-Mono
    buffer.clear();
    engine.processBlock(buffer, VoiceMode::PolyMultiMono, CrossModMode::FM, 0.05f, 0.05f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.2f, 0.2f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);
    float energyMultiMono = buffer.getMagnitude(0, 0, 256);
    TEST_ASSERT(energyMultiMono > 0.001f, "Poly Multi-Mono renders audio block");

    // Mode 3: Poly Voice-CrossMod (CyclicRing)
    buffer.clear();
    engine.setVoiceMode(VoiceMode::CyclicRing);
    engine.processBlock(buffer, VoiceMode::CyclicRing, CrossModMode::FM, 0.05f, 0.05f, 0.6f, // Active inter-voice cross-mod!
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 7, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.3f, 0.3f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);
    float energyXVoice = buffer.getMagnitude(0, 0, 256);
    TEST_ASSERT(energyXVoice > 0.001f, "Poly Voice-CrossMod renders coupled voice audio block");
    TEST_ASSERT(std::isfinite(energyXVoice), "Inter-voice cross-modulation is strictly numerically stable");
}

void testFxChain()
{
    std::cout << "\n=== Testing Studio Multi-FX Chain (Modulation, Delay, Reverb, Order) ===\n";
    FxChain chain;
    chain.prepare(44100.0);

    constexpr int N = 512;
    std::vector<float> l(N, 0.0f);
    std::vector<float> r(N, 0.0f);

    // Feed a burst into Left and Right
    for (int i = 0; i < 64; ++i)
    {
        l[i] = std::sin((float)i * 0.2f);
        r[i] = std::cos((float)i * 0.2f);
    }

    // Process with Chorus, Tape Delay, and Hall Reverb
    chain.process(l.data(), r.data(), N,
                  FxOrder::Mod_Dly_Rvb,
                  ModFxType::Chorus, 1.5f, 0.7f, 0.3f, 0.5f,
                  DelayFxType::Tape, 0.25f, 0.5f, 0.6f, 0.5f,
                  ReverbFxType::Hall, 3.0f, 0.4f, 0.7f, 0.4f);

    bool allFinite = true;
    float sumEnergy = 0.0f;
    for (int i = 0; i < N; ++i)
    {
        if (!std::isfinite(l[i]) || !std::isfinite(r[i]))
        {
            allFinite = false;
            break;
        }
        sumEnergy += std::abs(l[i]) + std::abs(r[i]);
    }

    TEST_ASSERT(allFinite, "FX Chain outputs strictly finite samples");
    TEST_ASSERT(sumEnergy > 0.01f, "FX Chain processes signal through all stages");

    // Test different FX orders
    chain.reset();
    for (int i = 0; i < 64; ++i) { l[i] = 0.5f; r[i] = 0.5f; }
    chain.process(l.data(), r.data(), N,
                  FxOrder::Rvb_Dly_Mod, // Reverse order
                  ModFxType::Flanger, 2.0f, 0.8f, 0.5f, 0.5f,
                  DelayFxType::PingPong, 0.15f, 0.6f, 0.7f, 0.5f,
                  ReverbFxType::Plate, 1.5f, 0.2f, 0.8f, 0.5f);
    TEST_ASSERT(std::isfinite(l[0]) && std::isfinite(r[0]), "Reverse FX Order (Rvb > Dly > Mod) is stable");
}

void testFactoryPresets()
{
    std::cout << "\n=== Testing Factory Presets ===\n";
    auto presets = FactoryPresets::getPresets();
    TEST_ASSERT(!presets.empty(), "Factory presets list is populated");
    TEST_ASSERT(presets.size() >= 11, "Has at least 11 handcrafted factory presets");

    for (const auto& p : presets)
    {
        TEST_ASSERT(!p.name.isEmpty(), "Preset has valid name: " + p.name.toStdString());
        TEST_ASSERT(!p.values.empty(), "Preset contains parameter values");
    }
}


class DummyTestProcessor : public juce::AudioProcessor
{
public:
    DummyTestProcessor() : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}
    const juce::String getName() const override { return "Dummy"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

void testPresetManager()
{
    std::cout << "\n=== Testing PresetManager File Operations ===\n";
    DummyTestProcessor proc;
    juce::AudioProcessorValueTreeState apvts(proc, nullptr, "Parameters", ParameterFactory::createParameterLayout());
    PresetManager pm(apvts);
    pm.initialize();

    TEST_ASSERT(pm.getNumPresets() >= 8, "PresetManager initialized with factory presets");
    TEST_ASSERT(pm.getPresetsDirectory().exists(), "Presets directory exists on disk");

    // Test saving a new user preset
    juce::String userPresetName = "ZZZ_TestUserPreset";
    bool saved = pm.saveCurrentPreset(userPresetName);
    TEST_ASSERT(saved, "Successfully saved custom user preset to disk");

    pm.rescanPresets();
    bool foundInList = false;
    for (int i = 0; i < pm.getNumPresets(); ++i)
    {
        if (pm.getPresetName(i) == userPresetName)
        {
            foundInList = true;
            break;
        }
    }
    TEST_ASSERT(foundInList, "Newly saved preset appears in preset list / roledown");

    // Clean up test preset file
    auto testFile = pm.getPresetsDirectory().getChildFile(userPresetName + ".crossmod");
    if (testFile.existsAsFile())
        testFile.deleteFile();
}

void testBpmSync()
{
    std::cout << "\n=== Testing BPM Sync for LFO and Delay ===\n";

    LFO lfo;
    lfo.setSampleRate(44100.0);
    lfo.setShape(LFOShape::Sine);

    // 120 BPM:
    // 1/4 note (index 8) = 1 beat = 0.5s -> 2.0 Hz
    lfo.setBpmAndSync(120.0, true, 8);
    int zeroCrossings = 0;
    float lastVal = 0.0f;
    for (int i = 0; i < 44100; ++i)
    {
        float s = lfo.getNextSample();
        if (lastVal < 0.0f && s >= 0.0f) zeroCrossings++;
        lastVal = s;
    }
    TEST_ASSERT(zeroCrossings == 2, "LFO at 120 BPM synced to 1/4 note completes 2 cycles per second (2 Hz)");

    // 1/8 note (index 5) = 0.5 beat = 0.25s -> 4.0 Hz
    lfo.setBpmAndSync(120.0, true, 5);
    zeroCrossings = 0;
    lastVal = 0.0f;
    for (int i = 0; i < 44100; ++i)
    {
        float s = lfo.getNextSample();
        if (lastVal < 0.0f && s >= 0.0f) zeroCrossings++;
        lastVal = s;
    }
    TEST_ASSERT(zeroCrossings == 4, "LFO at 120 BPM synced to 1/8 note completes 4 cycles per second (4 Hz)");

    // Delay BPM Sync
    DelayEffect delay;
    delay.prepare(44100.0);
    delay.setBpmAndSync(120.0, true, 8); // 1/4 note = 500ms
    std::vector<float> l(256, 0.0f), r(256, 0.0f);
    l[0] = 1.0f;
    delay.process(l.data(), r.data(), 256, DelayFxType::Digital, 0.1f, 0.0f, 0.5f, 1.0f);
    TEST_ASSERT(std::isfinite(l[0]), "Delay operates cleanly with BPM sync enabled");
}

void testDualAndPolyGlide()
{
    std::cout << "\n=== Testing Dual Oscillator & Polyphonic Glide ===\n";

    CrossModEngine engine;
    engine.prepare(44100.0);
    engine.setVoiceMode(VoiceMode::PolyMultiMono);

    // 1. Play first note: C4 (60)
    engine.noteOn(60, 0.8f);
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    // Fast glide for Osc 1 (0.01s), Slow glide for Osc 2 (0.4s)
    engine.processBlock(buffer, VoiceMode::PolyMultiMono, CrossModMode::FM, 0.01f, 0.40f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

    // 2. Play polyphonic second note: G4 (67)
    engine.noteOn(67, 0.8f);
    buffer.clear();
    engine.processBlock(buffer, VoiceMode::PolyMultiMono, CrossModMode::FM, 0.01f, 0.40f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

    float mag = buffer.getMagnitude(0, 0, 512);
    TEST_ASSERT(mag > 0.001f, "Polyphonic glide audio renders smoothly");
    TEST_ASSERT(std::isfinite(mag), "Polyphonic glide is strictly finite");
}

void testStereoSeparationAndWidth()
{
    std::cout << "\n=== Testing Stereo Oscillator Panning & Width Control ===\n";

    CrossModEngine engine;
    engine.prepare(44100.0, 512);
    engine.setVoiceMode(VoiceMode::Mono);

    // Test 1: Full Stereo Width (1.0) with only Oscillator 1 active
    engine.reset();
    engine.noteOn(60, 1.0f);
    juce::AudioBuffer<float> bufOsc1(2, 512);
    bufOsc1.clear();
    engine.processBlock(bufOsc1, VoiceMode::Mono, CrossModMode::FM, 0.0f, 0.0f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.0f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 1.0f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

    float magL1 = bufOsc1.getMagnitude(0, 0, 512);
    float magR1 = bufOsc1.getMagnitude(1, 0, 512);
    TEST_ASSERT(magL1 > 0.05f, "Oscillator 1 has strong signal on Left channel at 100% width");
    TEST_ASSERT(magR1 < 0.0001f, "Oscillator 1 is silent on Right channel at 100% width");

    // Test 2: Full Stereo Width (1.0) with only Oscillator 2 active
    engine.reset();
    engine.noteOn(60, 1.0f);
    juce::AudioBuffer<float> bufOsc2(2, 512);
    bufOsc2.clear();
    engine.processBlock(bufOsc2, VoiceMode::Mono, CrossModMode::FM, 0.0f, 0.0f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 1.0f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

    float magL2 = bufOsc2.getMagnitude(0, 0, 512);
    float magR2 = bufOsc2.getMagnitude(1, 0, 512);
    TEST_ASSERT(magR2 > 0.05f, "Oscillator 2 has strong signal on Right channel at 100% width");
    TEST_ASSERT(magL2 < 0.0001f, "Oscillator 2 is silent on Left channel at 100% width");

    // Test 3: Mono Width (0.0) -> Left and Right channels must be equal
    engine.reset();
    engine.noteOn(60, 1.0f);
    juce::AudioBuffer<float> bufMono(2, 512);
    bufMono.clear();
    engine.processBlock(bufMono, VoiceMode::Mono, CrossModMode::FM, 0.0f, 0.0f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.0f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

    float magMonoL = bufMono.getMagnitude(0, 0, 512);
    float magMonoR = bufMono.getMagnitude(1, 0, 512);
    TEST_ASSERT(std::abs(magMonoL - magMonoR) < 0.0001f, "Mono width (0%) creates identical Left and Right output");
}

void testCrossModModesAndKeyTrack()
{
    std::cout << "\n=== Testing 1V/Oct Key Tracking, Audio-Rate LFOs & Cross-Mod Modes ===\n";

    // 1. Test 1V/Oct Filter Key Tracking at Track = 0.5
    {
        CrossModVoice voice;
        voice.setSampleRate(48000.0);
        voice.setEnvelopes(0.001f, 0.1f, 1.0f, 0.1f,
                           0.001f, 0.1f, 1.0f, 0.1f,
                           0.001f, 0.1f, 1.0f, 0.1f);
        ModDestinationOffsets offsets;

        // Note 60 (C4)
        voice.noteOn(60, 1.0f, false);
        float outL = 0.0f, outR = 0.0f;
        int zeroCrossings60 = 0;
        float lastL = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            voice.renderNextSample(outL, outR, offsets,
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                   CrossModMode::FM,
                                   OscWaveform::Sine, 0, 0.0f, 0.0f,
                                   OscWaveform::Sine, 0, 0.0f, 0.0f,
                                   OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                                   0.0f, 0.0f, 0.0f,
                                   440.0f, 1.0f, FilterType::Lowpass24,
                                   0.1f, 0.5f, 0.0f, // 0.5 KeyTrack = 1V/Oct
                                   1.0f, 0.0f, 0.0f);
            if (i > 9600 && lastL < 0.0f && outL >= 0.0f)
                zeroCrossings60++;
            lastL = outL;
        }

        // Note 72 (C5 -> 1 octave higher -> 2x frequency expected)
        voice.reset();
        voice.noteOn(72, 1.0f, false);
        int zeroCrossings72 = 0;
        lastL = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            voice.renderNextSample(outL, outR, offsets,
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                   CrossModMode::FM,
                                   OscWaveform::Sine, 0, 0.0f, 0.0f,
                                   OscWaveform::Sine, 0, 0.0f, 0.0f,
                                   OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                                   0.0f, 0.0f, 0.0f,
                                   440.0f, 1.0f, FilterType::Lowpass24,
                                   0.1f, 0.5f, 0.0f, // 0.5 KeyTrack = 1V/Oct
                                   1.0f, 0.0f, 0.0f);
            if (i > 9600 && lastL < 0.0f && outL >= 0.0f)
                zeroCrossings72++;
            lastL = outL;
        }

        float freqRatio = (float)zeroCrossings72 / (float)zeroCrossings60;
        TEST_ASSERT(std::abs(freqRatio - 2.0f) < 0.05f,
                    "Self-oscillating filter key tracking at Track=0.5 doubles frequency (+1 octave) over 12 semitones");
    }

    // 2. Test 1V/Oct LFO Pitch Tracking via Mod Matrix and Audio-Rate Range (up to 2000 Hz)
    {
        LFO lfo;
        lfo.setSampleRate(48000.0);
        lfo.setShape(LFOShape::Sine);
        lfo.setRateHz(440.0f);

        int zc60 = 0;
        float lastVal = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            float s = lfo.getNextSample(0.0f);
            if (lastVal < 0.0f && s >= 0.0f) zc60++;
            lastVal = s;
        }
        TEST_ASSERT(std::abs(zc60 - 440) <= 1, "Base audio-rate LFO runs cleanly at 440 Hz");

        // At note 72: offset = 0.5 -> 2.0x -> 880 Hz
        int zc72 = 0;
        lastVal = 0.0f;
        lfo.resetPhase();
        for (int i = 0; i < 48000; ++i)
        {
            float s = lfo.getNextSample(0.5f);
            if (lastVal < 0.0f && s >= 0.0f) zc72++;
            lastVal = s;
        }
        TEST_ASSERT(std::abs(zc72 - 880) <= 1, "LFO key tracking in Mod Matrix with amount=0.5 tracks exact 1V/Oct pitch (880 Hz at C5)");

        // Audio-rate upper limit test: 2000 Hz
        lfo.setRateHz(2000.0f);
        int zc2000 = 0;
        lastVal = 0.0f;
        for (int i = 0; i < 48000; ++i)
        {
            float s = lfo.getNextSample(0.0f);
            if (lastVal < 0.0f && s >= 0.0f) zc2000++;
            lastVal = s;
        }
        TEST_ASSERT(zc2000 == 2000, "LFO reaches full 2000 Hz audio rate");
    }

    // 3. Test All 5 Cross-Modulation Modes (FM, PhaseMod, ThroughZeroFM, AM, RingMod)
    {
        CrossModVoice voice;
        voice.setSampleRate(48000.0);
        voice.setEnvelopes(0.001f, 0.1f, 0.8f, 0.1f,
                           0.001f, 0.1f, 0.8f, 0.1f,
                           0.001f, 0.1f, 0.8f, 0.1f);
        ModDestinationOffsets offsets;

        std::vector<CrossModMode> modes = {
            CrossModMode::FM,
            CrossModMode::PhaseMod,
            CrossModMode::ThroughZeroFM,
            CrossModMode::AM,
            CrossModMode::RingMod
        };

        for (auto mode : modes)
        {
            voice.reset();
            voice.noteOn(60, 0.9f, false);
            float outL = 0.0f, outR = 0.0f;
            bool finite = true;
            float totalMag = 0.0f;

            for (int i = 0; i < 2400; ++i)
            {
                voice.renderNextSample(outL, outR, offsets,
                                       0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.75f,
                                       mode,
                                       OscWaveform::Sine, 0, 0.0f, 0.8f,
                                       OscWaveform::Sine, 7, 0.0f, 0.8f,
                                       OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                                       0.75f, 0.75f,
                                       0.0f,
                                       12000.0f, 0.2f, FilterType::Lowpass24,
                                       0.1f, 0.5f, 0.0f,
                                       0.8f, 0.0f, 0.2f);
                if (!std::isfinite(outL) || !std::isfinite(outR))
                {
                    finite = false;
                    break;
                }
                totalMag += std::abs(outL) + std::abs(outR);
            }
            TEST_ASSERT(finite, "CrossModMode " + std::to_string(static_cast<int>(mode)) + " is strictly numerically finite");
            TEST_ASSERT(totalMag > 1.0f, "CrossModMode " + std::to_string(static_cast<int>(mode)) + " generates audio energy");
        }
    }

    // 4. Test All 4 Voice Cross-Modulation Algorithms (CyclicRing, SympatheticAll, RootDriver, ChaosDiffuse)
    {
        CrossModEngine engine;
        engine.prepare(48000.0, 512);

        std::vector<VoiceMode> algos = {
            VoiceMode::CyclicRing,
            VoiceMode::SympatheticAll,
            VoiceMode::RootDriver,
            VoiceMode::ChaosDiffuse
        };

        for (auto algo : algos)
        {
            engine.reset();
            engine.setVoiceMode(algo);
            engine.noteOn(48, 0.9f);
            engine.noteOn(52, 0.9f);
            engine.noteOn(55, 0.9f);

            juce::AudioBuffer<float> buf(2, 512);
            buf.clear();
            engine.processBlock(buf, algo, CrossModMode::FM,
                                0.0f, 0.0f, 0.75f,
                                OscWaveform::Sine, 0, 0.0f, 0.8f,
                                OscWaveform::Sine, 12, 0.0f, 0.8f,
                                OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                                0.3f, 0.3f,
                                12000.0f, 0.2f, FilterType::Lowpass24,
                                0.1f, 0.5f, 0.0f,
                                0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                                1.0f, LFOShape::Sine, false, 0, false,
                                1.0f, LFOShape::Sine, false, 0, false,
                                0.8f, 0.0f, 0.75f, 0.0f,
                                FxOrder::Mod_Dly_Rvb,
                                ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                                DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                                ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

            float magL = buf.getMagnitude(0, 0, 512);
            float magR = buf.getMagnitude(1, 0, 512);
            TEST_ASSERT(std::isfinite(magL) && std::isfinite(magR),
                        "VoiceMode algorithm " + std::to_string(static_cast<int>(algo)) + " produces strictly finite audio");
            TEST_ASSERT(magL > 0.01f && magR > 0.01f,
                        "VoiceMode algorithm " + std::to_string(static_cast<int>(algo)) + " generates active coupled stereo sound");
        }
    }
}

void testOscWaveformsAndSubOsc()
{
    std::cout << "\n=== Testing Oscillator Waveforms (Sine, Tri, Saw, Square) & Sub Oscillator ===\n";

    CrossModVoice voice;
    voice.setSampleRate(48000.0);
    voice.setEnvelopes(0.001f, 0.1f, 1.0f, 0.1f,
                       0.001f, 0.1f, 1.0f, 0.1f,
                       0.001f, 0.1f, 1.0f, 0.1f);
    ModDestinationOffsets offsets;

    // 1. Test that all waveforms for Osc 1 and Osc 2 render cleanly and are finite
    std::vector<OscWaveform> shapes = {
        OscWaveform::Sine,
        OscWaveform::Triangle,
        OscWaveform::Saw,
        OscWaveform::Square
    };

    for (auto shape1 : shapes)
    {
        for (auto shape2 : shapes)
        {
            voice.reset();
            voice.noteOn(60, 0.8f, false);
            float outL = 0.0f, outR = 0.0f;
            float totalEnergy = 0.0f;
            bool finite = true;

            for (int i = 0; i < 500; ++i)
            {
                voice.renderNextSample(outL, outR, offsets,
                                       0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
                                       CrossModMode::FM,
                                       shape1, 0, 0.0f, 0.8f,
                                       shape2, 0, 0.0f, 0.8f,
                                       OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                                       0.2f, 0.2f, 0.0f,
                                       10000.0f, 0.1f, FilterType::Lowpass24,
                                       0.0f, 0.0f, 0.0f,
                                       0.8f, 0.0f, 0.0f);

                if (!std::isfinite(outL) || !std::isfinite(outR))
                {
                    finite = false;
                    break;
                }
                totalEnergy += std::abs(outL) + std::abs(outR);
            }

            TEST_ASSERT(finite, "Osc1 shape " + std::to_string((int)shape1) + " and Osc2 shape " + std::to_string((int)shape2) + " are strictly finite");
            TEST_ASSERT(totalEnergy > 0.1f, "Osc1 shape " + std::to_string((int)shape1) + " and Osc2 shape " + std::to_string((int)shape2) + " produce sound");
        }
    }

    // 2. Test Sub Oscillator Isolation & Pitch (-1 Octave, -2 Octaves)
    // When Osc 1 and Osc 2 level are 0, Sub Osc alone produces signal
    {
        voice.reset();
        voice.noteOn(60, 1.0f, false);
        float outL = 0.0f, outR = 0.0f;
        int subZeroCrossingsMinus1 = 0;
        float lastL = 0.0f;

        // Sub Osc at -1 Octave (Middle C 60 -> C3: 130.81 Hz at 48kHz)
        for (int i = 0; i < 48000; ++i)
        {
            voice.renderNextSample(outL, outR, offsets,
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                   CrossModMode::FM,
                                   OscWaveform::Sine, 0, 0.0f, 0.0f, // Osc1 silent
                                   OscWaveform::Sine, 0, 0.0f, 0.0f, // Osc2 silent
                                   OscWaveform::Sine, SubOscOctave::OctaveMinus1, 1.0f, // Sub active!
                                   0.9f, 0.9f, 0.0f, // High cross-mod to prove Sub is isolated
                                   20000.0f, 0.0f, FilterType::Lowpass24,
                                   0.0f, 0.0f, 0.0f,
                                   1.0f, 0.0f, 0.0f);

            if (i > 4800 && lastL < 0.0f && outL >= 0.0f)
                subZeroCrossingsMinus1++;
            lastL = outL;
        }

        TEST_ASSERT(subZeroCrossingsMinus1 > 100, "Sub Oscillator renders audio when main oscillators are muted");

        // Sub Osc at -2 Octaves (C2: 65.4 Hz -> half the zero crossings of -1 Octave)
        voice.reset();
        voice.noteOn(60, 1.0f, false);
        int subZeroCrossingsMinus2 = 0;
        lastL = 0.0f;

        for (int i = 0; i < 48000; ++i)
        {
            voice.renderNextSample(outL, outR, offsets,
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                   CrossModMode::FM,
                                   OscWaveform::Sine, 0, 0.0f, 0.0f,
                                   OscWaveform::Sine, 0, 0.0f, 0.0f,
                                   OscWaveform::Sine, SubOscOctave::OctaveMinus2, 1.0f,
                                   0.9f, 0.9f, 0.0f,
                                   20000.0f, 0.0f, FilterType::Lowpass24,
                                   0.0f, 0.0f, 0.0f,
                                   1.0f, 0.0f, 0.0f);

            if (i > 4800 && lastL < 0.0f && outL >= 0.0f)
                subZeroCrossingsMinus2++;
            lastL = outL;
        }

        float octRatio = (float)subZeroCrossingsMinus1 / (float)subZeroCrossingsMinus2;
        TEST_ASSERT(std::abs(octRatio - 2.0f) < 0.05f, "Sub Osc Octave -2 is exactly one octave below Octave -1 (ratio ~ 2.0)");
    }

    // 3. Test Mod Matrix routing to SubOscLevel
    {
        ModMatrix matrix;
        matrix.setSlot(0, ModSource::ModWheel, ModDestination::SubOscLevel, 0.75f);
        ModSourceValues src;
        src.modWheel = 1.0f;
        ModDestinationOffsets dst;
        matrix.evaluate(src, dst);
        TEST_ASSERT(std::abs(dst.subOscLevel - 0.75f) < 0.001f, "SubOscLevel modulation in ModMatrix routes accurately");
    }
}

void testMidiLearn()
{
    std::cout << "\n=== Testing MidiLearnManager Real-Time Mapping & Persistence ===\n";
    MidiLearnManager mlm;

    TEST_ASSERT(!mlm.isLearning(), "MidiLearnManager starts not learning");

    // 1. Start learning mode
    mlm.startLearning("filterCutoff");
    TEST_ASSERT(mlm.isLearning(), "MidiLearnManager is in learning mode");
    TEST_ASSERT(mlm.isLearningParam("filterCutoff"), "isLearningParam correctly reports target param");
    TEST_ASSERT(mlm.getLearningParamID() == "filterCutoff", "getLearningParamID matches target");

    // Mock AudioProcessor with parameter
    struct TestAudioProcessor : public juce::AudioProcessor
    {
        TestAudioProcessor()
            : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
              apvts(*this, nullptr, "Parameters", createLayout())
        {}
        static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
        {
            std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID("filterCutoff", 1), "Cutoff", 20.0f, 20000.0f, 1000.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID("crossMod1to2", 1), "CrossMod", 0.0f, 1.0f, 0.0f));
            return { params.begin(), params.end() };
        }
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        const juce::String getName() const override { return "Test"; }
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return ""; }
        void changeProgramName(int, const juce::String&) override {}
        void getStateInformation(juce::MemoryBlock&) override {}
        void setStateInformation(const void*, int) override {}

        juce::AudioProcessorValueTreeState apvts;
    };

    TestAudioProcessor testProc;

    // 2. Intercept CC message during learning
    auto learnMsg = juce::MidiMessage::controllerEvent(1, 74, 64);
    mlm.processMidiController(learnMsg, testProc.apvts);

    TEST_ASSERT(!mlm.isLearning(), "Learning mode finishes after CC event");
    int cc = -1, ch = -1;
    bool mapped = mlm.getMappingForParam("filterCutoff", cc, ch);
    TEST_ASSERT(mapped && cc == 74, "filterCutoff successfully mapped to CC 74");

    // 3. Process normal CC control updates
    auto ctrlMsg = juce::MidiMessage::controllerEvent(1, 74, 127);
    mlm.processMidiController(ctrlMsg, testProc.apvts);
    float val = testProc.apvts.getRawParameterValue("filterCutoff")->load();
    TEST_ASSERT(val > 19000.0f, "Incoming CC 74 at max (127) scales parameter to top of range");

    // 4. Map another parameter explicitly
    mlm.setMapping("crossMod1to2", 71, 0);
    int cc2 = -1, ch2 = -1;
    TEST_ASSERT(mlm.getMappingForParam("crossMod1to2", cc2, ch2) && cc2 == 71, "Explicit mapping sets CC 71 for crossMod1to2");

    // 5. XML Serialization & Deserialization
    juce::XmlElement xml("State");
    mlm.saveToXml(xml);

    MidiLearnManager mlmRestored;
    mlmRestored.loadFromXml(xml);
    int restCC1 = -1, restCh1 = -1;
    int restCC2 = -1, restCh2 = -1;
    TEST_ASSERT(mlmRestored.getMappingForParam("filterCutoff", restCC1, restCh1) && restCC1 == 74,
                "Restored manager has filterCutoff mapped to CC 74");
    TEST_ASSERT(mlmRestored.getMappingForParam("crossMod1to2", restCC2, restCh2) && restCC2 == 71,
                "Restored manager has crossMod1to2 mapped to CC 71");

    // 6. Unassign and Clear
    mlm.removeMappingForParam("filterCutoff");
    TEST_ASSERT(!mlm.getMappingForParam("filterCutoff", cc, ch), "removeMappingForParam unmaps filterCutoff");
    mlm.clearAllMappings();
    TEST_ASSERT(mlm.getAllMappings().empty(), "clearAllMappings empties all mappings");
}

void testMonoUnison()
{
    std::cout << "\n=== Testing Mono Unison Mode (8 Voices, Detune & Stereo Spread) ===\n";
    CrossModEngine engine;
    engine.prepare(44100.0, 512);
    engine.setVoiceMode(VoiceMode::MonoUnison);

    // 1. Note On triggers 8 unison voices
    engine.noteOn(60, 0.9f);
    uint32_t mask = engine.getActiveVoiceMask();
    int count = 0;
    for (int v = 0; v < 16; ++v)
        if (mask & (1u << v)) count++;
    TEST_ASSERT(count == 8, "Mono Unison mode allocates exactly 8 unison voices (mask 0xFF)");

    // 2. Render with unison detune controlled by voiceCrossMod
    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    engine.processBlock(buf, VoiceMode::MonoUnison, CrossModMode::FM, 0.01f, 0.01f, 0.8f, // 0.8 voiceCrossMod = detune
                        OscWaveform::Saw, 0, 0.0f, 0.8f,
                        OscWaveform::Saw, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);

    float magL = buf.getMagnitude(0, 0, 512);
    float magR = buf.getMagnitude(1, 0, 512);
    TEST_ASSERT(std::isfinite(magL) && std::isfinite(magR), "Mono Unison renders strictly finite audio");
    TEST_ASSERT(magL > 0.05f && magR > 0.05f, "Mono Unison produces lush wide stereo sound");

    // 3. Legato handling in Mono Unison
    engine.noteOn(67, 0.8f);
    TEST_ASSERT((engine.getActiveVoiceMask() & 0xFF) == 0xFF, "All 8 unison voices remain active on legato transition to note 67");

    engine.noteOff(67);
    TEST_ASSERT((engine.getActiveVoiceMask() & 0xFF) == 0xFF, "All 8 unison voices fall back to note 60 in note stack");

    // 4. Release note
    engine.noteOff(60);
    // Render release
    buf.clear();
    for (int i = 0; i < 50; ++i)
    {
        engine.processBlock(buf, VoiceMode::MonoUnison, CrossModMode::FM, 0.01f, 0.01f, 0.8f,
                            OscWaveform::Saw, 0, 0.0f, 0.8f,
                            OscWaveform::Saw, 0, 0.0f, 0.8f,
                            OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                            0.0f, 0.0f,
                            10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                            0.01f, 0.01f, 0.0f, 0.01f, 0.01f, 0.01f, 0.0f, 0.01f, 0.01f, 0.01f, 0.0f, 0.01f,
                            1.0f, LFOShape::Sine, false, 0, false,
                            1.0f, LFOShape::Sine, false, 0, false,
                            0.8f, 0.0f, 0.75f, 0.0f,
                            FxOrder::Mod_Dly_Rvb,
                            ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                            DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                            ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);
    }
    TEST_ASSERT(engine.getActiveVoiceMask() == 0, "All 8 unison voices return to idle after note off and release");
}

void testPolyToMonoSwitch()
{
    std::cout << "\n=== Testing Poly to Mono Mode Switching Reliability ===\n";
    CrossModEngine engine;
    engine.prepare(44100.0, 512);

    // 1. Play chords in Poly mode
    engine.setVoiceMode(VoiceMode::PolyMultiMono);
    engine.noteOn(60, 0.8f);
    engine.noteOn(64, 0.8f);
    engine.noteOn(67, 0.8f);
    juce::AudioBuffer<float> buf(2, 256);
    buf.clear();
    engine.processBlock(buf, VoiceMode::PolyMultiMono, CrossModMode::FM, 0.0f, 0.0f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);
    TEST_ASSERT(buf.getMagnitude(0, 0, 256) > 0.01f, "Poly mode produces audio output");

    // 2. Switch to Mono mode
    engine.setVoiceMode(VoiceMode::Mono);

    // 3. Play single note in Mono mode
    engine.noteOn(62, 0.8f);
    buf.clear();
    engine.processBlock(buf, VoiceMode::Mono, CrossModMode::FM, 0.0f, 0.0f, 0.0f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                        0.0f, 0.0f,
                        10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                        0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                        1.0f, LFOShape::Sine, false, 0, false,
                        1.0f, LFOShape::Sine, false, 0, false,
                        0.8f, 0.0f, 0.75f, 0.0f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                        ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);
    float monoMag = buf.getMagnitude(0, 0, 256);
    TEST_ASSERT(monoMag > 0.01f, "Mono mode immediately and reliably emits sound after switching from Poly mode");
}

void testPolyXModAlgorithmsDiversity()
{
    std::cout << "\n=== Testing Polyphonic Cross-Modulation Algorithms Diversity ===\n";
    CrossModEngine engine;
    engine.prepare(44100.0, 512);

    auto renderChordWithMode = [&](VoiceMode mode, std::vector<float>& outSignal) {
        engine.reset();
        engine.setVoiceMode(mode);
        engine.noteOn(48, 0.9f); // C3
        engine.noteOn(55, 0.9f); // G3
        engine.noteOn(64, 0.9f); // E4

        juce::AudioBuffer<float> b(2, 512);
        b.clear();
        engine.processBlock(b, mode, CrossModMode::FM, 0.0f, 0.0f, 0.85f,
                            OscWaveform::Sine, 0, 0.0f, 0.8f,
                            OscWaveform::Sine, 0, 0.0f, 0.8f,
                            OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.0f,
                            0.0f, 0.0f,
                            10000.0f, 0.1f, FilterType::Lowpass24, 0.0f, 0.0f, 0.0f,
                            0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f, 0.01f, 0.1f, 0.8f, 0.01f,
                            1.0f, LFOShape::Sine, false, 0, false,
                            1.0f, LFOShape::Sine, false, 0, false,
                            0.8f, 0.0f, 0.75f, 0.0f,
                            FxOrder::Mod_Dly_Rvb,
                            ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.0f,
                            DelayFxType::Tape, 0.3f, 0.4f, 0.6f, 0.0f, false, 8,
                            ReverbFxType::Plate, 2.0f, 0.3f, 0.7f, 0.0f);
        outSignal.resize(512);
        for (int i = 0; i < 512; ++i)
            outSignal[i] = b.getSample(0, i);
    };

    std::vector<float> sigRing, sigSymp, sigRoot, sigChaos;
    renderChordWithMode(VoiceMode::CyclicRing, sigRing);
    renderChordWithMode(VoiceMode::SympatheticAll, sigSymp);
    renderChordWithMode(VoiceMode::RootDriver, sigRoot);
    renderChordWithMode(VoiceMode::ChaosDiffuse, sigChaos);

    auto computeDiff = [](const std::vector<float>& a, const std::vector<float>& b) {
        float sumSq = 0.0f;
        for (size_t i = 0; i < a.size(); ++i)
            sumSq += (a[i] - b[i]) * (a[i] - b[i]);
        return std::sqrt(sumSq / (float)a.size());
    };

    float diffRingSymp = computeDiff(sigRing, sigSymp);
    float diffRingChaos = computeDiff(sigRing, sigChaos);
    float diffSympRoot = computeDiff(sigSymp, sigRoot);
    float diffRootChaos = computeDiff(sigRoot, sigChaos);

    TEST_ASSERT(diffRingSymp > 0.01f, "CyclicRing and SympatheticAll have distinct acoustic signatures");
    TEST_ASSERT(diffRingChaos > 0.01f, "CyclicRing and ChaosDiffuse have distinct acoustic signatures");
    TEST_ASSERT(diffSympRoot > 0.01f, "SympatheticAll and RootDriver have distinct acoustic signatures");
    TEST_ASSERT(diffRootChaos > 0.01f, "RootDriver and ChaosDiffuse have distinct acoustic signatures");
}

void testAudioRateFilterModulationStability()
{
    std::cout << "\n=== Testing Audio-Rate Filter Modulation & Cyclic Ring Stability ===\n";

    // 1. Direct VintageFilter Stress Test under 2000 Hz cutoff modulation & extreme resonance
    VintageFilter filter;
    filter.setSampleRate(44100.0);

    for (auto type : { FilterType::Lowpass24, FilterType::Lowpass12, FilterType::Bandpass12, FilterType::Highpass12 })
    {
        bool allFinite = true;
        bool hasSignal = false;
        filter.reset();

        for (int i = 0; i < 44100; ++i)
        {
            // Rapid audio rate modulation of cutoff frequency at 1200 Hz between 50 Hz and 18000 Hz
            float modLfo = std::sin(i * (2.0f * 3.14159265f * 1200.0f / 44100.0f));
            float cutoff = 1000.0f * std::pow(2.0f, modLfo * 4.0f); // sweeps 62.5 Hz to 16 kHz
            filter.setParameters(cutoff, 0.99f, type, 0.8f);

            float inSig = std::sin(i * (2.0f * 3.14159265f * 110.0f / 44100.0f)); // 110 Hz input
            float out = filter.processSample(inSig);

            if (!std::isfinite(out) || std::abs(out) > 15.0f)
            {
                allFinite = false;
                break;
            }
            if (std::abs(out) > 0.001f)
                hasSignal = true;
        }

        TEST_ASSERT(allFinite, "VintageFilter remains 100% stable under 1.2kHz audio-rate cutoff modulation");
        TEST_ASSERT(hasSignal, "VintageFilter outputs audio signal under audio-rate modulation");
    }

    // 2. Full Engine Stress Test: Low Chord + Cyclic Ring + Voice X-Mod 1.0 + 880 Hz LFO Filter Modulation
    CrossModEngine engine;
    engine.prepare(44100.0, 512);
    engine.setVoiceMode(VoiceMode::CyclicRing);

    // Set ModMatrix: LFO 1 -> FilterCutoff at +1.0 depth
    engine.setModMatrixSlot(0, ModSource::LFO1, ModDestination::FilterCutoff, 1.0f);

    // Play low chord: C2 (36), G2 (43), C3 (48), E3 (52)
    engine.noteOn(36, 0.9f);
    engine.noteOn(43, 0.9f);
    engine.noteOn(48, 0.9f);
    engine.noteOn(52, 0.9f);

    juce::AudioBuffer<float> block(2, 512);
    bool engineStable = true;
    bool engineHasSound = false;

    // Process 40 blocks (~0.46 seconds of audio) with voice cross-mod at 1.0 and audio-rate LFO at 880 Hz
    for (int b = 0; b < 40; ++b)
    {
        block.clear();
        engine.processBlock(block,
                            VoiceMode::CyclicRing,
                            CrossModMode::FM,
                            0.0f, 0.0f,
                            1.0f, // Maximum voice cross-mod depth
                            OscWaveform::Saw, 0, 0.0f, 0.8f,
                            OscWaveform::Square, 0, 0.0f, 0.8f,
                            OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.5f,
                            0.75f, 0.75f, // Internal cross-mod
                            1500.0f, 0.95f, FilterType::Lowpass24, // High resonance
                            0.6f, 0.5f, 0.0f,
                            0.005f, 0.1f, 0.9f, 0.05f, // Envelopes
                            0.005f, 0.1f, 0.9f, 0.05f,
                            0.005f, 0.1f, 0.9f, 0.05f,
                            880.0f, LFOShape::Sine, false, 0, false, // LFO1 at 880 Hz audio rate!
                            2.0f, LFOShape::Triangle, false, 0, false,
                            0.8f, 0.0f, 0.8f, 0.3f,
                            FxOrder::Mod_Dly_Rvb,
                            ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.2f,
                            DelayFxType::Tape, 0.3f, 0.4f, 0.5f, 0.2f, false, 8,
                            ReverbFxType::Hall, 2.5f, 0.4f, 0.7f, 0.2f);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* p = block.getReadPointer(ch);
            for (int s = 0; s < 512; ++s)
            {
                if (!std::isfinite(p[s]) || std::abs(p[s]) > 20.0f)
                {
                    engineStable = false;
                    break;
                }
            }
        }

        if (block.getMagnitude(0, 0, 512) > 0.01f)
            engineHasSound = true;
    }

    TEST_ASSERT(engineStable, "Engine buffer is completely free of NaNs/Infs during Cyclic Ring + Audio-rate filter mod");
    TEST_ASSERT(engineHasSound, "Engine maintains continuous sound without audio dropouts or silence lockup");

    // 3. Verify Note Off and re-triggering recovers cleanly
    engine.allNotesOff();
    block.clear();
    for (int b = 0; b < 10; ++b)
    {
        engine.processBlock(block, VoiceMode::CyclicRing, CrossModMode::FM, 0.0f, 0.0f, 1.0f,
                            OscWaveform::Saw, 0, 0.0f, 0.8f,
                            OscWaveform::Square, 0, 0.0f, 0.8f,
                            OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.5f,
                            0.75f, 0.75f, 1500.0f, 0.95f, FilterType::Lowpass24,
                            0.6f, 0.5f, 0.0f,
                            0.005f, 0.1f, 0.9f, 0.01f,
                            0.005f, 0.1f, 0.9f, 0.01f,
                            0.005f, 0.1f, 0.9f, 0.01f,
                            880.0f, LFOShape::Sine, false, 0, false,
                            2.0f, LFOShape::Triangle, false, 0, false,
                            0.8f, 0.0f, 0.8f, 0.3f,
                            FxOrder::Mod_Dly_Rvb,
                            ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.2f,
                            DelayFxType::Tape, 0.3f, 0.4f, 0.5f, 0.2f, false, 8,
                            ReverbFxType::Hall, 2.5f, 0.4f, 0.7f, 0.2f);
    }

    // Retrigger a note
    engine.noteOn(60, 0.8f);
    block.clear();
    engine.processBlock(block, VoiceMode::CyclicRing, CrossModMode::FM, 0.0f, 0.0f, 1.0f,
                        OscWaveform::Saw, 0, 0.0f, 0.8f,
                        OscWaveform::Square, 0, 0.0f, 0.8f,
                        OscWaveform::Sine, SubOscOctave::OctaveMinus1, 0.5f,
                        0.75f, 0.75f, 1500.0f, 0.95f, FilterType::Lowpass24,
                        0.6f, 0.5f, 0.0f,
                        0.005f, 0.1f, 0.9f, 0.05f,
                        0.005f, 0.1f, 0.9f, 0.05f,
                        0.005f, 0.1f, 0.9f, 0.05f,
                        880.0f, LFOShape::Sine, false, 0, false,
                        2.0f, LFOShape::Triangle, false, 0, false,
                        0.8f, 0.0f, 0.8f, 0.3f,
                        FxOrder::Mod_Dly_Rvb,
                        ModFxType::Chorus, 1.0f, 0.5f, 0.2f, 0.2f,
                        DelayFxType::Tape, 0.3f, 0.4f, 0.5f, 0.2f, false, 8,
                        ReverbFxType::Hall, 2.5f, 0.4f, 0.7f, 0.2f);

    TEST_ASSERT(block.getMagnitude(0, 0, 512) > 0.01f, "Engine cleanly sounds new notes after stress recovery");
}

int main()
{
    std::cout << "========================================\n";
    std::cout << "Running CrossMod Synthesizer Unit Tests\n";
    std::cout << "========================================\n";

    testCrossModVoiceStability();
    testVintageFilter();
    testModMatrixRouting();
    testEnginePolyphonyModes();
    testMonoUnison();
    testPolyToMonoSwitch();
    testPolyXModAlgorithmsDiversity();
    testAudioRateFilterModulationStability();
    testFxChain();
    testFactoryPresets();
    testPresetManager();
    testBpmSync();
    testDualAndPolyGlide();
    testStereoSeparationAndWidth();
    testCrossModModesAndKeyTrack();
    testOscWaveformsAndSubOsc();
    testMidiLearn();

    std::cout << "\n========================================\n";
    if (g_testFailures == 0)
    {
        std::cout << "ALL TESTS PASSED SUCCESSFULLY!\n";
        return 0;
    }
    else
    {
        std::cerr << g_testFailures << " TEST(S) FAILED!\n";
        return 1;
    }
}

