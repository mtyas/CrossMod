#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class DistortionWavefolderEffect : public BaseEffect
{
public:
    DistortionWavefolderEffect() = default;

    EffectType getType() const override { return EffectType::DistortionWavefolder; }
    juce::String getName() const override { return "Wavefolder Distortion"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        toneL = 0.0f;
        toneR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Drive (1x to 20x)
        drive = 1.0f + std::clamp(p1, 0.0f, 1.0f) * 19.0f;

        // P2: Fold Stages (1.0 to 6.0)
        folds = 1.0f + std::clamp(p2, 0.0f, 1.0f) * 5.0f;

        // P3: Asymmetry Bias (0.0 = Perfectly Centered / Symmetrical, up to 0.8)
        symmetry = std::clamp(p3, 0.0f, 1.0f) * 0.8f;

        // P4: Tone Filter (Lowpass smoothing)
        const double cutoffHz = 600.0 * std::pow(30.0, std::clamp(p4, 0.0f, 1.0f));
        toneCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P5: Fold Warmth / Soft Knee
        warmth = 1.0f + std::clamp(p5, 0.0f, 1.0f) * 1.5f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            auto fold = [this](float in, float& tState) -> float
            {
                // Centered sine wavefolding with variable asymmetry bias and soft warmth
                float x = (in + symmetry * 0.35f) * drive;
                float folded = std::sin(x * folds * 1.57079632679f);
                folded = std::tanh(folded * warmth) / warmth;

                tState += toneCoeff * (folded - tState);
                return tState;
            };

            const float wetL = fold(dryL, toneL);
            const float wetR = fold(dryR, toneR);

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float drive = 3.0f;
    float folds = 2.0f;
    float symmetry = 0.0f;
    float toneCoeff = 0.8f;
    float warmth = 1.0f;
    float wetMix = 1.0f;

    float toneL = 0.0f;
    float toneR = 0.0f;
};
