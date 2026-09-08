#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

namespace IDs
{
    // Pre-Processors: Transient Splitter
    inline const juce::ParameterID transientAttack { "trans_attack", 1 };
    inline const juce::ParameterID transientRelease { "trans_release", 1 };
    inline const juce::ParameterID transientSensitivity { "trans_sens", 1 };
    inline const juce::ParameterID transientThreshold { "trans_thresh", 1 };
    inline const juce::ParameterID detectionMode { "trans_detect_mode", 1 };
    inline const juce::ParameterID lookahead { "trans_lookahead", 1 };
    inline const juce::ParameterID splitAttack { "trans_attack", 1 };
    inline const juce::ParameterID splitRelease { "trans_release", 1 };
    inline const juce::ParameterID splitSensitivity { "trans_sens", 1 };
    inline const juce::ParameterID splitThreshold { "trans_thresh", 1 };
    inline const juce::ParameterID splitMode { "trans_detect_mode", 1 };
    inline const juce::ParameterID splitLookahead { "trans_lookahead", 1 };
    inline const juce::ParameterID soloMode { "trans_solo_mode", 1 };
    inline const juce::ParameterID transientGain { "trans_gain", 1 };
    inline const juce::ParameterID sustainGain { "sust_gain", 1 };
    inline const juce::ParameterID transientPan { "trans_pan", 1 };
    inline const juce::ParameterID sustainPan { "sust_pan", 1 };
    inline const juce::ParameterID transientSolo { "trans_solo", 1 };
    inline const juce::ParameterID sustainSolo { "sust_solo", 1 };
    inline const juce::ParameterID transientMute { "trans_mute", 1 };
    inline const juce::ParameterID sustainMute { "sust_mute", 1 };
    inline const juce::ParameterID engineMode { "engine_mode", 1 };
    inline const juce::ParameterID topViewTab { "top_view_tab", 1 };
    inline const juce::ParameterID preProcSubTab { "preproc_subtab", 1 };

    // Pre-Processors: 3-Band ZDF Filter Bank
    inline juce::ParameterID getFilterType(int b)   { return { "filt" + juce::String(b + 1) + "_type", 1 }; }
    inline juce::ParameterID getFilterFreq(int b)   { return { "filt" + juce::String(b + 1) + "_freq", 1 }; }
    inline juce::ParameterID getFilterQ(int b)      { return { "filt" + juce::String(b + 1) + "_q", 1 }; }
    inline juce::ParameterID getFilterGain(int b)   { return { "filt" + juce::String(b + 1) + "_gain", 1 }; }
    inline juce::ParameterID getFilterDrive(int b)  { return { "filt" + juce::String(b + 1) + "_drive", 1 }; }
    inline juce::ParameterID getFilterBypass(int b) { return { "filt" + juce::String(b + 1) + "_bypass", 1 }; }
    inline juce::ParameterID getFilterSolo(int b)   { return { "filt" + juce::String(b + 1) + "_solo", 1 }; }
    inline juce::ParameterID getFilterMute(int b)   { return { "filt" + juce::String(b + 1) + "_mute", 1 }; }

    // Pre-Processors: Mid/Side & Stereo
    inline const juce::ParameterID midSideWidth   { "midside_width", 1 };
    inline const juce::ParameterID midSideBalance { "midside_balance", 1 };

    // Pre-Processors: Dynamic Splitter (Loud vs Quiet)
    inline const juce::ParameterID dynSplitThreshold { "dynsplit_thresh", 1 };
    inline const juce::ParameterID dynSplitAttack    { "dynsplit_attack", 1 };
    inline const juce::ParameterID dynSplitRelease   { "dynsplit_release", 1 };
    inline const juce::ParameterID dynSplitKnee      { "dynsplit_knee", 1 };

    // Pre-Processors: Spectral & Harmonic Decomposition
    inline const juce::ParameterID spectralHarmonicSens { "spectral_sens", 1 };
    inline const juce::ParameterID spectralFocus        { "spectral_focus", 1 };
    inline const juce::ParameterID spectralSmoothing    { "spectral_smooth", 1 };

    // Pre-Processors: Phase & Spatial Splitting
    inline const juce::ParameterID phaseSpatialThresh { "phase_thresh", 1 };
    inline const juce::ParameterID phaseSpatialWindow { "phase_window", 1 };
    inline const juce::ParameterID phaseSpatialSpread { "phase_spread", 1 };

