#pragma once

#include <array>
#include <atomic>
#include "../Parameters/ParameterIDs.h"

struct ModSourceValues
{
    float lfo1 = 0.0f;
    float lfo2 = 0.0f;
    float ampEnv = 0.0f;
    float filterEnv = 0.0f;
    float modEnv = 0.0f;
    float randomSH = 0.0f;
    float velocity = 0.0f;
    float modWheel = 0.0f;
    float pitchBend = 0.0f;
    float aftertouch = 0.0f;
    float keyTrack = 0.0f;

    float get(ModSource src) const
    {
        switch (src)
        {
            case ModSource::LFO1:       return lfo1;
            case ModSource::LFO2:       return lfo2;
            case ModSource::AmpEnv:     return ampEnv;
            case ModSource::FilterEnv:  return filterEnv;
            case ModSource::ModEnv:     return modEnv;
            case ModSource::RandomSH:   return randomSH;
            case ModSource::Velocity:   return velocity;
            case ModSource::ModWheel:   return modWheel;
            case ModSource::PitchBend:  return pitchBend;
            case ModSource::Aftertouch: return aftertouch;
            case ModSource::KeyTrack:   return keyTrack;
            default:                    return 0.0f;
        }
    }
};

struct ModDestinationOffsets
{
    float osc1PitchSemis = 0.0f;
    float osc2PitchSemis = 0.0f;
    float crossMod1to2 = 0.0f;
    float crossMod2to1 = 0.0f;
    float osc1Level = 0.0f;
    float osc2Level = 0.0f;
    float subOscLevel = 0.0f;
    float filterCutoffOctaves = 0.0f;
    float filterResonance = 0.0f;
    float vcaLevel = 0.0f;
    float vcaPan = 0.0f;
    float lfo1RateMod = 0.0f;
    float lfo2RateMod = 0.0f;
    float voiceCrossMod = 0.0f;

    // Mod FX
    float modFxRate = 0.0f;
    float modFxDepth = 0.0f;
    float modFxFeedback = 0.0f;
    float modFxMix = 0.0f;

    // Delay FX
    float delayFxTime = 0.0f;
    float delayFxFeedback = 0.0f;
    float delayFxTone = 0.0f;
    float delayFxMix = 0.0f;

    // Reverb FX
    float reverbFxDecay = 0.0f;
    float reverbFxDamping = 0.0f;
    float reverbFxTone = 0.0f;
    float reverbFxMix = 0.0f;

    // Glide Speeds & Stereo
    float osc1Glide = 0.0f;
    float osc2Glide = 0.0f;
    float stereoWidth = 0.0f;

    void reset()
    {
        osc1PitchSemis = 0.0f;
        osc2PitchSemis = 0.0f;
        crossMod1to2 = 0.0f;
        crossMod2to1 = 0.0f;
        osc1Level = 0.0f;
        osc2Level = 0.0f;
        subOscLevel = 0.0f;
        filterCutoffOctaves = 0.0f;
        filterResonance = 0.0f;
        vcaLevel = 0.0f;
        vcaPan = 0.0f;
        lfo1RateMod = 0.0f;
        lfo2RateMod = 0.0f;
        voiceCrossMod = 0.0f;

        modFxRate = 0.0f;
        modFxDepth = 0.0f;
        modFxFeedback = 0.0f;
        modFxMix = 0.0f;

        delayFxTime = 0.0f;
        delayFxFeedback = 0.0f;
        delayFxTone = 0.0f;
        delayFxMix = 0.0f;

        reverbFxDecay = 0.0f;
        reverbFxDamping = 0.0f;
        reverbFxTone = 0.0f;
        reverbFxMix = 0.0f;

        osc1Glide = 0.0f;
        osc2Glide = 0.0f;
        stereoWidth = 0.0f;
    }
};

struct ModMatrixSlot
{
    ModSource source = ModSource::None;
    ModDestination destination = ModDestination::None;
    float amount = 0.0f;
};

class ModMatrix
{
public:
    ModMatrix()
    {
        numActiveSlots = 0;
    }

    void setSlot(int slotIndex, ModSource src, ModDestination dest, float amount)
    {
        if (slotIndex >= 0 && slotIndex < CrossModIDs::NUM_MOD_SLOTS)
        {
            slots[slotIndex].source = src;
            slots[slotIndex].destination = dest;
            slots[slotIndex].amount = amount;
            updateActiveSlots();
        }
    }

