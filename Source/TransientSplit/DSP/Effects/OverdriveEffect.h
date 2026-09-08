#pragma once

#include "BaseEffect.h"

class OverdriveEffect : public BaseEffect
{
public:
    OverdriveEffect() = default;

    EffectType getType() const override { return EffectType::Overdrive; }
    juce::String getName() const override { return "Overdrive"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        dcPrevInL = 0.0f; dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f; dcPrevOutR = 0.0f;
        preFilterL = 0.0f; preFilterR = 0.0f;
        postFilterL = 0.0f; postFilterR = 0.0f;
        dynEnvL = 0.0f; dynEnvR = 0.0f;
    }

    void setParameters(float p1Drive, float p2Tone, float p3Warmth, float p4Dynamics, float p5Out, float mixVal) override
    {
        driveGain = std::pow(10.0f, (std::clamp(p1Drive, 0.0f, 1.0f) * 36.0f) * 0.05f);
        toneVal = std::clamp(p2Tone, 0.0f, 1.0f);
        warmthAmount = std::clamp(p3Warmth, 0.0f, 1.0f);
        dynamicsAmount = std::clamp(p4Dynamics, 0.0f, 1.0f);
        outLevel = 0.5f + std::clamp(p5Out, 0.0f, 1.0f) * 1.0f;
        wetMix = std::clamp(mixVal, 0.0f, 1.0f);

        // Pre-emphasis filter: boosts mids before clipping
        const double wPre = 2.0 * 3.141592653589793 * (720.0 / currentSampleRate);
        preAlpha = static_cast<float>(std::clamp(wPre / (wPre + 1.0), 0.001, 0.999));

        // Post-emphasis tone filter: adjusts top end roll-off (1.5kHz to 10kHz)
        const double cutoff = 1500.0 + toneVal * 8500.0;
        const double wPost = 2.0 * 3.141592653589793 * (cutoff / currentSampleRate);
        postAlpha = static_cast<float>(std::clamp(wPost / (wPost + 1.0), 0.001, 0.999));
    }

    void process(float* left, float* right, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left ? left[i] : 0.0f;
            const float dryR = right ? right[i] : 0.0f;

            // Pre-filtering: Tube style bass cut & mid focus
            preFilterL += preAlpha * (dryL - preFilterL);
            preFilterR += preAlpha * (dryR - preFilterR);
            const float midFocusL = dryL - preFilterL * 0.6f;
            const float midFocusR = dryR - preFilterR * 0.6f;

            // Dynamic envelope follower for touch-responsive compression
            const float absL = std::abs(dryL);
            const float absR = std::abs(dryR);
            dynEnvL = (absL > dynEnvL) ? (0.99f * dynEnvL + 0.01f * absL) : (0.999f * dynEnvL + 0.001f * absL);
            dynEnvR = (absR > dynEnvR) ? (0.99f * dynEnvR + 0.01f * absR) : (0.999f * dynEnvR + 0.001f * absR);

            const float dynCompL = 1.0f / (1.0f + dynamicsAmount * dynEnvL * 3.0f);
            const float dynCompR = 1.0f / (1.0f + dynamicsAmount * dynEnvR * 3.0f);

            // Drive stage with dynamic touch
            const float drivenL = midFocusL * driveGain * dynCompL;
            const float drivenR = midFocusR * driveGain * dynCompR;

            // Tube-style asymmetric saturation: 2nd harmonic warmth + soft tanh knee
            float wetL = tubeSaturate(drivenL, warmthAmount);
            float wetR = tubeSaturate(drivenR, warmthAmount);

            // High-precision DC Blocker (~2.5 Hz)
            const float dcR = 0.9995f;
            const float filteredDcL = wetL - dcPrevInL + dcR * dcPrevOutL;
            dcPrevInL = wetL; dcPrevOutL = filteredDcL;
            wetL = filteredDcL;

            const float filteredDcR = wetR - dcPrevInR + dcR * dcPrevOutR;
            dcPrevInR = wetR; dcPrevOutR = filteredDcR;
            wetR = filteredDcR;

            // Post-filtering
            postFilterL += postAlpha * (wetL - postFilterL);
            postFilterR += postAlpha * (wetR - postFilterR);
            wetL = postFilterL;
            wetR = postFilterR;

            // Level compensation
            const float comp = 1.0f / (1.0f + 0.25f * std::log10(driveGain + 1.0f));
            wetL *= comp * 1.2f * outLevel;
            wetR *= comp * 1.2f * outLevel;

            if (left) left[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (right) right[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

    EffectParamInfo getParamInfo(int paramIndex) const override
    {
        switch (paramIndex)
        {
            case 0: return { "Drive", "Drive", 0.5f, 0.0f, 1.0f };
            case 1: return { "Tone", "Tone", 0.5f, 0.0f, 1.0f };
            case 2: return { "Warmth", "Tube Warmth", 0.4f, 0.0f, 1.0f };
            case 3: return { "Dynamics", "Touch Dynamics", 0.3f, 0.0f, 1.0f };
            case 4: return { "Output", "Output Level", 0.5f, 0.0f, 1.0f };
            default: return { "Param", "Param", 0.0f, 0.0f, 1.0f };
        }
    }

private:
    static float tubeSaturate(float x, float warmth)
    {
        // Add second harmonic term for warmth
        const float xAsym = x + warmth * 0.2f * (x * x);
        // Soft tube clipping curve
        return (2.0f / 3.14159265f) * std::atan(xAsym * 1.5707963f);
    }

    float driveGain = 1.0f;
    float toneVal = 0.5f;
    float warmthAmount = 0.4f;
    float dynamicsAmount = 0.3f;
    float outLevel = 1.0f;
    float wetMix = 0.5f;

    float preAlpha = 0.1f;
    float postAlpha = 0.5f;
    float preFilterL = 0.0f; float preFilterR = 0.0f;
    float postFilterL = 0.0f; float postFilterR = 0.0f;
    float dynEnvL = 0.0f; float dynEnvR = 0.0f;

    float dcPrevInL = 0.0f; float dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f; float dcPrevOutR = 0.0f;
};
