#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class CompressorVCAEffect : public BaseEffect
{
public:
    CompressorVCAEffect() = default;

    EffectType getType() const override { return EffectType::CompressorVCA; }
    juce::String getName() const override { return "VCA Compressor"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        envL = 0.0f;
        envR = 0.0f;
        gainReductionL = 1.0f;
        gainReductionR = 1.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Threshold (-40 dB to 0 dB)
        thresholdDb = -40.0f + std::clamp(p1, 0.0f, 1.0f) * 40.0f;

        // P2: Ratio (1:1 to 20:1)
        ratio = 1.0f + std::clamp(p2, 0.0f, 1.0f) * 19.0f;

        // P3: Attack (0.2ms to 60ms)
        const double attMs = 0.2 + std::clamp(p3, 0.0f, 1.0f) * 59.8;

        // P4: Release (20ms to 600ms)
        const double relMs = 20.0 + std::clamp(p4, 0.0f, 1.0f) * 580.0;

        // P5: Knee / Extra Makeup Gain (0 to 12 dB extra boost)
        kneeMakeupDb = std::clamp(p5, 0.0f, 1.0f) * 12.0f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        alphaAttack = static_cast<float>(std::exp(-1.0 / (attMs * 0.001 * currentSampleRate)));
        alphaRelease = static_cast<float>(std::exp(-1.0 / (relMs * 0.001 * currentSampleRate)));
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        const float threshLinear = std::pow(10.0f, thresholdDb * 0.05f);
        const float invRatio = 1.0f / ratio;
        const float makeupExtra = std::pow(10.0f, kneeMakeupDb * 0.05f);

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            const float absL = std::abs(dryL);
            const float absR = std::abs(dryR);

            if (absL > envL) envL = alphaAttack * envL + (1.0f - alphaAttack) * absL;
            else envL = alphaRelease * envL + (1.0f - alphaRelease) * absL;

            if (absR > envR) envR = alphaAttack * envR + (1.0f - alphaAttack) * absR;
            else envR = alphaRelease * envR + (1.0f - alphaRelease) * absR;

            auto calcGr = [&](float env) -> float
            {
                if (env <= threshLinear || env < 1e-6f) return 1.0f;
                const float envDb = 20.0f * std::log10(env);
                const float overDb = envDb - thresholdDb;
                const float compDb = thresholdDb + overDb * invRatio;
                const float grDb = compDb - envDb;
                return std::pow(10.0f, grDb * 0.05f);
            };

            const float targetL = calcGr(envL);
            const float targetR = calcGr(envR);

            gainReductionL += 0.15f * (targetL - gainReductionL);
            gainReductionR += 0.15f * (targetR - gainReductionR);

            const float makeupDb = (1.0f - invRatio) * std::abs(thresholdDb) * 0.45f;
            const float makeup = std::pow(10.0f, makeupDb * 0.05f) * makeupExtra;

            const float wetL = dryL * gainReductionL * makeup;
            const float wetR = dryR * gainReductionR * makeup;

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float kneeMakeupDb = 0.0f;
    float wetMix = 1.0f;
    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;
    float envL = 0.0f;
    float envR = 0.0f;
    float gainReductionL = 1.0f;
    float gainReductionR = 1.0f;
};
