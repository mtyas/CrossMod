#include "ParameterIDs.h"

juce::StringArray ParameterFactory::getVoiceModeChoices()
{
    return {
        "Mono",
        "Mono Unison",
        "Poly (Multi-Mono)",
        "Cyclic Ring",
        "Sympathetic All",
        "Root Driver",
        "Chaos Diffuse"
    };
}

juce::StringArray ParameterFactory::getVoiceXModAlgoChoices()
{
    return { "Cyclic Ring", "Sympathetic All", "Root Driver", "Chaotic Diffuse" };
}

juce::StringArray ParameterFactory::getCrossModModeChoices()
{
    return { "FM (Freq Mod)", "PM (Phase Mod)", "TZ-FM (Through-Zero)", "AM (Amplitude Mod)", "Ring Mod" };
}

juce::StringArray ParameterFactory::getOscWaveformChoices()
{
    return { "Sine", "Triangle", "Saw", "Square" };
}

juce::StringArray ParameterFactory::getSubOscOctaveChoices()
{
    return { "-1 Octave", "-2 Octaves" };
}

juce::StringArray ParameterFactory::getFilterTypeChoices()
{
    return { "Lowpass 24dB", "Lowpass 12dB", "Bandpass 12dB", "Highpass 12dB" };
}

juce::StringArray ParameterFactory::getLFOShapeChoices()
{
    return { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Sample & Hold" };
}

juce::StringArray ParameterFactory::getSyncRateChoices()
{
    return {
        "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
        "1/4T", "1/4", "1/4D", "1/2", "1/2D", "1 Bar", "2 Bars", "4 Bars"
    };
}

juce::StringArray ParameterFactory::getModSourceChoices()
{
    return {
        "Off",
        "LFO 1",
        "LFO 2",
        "Amp Env",
        "Filter Env",
        "Mod Env",
        "Random S&H",
        "Velocity",
        "Mod Wheel",
        "Pitch Bend",
        "Aftertouch",
        "Key Track"
    };
}

juce::StringArray ParameterFactory::getModDestinationChoices()
{
    return {
        "Off",
        "Osc 1 Pitch",
        "Osc 2 Pitch",
        "CrossMod 1->2",
        "CrossMod 2->1",
        "Osc 1 Level",
        "Osc 2 Level",
        "Sub Osc Level",
        "VCF Cutoff",
        "VCF Resonance",
        "VCA Level",
        "VCA Pan",
        "LFO 1 Rate",
        "LFO 2 Rate",
        "Voice Cross-Mod",
        "Mod FX Rate",
        "Mod FX Depth",
        "Mod FX Feedback",
        "Mod FX Amount",
        "Delay FX Time",
        "Delay FX Feedback",
        "Delay FX Tone",
        "Delay FX Amount",
        "Reverb FX Decay",
        "Reverb FX Damping",
        "Reverb FX Tone",
        "Reverb FX Amount",
        "Osc 1 Glide",
        "Osc 2 Glide",
        "Stereo Width"
    };
}

juce::StringArray ParameterFactory::getModFxTypeChoices()
{
    return { "Chorus", "Flanger", "Phaser", "Ensemble" };
}

juce::StringArray ParameterFactory::getDelayFxTypeChoices()
{
    return { "Tape Delay", "BBD Analog", "Digital", "Ping-Pong" };
}

juce::StringArray ParameterFactory::getReverbFxTypeChoices()
{
    return { "Plate Reverb", "Room Reverb", "Hall Reverb" };
}

juce::StringArray ParameterFactory::getFxOrderChoices()
{
    return { "MOD > DLY > RVB", "DLY > MOD > RVB", "MOD > RVB > DLY", "RVB > DLY > MOD" };
}

juce::AudioProcessorValueTreeState::ParameterLayout ParameterFactory::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Voice & Master
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::voiceMode, "Voice Mode", getVoiceModeChoices(), 2)); // Default Poly (Multi-Mono)

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::voiceXModAlgo, "Voice X-Mod Algorithm", getVoiceXModAlgoChoices(), 0)); // Default Cyclic Ring

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::glideTime, "Glide Time",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.35f), 0.05f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::osc1Glide, "Osc 1 Glide",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.35f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::osc2Glide, "Osc 2 Glide",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.35f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::voiceCrossMod, "Voice Cross-Mod Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::masterVolume, "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::masterPan, "Master Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::stereoWidth, "Stereo Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.75f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::vcaSaturation, "VCA Warmth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

    // Oscillator 1
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::osc1Waveform, "Osc 1 Waveform", getOscWaveformChoices(), 0)); // Default Sine

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        CrossModIDs::osc1Coarse, "Osc 1 Coarse", -36, 36, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::osc1Fine, "Osc 1 Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::osc1Level, "Osc 1 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // Sub Oscillator (Independent, isolated from cross-mod)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::subOscWaveform, "Sub Osc Waveform", getOscWaveformChoices(), 0)); // Default Sine

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::subOscOctave, "Sub Osc Octave", getSubOscOctaveChoices(), 0)); // Default -1 Octave

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::subOscLevel, "Sub Osc Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f)); // Default 0.0

    // Cross Modulation
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::crossModMode, "Cross-Mod Mode", getCrossModModeChoices(), 0)); // Default FM

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::crossMod1to2, "Cross-Mod 1->2",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.4f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::crossMod2to1, "Cross-Mod 2->1",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.4f), 0.0f));

    // Oscillator 2
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::osc2Waveform, "Osc 2 Waveform", getOscWaveformChoices(), 0)); // Default Sine

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        CrossModIDs::osc2Coarse, "Osc 2 Coarse", -36, 36, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::osc2Fine, "Osc 2 Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::osc2Level, "Osc 2 Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // Filter (VCF)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterCutoff, "Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f), 12000.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterResonance, "Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::filterType, "Filter Type", getFilterTypeChoices(), 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterDrive, "Filter Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterKeyTrack, "Key Tracking",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterEnvAmt, "Filter Env Amount",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    // Amp Envelope
    auto envTimeRange = juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::ampAttack, "Amp Attack", envTimeRange, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::ampDecay, "Amp Decay", envTimeRange, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::ampSustain, "Amp Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::ampRelease, "Amp Release", envTimeRange, 0.2f));

    // Filter Envelope
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterAttack, "Filter Attack", envTimeRange, 0.02f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterDecay, "Filter Decay", envTimeRange, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterSustain, "Filter Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::filterRelease, "Filter Release", envTimeRange, 0.3f));

    // Mod Envelope
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modAttack, "Mod Attack", envTimeRange, 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modDecay, "Mod Decay", envTimeRange, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modSustain, "Mod Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modRelease, "Mod Release", envTimeRange, 0.3f));

    // LFO 1 (Audio-rate range up to 2000 Hz)
    auto lfoRateRange = juce::NormalisableRange<float>(0.01f, 2000.0f, 0.01f, 0.25f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::lfo1Rate, "LFO 1 Rate", lfoRateRange, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::lfo1Shape, "LFO 1 Shape", getLFOShapeChoices(), 0)); // Sine
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        CrossModIDs::lfo1Sync, "LFO 1 Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::lfo1SyncRate, "LFO 1 Sync Rate", getSyncRateChoices(), 8)); // 1/4
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        CrossModIDs::lfo1Retrig, "LFO 1 Retrigger", true));

    // LFO 2 (Audio-rate range up to 2000 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::lfo2Rate, "LFO 2 Rate", lfoRateRange, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::lfo2Shape, "LFO 2 Shape", getLFOShapeChoices(), 1)); // Triangle
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        CrossModIDs::lfo2Sync, "LFO 2 Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::lfo2SyncRate, "LFO 2 Sync Rate", getSyncRateChoices(), 5)); // 1/8
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        CrossModIDs::lfo2Retrig, "LFO 2 Retrigger", false));

    // Modulation Matrix (8 Slots)
    auto sourceChoices = getModSourceChoices();
    auto destChoices = getModDestinationChoices();

    for (int i = 0; i < CrossModIDs::NUM_MOD_SLOTS; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            CrossModIDs::modSlotSource(i), "Slot " + juce::String(i + 1) + " Source",
            sourceChoices, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            CrossModIDs::modSlotDest(i), "Slot " + juce::String(i + 1) + " Destination",
            destChoices, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            CrossModIDs::modSlotAmount(i), "Slot " + juce::String(i + 1) + " Amount",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.005f), 0.0f));
    }

    // Modulation Effect
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::modFxType, "Mod FX Type", getModFxTypeChoices(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modFxRate, "Mod FX Rate",
        juce::NormalisableRange<float>(0.05f, 15.0f, 0.01f, 0.35f), 1.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modFxDepth, "Mod FX Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modFxFeedback, "Mod FX Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::modFxMix, "Mod FX Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // Delay Effect
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::delayFxType, "Delay FX Type", getDelayFxTypeChoices(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::delayFxTime, "Delay FX Time",
        juce::NormalisableRange<float>(0.01f, 1.8f, 0.001f, 0.35f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::delayFxFeedback, "Delay FX Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::delayFxTone, "Delay FX Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::delayFxMix, "Delay FX Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        CrossModIDs::delayFxSync, "Delay BPM Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::delayFxSyncRate, "Delay Sync Rate", getSyncRateChoices(), 8)); // 1/4

    // Reverb Effect
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::reverbFxType, "Reverb FX Type", getReverbFxTypeChoices(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::reverbFxDecay, "Reverb FX Decay",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.35f), 2.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::reverbFxDamping, "Reverb FX Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::reverbFxTone, "Reverb FX Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        CrossModIDs::reverbFxMix, "Reverb FX Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // FX Routing Order
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        CrossModIDs::fxRoutingOrder, "FX Routing Order", getFxOrderChoices(), 0));

    return { params.begin(), params.end() };
}
