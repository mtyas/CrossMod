#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class FuzzGermaniumEffect : public BaseEffect
{
public:
    FuzzGermaniumEffect() = default;

    EffectType getType() const override { return EffectType::FuzzGermanium; }
    juce::String getName() const override { return "Germanium Fuzz"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        toneL = 0.0f;
        toneR = 0.0f;
        dcL = 0.0f;
        dcR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Fuzz Gain (2x to 80x)
        fuzzGain = 2.0f + std::clamp(p1, 0.0f, 1.0f) * 78.0f;

        // P2: Voltage Bias / Starve (Gated breakup to open warm fuzz)
        biasVolt = 0.1f + std::clamp(p2, 0.0f, 1.0f) * 0.9f;

        // P3: Tone Filter (600Hz to 14000Hz)
        const double cutoffHz = 600.0 * std::pow(23.33, std::clamp(p3, 0.0f, 1.0f));
        toneCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P4: Body / Bass response (0.5 to 2.0)
        body = 0.5f + std::clamp(p4, 0.0f, 1.0f) * 1.5f;

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

            auto processGerm = [this](float in, float& tState, float& dcState) -> float
            {
                float driven = in * fuzzGain * body;
                float clamped = 0.0f;

                if (std::abs(driven) < (1.0f - biasVolt) * 0.3f)
                {
                    clamped = 0.0f;
                }
                else
                {
                    if (driven > 0.0f)
                        clamped = 1.0f - std::exp(-driven * 1.2f);
                    else
                        clamped = -0.7f * (1.0f - std::exp(driven * 1.8f));
                }

                dcState = clamped - dcState * 0.995f;
                float ac = (clamped - dcState) * outLevel;

                tState += toneCoeff * (ac - tState);
                return tState;
            };

            const float wetL = processGerm(dryL, toneL, dcL);
            const float wetR = processGerm(dryR, toneR, dcR);

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float fuzzGain = 20.0f;
    float biasVolt = 0.8f;
    float toneCoeff = 0.8f;
    float body = 1.0f;
    float outLevel = 1.0f;
    float wetMix = 1.0f;

    float toneL = 0.0f;
    float toneR = 0.0f;
    float dcL = 0.0f;
    float dcR = 0.0f;
};
