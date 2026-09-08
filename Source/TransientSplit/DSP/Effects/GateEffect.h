#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class GateEffect : public BaseEffect
{
public:
    GateEffect() = default;

    EffectType getType() const override { return EffectType::Gate; }
    juce::String getName() const override { return "Noise Gate / Expander"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        envelopeL = 0.0f;
        envelopeR = 0.0f;
        gainL = 1.0f;
        gainR = 1.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Threshold (-60 dB to 0 dB)
        thresholdDb = -60.0f + std::clamp(p1, 0.0f, 1.0f) * 60.0f;

        // P2: Attack (0.1ms to 40ms)
        const double attMs = 0.1 + std::clamp(p2, 0.0f, 1.0f) * 39.9;

        // P3: Release (10ms to 600ms)
        const double relMs = 10.0 + std::clamp(p3, 0.0f, 1.0f) * 590.0;

        // P4: Range / Floor (-80dB to -6dB)
        floorDb = -80.0f + std::clamp(p4, 0.0f, 1.0f) * 74.0f;

        // P5: Hysteresis (0 to 12 dB below threshold for closing)
        hysteresisDb = std::clamp(p5, 0.0f, 1.0f) * 12.0f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        alphaAttack = static_cast<float>(std::exp(-1.0 / (attMs * 0.001 * currentSampleRate)));
        alphaRelease = static_cast<float>(std::exp(-1.0 / (relMs * 0.001 * currentSampleRate)));
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        const float threshLin = std::pow(10.0f, thresholdDb * 0.05f);
        const float closeThreshLin = std::pow(10.0f, (thresholdDb - hysteresisDb) * 0.05f);
        const float floorLin = std::pow(10.0f, floorDb * 0.05f);

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            const float absL = std::abs(dryL);
            const float absR = std::abs(dryR);

            if (absL > envelopeL) envelopeL = alphaAttack * envelopeL + (1.0f - alphaAttack) * absL;
            else envelopeL = alphaRelease * envelopeL + (1.0f - alphaRelease) * absL;

            if (absR > envelopeR) envelopeR = alphaAttack * envelopeR + (1.0f - alphaAttack) * absR;
            else envelopeR = alphaRelease * envelopeR + (1.0f - alphaRelease) * absR;

            auto calcTarget = [&](float env, float currentG) -> float
            {
                if (currentG < 0.5f)
                {
                    // Gate is currently closed, needs to cross upper threshold
                    return (env >= threshLin) ? 1.0f : floorLin;
                }
                else
                {
                    // Gate is currently open, stays open until below close threshold
                    return (env >= closeThreshLin) ? 1.0f : floorLin;
                }
            };

            const float targetL = calcTarget(envelopeL, gainL);
            const float targetR = calcTarget(envelopeR, gainR);

            gainL += 0.2f * (targetL - gainL);
            gainR += 0.2f * (targetR - gainR);

            const float wetL = dryL * gainL;
            const float wetR = dryR * gainR;

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float thresholdDb = -30.0f;
    float floorDb = -60.0f;
    float hysteresisDb = 3.0f;
    float wetMix = 1.0f;
    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;
    float envelopeL = 0.0f;
    float envelopeR = 0.0f;
    float gainL = 1.0f;
    float gainR = 1.0f;
};
