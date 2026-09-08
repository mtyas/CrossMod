#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class FuzzOctaveEffect : public BaseEffect
{
public:
    FuzzOctaveEffect() = default;

    EffectType getType() const override { return EffectType::FuzzOctave; }
    juce::String getName() const override { return "Octave Fuzz"; }

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
        // P1: Fuzz Gain (2x to 60x)
        fuzzGain = 2.0f + std::clamp(p1, 0.0f, 1.0f) * 58.0f;

        // P2: Octave Blend (0.0 to 1.0)
        octaveBlend = std::clamp(p2, 0.0f, 1.0f);

        // P3: Tone Filter (800Hz to 16000Hz)
        const double cutoffHz = 800.0 * std::pow(20.0, std::clamp(p3, 0.0f, 1.0f));
        toneCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P4: Rectify Peak / Sharpness (1.0 to 3.0)
        rectifySharpness = 1.0f + std::clamp(p4, 0.0f, 1.0f) * 2.0f;

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

            auto processOct = [this](float in, float& tState) -> float
            {
                float driven = in * fuzzGain;
                float fund = std::tanh(driven);
                float oct = std::abs(driven) * (1.0f + rectifySharpness * 0.4f) - 0.7f;
                oct = std::tanh(oct);

                float mixed = (fund * (1.0f - octaveBlend) + oct * octaveBlend) * outLevel;
                tState += toneCoeff * (mixed - tState);
                return tState;
            };

            const float wetL = processOct(dryL, toneL);
            const float wetR = processOct(dryR, toneR);

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float fuzzGain = 15.0f;
    float octaveBlend = 0.5f;
    float toneCoeff = 0.8f;
    float rectifySharpness = 1.0f;
    float outLevel = 1.0f;
    float wetMix = 1.0f;

    float toneL = 0.0f;
    float toneR = 0.0f;
};
