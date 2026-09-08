#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

enum class CompressorModel
{
    VCA = 0,
    Opto = 1,
    FET = 2
};

class CompressorEffect : public BaseEffect
{
public:
    CompressorEffect() = default;

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        envDetectorL = 0.0f;
        envDetectorR = 0.0f;
        optoMemoryL = 0.0f;
        optoMemoryR = 0.0f;
        gainReductionL = 1.0f;
        gainReductionR = 1.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float mix) override
    {
        // P1: Threshold (-40 dB to 0 dB)
        thresholdDb = -40.0f + std::clamp(p1, 0.0f, 1.0f) * 40.0f;

        // P2: Ratio (1.0 to 20.0)
        ratio = 1.0f + std::clamp(p2, 0.0f, 1.0f) * 19.0f;

        // P3: Model (0: VCA, 1: Opto LA-2A, 2: FET 1176)
        const int modelInt = std::clamp(static_cast<int>(std::floor(p3 + 0.5f)), 0, 2);
        model = static_cast<CompressorModel>(modelInt);

        // P4: Speed / Timing modifier
        speedModifier = std::clamp(p4, 0.05f, 1.0f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        // Compute attack & release time constants based on active model
        double attackMs = 10.0;
        double releaseMs = 120.0;

        switch (model)
        {
            case CompressorModel::VCA:
                // Crisp, punchy VCA (Attack: 1ms - 50ms, Release: 20ms - 400ms)
                attackMs = 1.0 + (1.0 - speedModifier) * 49.0;
                releaseMs = 20.0 + (1.0 - speedModifier) * 380.0;
                break;

            case CompressorModel::Opto:
                // Smooth optical cell (Attack: 10ms, Dual-stage release: 60ms fast + 1500ms slow tail)
                attackMs = 10.0;
                releaseMs = 120.0 + (1.0 - speedModifier) * 1200.0;
                break;

            case CompressorModel::FET:
                // Ultra-fast FET 1176 (Attack: 0.05ms - 1.2ms, Release: 10ms - 200ms)
                attackMs = 0.05 + (1.0 - speedModifier) * 1.15;
                releaseMs = 10.0 + (1.0 - speedModifier) * 190.0;
                break;
        }

        alphaAttack = static_cast<float>(std::exp(-1.0 / (attackMs * 0.001 * currentSampleRate)));
        alphaRelease = static_cast<float>(std::exp(-1.0 / (releaseMs * 0.001 * currentSampleRate)));
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        const float threshLinear = std::pow(10.0f, thresholdDb * 0.05f);
        const float invRatio = 1.0f / ratio;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            // 1. Peak Envelope Detection
            const float absL = std::abs(dryL);
            const float absR = std::abs(dryR);

            if (absL > envDetectorL)
                envDetectorL = alphaAttack * envDetectorL + (1.0f - alphaAttack) * absL;
            else
                envDetectorL = alphaRelease * envDetectorL + (1.0f - alphaRelease) * absL;

            if (absR > envDetectorR)
                envDetectorR = alphaAttack * envDetectorR + (1.0f - alphaAttack) * absR;
            else
                envDetectorR = alphaRelease * envDetectorR + (1.0f - alphaRelease) * absR;

            // 2. Gain Computer
            auto computeGain = [&](float env, float& optoMem) -> float
            {
                if (env <= threshLinear || env < 1e-6f)
                    return 1.0f;

                const float envDb = 20.0f * std::log10(env);
                const float overDb = envDb - thresholdDb;

                // Compressed target level in dB
                const float compDb = thresholdDb + overDb * invRatio;
                float grDb = compDb - envDb; // negative dB

                if (model == CompressorModel::Opto)
                {
                    // Optical memory dual-stage release smoothing
                    optoMem = 0.999f * optoMem + 0.001f * grDb;
                    grDb = 0.7f * grDb + 0.3f * optoMem;
                }
                else if (model == CompressorModel::FET)
                {
                    // Aggressive FET saturation at high compression
                    if (grDb < -12.0f)
                        grDb *= 1.15f;
                }

                return std::pow(10.0f, grDb * 0.05f);
            };

            const float targetGrL = computeGain(envDetectorL, optoMemoryL);
            const float targetGrR = computeGain(envDetectorR, optoMemoryR);

            gainReductionL += 0.1f * (targetGrL - gainReductionL);
            gainReductionR += 0.1f * (targetGrR - gainReductionR);

            // Auto-makeup gain approximation
            const float makeupDb = (1.0f - invRatio) * std::abs(thresholdDb) * 0.4f;
            const float makeupLin = std::pow(10.0f, makeupDb * 0.05f);

            float wetL = dryL * gainReductionL * makeupLin;
            float wetR = dryR * gainReductionR * makeupLin;

            // Subtle analogFET saturation
            if (model == CompressorModel::FET)
            {
                wetL = std::tanh(wetL * 1.1f);
                wetR = std::tanh(wetR * 1.1f);
            }

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;

    CompressorModel model = CompressorModel::VCA;
    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float speedModifier = 0.5f;
    float wetMix = 1.0f;

    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;

    float envDetectorL = 0.0f;
    float envDetectorR = 0.0f;
    float optoMemoryL = 0.0f;
    float optoMemoryR = 0.0f;
    float gainReductionL = 1.0f;
    float gainReductionR = 1.0f;
};