    inline void evaluate(const ModSourceValues& sources, ModDestinationOffsets& offsets) const noexcept
    {
        for (int idx = 0; idx < numActiveSlots; ++idx)
        {
            const auto& slot = slots[activeSlotIndices[idx]];
            float rawSrc = sources.get(slot.source);
            float modulation = rawSrc * slot.amount;

            switch (slot.destination)
            {
                case ModDestination::Osc1Pitch:
                    // Up to +/- 24 semitones of pitch modulation
                    offsets.osc1PitchSemis += modulation * 24.0f;
                    break;

                case ModDestination::Osc2Pitch:
                    offsets.osc2PitchSemis += modulation * 24.0f;
                    break;

                case ModDestination::CrossMod1to2:
                    offsets.crossMod1to2 += modulation;
                    break;

                case ModDestination::CrossMod2to1:
                    offsets.crossMod2to1 += modulation;
                    break;

                case ModDestination::Osc1Level:
                    offsets.osc1Level += modulation;
                    break;

                case ModDestination::Osc2Level:
                    offsets.osc2Level += modulation;
                    break;

                case ModDestination::SubOscLevel:
                    offsets.subOscLevel += modulation;
                    break;

                case ModDestination::FilterCutoff:
                    // Up to +/- 5 octaves of cutoff modulation
                    offsets.filterCutoffOctaves += modulation * 5.0f;
                    break;

                case ModDestination::FilterResonance:
                    offsets.filterResonance += modulation * 0.8f;
                    break;

                case ModDestination::VCALevel:
                    offsets.vcaLevel += modulation;
                    break;

                case ModDestination::VCAPan:
                    offsets.vcaPan += modulation;
                    break;

                case ModDestination::LFO1Rate:
                    offsets.lfo1RateMod += modulation;
                    break;

                case ModDestination::LFO2Rate:
                    offsets.lfo2RateMod += modulation;
                    break;

                case ModDestination::VoiceCrossModDepth:
                    offsets.voiceCrossMod += modulation;
                    break;

                // Mod FX
                case ModDestination::ModFxRate:
                    offsets.modFxRate += modulation * 5.0f; // up to +/- 5 Hz
                    break;

                case ModDestination::ModFxDepth:
                    offsets.modFxDepth += modulation;
                    break;

                case ModDestination::ModFxFeedback:
                    offsets.modFxFeedback += modulation * 0.5f;
                    break;

                case ModDestination::ModFxMix:
                    offsets.modFxMix += modulation;
                    break;

                // Delay FX
                case ModDestination::DelayFxTime:
                    offsets.delayFxTime += modulation * 0.8f; // up to +/- 0.8 seconds
                    break;

                case ModDestination::DelayFxFeedback:
                    offsets.delayFxFeedback += modulation * 0.5f;
                    break;

                case ModDestination::DelayFxTone:
                    offsets.delayFxTone += modulation;
                    break;

                case ModDestination::DelayFxMix:
                    offsets.delayFxMix += modulation;
                    break;

                // Reverb FX
                case ModDestination::ReverbFxDecay:
                    offsets.reverbFxDecay += modulation * 4.0f; // up to +/- 4 seconds
                    break;

                case ModDestination::ReverbFxDamping:
                    offsets.reverbFxDamping += modulation;
                    break;

                case ModDestination::ReverbFxTone:
                    offsets.reverbFxTone += modulation;
                    break;

                case ModDestination::ReverbFxMix:
                    offsets.reverbFxMix += modulation;
                    break;

                // Glide Speeds
                case ModDestination::Osc1Glide:
                    offsets.osc1Glide += modulation;
                    break;

                case ModDestination::Osc2Glide:
                    offsets.osc2Glide += modulation;
                    break;

                case ModDestination::StereoWidth:
                    offsets.stereoWidth += modulation;
                    break;

                default:
                    break;
            }
        }
    }

private:
    void updateActiveSlots()
    {
        int count = 0;
        for (int i = 0; i < CrossModIDs::NUM_MOD_SLOTS; ++i)
        {
            if (slots[i].source != ModSource::None &&
                slots[i].destination != ModDestination::None &&
                std::abs(slots[i].amount) > 0.0001f)
            {
                activeSlotIndices[count++] = i;
            }
        }
        numActiveSlots = count;
    }

    std::array<ModMatrixSlot, CrossModIDs::NUM_MOD_SLOTS> slots;
    std::array<int, CrossModIDs::NUM_MOD_SLOTS> activeSlotIndices {};
    int numActiveSlots = 0;
};

