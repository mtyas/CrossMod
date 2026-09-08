#include "ParameterIDs.h"

juce::AudioProcessorValueTreeState::ParameterLayout ParameterFactory::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const juce::StringArray lfoWaveChoices = {
        "Sine", "Triangle", "Sawtooth", "Square", "Sample & Hold", "Random Smooth"
    };

    const juce::StringArray lfoSubdivChoices = {
        "1/32", "1/16", "1/8", "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars", "8 Bars"
    };

    const juce::StringArray envSourceChoices = {
        "Input Audio", "Transient (Attack)", "Sustain (Body)",
        "Filter 1", "Filter 2", "Filter 3",
        "Left (L)", "Right (R)", "Mid (M)", "Side (S)",
        "Dynamic High", "Dynamic Low", "Harmonic", "Noise", "In-Phase", "Spatial"
    };

    const juce::StringArray fxTypeChoices = {
        "Bypass",
        "VCA Compressor", "Opto Compressor", "FET Compressor", "Noise Gate",
        "Parametric EQ", "Graphic EQ", "Tilt EQ", "Dual HP/LP Filter", "Bandpass & Notch",
        "Hard Clip", "Tube Saturation", "Wavefolder", "Germanium Fuzz", "Octave Fuzz", "Overdrive", "Tape Saturation",
        "Bitcrusher", "Stutter Repeater", "Tape Stop Glitch", "Reverse Slice", "Granular Jitter",
        "Pitch Shifter", "Frequency Shifter", "Stereo Chorus", "Flanger", "Phaser", "Tremolo / Auto-Pan",
        "Stereo Delay", "Ping-Pong Delay", "Tape Echo",
        "Room Reverb", "Hall Reverb", "Plate Reverb", "Spring Reverb", "Shimmer Reverb"
    };

    const juce::StringArray slotSourceChoices = {
        "Disconnected",      // 0
        "Stereo In",         // 1
        "Attack",            // 2
        "Body",              // 3
        "Filter 1",          // 4
        "Filter 2",          // 5
        "Filter 3",          // 6
        "Left (L)",          // 7
        "Right (R)",         // 8
        "Mid (M)",           // 9
        "Side (S)",          // 10
        "Dynamic High",      // 11
        "Dynamic Low",       // 12
        "Harmonic",          // 13
        "Noise",             // 14
        "In-Phase",          // 15
        "Spatial",           // 16
        "Slot 1 Out",        // 17
        "Slot 2 Out",        // 18
        "Slot 3 Out",        // 19
        "Slot 4 Out",        // 20
        "Slot 5 Out",        // 21
        "Slot 6 Out",        // 22
        "Slot 7 Out",        // 23
        "Slot 8 Out"         // 24
    };

    // 4 Stereo Output Bus routing choices
    const juce::StringArray slotDestChoices = {
        "Disconnected",
        "Bus 1 (Main 1+2)",
        "Bus 2 (Aux 3+4)",
        "Bus 3 (Aux 5+6)",
        "Bus 4 (Aux 7+8)",
        "All Buses (1..4)",
        "Next Slot",
        "-> Slot 1", "-> Slot 2", "-> Slot 3", "-> Slot 4",
        "-> Slot 5", "-> Slot 6", "-> Slot 7", "-> Slot 8"
    };

    const juce::StringArray modSourceChoices = {
        "None",
        "LFO 1", "LFO 2", "LFO 3",
        "Env Follow 1", "Env Follow 2", "Env Follow 3",
        "Turing 1", "Turing 2",
        "Macro 1", "Macro 2", "Macro 3", "Macro 4",
        "XY Pad 1 X", "XY Pad 1 Y", "XY Pad 2 X", "XY Pad 2 Y",
        "Audio: Transient Env", "Audio: Sustain Env",
        "Audio: Filter 1 Env", "Audio: Filter 2 Env", "Audio: Filter 3 Env"
    };

    juce::StringArray modDestChoices;
    modDestChoices.add("None");

    // Pre-Processors
    modDestChoices.add("Pre: Split Attack");
    modDestChoices.add("Pre: Split Release");
    modDestChoices.add("Pre: Split Sens");
    modDestChoices.add("Pre: Split Thresh");

    modDestChoices.add("Pre: Filt 1 Freq");
    modDestChoices.add("Pre: Filt 1 Q");
    modDestChoices.add("Pre: Filt 1 Gain");
    modDestChoices.add("Pre: Filt 1 Drive");

    modDestChoices.add("Pre: Filt 2 Freq");
    modDestChoices.add("Pre: Filt 2 Q");
    modDestChoices.add("Pre: Filt 2 Gain");
    modDestChoices.add("Pre: Filt 2 Drive");

    modDestChoices.add("Pre: Filt 3 Freq");
    modDestChoices.add("Pre: Filt 3 Q");
    modDestChoices.add("Pre: Filt 3 Gain");
    modDestChoices.add("Pre: Filt 3 Drive");

    // Slots 1 to 8 (17 to 64)
    for (int s = 1; s <= 8; ++s)
    {
        for (int p = 1; p <= 5; ++p)
            modDestChoices.add("Slot " + juce::String(s) + " Param " + juce::String(p));
        modDestChoices.add("Slot " + juce::String(s) + " Mix");
    }

    // Modulators Speed
    modDestChoices.add("Mod: LFO 1 Rate");
    modDestChoices.add("Mod: LFO 2 Rate");
    modDestChoices.add("Mod: LFO 3 Rate");
    modDestChoices.add("Mod: XY 1 Speed");
    modDestChoices.add("Mod: XY 2 Speed");
    modDestChoices.add("Mod: Turing 1 Prob");
    modDestChoices.add("Mod: Turing 2 Prob");

    // Master
    modDestChoices.add("Master: Balance");
    modDestChoices.add("Master: Dry/Wet");
    modDestChoices.add("Master: Gain");

    const juce::StringArray filterTypeChoices = {
        "Low-Pass 12dB", "Low-Pass 24dB", "High-Pass 12dB", "High-Pass 24dB",
        "Band-Pass", "Peak / Bell", "Notch", "Low Shelf", "High Shelf"
    };

    const juce::StringArray outputRoutingChoices = {
        "Stereo Mix (2 Ch)", "Split Direct (4 Ch)", "Auto (DAW Bus)"
    };

    // 1. Pre-Processors: Transient Splitter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::transientAttack, "Attack Time",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f, 0.4f), 15.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::transientRelease, "Release Time",
        juce::NormalisableRange<float>(10.0f, 500.0f, 0.1f, 0.4f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::transientSensitivity, "Sensitivity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::transientThreshold, "Detection Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -24.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        IDs::detectionMode, "Detection Mode",
        juce::StringArray { "Fast / Sharp", "Smooth / RMS", "Hybrid Dual-Band" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::lookahead, "Lookahead",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::transientGain, "Dry Transient Level",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::sustainGain, "Dry Sustain Level",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::transientPan, "Transient Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::sustainPan, "Sustain Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        IDs::transientSolo, "Transient Solo", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        IDs::sustainSolo, "Sustain Solo", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        IDs::transientMute, "Transient Mute", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        IDs::sustainMute, "Sustain Mute", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        IDs::engineMode, "Engine Mode",
        juce::StringArray { "Split Mode", "Modular FX Mode" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        IDs::topViewTab, "Top View",
        juce::StringArray { "Transient Splitter", "3-Band Filter Bank", "Mod Matrix" }, 0));

    // 2. Pre-Processors: 3-Band ZDF Filter Bank
    const float defaultFreqs[3] = { 250.0f, 1500.0f, 5000.0f };
    const float defaultQs[3]    = { 0.707f, 1.0f, 0.707f };
    const int defaultTypes[3]   = { 0, 4, 2 };

    for (int b = 0; b < 3; ++b)
    {
        const juce::String bPrefix = "Filter " + juce::String(b + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            IDs::getFilterType(b), bPrefix + "Type", filterTypeChoices, defaultTypes[b]));

        juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 1.0f);
        freqRange.setSkewForCentre(1000.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getFilterFreq(b), bPrefix + "Frequency", freqRange, defaultFreqs[b],
            juce::AudioParameterFloatAttributes().withLabel("Hz")));

        juce::NormalisableRange<float> qRange(0.1f, 10.0f, 0.01f);
        qRange.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getFilterQ(b), bPrefix + "Q / Reso", qRange, defaultQs[b]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getFilterGain(b), bPrefix + "Gain",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getFilterDrive(b), bPrefix + "Drive",
            juce::NormalisableRange<float>(1.0f, 4.0f, 0.01f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            IDs::getFilterBypass(b), bPrefix + "Bypass", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            IDs::getFilterSolo(b), bPrefix + "Solo", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            IDs::getFilterMute(b), bPrefix + "Mute", false));
    }

    // 2b. Pre-Processors: Sub-Tab Selection
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        IDs::preProcSubTab, "Pre-Processor View",
        juce::StringArray { "Transient", "3-Band Filter", "Mid / Side", "Dynamic", "Spectral", "Spatial" }, 0));

    // 2c. Pre-Processors: Mid/Side & Stereo
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::midSideWidth, "M/S Stereo Width", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::midSideBalance, "L/R Balance", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    // 2d. Pre-Processors: Dynamic Splitter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::dynSplitThreshold, "Dyn Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::dynSplitAttack, "Dyn Attack", juce::NormalisableRange<float>(0.5f, 300.0f, 0.1f, 0.4f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::dynSplitRelease, "Dyn Release", juce::NormalisableRange<float>(5.0f, 1500.0f, 0.1f, 0.4f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::dynSplitKnee, "Dyn Knee", juce::NormalisableRange<float>(0.1f, 24.0f, 0.1f), 6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // 2e. Pre-Processors: Spectral & Harmonic Decomposition
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::spectralHarmonicSens, "Harmonic Sensitivity", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::spectralFocus, "Spectral Focus", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::spectralSmoothing, "Spectral Smoothing", juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // 2f. Pre-Processors: Phase & Spatial Splitting
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::phaseSpatialThresh, "Spatial Threshold", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::phaseSpatialWindow, "Spatial Window", juce::NormalisableRange<float>(2.0f, 80.0f, 0.1f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::phaseSpatialSpread, "Spatial Spread", juce::NormalisableRange<float>(0.5f, 2.0f, 0.01f), 1.0f));

    // 3. Master Controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::masterBalance, "Split Balance",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::masterDryWet, "Dry / Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        IDs::masterGain, "Output Level",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        IDs::masterLimiter, "Soft Ceiling Limiter", true));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        IDs::outputRoutingMode, "Output Routing", outputRoutingChoices, 0));

    // 4. Slots 1..4 (Transient Chain)
    for (int i = 0; i < 4; ++i)
    {
        const juce::String prefix = "T-Slot " + juce::String(i + 1) + " ";
        const bool defaultBypass = false; // All Slots ON by default
        const int defaultSrc = (i == 0) ? 1 : 0; // Slot 1 defaults to Stereo In, Slots 2-4 to Disconnected
        const int defaultDst = (i == 0) ? 1 : 0; // Slot 1 defaults to Bus 1 Main, Slots 2-4 to Disconnected

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            IDs::getTransSlotSrc(i), prefix + "Input Source", slotSourceChoices, defaultSrc));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            IDs::getTransSlotType(i), prefix + "Type", fxTypeChoices, 0));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            IDs::getTransSlotBypass(i), prefix + "Bypass", defaultBypass));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotP1(i), prefix + "Param 1", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotP2(i), prefix + "Param 2", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotP3(i), prefix + "Param 3", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotP4(i), prefix + "Param 4", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotP5(i), prefix + "Param 5", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotMix(i), prefix + "Mix", 0.0f, 1.0f, 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotInGain(i), prefix + "In Level",
            juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getTransSlotOutGain(i), prefix + "Out Level",
            juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            IDs::getTransSlotDest(i), prefix + "Routing Dest", slotDestChoices, defaultDst));
    }

    // 5. Slots 5..8 (Sustain Chain)
    for (int i = 0; i < 4; ++i)
    {
        const juce::String prefix = "S-Slot " + juce::String(i + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            IDs::getSustSlotSrc(i), prefix + "Input Source", slotSourceChoices, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            IDs::getSustSlotType(i), prefix + "Type", fxTypeChoices, 0));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            IDs::getSustSlotBypass(i), prefix + "Bypass", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotP1(i), prefix + "Param 1", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotP2(i), prefix + "Param 2", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotP3(i), prefix + "Param 3", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotP4(i), prefix + "Param 4", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotP5(i), prefix + "Param 5", 0.0f, 1.0f, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotMix(i), prefix + "Mix", 0.0f, 1.0f, 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotInGain(i), prefix + "In Level",
            juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getSustSlotOutGain(i), prefix + "Out Level",
            juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            IDs::getSustSlotDest(i), prefix + "Routing Dest", slotDestChoices, 0));
    }

    // 6. 3 LFOs
    auto registerLfo = [&](const juce::ParameterID& waveId, const juce::ParameterID& rateId,
                           const juce::ParameterID& syncId, const juce::ParameterID& subdivId,
                           const juce::ParameterID& smoothId, const juce::String& name)
    {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(waveId, name + " Waveform", lfoWaveChoices, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            rateId, name + " Rate", juce::NormalisableRange<float>(0.01f, 50.0f, 0.01f, 0.35f), 1.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
        params.push_back(std::make_unique<juce::AudioParameterBool>(syncId, name + " Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(subdivId, name + " Division", lfoSubdivChoices, 3));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(smoothId, name + " Smooth", 0.0f, 1.0f, 0.0f));
    };

    registerLfo(IDs::lfo1Wave, IDs::lfo1Rate, IDs::lfo1Sync, IDs::lfo1Subdiv, IDs::lfo1Smooth, "LFO 1");
    registerLfo(IDs::lfo2Wave, IDs::lfo2Rate, IDs::lfo2Sync, IDs::lfo2Subdiv, IDs::lfo2Smooth, "LFO 2");
    registerLfo(IDs::lfo3Wave, IDs::lfo3Rate, IDs::lfo3Sync, IDs::lfo3Subdiv, IDs::lfo3Smooth, "LFO 3");

    // 7. 3 Envelope Followers
    auto registerEnv = [&](const juce::ParameterID& srcId, const juce::ParameterID& attId,
                           const juce::ParameterID& relId, const juce::ParameterID& gainId,
                           const juce::String& name)
    {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(srcId, name + " Source", envSourceChoices, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            attId, name + " Attack", juce::NormalisableRange<float>(1.0f, 500.0f, 0.1f, 0.35f), 20.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            relId, name + " Release", juce::NormalisableRange<float>(5.0f, 2000.0f, 0.1f, 0.35f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            gainId, name + " Gain", juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    };

    registerEnv(IDs::env1Source, IDs::env1Attack, IDs::env1Release, IDs::env1Gain, "Env 1");
    registerEnv(IDs::env2Source, IDs::env2Attack, IDs::env2Release, IDs::env2Gain, "Env 2");
    registerEnv(IDs::env3Source, IDs::env3Attack, IDs::env3Release, IDs::env3Gain, "Env 3");

    // 8. 2 Turing Machines
    auto registerTuring = [&](const juce::ParameterID& probId, const juce::ParameterID& lenId,
                             const juce::ParameterID& syncId, const juce::ParameterID& subdivId,
                             const juce::ParameterID& rateId, const juce::ParameterID& glideId,
                             const juce::String& name)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(probId, name + " Probability", 0.0f, 1.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            lenId, name + " Length", juce::StringArray { "4", "8", "16", "32" }, 2));
        params.push_back(std::make_unique<juce::AudioParameterBool>(syncId, name + " Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(subdivId, name + " Division", lfoSubdivChoices, 3));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            rateId, name + " Free Rate", juce::NormalisableRange<float>(0.1f, 50.0f, 0.1f, 0.4f), 4.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(glideId, name + " Glide", 0.0f, 0.99f, 0.0f));
    };

    registerTuring(IDs::turing1Prob, IDs::turing1Length, IDs::turing1Sync, IDs::turing1Subdiv, IDs::turing1Rate, IDs::turing1Glide, "Turing 1");
    registerTuring(IDs::turing2Prob, IDs::turing2Length, IDs::turing2Sync, IDs::turing2Subdiv, IDs::turing2Rate, IDs::turing2Glide, "Turing 2");

    // 9. 4 Macros
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::macro1, "Macro 1", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::macro2, "Macro 2", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::macro3, "Macro 3", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::macro4, "Macro 4", 0.0f, 1.0f, 0.0f));

    // 10. 2 Loopable XY Pads
    auto registerXY = [&](const juce::ParameterID& xId, const juce::ParameterID& yId,
                         const juce::ParameterID& spdId, const juce::ParameterID& recId,
                         const juce::ParameterID& playId, const juce::String& name)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(xId, name + " X", -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(yId, name + " Y", -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            spdId, name + " Speed", juce::NormalisableRange<float>(0.1f, 8.0f, 0.01f, 0.5f), 1.0f,
            juce::AudioParameterFloatAttributes().withLabel("x")));
        params.push_back(std::make_unique<juce::AudioParameterBool>(recId, name + " Record", false));
        params.push_back(std::make_unique<juce::AudioParameterBool>(playId, name + " Playback", false));
    };

    registerXY(IDs::xy1_x, IDs::xy1_y, IDs::xy1_speed, IDs::xy1_rec, IDs::xy1_play, "XY Pad 1");
    registerXY(IDs::xy2_x, IDs::xy2_y, IDs::xy2_speed, IDs::xy2_rec, IDs::xy2_play, "XY Pad 2");

    // 11. Modulation Matrix (8 routing slots)
    for (int i = 0; i < 8; ++i)
    {
        const juce::String prefix = "Mod Slot " + juce::String(i + 1) + " ";
        params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::getModSlotSource(i), prefix + "Source", modSourceChoices, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(IDs::getModSlotDest(i), prefix + "Destination", modDestChoices, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            IDs::getModSlotDepth(i), prefix + "Amount",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f), 0.0f));
    }

    return { params.begin(), params.end() };
}
