#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <string>

enum class ModSource
{
    None = 0,
    LFO1 = 1,
    LFO2 = 2,
    LFO3 = 3,
    EnvFollower1 = 4,
    EnvFollower2 = 5,
    EnvFollower3 = 6,
    Turing1 = 7,
    Turing2 = 8,
    Macro1 = 9,
    Macro2 = 10,
    Macro3 = 11,
    Macro4 = 12,
    XY1_X = 13,
    XY1_Y = 14,
    XY2_X = 15,
    XY2_Y = 16,
    TransientEnv = 17,
    SustainEnv = 18,
    Filt1Env = 19,
    Filt2Env = 20,
    Filt3Env = 21
};

enum class ModDest
{
    None = 0,

    // Pre-Processors: Transient Splitter
    SplitAttack = 1,
    SplitRelease = 2,
    SplitSensitivity = 3,
    SplitThreshold = 4,

    // Pre-Processors: 3-Band Filters
    Filt1Freq = 5, Filt1Q = 6, Filt1Gain = 7, Filt1Drive = 8,
    Filt2Freq = 9, Filt2Q = 10, Filt2Gain = 11, Filt2Drive = 12,
    Filt3Freq = 13, Filt3Q = 14, Filt3Gain = 15, Filt3Drive = 16,

    // Slots 1 to 8: Parameters 1..5 and Mix
    Slot1_P1 = 17, Slot1_P2 = 18, Slot1_P3 = 19, Slot1_P4 = 20, Slot1_P5 = 21, Slot1_Mix = 22,
    Slot2_P1 = 23, Slot2_P2 = 24, Slot2_P3 = 25, Slot2_P4 = 26, Slot2_P5 = 27, Slot2_Mix = 28,
    Slot3_P1 = 29, Slot3_P2 = 30, Slot3_P3 = 31, Slot3_P4 = 32, Slot3_P5 = 33, Slot3_Mix = 34,
    Slot4_P1 = 35, Slot4_P2 = 36, Slot4_P3 = 37, Slot4_P4 = 38, Slot4_P5 = 39, Slot4_Mix = 40,
    Slot5_P1 = 41, Slot5_P2 = 42, Slot5_P3 = 43, Slot5_P4 = 44, Slot5_P5 = 45, Slot5_Mix = 46,
    Slot6_P1 = 47, Slot6_P2 = 48, Slot6_P3 = 49, Slot6_P4 = 50, Slot6_P5 = 51, Slot6_Mix = 52,
    Slot7_P1 = 53, Slot7_P2 = 54, Slot7_P3 = 55, Slot7_P4 = 56, Slot7_P5 = 57, Slot7_Mix = 58,
    Slot8_P1 = 59, Slot8_P2 = 60, Slot8_P3 = 61, Slot8_P4 = 62, Slot8_P5 = 63, Slot8_Mix = 64,

    // Modulators Speed & Rate
    LFO1_Rate = 65,
    LFO2_Rate = 66,
    LFO3_Rate = 67,
    XY1_Speed = 68,
    XY2_Speed = 69,
    Turing1_Prob = 70,
    Turing2_Prob = 71,

    // Master
    Master_Balance = 72,
    Master_DryWet = 73,
    Master_Gain = 74
};

struct ModRoutingSlot
{
    ModSource source = ModSource::None;
    ModDest dest = ModDest::None;
    float depth = 0.0f; // Bipolar -1.0 to 1.0
    float liveOutput = 0.0f; // Live calculated value: srcVal * depth
};

class ModMatrix
{
public:
    static constexpr int NumRoutingSlots = 8;
    static constexpr int NumDestinations = 75;
    static constexpr int NumSources = 22;

    ModMatrix() = default;

    void setSlot(int index, ModSource src, ModDest dst, float amount)
    {
        if (index >= 0 && index < NumRoutingSlots)
        {
            routings[index].source = src;
            routings[index].dest = dst;
            routings[index].depth = std::clamp(amount, -1.0f, 1.0f);
        }
    }

    const ModRoutingSlot& getSlot(int index) const noexcept
    {
        return routings[std::clamp(index, 0, NumRoutingSlots - 1)];
    }

    float getSlotLiveOutput(int index) const noexcept
    {
        if (index >= 0 && index < NumRoutingSlots)
            return routings[index].liveOutput;
        return 0.0f;
    }

