#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

class MidSideSplitter
{
public:
    MidSideSplitter() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        currentMaxBlock = std::max(64, maxBlockSize);
    }

    void reset() noexcept
    {
        // Stateless linear matrix
    }

    void setParameters(float widthPct, float balancePan) noexcept
    {
        stereoWidth = std::clamp(widthPct, 0.0f, 2.0f);
        balance = std::clamp(balancePan, -1.0f, 1.0f);
    }

    void process(const float* inL, const float* inR,
                 float* leftL, float* leftR,
                 float* rightL, float* rightR,
                 float* midL, float* midR,
                 float* sideL, float* sideR,
                 int numSamples)
    {
        if (numSamples <= 0 || !inL) return;

        const float invSqrt2 = 0.7071067811865475f;
        const float balL = (balance <= 0.0f) ? 1.0f : (1.0f - balance);
        const float balR = (balance >= 0.0f) ? 1.0f : (1.0f + balance);

        for (int i = 0; i < numSamples; ++i)
        {
            const float l = inL[i] * balL;
            const float r = (inR ? inR[i] : inL[i]) * balR;

            // 1. Isolated Left Channel (centered on both channels for processing)
            if (leftL) leftL[i] = l;
            if (leftR) leftR[i] = l;

            // 2. Isolated Right Channel (centered on both channels for processing)
            if (rightL) rightL[i] = r;
            if (rightR) rightR[i] = r;

            // 3. Mid (Sum / Center)
            const float m = (l + r) * invSqrt2;
            if (midL) midL[i] = m;
            if (midR) midR[i] = m;

            // 4. Side (Difference / Stereo Width)
            const float s = (l - r) * invSqrt2 * stereoWidth;
            if (sideL) sideL[i] = s;
            if (sideR) sideR[i] = -s;
        }
    }

private:
    double currentSampleRate = 48000.0;
    int currentMaxBlock = 512;
    float stereoWidth = 1.0f;
    float balance = 0.0f;
};
