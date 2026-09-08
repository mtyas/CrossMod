#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class DistortionTubeEffect : public BaseEffect
{
public:
    DistortionTubeEffect() = default;

    EffectType getType() const override { return EffectType::DistortionTube; }
    juce::String getName() const override { return "Tube Saturation"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        sagL = 1.0f;
        sagR = 1.0f;
        lpStateL = 0.0f;
        lpStateR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Drive (1x to 25x)
        drive = 1.0f + std::clamp(p1, 0.0f, 1.0f) * 24.0f;

        // P2: Bias Warmth (Even Harmonic Injection)
        bias = std::clamp(p2, 0.0f, 1.0f) * 0.4f;

        // P3: Sag Dynamics (Power supply sag compression)
        sagAmount = std::clamp(p3, 0.0f, 1.0f) * 0.5f;

        // P4: High Cut Tone (800Hz to 18000Hz)
        const double cutoffHz = 800.0 * std::pow(22.5, std::clamp(p4, 0.0f, 1.0f));
        lpCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

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

            auto processTube = [this](float in, float& sag, float& lp) -> float
            {
                const float absIn = std::abs(in);
                sag += 0.005f * (1.0f - absIn * sagAmount - sag);
                sag = std::clamp(sag, 0.2f, 1.0f);

                const float biased = (in * drive * sag) + bias;
                float sat = 0.0f;
                if (biased > 0.0f)
                    sat = 1.5f * (biased / (1.0f + biased));
                else
                    sat = -1.0f * (std::abs(biased) / (1.0f + std::abs(biased)));

                lp += lpCoeff * (sat * outLevel - lp);
                return lp;
            };

            const float wetL = processTube(dryL, sagL, lpStateL);
            const float wetR = processTube(dryR, sagR, lpStateR);

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float drive = 3.0f;
    float bias = 0.1f;
    float sagAmount = 0.2f;
    float lpCoeff = 0.85f;
    float outLevel = 1.0f;
    float wetMix = 1.0f;

    float sagL = 1.0f;
    float sagR = 1.0f;
    float lpStateL = 0.0f;
    float lpStateR = 0.0f;
};
