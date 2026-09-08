#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class CompressorOptoEffect : public BaseEffect
{
public:
    CompressorOptoEffect() = default;

    EffectType getType() const override { return EffectType::CompressorOpto; }
    juce::String getName() const override { return "Opto Compressor (LA-2A)"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        envL = 0.0f;
        envR = 0.0f;
        optoMemoryL = 0.0f;
        optoMemoryR = 0.0f;
        gainReductionL = 1.0f;
        gainReductionR = 1.0f;
        hfStateL = 0.0f;
        hfStateR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Peak Reduction (Threshold from -10dB down to -50dB)
        thresholdDb = -10.0f - std::clamp(p1, 0.0f, 1.0f) * 40.0f;

        // P2: Makeup Gain (0dB to +30dB)
        makeupGain = std::pow(10.0f, (std::clamp(p2, 0.0f, 1.0f) * 30.0f) * 0.05f);

        // P3: Response Speed (Attack 5ms - 20ms, Release 100ms - 1500ms)
        const float speed = std::clamp(p3, 0.05f, 1.0f);
        const double attMs = 20.0 - speed * 15.0;
        const double relMs = 1500.0 - speed * 1300.0;

        // P4: Warmth Bias
        warmth = std::clamp(p4, 0.0f, 1.0f);

        // P5: HF Emphasis (High-Frequency Sidechain Focus / De-Ess)
        hfEmphasis = std::clamp(p5, 0.0f, 1.0f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        alphaAttack = static_cast<float>(std::exp(-1.0 / (attMs * 0.001 * currentSampleRate)));
        alphaRelease = static_cast<float>(std::exp(-1.0 / (relMs * 0.001 * currentSampleRate)));
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        const float threshLin = std::pow(10.0f, thresholdDb * 0.05f);
        const float invRatio = 1.0f / 3.5f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            // HF Emphasis in detection circuit
            hfStateL += 0.2f * (dryL - hfStateL);
            hfStateR += 0.2f * (dryR - hfStateR);
            const float sidechainL = dryL + (dryL - hfStateL) * hfEmphasis * 2.0f;
            const float sidechainR = dryR + (dryR - hfStateR) * hfEmphasis * 2.0f;

            const float absL = std::abs(sidechainL);
            const float absR = std::abs(sidechainR);

            if (absL > envL) envL = alphaAttack * envL + (1.0f - alphaAttack) * absL;
            else envL = alphaRelease * envL + (1.0f - alphaRelease) * absL;

            if (absR > envR) envR = alphaAttack * envR + (1.0f - alphaAttack) * absR;
            else envR = alphaRelease * envR + (1.0f - alphaRelease) * absR;

            auto calcOpto = [&](float env, float& mem) -> float
            {
                if (env <= threshLin || env < 1e-6f) return 1.0f;
                const float envDb = 20.0f * std::log10(env);
                const float overDb = envDb - thresholdDb;
                const float compDb = thresholdDb + overDb * invRatio;
                float grDb = compDb - envDb;

                mem = 0.9992f * mem + 0.0008f * grDb;
                grDb = 0.65f * grDb + 0.35f * mem;
                return std::pow(10.0f, grDb * 0.05f);
            };

            const float targetL = calcOpto(envL, optoMemoryL);
            const float targetR = calcOpto(envR, optoMemoryR);

            gainReductionL += 0.05f * (targetL - gainReductionL);
            gainReductionR += 0.05f * (targetR - gainReductionR);

            float wetL = dryL * gainReductionL * makeupGain;
            float wetR = dryR * gainReductionR * makeupGain;

            if (warmth > 0.01f)
            {
                const float driveFactor = 1.0f + warmth * 1.5f;
                wetL = std::tanh(wetL * driveFactor) / driveFactor;
                wetR = std::tanh(wetR * driveFactor) / driveFactor;
            }

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    float thresholdDb = -20.0f;
    float makeupGain = 1.0f;
    float warmth = 0.2f;
    float hfEmphasis = 0.0f;
    float wetMix = 1.0f;

    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;
    float envL = 0.0f;
    float envR = 0.0f;
    float optoMemoryL = 0.0f;
    float optoMemoryR = 0.0f;
    float gainReductionL = 1.0f;
    float gainReductionR = 1.0f;
    float hfStateL = 0.0f;
    float hfStateR = 0.0f;
};
