#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace CrossModIDs
{
    // Voice & Master
    inline const juce::ParameterID voiceMode        { "voiceMode", 1 };
    inline const juce::ParameterID voiceXModAlgo    { "voiceXModAlgo", 1 };
    inline const juce::ParameterID glideTime        { "glideTime", 1 };
    inline const juce::ParameterID osc1Glide        { "osc1Glide", 1 };
    inline const juce::ParameterID osc2Glide        { "osc2Glide", 1 };
    inline const juce::ParameterID voiceCrossMod    { "voiceCrossMod", 1 };
    inline const juce::ParameterID masterVolume     { "masterVolume", 1 };
    inline const juce::ParameterID masterPan        { "masterPan", 1 };
    inline const juce::ParameterID stereoWidth      { "stereoWidth", 1 };
    inline const juce::ParameterID vcaSaturation    { "vcaSaturation", 1 };

    // Oscillator 1
    inline const juce::ParameterID osc1Waveform     { "osc1Waveform", 1 };
    inline const juce::ParameterID osc1Coarse       { "osc1Coarse", 1 };
    inline const juce::ParameterID osc1Fine         { "osc1Fine", 1 };
    inline const juce::ParameterID osc1Level        { "osc1Level", 1 };

    // Sub Oscillator (Independent, isolated from cross-mod)
    inline const juce::ParameterID subOscWaveform   { "subOscWaveform", 1 };
    inline const juce::ParameterID subOscOctave     { "subOscOctave", 1 };
    inline const juce::ParameterID subOscLevel      { "subOscLevel", 1 };

    // Cross Modulation
    inline const juce::ParameterID crossModMode     { "crossModMode", 1 };
    inline const juce::ParameterID crossMod1to2     { "crossMod1to2", 1 };
    inline const juce::ParameterID crossMod2to1     { "crossMod2to1", 1 };

    // Oscillator 2
    inline const juce::ParameterID osc2Waveform     { "osc2Waveform", 1 };
    inline const juce::ParameterID osc2Coarse       { "osc2Coarse", 1 };
    inline const juce::ParameterID osc2Fine         { "osc2Fine", 1 };
    inline const juce::ParameterID osc2Level        { "osc2Level", 1 };

    // Filter (VCF)
    inline const juce::ParameterID filterCutoff     { "filterCutoff", 1 };
    inline const juce::ParameterID filterResonance  { "filterResonance", 1 };
    inline const juce::ParameterID filterType       { "filterType", 1 };
    inline const juce::ParameterID filterDrive      { "filterDrive", 1 };
    inline const juce::ParameterID filterKeyTrack   { "filterKeyTrack", 1 };
    inline const juce::ParameterID filterEnvAmt     { "filterEnvAmt", 1 };

    // Amp Envelope
    inline const juce::ParameterID ampAttack        { "ampAttack", 1 };
    inline const juce::ParameterID ampDecay         { "ampDecay", 1 };
    inline const juce::ParameterID ampSustain       { "ampSustain", 1 };
    inline const juce::ParameterID ampRelease       { "ampRelease", 1 };

    // Filter Envelope
    inline const juce::ParameterID filterAttack     { "filterAttack", 1 };
    inline const juce::ParameterID filterDecay      { "filterDecay", 1 };
    inline const juce::ParameterID filterSustain    { "filterSustain", 1 };
    inline const juce::ParameterID filterRelease    { "filterRelease", 1 };

    // Mod Envelope
    inline const juce::ParameterID modAttack        { "modAttack", 1 };
    inline const juce::ParameterID modDecay         { "modDecay", 1 };
    inline const juce::ParameterID modSustain       { "modSustain", 1 };
    inline const juce::ParameterID modRelease       { "modRelease", 1 };

    // LFO 1
    inline const juce::ParameterID lfo1Rate         { "lfo1Rate", 1 };
    inline const juce::ParameterID lfo1Shape        { "lfo1Shape", 1 };
    inline const juce::ParameterID lfo1Sync         { "lfo1Sync", 1 };
    inline const juce::ParameterID lfo1SyncRate     { "lfo1SyncRate", 1 };
    inline const juce::ParameterID lfo1Retrig       { "lfo1Retrig", 1 };

    // LFO 2
    inline const juce::ParameterID lfo2Rate         { "lfo2Rate", 1 };
    inline const juce::ParameterID lfo2Shape        { "lfo2Shape", 1 };
    inline const juce::ParameterID lfo2Sync         { "lfo2Sync", 1 };
    inline const juce::ParameterID lfo2SyncRate     { "lfo2SyncRate", 1 };
    inline const juce::ParameterID lfo2Retrig       { "lfo2Retrig", 1 };

    // Modulation Matrix Slot IDs (8 slots)
    constexpr int NUM_MOD_SLOTS = 8;
    inline juce::ParameterID modSlotSource(int slot)
    {
        return juce::ParameterID("modSlotSource_" + juce::String(slot), 1);
    }
    inline juce::ParameterID modSlotDest(int slot)
    {
        return juce::ParameterID("modSlotDest_" + juce::String(slot), 1);
    }
    inline juce::ParameterID modSlotAmount(int slot)
    {
        return juce::ParameterID("modSlotAmount_" + juce::String(slot), 1);
    }

    // Modulation Effect
    inline const juce::ParameterID modFxType        { "modFxType", 1 };
    inline const juce::ParameterID modFxRate        { "modFxRate", 1 };
    inline const juce::ParameterID modFxDepth       { "modFxDepth", 1 };
    inline const juce::ParameterID modFxFeedback    { "modFxFeedback", 1 };
    inline const juce::ParameterID modFxMix         { "modFxMix", 1 };

    // Delay Effect
    inline const juce::ParameterID delayFxType      { "delayFxType", 1 };
    inline const juce::ParameterID delayFxTime      { "delayFxTime", 1 };
    inline const juce::ParameterID delayFxFeedback  { "delayFxFeedback", 1 };
    inline const juce::ParameterID delayFxTone      { "delayFxTone", 1 };
    inline const juce::ParameterID delayFxMix       { "delayFxMix", 1 };
    inline const juce::ParameterID delayFxSync      { "delayFxSync", 1 };
    inline const juce::ParameterID delayFxSyncRate  { "delayFxSyncRate", 1 };

    // Reverb Effect
    inline const juce::ParameterID reverbFxType     { "reverbFxType", 1 };
    inline const juce::ParameterID reverbFxDecay    { "reverbFxDecay", 1 };
    inline const juce::ParameterID reverbFxDamping  { "reverbFxDamping", 1 };
    inline const juce::ParameterID reverbFxTone     { "reverbFxTone", 1 };
    inline const juce::ParameterID reverbFxMix      { "reverbFxMix", 1 };

    // FX Order
    inline const juce::ParameterID fxRoutingOrder   { "fxRoutingOrder", 1 };
}

