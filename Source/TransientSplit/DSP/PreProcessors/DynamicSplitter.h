#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

class DynamicSplitter
{
public:
    DynamicSplitter() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        currentMaxBlock = std::max(64, maxBlockSize);
        reset();
    }

    void reset() noexcept
    {
        envL = 0.0f;
        envR = 0.0f;
    }

    void setParameters(float thresholdDb, float attackMs, float releaseMs, float kneeDb) noexcept
    {
        threshDb = std::clamp(thresholdDb, -60.0f, 0.0f);
        kneeWidthDb = std::clamp(kneeDb, 0.1f, 24.0f);

        const double attSec = std::clamp(attackMs, 0.5f, 300.0f) * 0.001;
        const double relSec = std::clamp(releaseMs, 5.0f, 1500.0f) * 0.001;

        alphaAttack = static_cast<float>(std::exp(-1.0 / (attSec * currentSampleRate)));
        alphaRelease = static_cast<float>(std::exp(-1.0 / (relSec * currentSampleRate)));
    }

    void process(const float* inL, const float* inR,
                 float* highL, float* highR,
                 float* lowL, float* lowR,
                 int numSamples)
    {
        if (numSamples <= 0 || !inL) return;

        const float halfKnee = kneeWidthDb * 0.5f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float l = inL[i];
            const float r = inR ? inR[i] : l;

            const float absL = std::abs(l);
            const float absR = std::abs(r);

            // Envelope tracking
            if (absL > envL)
                envL = alphaAttack * envL + (1.0f - alphaAttack) * absL;
            else
                envL = alphaRelease * envL + (1.0f - alphaRelease) * absL;

            if (absR > envR)
                envR = alphaAttack * envR + (1.0f - alphaAttack) * absR;
            else
                envR = alphaRelease * envR + (1.0f - alphaRelease) * absR;

            auto calcHighGain = [this, halfKnee](float envVal) -> float
            {
                const float envDb = (envVal > 1e-6f) ? (20.0f * std::log10(envVal)) : -120.0f;
                const float delta = envDb - threshDb;

                if (delta <= -halfKnee)
                    return 0.0f; // Pure low / quiet
                if (delta >= halfKnee)
                    return 1.0f; // Pure high / loud

                // Smooth quadratic Hermite soft-knee interpolation
                const float t = (delta + halfKnee) / kneeWidthDb;
                return t * t * (3.0f - 2.0f * t);
            };

            const float gainHighL = calcHighGain(envL);
            const float gainHighR = calcHighGain(envR);
            const float gainLowL = 1.0f - gainHighL;
            const float gainLowR = 1.0f - gainHighR;

            if (highL) highL[i] = l * gainHighL;
            if (highR) highR[i] = r * gainHighR;
            if (lowL) lowL[i] = l * gainLowL;
            if (lowR) lowR[i] = r * gainLowR;
        }
    }

private:
    double currentSampleRate = 48000.0;
    int currentMaxBlock = 512;

    float threshDb = -20.0f;
    float kneeWidthDb = 6.0f;
    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;

    float envL = 0.0f;
    float envR = 0.0f;
};
