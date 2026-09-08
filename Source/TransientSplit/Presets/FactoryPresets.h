#pragma once

#include <juce_core/juce_core.h>
#include "../DSP/Effects/BaseEffect.h"
#include <vector>
#include <map>

struct PresetData
{
    juce::String name;
    juce::String category;
    std::map<juce::String, float> paramValues;
};

class FactoryPresets
{
public:
    static std::vector<PresetData> getPresets()
    {
        std::vector<PresetData> presets;

        // 1. Default Clean Split
        {
            PresetData p;
            p.name = "01 Clean Split";
            p.category = "Utility";
            p.paramValues["splitAttack"] = 20.0f;
            p.paramValues["splitRelease"] = 150.0f;
            p.paramValues["splitSensitivity"] = 1.0f;
            p.paramValues["splitThreshold"] = -30.0f;
            p.paramValues["splitMode"] = 0.0f;
            p.paramValues["masterBalance"] = 0.0f;
            p.paramValues["masterDryWet"] = 1.0f;
            p.paramValues["masterGain"] = 0.0f;
            p.paramValues["masterLimiter"] = 1.0f;
            presets.push_back(p);
        }

        // 2. Punchy Drums (FET Compressor on Transients + Tape & Room Reverb on Sustain)
        {
            PresetData p;
            p.name = "02 Punchy Drums";
            p.category = "Drums";
            p.paramValues["splitAttack"] = 15.0f;
            p.paramValues["splitRelease"] = 120.0f;
            p.paramValues["splitSensitivity"] = 1.3f;
            p.paramValues["transientGain"] = 3.0f;
            // Transient Slot 0: FET Compressor
            p.paramValues["transSlot0_type"] = static_cast<float>(EffectType::CompressorFET);
            p.paramValues["transSlot0_p1"] = 0.6f; // Drive
            p.paramValues["transSlot0_p2"] = 0.5f; // Ratio
            p.paramValues["transSlot0_p3"] = 0.7f; // Fast Attack
            p.paramValues["transSlot0_p4"] = 0.8f; // Fast Release
            p.paramValues["transSlot0_mix"] = 0.85f;
            // Sustain Slot 0: Tape Saturation
            p.paramValues["sustSlot0_type"] = static_cast<float>(EffectType::TapeSat);
            p.paramValues["sustSlot0_p1"] = 0.6f;
            p.paramValues["sustSlot0_p2"] = 0.15f;
            p.paramValues["sustSlot0_p3"] = 0.8f;
            p.paramValues["sustSlot0_mix"] = 0.7f;
            // Sustain Slot 1: Room Reverb
            p.paramValues["sustSlot1_type"] = static_cast<float>(EffectType::ReverbRoom);
            p.paramValues["sustSlot1_p1"] = 0.4f;
            p.paramValues["sustSlot1_p2"] = 0.35f;
            p.paramValues["sustSlot1_p3"] = 0.4f;
            p.paramValues["sustSlot1_p4"] = 0.2f;
            p.paramValues["sustSlot1_mix"] = 0.25f;
            presets.push_back(p);
        }

        // 3. Lush Shimmer Pluck (Germanium Fuzz Transients + Shimmer Reverb & Delay on Sustain)
        {
            PresetData p;
            p.name = "03 Shimmer Pluck";
            p.category = "Synth";
            p.paramValues["splitAttack"] = 35.0f;
            p.paramValues["splitRelease"] = 200.0f;
            p.paramValues["splitSensitivity"] = 1.1f;
            // Transient Slot 0: Germanium Fuzz
            p.paramValues["transSlot0_type"] = static_cast<float>(EffectType::FuzzGermanium);
            p.paramValues["transSlot0_p1"] = 0.35f;
            p.paramValues["transSlot0_p2"] = 0.65f;
            p.paramValues["transSlot0_p3"] = 0.7f;
            p.paramValues["transSlot0_mix"] = 0.6f;
            // Sustain Slot 0: Stereo Chorus
            p.paramValues["sustSlot0_type"] = static_cast<float>(EffectType::ModulationChorus);
            p.paramValues["sustSlot0_p1"] = 0.25f;
            p.paramValues["sustSlot0_p2"] = 0.7f;
            p.paramValues["sustSlot0_p3"] = 0.6f;
            p.paramValues["sustSlot0_mix"] = 0.8f;
            // Sustain Slot 1: Stereo Delay
            p.paramValues["sustSlot1_type"] = static_cast<float>(EffectType::DelayStereo);
            p.paramValues["sustSlot1_p1"] = 0.4f;
            p.paramValues["sustSlot1_p2"] = 0.5f;
            p.paramValues["sustSlot1_p3"] = 0.6f;
            p.paramValues["sustSlot1_p4"] = 0.7f;
            p.paramValues["sustSlot1_mix"] = 0.4f;
            // Sustain Slot 2: Shimmer Reverb
            p.paramValues["sustSlot2_type"] = static_cast<float>(EffectType::ReverbShimmer);
            p.paramValues["sustSlot2_p1"] = 0.8f;
            p.paramValues["sustSlot2_p2"] = 0.75f;
            p.paramValues["sustSlot2_p3"] = 0.8f;
            p.paramValues["sustSlot2_mix"] = 0.55f;
            presets.push_back(p);
        }

        // 4. Glitch & Bode Cyber Drum (Glitch Stutter on Transients + Bode Shift on Sustain)
        {
            PresetData p;
            p.name = "04 Cyber Stutter";
            p.category = "Glitch";
            p.paramValues["splitAttack"] = 14.0f;
            p.paramValues["splitRelease"] = 130.0f;
            // Transient Slot 0: Glitch Stutter
            p.paramValues["transSlot0_type"] = static_cast<float>(EffectType::GlitchStutter);
            p.paramValues["transSlot0_p1"] = 0.25f; // 1/16 slice
            p.paramValues["transSlot0_p2"] = 0.7f;  // Trigger prob
            p.paramValues["transSlot0_p3"] = 0.4f;  // Burst count
            p.paramValues["transSlot0_p4"] = 0.3f;  // Decay feedback
            p.paramValues["transSlot0_mix"] = 0.85f;
            // Sustain Slot 0: Bode Frequency Shifter
            p.paramValues["sustSlot0_type"] = static_cast<float>(EffectType::FrequencyShifter);
            p.paramValues["sustSlot0_p1"] = 0.53f; // Shift +120Hz
            p.paramValues["sustSlot0_p2"] = 0.5f;
            p.paramValues["sustSlot0_p3"] = 0.5f;
            p.paramValues["sustSlot0_p4"] = 0.2f;
            p.paramValues["sustSlot0_mix"] = 0.6f;
            // Sustain Slot 1: Ping-Pong Delay
            p.paramValues["sustSlot1_type"] = static_cast<float>(EffectType::DelayPingPong);
            p.paramValues["sustSlot1_p1"] = 0.35f;
            p.paramValues["sustSlot1_p2"] = 0.45f;
            p.paramValues["sustSlot1_mix"] = 0.4f;
            presets.push_back(p);
        }

        // 5. Aggressive Bass (Hard Clip Distortion on Transients + Opto Glue on Sustain)
        {
            PresetData p;
            p.name = "05 Aggressive Bass";
            p.category = "Bass";
            p.paramValues["splitAttack"] = 25.0f;
            p.paramValues["splitRelease"] = 180.0f;
            p.paramValues["transientGain"] = 2.0f;
            // Transient Slot 0: Distortion (Hard clip)
            p.paramValues["transSlot0_type"] = static_cast<float>(EffectType::DistortionHardClip);
            p.paramValues["transSlot0_p1"] = 0.65f;
            p.paramValues["transSlot0_p2"] = 0.8f;
            p.paramValues["transSlot0_p3"] = 0.5f;
            p.paramValues["transSlot0_mix"] = 0.9f;
            // Sustain Slot 0: Opto Compressor
            p.paramValues["sustSlot0_type"] = static_cast<float>(EffectType::CompressorOpto);
            p.paramValues["sustSlot0_p1"] = 0.55f;
            p.paramValues["sustSlot0_p2"] = 0.6f;
            p.paramValues["sustSlot0_p3"] = 0.5f;
            p.paramValues["sustSlot0_p4"] = 0.4f;
            p.paramValues["sustSlot0_mix"] = 0.8f;
            // Sustain Slot 1: Stereo Chorus
            p.paramValues["sustSlot1_type"] = static_cast<float>(EffectType::ModulationChorus);
            p.paramValues["sustSlot1_p1"] = 0.2f;
            p.paramValues["sustSlot1_p2"] = 0.4f;
            p.paramValues["sustSlot1_mix"] = 0.35f;
            presets.push_back(p);
        }

        // 6. Ambient Swells (Tilt EQ & Spring Reverb on Sustain)
        {
            PresetData p;
            p.name = "06 Ambient Swells";
            p.category = "Ambient";
            p.paramValues["splitMode"] = 1.0f; // Extended Attack mode
            p.paramValues["splitAttack"] = 80.0f;
            p.paramValues["splitRelease"] = 350.0f;
            p.paramValues["transientGain"] = -4.0f;
            p.paramValues["sustainGain"] = 3.0f;
            // Sustain Slot 0: Tilt EQ
            p.paramValues["sustSlot0_type"] = static_cast<float>(EffectType::EqualizerTilt);
            p.paramValues["sustSlot0_p1"] = 0.7f; // Warm tilt
            p.paramValues["sustSlot0_p2"] = 0.65f;
            p.paramValues["sustSlot0_mix"] = 1.0f;
            // Sustain Slot 1: Spring Reverb
            p.paramValues["sustSlot1_type"] = static_cast<float>(EffectType::ReverbSpring);
            p.paramValues["sustSlot1_p1"] = 0.85f;
            p.paramValues["sustSlot1_p2"] = 0.5f;
            p.paramValues["sustSlot1_p3"] = 0.4f;
            p.paramValues["sustSlot1_p4"] = 0.1f;
            p.paramValues["sustSlot1_mix"] = 0.7f;
            presets.push_back(p);
        }

        // 7. Pitch-Shifted Snare (+12 Semitones on Transients + Tape Echo)
        {
            PresetData p;
            p.name = "07 Octave Snare";
            p.category = "Drums";
            p.paramValues["splitAttack"] = 12.0f;
            p.paramValues["splitRelease"] = 100.0f;
            p.paramValues["splitSensitivity"] = 1.4f;
            // Transient Slot 0: Pitch Shifter (+12 Semitones)
            p.paramValues["transSlot0_type"] = static_cast<float>(EffectType::PitchShifter);
            p.paramValues["transSlot0_p1"] = 0.75f; // +12 st
            p.paramValues["transSlot0_p2"] = 0.5f;  // 0 cents
            p.paramValues["transSlot0_p3"] = 0.35f; // 40ms window
            p.paramValues["transSlot0_mix"] = 0.75f;
            // Sustain Slot 0: Tape Echo (Slapback)
            p.paramValues["sustSlot0_type"] = static_cast<float>(EffectType::DelayTapeEcho);
            p.paramValues["sustSlot0_p1"] = 0.08f; // ~90ms
            p.paramValues["sustSlot0_p2"] = 0.2f;
            p.paramValues["sustSlot0_p3"] = 0.6f;
            p.paramValues["sustSlot0_mix"] = 0.6f;
            presets.push_back(p);
        }

        // 8. Lo-Fi Nostalgia (Bitcrusher on Transients + Tape Sat & Plate on Sustain)
        {
            PresetData p;
            p.name = "08 Lo-Fi Nostalgia";
            p.category = "Keys";
            p.paramValues["splitAttack"] = 30.0f;
            p.paramValues["splitRelease"] = 220.0f;
            // Transient Slot 0: Bitcrusher
            p.paramValues["transSlot0_type"] = static_cast<float>(EffectType::Bitcrush);
            p.paramValues["transSlot0_p1"] = 0.6f; // ~10 bits
            p.paramValues["transSlot0_p2"] = 0.4f; // ~18kHz downsample
            p.paramValues["transSlot0_p3"] = 0.2f;
            p.paramValues["transSlot0_mix"] = 0.8f;
            // Sustain Slot 0: Tape Saturation
            p.paramValues["sustSlot0_type"] = static_cast<float>(EffectType::TapeSat);
            p.paramValues["sustSlot0_p1"] = 0.6f;
            p.paramValues["sustSlot0_p2"] = 0.7f;
            p.paramValues["sustSlot0_p3"] = 0.35f;
            p.paramValues["sustSlot0_mix"] = 0.9f;
            // Sustain Slot 1: Plate Reverb
            p.paramValues["sustSlot1_type"] = static_cast<float>(EffectType::ReverbPlate);
            p.paramValues["sustSlot1_p1"] = 0.65f;
            p.paramValues["sustSlot1_p2"] = 0.4f;
            p.paramValues["sustSlot1_p3"] = 0.7f;
            p.paramValues["sustSlot1_mix"] = 0.4f;
            presets.push_back(p);
        }

        return presets;
    }
};
