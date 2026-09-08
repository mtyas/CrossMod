#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class CompressorFETEffect : public BaseEffect
{
public:
    CompressorFETEffect() = default;

    EffectType getType() const override { return EffectType::CompressorFET; }
    juce::String getName() const override { return "FET Compressor (1176)"; }

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
        // P1: Input Drive (0 dB to +30 dB)
        inputDriveLinear = std::pow(10.0f, (std::clamp(p1, 0.0f, 1.0f) * 30.0f) * 0.05f);

        // P2: Ratio (4:1 up to All-Buttons 20:1 slam)
        ratio = 4.0f + std::clamp(p2, 0.0f, 1.0f) * 16.0f;
        isAllButtons = (p2 > 0.9f);

        // P3: Fast Attack (0.02ms to 0.8ms)
        const double attMs = 0.02 + (1.0 - std::clamp(p3, 0.0f, 1.0f)) * 0.78;

        // P4: Fast Release (50ms to 1100ms)
        const double relMs = 50.0 + (1.0 - std::clamp(p4, 0.0f, 1.0f)) * 1050.0;

        // P5: Punch / Output Level (0.5x to 2.0x)
        punchOut = 0.5f + std::clamp(p5, 0.0f, 1.0f) * 1.5f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        alphaAttack = static_cast<float>(std::exp(-1.0 / (attMs * 0.001 * currentSampleRate)));
        alphaRelease = static_cast<float>(std::exp(-1.0 / (relMs * 0.001 * currentSampleRate)));
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        const float threshDb = -18.0f;
        const float threshLin = std::pow(10.0f, threshDb * 0.05f);
        const float invRatio = 1.0f / ratio;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            const float drivenL = dryL * inputDriveLinear;
            const float drivenR = dryR * inputDriveLinear;

            const float absL = std::abs(drivenL);
            const float absR = std::abs(drivenR);

            if (absL > envL) envL = alphaAttack * envL + (1.0f - alphaAttack) * absL;
            else envL = alphaRelease * envL + (1.0f - alphaRelease) * absL;

            if (absR > envR) envR = alphaAttack * envR + (1.0f - alphaAttack) * absR;
            else envR = alphaRelease * envR + (1.0f - alphaRelease) * absR;

            auto calcFetGr = [&](float env) -> float
            {
                if (env <= threshLin || env < 1e-6f) return 1.0f;
                const float envDb = 20.0f * std::log10(env);
                const float overDb = envDb - threshDb;
                const float compDb = threshDb + overDb * invRatio;
                float grDb = compDb - envDb;

                if (isAllButtons)
                    grDb *= 1.25f;

                return std::pow(10.0f, grDb * 0.05f);
            };

            const float targetL = calcFetGr(envL);
            const float targetR = calcFetGr(envR);

            gainReductionL += 0.2f * (targetL - gainReductionL);
            gainReductionR += 0.2f * (targetR - gainReductionR);

            float wetL = std::tanh(drivenL * gainReductionL) * punchOut;
            float wetR = std::tanh(drivenR * gainReductionR) * punchOut;

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float inputDriveLinear = 1.0f;
    float ratio = 4.0f;
    bool isAllButtons = false;
    float punchOut = 1.0f;
    float wetMix = 1.0f;

    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;
    float envL = 0.0f;
    float envR = 0.0f;
    float gainReductionL = 1.0f;
    float gainReductionR = 1.0f;
};