enum class VoiceMode
{
    Mono = 0,
    MonoUnison = 1,
    PolyMultiMono = 2,
    CyclicRing = 3,
    SympatheticAll = 4,
    RootDriver = 5,
    ChaosDiffuse = 6
};

enum class VoiceXModAlgo
{
    CyclicRing = 0,
    SympatheticAll = 1,
    RootDriver = 2,
    ChaoticDiffuse = 3
};

enum class CrossModMode
{
    FM = 0,
    PhaseMod = 1,
    ThroughZeroFM = 2,
    AM = 3,
    RingMod = 4
};

enum class OscWaveform
{
    Sine = 0,
    Triangle = 1,
    Saw = 2,
    Square = 3
};

enum class SubOscOctave
{
    OctaveMinus1 = 0,
    OctaveMinus2 = 1
};

enum class FilterType
{
    Lowpass24 = 0,
    Lowpass12 = 1,
    Bandpass12 = 2,
    Highpass12 = 3
};

enum class LFOShape
{
    Sine = 0,
    Triangle = 1,
    SawUp = 2,
    SawDown = 3,
    Square = 4,
    SampleAndHold = 5
};

enum class ModSource
{
    None = 0,
    LFO1,
    LFO2,
    AmpEnv,
    FilterEnv,
    ModEnv,
    RandomSH,
    Velocity,
    ModWheel,
    PitchBend,
    Aftertouch,
    KeyTrack,
    Count
};

enum class ModDestination
{
    None = 0,
    Osc1Pitch,
    Osc2Pitch,
    CrossMod1to2,
    CrossMod2to1,
    Osc1Level,
    Osc2Level,
    SubOscLevel,
    FilterCutoff,
    FilterResonance,
    VCALevel,
    VCAPan,
    LFO1Rate,
    LFO2Rate,
    VoiceCrossModDepth,
    // Mod FX
    ModFxRate,
    ModFxDepth,
    ModFxFeedback,
    ModFxMix,
    // Delay FX
    DelayFxTime,
    DelayFxFeedback,
    DelayFxTone,
    DelayFxMix,
    // Reverb FX
    ReverbFxDecay,
    ReverbFxDamping,
    ReverbFxTone,
    ReverbFxMix,
    Osc1Glide,
    Osc2Glide,
    StereoWidth,
    Count
};

class ParameterFactory
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::StringArray getVoiceModeChoices();
    static juce::StringArray getVoiceXModAlgoChoices();
    static juce::StringArray getCrossModModeChoices();
    static juce::StringArray getOscWaveformChoices();
    static juce::StringArray getSubOscOctaveChoices();
    static juce::StringArray getFilterTypeChoices();
    static juce::StringArray getLFOShapeChoices();
    static juce::StringArray getSyncRateChoices();
    static juce::StringArray getModSourceChoices();
    static juce::StringArray getModDestinationChoices();
    static juce::StringArray getModFxTypeChoices();
    static juce::StringArray getDelayFxTypeChoices();
    static juce::StringArray getReverbFxTypeChoices();
    static juce::StringArray getFxOrderChoices();
};
