#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class DistortionHardClipEffect : public BaseEffect
{
public:
    DistortionHardClipEffect() = default;

    EffectType getType() const override { return EffectType::DistortionHardClip; }
    juce::String getName() const override { return "Hard Clip Distortion"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        toneStateL = 0.0f;
        toneStateR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Drive (1x to 40x)
        drive = 1.0f + std::clamp(p1, 0.0f, 1.0f) * 39.0f;

        // P2: Tone (Lowpass Cutoff 800Hz to 18000Hz)
        const double cutoffHz = 800.0 * std::pow(22.5, std::clamp(p2, 0.0f, 1.0f));
        toneCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P3: Asymmetry Bias (-0.5 to +0.5)
        bias = -0.5f + std::clamp(p3, 0.0f, 1.0f) * 1.0f;

        // P4: Bass Pre-Boost (0dB to +12dB)
        bassBoost = 1.0f + std::clamp(p4, 0.0f, 1.0f) * 3.0f;

        // P5: Output Level (0.2x to 1.5x)
        outLevel = 0.2f + std::clamp(p5, 0.0f, 1.0f) * 1.3f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            auto clip = [this](float x, float& tState) -> float
            {
                float driven = (x * bassBoost + bias * 0.2f) * drive;
                float clipped = std::clamp(driven, -1.0f, 1.0f) * outLevel;
                tState += toneCoeff * (clipped - tState);
                return tState;
            };

            const float wetL = clip(dryL, toneStateL);
            const float wetR = clip(dryR, toneStateR);

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float drive = 5.0f;
    float toneCoeff = 0.8f;
    float bias = 0.0f;
    float bassBoost = 1.0f;
    float outLevel = 1.0f;
    float wetMix = 1.0f;

    float toneStateL = 0.0f;
    float toneStateR = 0.0f;
};
