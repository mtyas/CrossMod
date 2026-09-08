#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

class GlitchGranularEffect : public BaseEffect
{
public:
    GlitchGranularEffect() = default;

    EffectType getType() const override { return EffectType::GlitchGranular; }
    juce::String getName() const override { return "Granular Jitter"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxBufferSize = static_cast<int>(currentSampleRate * 1.5);
        bufferL.assign(maxBufferSize, 0.0f);
        bufferR.assign(maxBufferSize, 0.0f);
        reset();
    }

    void reset() override
    {
        std::fill(bufferL.begin(), bufferL.end(), 0.0f);
        std::fill(bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;
        grainTimer = 0;
        grainReadPosL = 0.0f;
        grainReadPosR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync Grain Rate
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        double grainMs = 10.0 + std::clamp(p1, 0.0f, 1.0f) * 140.0;
        if (syncSec > 0.0)
            grainMs = std::min(syncSec * 1000.0, 500.0);

        grainSamples = std::max(64, static_cast<int>(grainMs * 0.001 * currentSampleRate));

        // P2: Density Jitter
        densityJitter = std::clamp(p2, 0.0f, 1.0f);

        // P3: Pitch Drift
        pitchDrift = std::clamp(p3, 0.0f, 1.0f) * 0.5f;

        // P4: Spread Width
        stereoSpread = std::clamp(p4, 0.0f, 1.0f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0 || maxBufferSize <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            bufferL[writePos] = dryL;
            bufferR[writePos] = dryR;

            if (++grainTimer >= grainSamples)
            {
                grainTimer = 0;
                const float randOffset = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * grainSamples * densityJitter;
                float dir = 1.0f;
                if ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) < reverseProb)
                    dir = -1.0f;

                const float speedL = (1.0f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * pitchDrift) * dir;
                const float speedR = (1.0f + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * pitchDrift) * dir;

                grainSpeedL = speedL;
                grainSpeedR = speedR;

                grainReadPosL = static_cast<float>(writePos) - grainSamples - randOffset;
                grainReadPosR = static_cast<float>(writePos) - grainSamples - randOffset * (1.0f + stereoSpread);
            }

            grainReadPosL += grainSpeedL;
            grainReadPosR += grainSpeedR;

            auto readCirc = [this](const std::vector<float>& buf, float rPos) -> float
            {
                while (rPos < 0.0f) rPos += static_cast<float>(maxBufferSize);
                while (rPos >= static_cast<float>(maxBufferSize)) rPos -= static_cast<float>(maxBufferSize);
                const int r0 = static_cast<int>(rPos);
                const int r1 = (r0 + 1) % maxBufferSize;
                const float frac = rPos - static_cast<float>(r0);
                return buf[r0] + frac * (buf[r1] - buf[r0]);
            };

            const float phase = static_cast<float>(grainTimer) / static_cast<float>(grainSamples);
            const float win = 0.5f * (1.0f - std::cos(phase * 6.283185307179586f));

            const float wetL = readCirc(bufferL, grainReadPosL) * win;
            const float wetR = readCirc(bufferR, grainReadPosR) * win;

            writePos = (writePos + 1) % maxBufferSize;

            bufL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufR)
                bufR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;
    int maxBufferSize = 72000;
    std::vector<float> bufferL;
    std::vector<float> bufferR;

    int writePos = 0;
    int grainTimer = 0;
    int grainSamples = 2000;
    float densityJitter = 0.5f;
    float pitchDrift = 0.1f;
    float stereoSpread = 0.5f;
    float reverseProb = 0.0f;
    float wetMix = 1.0f;

    float grainReadPosL = 0.0f;
    float grainReadPosR = 0.0f;
    float grainSpeedL = 1.0f;
    float grainSpeedR = 1.0f;
};