    // Master
    inline const juce::ParameterID masterBalance { "master_balance", 1 };
    inline const juce::ParameterID masterDryWet { "master_drywet", 1 };
    inline const juce::ParameterID masterGain { "master_gain", 1 };
    inline const juce::ParameterID masterLimiter { "master_limiter", 1 };
    inline const juce::ParameterID outputRoutingMode { "output_routing_mode", 1 };

    // Transient Chain Slots (Slots 1..4)
    inline juce::ParameterID getTransSlotSrc(int i)     { return { "transSlot" + juce::String(i) + "_src", 1 }; }
    inline juce::ParameterID getTransSlotType(int i)    { return { "transSlot" + juce::String(i) + "_type", 1 }; }
    inline juce::ParameterID getTransSlotBypass(int i)  { return { "transSlot" + juce::String(i) + "_bypass", 1 }; }
    inline juce::ParameterID getTransSlotInGain(int i)  { return { "transSlot" + juce::String(i) + "_ingain", 1 }; }
    inline juce::ParameterID getTransSlotOutGain(int i) { return { "transSlot" + juce::String(i) + "_outgain", 1 }; }
    inline juce::ParameterID getTransSlotP1(int i)      { return { "transSlot" + juce::String(i) + "_p1", 1 }; }
    inline juce::ParameterID getTransSlotP2(int i)      { return { "transSlot" + juce::String(i) + "_p2", 1 }; }
    inline juce::ParameterID getTransSlotP3(int i)      { return { "transSlot" + juce::String(i) + "_p3", 1 }; }
    inline juce::ParameterID getTransSlotP4(int i)      { return { "transSlot" + juce::String(i) + "_p4", 1 }; }
    inline juce::ParameterID getTransSlotP5(int i)      { return { "transSlot" + juce::String(i) + "_p5", 1 }; }
    inline juce::ParameterID getTransSlotMix(int i)     { return { "transSlot" + juce::String(i) + "_mix", 1 }; }
    inline juce::ParameterID getTransSlotDest(int i)    { return { "transSlot" + juce::String(i) + "_dest", 1 }; }

    // Sustain Chain Slots (Slots 5..8)
    inline juce::ParameterID getSustSlotSrc(int i)     { return { "sustSlot" + juce::String(i) + "_src", 1 }; }
    inline juce::ParameterID getSustSlotType(int i)    { return { "sustSlot" + juce::String(i) + "_type", 1 }; }
    inline juce::ParameterID getSustSlotBypass(int i)  { return { "sustSlot" + juce::String(i) + "_bypass", 1 }; }
    inline juce::ParameterID getSustSlotInGain(int i)  { return { "sustSlot" + juce::String(i) + "_ingain", 1 }; }
    inline juce::ParameterID getSustSlotOutGain(int i) { return { "sustSlot" + juce::String(i) + "_outgain", 1 }; }
    inline juce::ParameterID getSustSlotP1(int i)      { return { "sustSlot" + juce::String(i) + "_p1", 1 }; }
    inline juce::ParameterID getSustSlotP2(int i)      { return { "sustSlot" + juce::String(i) + "_p2", 1 }; }
    inline juce::ParameterID getSustSlotP3(int i)      { return { "sustSlot" + juce::String(i) + "_p3", 1 }; }
    inline juce::ParameterID getSustSlotP4(int i)      { return { "sustSlot" + juce::String(i) + "_p4", 1 }; }
    inline juce::ParameterID getSustSlotP5(int i)      { return { "sustSlot" + juce::String(i) + "_p5", 1 }; }
    inline juce::ParameterID getSustSlotMix(int i)     { return { "sustSlot" + juce::String(i) + "_mix", 1 }; }
    inline juce::ParameterID getSustSlotDest(int i)    { return { "sustSlot" + juce::String(i) + "_dest", 1 }; }

    // Modulators: 3 LFOs
    inline const juce::ParameterID lfo1Wave { "lfo1_wave", 1 };
    inline const juce::ParameterID lfo1Rate { "lfo1_rate", 1 };
    inline const juce::ParameterID lfo1Sync { "lfo1_sync", 1 };
    inline const juce::ParameterID lfo1Subdiv { "lfo1_subdiv", 1 };
    inline const juce::ParameterID lfo1Smooth { "lfo1_smooth", 1 };

