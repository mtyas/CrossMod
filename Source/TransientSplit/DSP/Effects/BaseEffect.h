#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

enum class EffectType
{
    Bypass = 0,

    // 1. Dynamics (1..4)
    CompressorVCA = 1,
    CompressorOpto = 2,
    CompressorFET = 3,
    Gate = 4,

    // 2. EQ & Filter (5..9)
    EqualizerParametric = 5,
    EqualizerGraphic = 6,
    EqualizerTilt = 7,
    FilterDualHPLP = 8,
    FilterBandpass = 9,

    // 3. Distortion & Saturation (10..16)
    DistortionHardClip = 10,
    DistortionTube = 11,
    DistortionWavefolder = 12,
    FuzzGermanium = 13,
    FuzzOctave = 14,
    Overdrive = 15,
    TapeSat = 16,

    // 4. Lo-Fi & Glitch (17..21)
    Bitcrush = 17,
    GlitchStutter = 18,
    GlitchTapeStop = 19,
    GlitchReverse = 20,
    GlitchGranular = 21,

    // 5. Pitch & Modulation (22..27)
    PitchShifter = 22,
    FrequencyShifter = 23,
    ModulationChorus = 24,
    ModulationFlanger = 25,
    ModulationPhaser = 26,
    ModulationTremolo = 27,

    // 6. Delay (28..30)
    DelayStereo = 28,
    DelayPingPong = 29,
    DelayTapeEcho = 30,

    // 7. Reverb (31..35)
    ReverbRoom = 31,
    ReverbHall = 32,
    ReverbPlate = 33,
    ReverbSpring = 34,
    ReverbShimmer = 35
};

struct EffectParamInfo
{
    juce::String name;
    juce::String label;
    float defaultVal;
    float minVal;
    float maxVal;
};

struct TempoSyncHelper
{
    // Returns subdivision duration in seconds based on bpm
    // divisions: 0 = Free, 1 = 1/32, 2 = 1/16, 3 = 1/8T, 4 = 1/16D, 5 = 1/8, 6 = 1/4T, 7 = 1/8D, 8 = 1/4, 9 = 1/2T, 10 = 1/4D, 11 = 1/2, 12 = 1 Bar, 13 = 2 Bars, 14 = 4 Bars
    static double getSubdivisionSeconds(float syncKnobVal, double bpm)
    {
        if (syncKnobVal <= 0.04f) return 0.0; // 0 = Free
        if (bpm <= 10.0 || bpm > 400.0) bpm = 120.0;
        const double beatSec = 60.0 / bpm; // 1/4 note duration

        const int div = std::clamp(static_cast<int>(syncKnobVal * 14.0f + 0.5f), 1, 14);
        switch (div)
        {
            case 1:  return beatSec * 0.125;       // 1/32
            case 2:  return beatSec * 0.25;        // 1/16
            case 3:  return beatSec * (1.0 / 3.0); // 1/8T
            case 4:  return beatSec * 0.375;       // 1/16D
            case 5:  return beatSec * 0.5;         // 1/8
            case 6:  return beatSec * (2.0 / 3.0); // 1/4T
            case 7:  return beatSec * 0.75;        // 1/8D
            case 8:  return beatSec * 1.0;         // 1/4
            case 9:  return beatSec * (4.0 / 3.0); // 1/2T
            case 10: return beatSec * 1.5;         // 1/4D
            case 11: return beatSec * 2.0;         // 1/2
            case 12: return beatSec * 4.0;         // 1 Bar
            case 13: return beatSec * 8.0;         // 2 Bars
            case 14: return beatSec * 16.0;        // 4 Bars
            default: return 0.0;
        }
    }
};

class BaseEffect
{
public:
    virtual ~BaseEffect() = default;

    virtual EffectType getType() const { return EffectType::Bypass; }
    virtual juce::String getName() const { return "Effect"; }

    virtual void prepare(double sampleRate, int maxBlockSize) { currentSampleRate = std::max(1000.0, sampleRate); }
    virtual void reset() = 0;
    virtual void setBpm(double bpm) { currentBpm = (bpm > 20.0 && bpm < 400.0) ? bpm : 120.0; }

    virtual void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) = 0;
    virtual void process(float* bufferL, float* bufferR, int numSamples) = 0;
    virtual EffectParamInfo getParamInfo(int /*index*/) const { return { "Param", "", 0.0f, 0.0f, 1.0f }; }

protected:
    double currentSampleRate = 48000.0;
    double currentBpm = 120.0;
};
