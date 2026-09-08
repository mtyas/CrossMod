#pragma once

#include "BaseEffect.h"

class FuzzEffect : public BaseEffect
{
public:
    FuzzEffect() = default;

    EffectType getType() const override { return EffectType::Fuzz; }
    juce::String getName() const override { return "Fuzz"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        dcPrevInL = 0.0f; dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f; dcPrevOutR = 0.0f;
        lpStateL = 0.0f; lpStateR = 0.0f;
        hpStateL = 0.0f; hpStateR = 0.0f;
    }

    void setParameters(float p1Gain, float p2Tone, float p3Octave, float p4Gate, float mixVal) override
    {
        fuzzGain = std::pow(10.0f, (12.0f + std::clamp(p1Gain, 0.0f, 1.0f) * 45.0f) * 0.05f); // up to +57 dB
        toneScoop = std::clamp(p2Tone, 0.0f, 1.0f);
        octaveAmount = std::clamp(p3Octave, 0.0f, 1.0f);
        gateThreshold = std::clamp(p4Gate, 0.0f, 0.95f) * 0.08f; // Voltage starve / velcro gating
        wetMix = std::clamp(mixVal, 0.0f, 1.0f);

        // Pre-emphasis filter coefficients
        const double wLp = 2.0 * 3.141592653589793 * (std::clamp(1500.0f + toneScoop * 8000.0f, 500.0f, 18000.0f) / currentSampleRate);
        lpAlpha = static_cast<float>(std::clamp(wLp / (wLp + 1.0), 0.001, 0.999));

        const double wHp = 2.0 * 3.141592653589793 * (std::clamp(100.0f + (1.0f - toneScoop) * 600.0f, 40.0f, 2000.0f) / currentSampleRate);
        hpAlpha = static_cast<float>(std::clamp(wHp / (wHp + 1.0), 0.001, 0.999));
    }

    void process(float* left, float* right, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left ? left[i] : 0.0f;
            const float dryR = right ? right[i] : 0.0f;

            float xL = dryL * fuzzGain;
            float xR = dryR * fuzzGain;

            // Voltage starve / velcro gating effect
            if (gateThreshold > 0.0001f)
            {
                if (std::abs(xL) < gateThreshold * fuzzGain) xL *= 0.05f;
                if (std::abs(xR) < gateThreshold * fuzzGain) xR *= 0.05f;
            }

            // Octave fuzz rectification: blend between normal signal and full-wave rectified signal
            const float octL = (std::abs(xL) * 2.0f) - 1.0f;
            const float octR = (std::abs(xR) * 2.0f) - 1.0f;

            float satInL = xL * (1.0f - octaveAmount) + octL * octaveAmount;
            float satInR = xR * (1.0f - octaveAmount) + octR * octaveAmount;

            // Germanium transistor saturation model
            float wetL = germaniumSaturate(satInL);
            float wetR = germaniumSaturate(satInR);

            // High-precision DC Blocker (~2.5 Hz)
            const float dcR = 0.9995f;
            const float filteredDcL = wetL - dcPrevInL + dcR * dcPrevOutL;
            dcPrevInL = wetL; dcPrevOutL = filteredDcL;
            wetL = filteredDcL;

            const float filteredDcR = wetR - dcPrevInR + dcR * dcPrevOutR;
            dcPrevInR = wetR; dcPrevOutR = filteredDcR;
            wetR = filteredDcR;

            // Tone Stack (Low pass + High pass blend)
            lpStateL += lpAlpha * (wetL - lpStateL);
            lpStateR += lpAlpha * (wetR - lpStateR);

            hpStateL += hpAlpha * (wetL - hpStateL);
            hpStateR += hpAlpha * (wetR - hpStateR);
            const float highPassedL = wetL - hpStateL;
            const float highPassedR = wetR - hpStateR;

            wetL = lpStateL * (1.0f - toneScoop * 0.5f) + highPassedL * (toneScoop * 0.8f);
            wetR = lpStateR * (1.0f - toneScoop * 0.5f) + highPassedR * (toneScoop * 0.8f);

            // Level normalization
            const float outLevel = 0.7f;
            wetL *= outLevel;
            wetR *= outLevel;

            if (left) left[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (right) right[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

    EffectParamInfo getParamInfo(int paramIndex) const override
    {
        switch (paramIndex)
        {
            case 0: return { "Gain", "Fuzz Gain", 0.6f, 0.0f, 1.0f };
            case 1: return { "Tone", "Tone / Scoop", 0.5f, 0.0f, 1.0f };
            case 2: return { "Octave", "Octave Blend", 0.0f, 0.0f, 1.0f };
            case 3: return { "Starve", "Gate / Starve", 0.0f, 0.0f, 1.0f };
            default: return { "Param", "Param", 0.0f, 0.0f, 1.0f };
        }
    }

private:
    static float germaniumSaturate(float x)
    {
        // Smooth soft clipping with odd + even harmonic asymmetry
        if (x > 0.0f)
            return std::tanh(x * 1.2f) * 0.9f + (x / (1.0f + std::abs(x))) * 0.1f;
        else
            return std::tanh(x * 0.85f);
    }

    float fuzzGain = 10.0f;
    float toneScoop = 0.5f;
    float octaveAmount = 0.0f;
    float gateThreshold = 0.0f;
    float wetMix = 0.5f;

    float lpAlpha = 0.5f;
    float hpAlpha = 0.1f;
    float lpStateL = 0.0f; float lpStateR = 0.0f;
    float hpStateL = 0.0f; float hpStateR = 0.0f;

    float dcPrevInL = 0.0f; float dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f; float dcPrevOutR = 0.0f;
};
