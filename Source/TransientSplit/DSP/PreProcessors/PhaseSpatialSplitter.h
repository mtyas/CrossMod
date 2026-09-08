#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

class PhaseSpatialSplitter
{
public:
    PhaseSpatialSplitter() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        currentMaxBlock = std::max(64, maxBlockSize);
        reset();
    }

    void reset() noexcept
    {
        dotLR = 0.0f;
        dotLL = 0.0f;
        dotRR = 0.0f;
        smoothedCorr = 1.0f;
    }

    void setParameters(float threshold, float windowMs, float spatialSpread) noexcept
    {
        corrThreshold = -1.0f + std::clamp(threshold, 0.0f, 1.0f) * 2.0f; // -1.0 to +1.0
        spread = std::clamp(spatialSpread, 0.5f, 2.0f);

        const double winSec = std::clamp(windowMs, 2.0f, 80.0f) * 0.001;
        alpha = static_cast<float>(1.0 - std::exp(-1.0 / (winSec * currentSampleRate)));
    }

    void process(const float* inL, const float* inR,
                 float* inPhaseL, float* inPhaseR,
                 float* spatialL, float* spatialR,
                 int numSamples)
    {
        if (numSamples <= 0 || !inL) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float l = inL[i];
            const float r = inR ? inR[i] : l;

            // Instantaneous cross-channel energy accumulators
            dotLR += alpha * (l * r - dotLR);
            dotLL += alpha * (l * l - dotLL);
            dotRR += alpha * (r * r - dotRR);

            const float denom = dotLL + dotRR + 1e-7f;
            const float rawCorr = std::clamp((2.0f * dotLR) / denom, -1.0f, 1.0f);

            smoothedCorr += alpha * (rawCorr - smoothedCorr);

            // Calculate In-Phase vs Spatial gain weight
            // Correlation above threshold favors in-phase mono coherent
            float normCorr = (smoothedCorr - corrThreshold) / (1.0f - corrThreshold + 1e-4f);
            normCorr = std::clamp(normCorr, 0.0f, 1.0f);

            // Smooth Hermite curve
            const float gainInPhase = normCorr * normCorr * (3.0f - 2.0f * normCorr);
            const float gainSpatial = 1.0f - gainInPhase;

            if (inPhaseL) inPhaseL[i] = l * gainInPhase;
            if (inPhaseR) inPhaseR[i] = r * gainInPhase;

            if (spatialL) spatialL[i] = l * gainSpatial * spread;
            if (spatialR) spatialR[i] = r * gainSpatial * spread;
        }
    }

private:
    double currentSampleRate = 48000.0;
    int currentMaxBlock = 512;

    float corrThreshold = 0.0f;
    float spread = 1.0f;
    float alpha = 0.05f;

    float dotLR = 0.0f;
    float dotLL = 0.0f;
    float dotRR = 0.0f;
    float smoothedCorr = 1.0f;
};
