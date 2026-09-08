#pragma once

#include "BaseEffect.h"

class FilterEffect : public BaseEffect
{
public:
    FilterEffect() = default;

    EffectType getType() const override { return EffectType::Filter; }
    juce::String getName() const override { return "Tone / Filter"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        s1L = 0.0f; s2L = 0.0f;
        s1R = 0.0f; s2R = 0.0f;
    }

    void setParameters(float p1Freq, float p2Res, float p3Type, float p4Gain, float mixVal) override
    {
        cutoffHz = 20.0f * std::pow(1000.0f, std::clamp(p1Freq, 0.0f, 1.0f)); // 20Hz to 20kHz
        qFactor = 0.5f + std::clamp(p2Res, 0.0f, 1.0f) * 9.5f; // 0.5 to 10.0
        filterMode = std::clamp(static_cast<int>(std::round(p3Type)), 0, 3);
        driveDb = (std::clamp(p4Gain, 0.0f, 1.0f) - 0.5f) * 36.0f; // -18dB to +18dB
        filterGainLinear = std::pow(10.0f, driveDb * 0.05f);
        wetMix = std::clamp(mixVal, 0.0f, 1.0f);

        // State Variable Filter (Chamberlin / Trapezoidal SVF)
        const double w = 3.141592653589793 * (std::clamp(cutoffHz, 15.0f, static_cast<float>(currentSampleRate * 0.48)) / currentSampleRate);
        g = static_cast<float>(std::tan(w));
        k = 1.0f / qFactor;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void process(float* left, float* right, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left ? left[i] : 0.0f;
            const float dryR = right ? right[i] : 0.0f;

            float wetL = processSvf(dryL, s1L, s2L);
            float wetR = processSvf(dryR, s1R, s2R);

            if (left) left[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (right) right[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

    EffectParamInfo getParamInfo(int paramIndex) const override
    {
        switch (paramIndex)
        {
            case 0: return { "Freq", "Cutoff Freq", 0.6f, 0.0f, 1.0f };
            case 1: return { "Res", "Resonance / Q", 0.2f, 0.0f, 1.0f };
            case 2: return { "Type", "Mode (LP, HP, BP, Peak)", 0.0f, 0.0f, 3.0f };
            case 3: return { "Gain", "Filter Drive / Gain", 0.5f, 0.0f, 1.0f };
            default: return { "Param", "Param", 0.0f, 0.0f, 1.0f };
        }
    }

private:
    float processSvf(float x, float& s1, float& s2)
    {
        // Non-linear SVF with soft saturation
        const float v0 = x;
        const float v3 = v0 - s2;
        const float v1 = a1 * s1 + a2 * v3;
        const float v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;

        const float lp = v2;
        const float bp = v1;
        const float hp = v0 - k * v1 - v2;

        float out = 0.0f;
        switch (filterMode)
        {
            case 0: out = lp; break;
            case 1: out = hp; break;
            case 2: out = bp; break;
            case 3: out = v0 + (filterGainLinear - 1.0f) * bp; break; // Peak Bell
            default: out = lp; break;
        }
        return std::tanh(out * std::max(1.0f, filterGainLinear));
    }

    float cutoffHz = 2000.0f;
    float qFactor = 0.707f;
    int filterMode = 0;
    float driveDb = 0.0f;
    float filterGainLinear = 1.0f;
    float wetMix = 1.0f;

    float g = 0.1f;
    float k = 1.414f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;

    float s1L = 0.0f; float s2L = 0.0f;
    float s1R = 0.0f; float s2R = 0.0f;
};
