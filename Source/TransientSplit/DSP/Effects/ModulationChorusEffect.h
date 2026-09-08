#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>

class ModulationChorusEffect : public BaseEffect
{
public:
    ModulationChorusEffect() = default;

    EffectType getType() const override { return EffectType::ModulationChorus; }
    juce::String getName() const override { return "Stereo Chorus"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxDelaySamples = static_cast<int>(currentSampleRate * 0.06) + 16;
        bufferL.assign(maxDelaySamples, 0.0f);
        bufferR.assign(maxDelaySamples, 0.0f);
        reset();
    }

    void reset() override
    {
        std::fill(bufferL.begin(), bufferL.end(), 0.0f);
        std::fill(bufferR.begin(), bufferR.end(), 0.0f);
        writeIndex = 0;
        lfoPhase = 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free Hz, >0 = Synced subdivisions 1/16..4 Bars)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        float rateHz = 0.1f + std::clamp(p1, 0.0f, 1.0f) * 7.9f;
        if (syncSec > 0.0)
            rateHz = static_cast<float>(1.0 / syncSec);

        lfoInc = static_cast<float>(6.283185307179586 * rateHz / currentSampleRate);

        // P2: Depth (0% to 100%)
        depthSamples = std::clamp(p2, 0.0f, 1.0f) * static_cast<float>(currentSampleRate * 0.006);

        // P3: Stereo Width (0 to 180 degrees phase offset)
        stereoPhaseOffset = std::clamp(p3, 0.0f, 1.0f) * 3.14159265f;

        // P4: Pre-Delay (5ms to 30ms center delay)
        const double preMs = 5.0 + std::clamp(p4, 0.0f, 1.0f) * 25.0;
        centerDelay = static_cast<float>(preMs * 0.001 * currentSampleRate);

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0 || maxDelaySamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            bufferL[writeIndex] = dryL + feedbackL * feedbackAmount;
            bufferR[writeIndex] = dryR + feedbackR * feedbackAmount;

            lfoPhase += lfoInc;
            if (lfoPhase >= 6.2831853f) lfoPhase -= 6.2831853f;

            const float modL = std::sin(lfoPhase);
            const float modR = std::sin(lfoPhase + stereoPhaseOffset);

            const float rL = static_cast<float>(writeIndex) - (centerDelay + modL * depthSamples);
            const float rR = static_cast<float>(writeIndex) - (centerDelay + modR * depthSamples);

            auto readCirc = [this](const std::vector<float>& buf, float rPos) -> float
            {
                while (rPos < 0.0f) rPos += static_cast<float>(maxDelaySamples);
                while (rPos >= static_cast<float>(maxDelaySamples)) rPos -= static_cast<float>(maxDelaySamples);
                const int r0 = static_cast<int>(rPos);
                const int r1 = (r0 + 1) % maxDelaySamples;
                const float frac = rPos - static_cast<float>(r0);
                return buf[r0] + frac * (buf[r1] - buf[r0]);
            };

            const float wetL = readCirc(bufferL, rL);
            const float wetR = readCirc(bufferR, rR);

            feedbackL = wetL;
            feedbackR = wetR;

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            bufL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufR)
                bufR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;
    int maxDelaySamples = 3000;
    std::vector<float> bufferL;
    std::vector<float> bufferR;
    int writeIndex = 0;

    float lfoPhase = 0.0f;
    float lfoInc = 0.0f;
    float centerDelay = 500.0f;
    float depthSamples = 150.0f;
    float stereoPhaseOffset = 1.57f;
    float feedbackAmount = 0.0f;
    float wetMix = 1.0f;

    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
};