    void calculateOffsets(const std::array<float, NumSources>& sourceValues,
                          std::array<float, NumDestinations>& destOffsets)
    {
        destOffsets.fill(0.0f);

        for (int i = 0; i < NumRoutingSlots; ++i)
        {
            const auto& slot = routings[i];
            if (slot.source == ModSource::None || slot.dest == ModDest::None)
            {
                routings[i].liveOutput = 0.0f;
                continue;
            }

            const int srcIdx = static_cast<int>(slot.source);
            float srcVal = 0.0f;
            if (srcIdx >= 0 && srcIdx < NumSources)
                srcVal = sourceValues[srcIdx];

            const float slotMod = srcVal * slot.depth;
            routings[i].liveOutput = slotMod;

            const int dstIdx = static_cast<int>(slot.dest);
            if (dstIdx >= 0 && dstIdx < NumDestinations)
            {
                destOffsets[dstIdx] += slotMod;
            }
        }
    }

    static juce::String getSourceName(ModSource src)
    {
        switch (src)
        {
            case ModSource::None:         return "None";
            case ModSource::LFO1:         return "LFO 1";
            case ModSource::LFO2:         return "LFO 2";
            case ModSource::LFO3:         return "LFO 3";
            case ModSource::EnvFollower1: return "Env Follow 1";
            case ModSource::EnvFollower2: return "Env Follow 2";
            case ModSource::EnvFollower3: return "Env Follow 3";
            case ModSource::Turing1:      return "Turing 1";
            case ModSource::Turing2:      return "Turing 2";
            case ModSource::Macro1:       return "Macro 1";
            case ModSource::Macro2:       return "Macro 2";
            case ModSource::Macro3:       return "Macro 3";
            case ModSource::Macro4:       return "Macro 4";
            case ModSource::XY1_X:        return "XY Pad 1 X";
            case ModSource::XY1_Y:        return "XY Pad 1 Y";
            case ModSource::XY2_X:        return "XY Pad 2 X";
            case ModSource::XY2_Y:        return "XY Pad 2 Y";
            case ModSource::TransientEnv: return "Audio: Transient Env";
            case ModSource::SustainEnv:   return "Audio: Sustain Env";
            case ModSource::Filt1Env:     return "Audio: Filter 1 Env";
            case ModSource::Filt2Env:     return "Audio: Filter 2 Env";
            case ModSource::Filt3Env:     return "Audio: Filter 3 Env";
            default:                      return "None";
        }
    }

    static juce::String getDestName(ModDest dst)
    {
        const int d = static_cast<int>(dst);
        if (d == 0) return "None";

        if (d == 1) return "Pre: Split Attack";
        if (d == 2) return "Pre: Split Release";
        if (d == 3) return "Pre: Split Sens";
        if (d == 4) return "Pre: Split Thresh";

        if (d == 5) return "Pre: Filt 1 Freq";
        if (d == 6) return "Pre: Filt 1 Q";
        if (d == 7) return "Pre: Filt 1 Gain";
        if (d == 8) return "Pre: Filt 1 Drive";

        if (d == 9) return "Pre: Filt 2 Freq";
        if (d == 10) return "Pre: Filt 2 Q";
        if (d == 11) return "Pre: Filt 2 Gain";
        if (d == 12) return "Pre: Filt 2 Drive";

        if (d == 13) return "Pre: Filt 3 Freq";
        if (d == 14) return "Pre: Filt 3 Q";
        if (d == 15) return "Pre: Filt 3 Gain";
        if (d == 16) return "Pre: Filt 3 Drive";

        // Slots 1 to 8 (17 to 64)
        if (d >= 17 && d <= 64)
        {
            int slotIdx = (d - 17) / 6 + 1;
            int paramIdx = (d - 17) % 6;
            if (paramIdx < 5)
                return "Slot " + juce::String(slotIdx) + " Param " + juce::String(paramIdx + 1);
            return "Slot " + juce::String(slotIdx) + " Mix";
        }

        if (d == 65) return "Mod: LFO 1 Rate";
        if (d == 66) return "Mod: LFO 2 Rate";
        if (d == 67) return "Mod: LFO 3 Rate";
        if (d == 68) return "Mod: XY 1 Speed";
        if (d == 69) return "Mod: XY 2 Speed";
        if (d == 70) return "Mod: Turing 1 Prob";
        if (d == 71) return "Mod: Turing 2 Prob";

        if (d == 72) return "Master: Balance";
        if (d == 73) return "Master: Dry/Wet";
        if (d == 74) return "Master: Gain";

        return "None";
    }

private:
    std::array<ModRoutingSlot, NumRoutingSlots> routings;
};