    inline const juce::ParameterID lfo2Wave { "lfo2_wave", 1 };
    inline const juce::ParameterID lfo2Rate { "lfo2_rate", 1 };
    inline const juce::ParameterID lfo2Sync { "lfo2_sync", 1 };
    inline const juce::ParameterID lfo2Subdiv { "lfo2_subdiv", 1 };
    inline const juce::ParameterID lfo2Smooth { "lfo2_smooth", 1 };

    inline const juce::ParameterID lfo3Wave { "lfo3_wave", 1 };
    inline const juce::ParameterID lfo3Rate { "lfo3_rate", 1 };
    inline const juce::ParameterID lfo3Sync { "lfo3_sync", 1 };
    inline const juce::ParameterID lfo3Subdiv { "lfo3_subdiv", 1 };
    inline const juce::ParameterID lfo3Smooth { "lfo3_smooth", 1 };

    // Modulators: 3 Envelope Followers
    inline const juce::ParameterID env1Source { "env1_source", 1 };
    inline const juce::ParameterID env1Attack { "env1_attack", 1 };
    inline const juce::ParameterID env1Release { "env1_release", 1 };
    inline const juce::ParameterID env1Gain { "env1_gain", 1 };

    inline const juce::ParameterID env2Source { "env2_source", 1 };
    inline const juce::ParameterID env2Attack { "env2_attack", 1 };
    inline const juce::ParameterID env2Release { "env2_release", 1 };
    inline const juce::ParameterID env2Gain { "env2_gain", 1 };

    inline const juce::ParameterID env3Source { "env3_source", 1 };
    inline const juce::ParameterID env3Attack { "env3_attack", 1 };
    inline const juce::ParameterID env3Release { "env3_release", 1 };
    inline const juce::ParameterID env3Gain { "env3_gain", 1 };

    // Modulators: 2 Turing Machines
    inline const juce::ParameterID turing1Prob { "turing1_prob", 1 };
    inline const juce::ParameterID turing1Length { "turing1_length", 1 };
    inline const juce::ParameterID turing1Sync { "turing1_sync", 1 };
    inline const juce::ParameterID turing1Subdiv { "turing1_subdiv", 1 };
    inline const juce::ParameterID turing1Rate { "turing1_rate", 1 };
    inline const juce::ParameterID turing1Glide { "turing1_glide", 1 };

    inline const juce::ParameterID turing2Prob { "turing2_prob", 1 };
    inline const juce::ParameterID turing2Length { "turing2_length", 1 };
    inline const juce::ParameterID turing2Sync { "turing2_sync", 1 };
    inline const juce::ParameterID turing2Subdiv { "turing2_subdiv", 1 };
    inline const juce::ParameterID turing2Rate { "turing2_rate", 1 };
    inline const juce::ParameterID turing2Glide { "turing2_glide", 1 };

    // Modulators: 4 Macros
    inline const juce::ParameterID macro1 { "macro1", 1 };
    inline const juce::ParameterID macro2 { "macro2", 1 };
    inline const juce::ParameterID macro3 { "macro3", 1 };
    inline const juce::ParameterID macro4 { "macro4", 1 };

    // Modulators: 2 Loopable XY Pads
    inline const juce::ParameterID xy1_x { "xy1_x", 1 };
    inline const juce::ParameterID xy1_y { "xy1_y", 1 };
    inline const juce::ParameterID xy1_speed { "xy1_speed", 1 };
    inline const juce::ParameterID xy1_rec { "xy1_rec", 1 };
    inline const juce::ParameterID xy1_play { "xy1_play", 1 };

    inline const juce::ParameterID xy2_x { "xy2_x", 1 };
    inline const juce::ParameterID xy2_y { "xy2_y", 1 };
    inline const juce::ParameterID xy2_speed { "xy2_speed", 1 };
    inline const juce::ParameterID xy2_rec { "xy2_rec", 1 };
    inline const juce::ParameterID xy2_play { "xy2_play", 1 };

    // Modulation Matrix (8 routing slots)
    inline juce::ParameterID getModSlotSource(int i) { return { "mod_slot" + juce::String(i) + "_src", 1 }; }
    inline juce::ParameterID getModSlotDest(int i)   { return { "mod_slot" + juce::String(i) + "_dest", 1 }; }
    inline juce::ParameterID getModSlotDepth(int i)  { return { "mod_slot" + juce::String(i) + "_depth", 1 }; }
}

class ParameterFactory
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
};
