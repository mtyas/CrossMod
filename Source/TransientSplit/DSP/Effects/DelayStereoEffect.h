#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>

class DelayStereoEffect : public BaseEffect
{
public:
    DelayStereoEffect() = default;

    EffectType getType() const override { return EffectType::DelayStereo; }
    juce::String getName() const override { return "Stereo Delay"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxDelaySamples = static_cast<int>(currentSampleRate * 2.0) + 32;
        bufferL.assign(maxDelaySamples, 0.0f);
        bufferR.assign(maxDelaySamples, 0.0f);
        reset();
    }

    void reset() override
    {
        std::fill(bufferL.begin(), bufferL.end(), 0.0f);
        std::fill(bufferR.begin(), bufferR.end(), 0.0f);
        writeIndex = 0;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
        dampStateL = 0.0f;
        dampStateR = 0.0f;
        duckEnv = 0.0f;
        smoothedDelaySamplesL = targetDelaySamplesL;
        smoothedDelaySamplesR = targetDelaySamplesR;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free ms, >0 = Synced subdivisions 1/32..1 Bar)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        double timeSec = 0.01 + std::clamp(p1, 0.0f, 1.0f) * 1.19;
        if (syncSec > 0.0)
            timeSec = syncSec;

        targetDelaySamplesL = static_cast<float>(timeSec * currentSampleRate);

        // P2: Feedback (0% to 95%)
        feedbackAmount = std::clamp(p2, 0.0f, 0.95f);

        // P3: Stereo Offset (-50% to +50% L/R time difference)
        const float offset = -0.5f + std::clamp(p3, 0.0f, 1.0f) * 1.0f;
        targetDelaySamplesR = std::clamp(targetDelaySamplesL * (1.0f + offset * 0.5f), 10.0f, static_cast<float>(maxDelaySamples - 4));

        if (smoothedDelaySamplesL <= 0.0f)
        {
            smoothedDelaySamplesL = targetDelaySamplesL;
            smoothedDelaySamplesR = targetDelaySamplesR;
        }

        // P4: Damping Filter (800Hz to 18000Hz)
        const double cutoffHz = 800.0 * std::pow(22.5, std::clamp(p4, 0.0f, 1.0f));
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0 || maxDelaySamples <= 0) return;

        // Smooth parameter slewing coefficient (~60ms time constant)
        const float smoothAlpha = static_cast<float>(1.0 - std::exp(-6.2831853 * 4.0 / currentSampleRate));

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            // Butter-smooth exponential delay parameter slewing (eliminates all clicks / zipper noise)
            smoothedDelaySamplesL += smoothAlpha * (targetDelaySamplesL - smoothedDelaySamplesL);
            smoothedDelaySamplesR += smoothAlpha * (targetDelaySamplesR - smoothedDelaySamplesR);

            // Dynamic ducking follower
            const float inAbs = (std::abs(dryL) + std::abs(dryR)) * 0.5f;
            duckEnv += 0.005f * (inAbs - duckEnv);
            const float duckGain = 1.0f / (1.0f + duckEnv * duckAmount * 6.0f);

            bufferL[writeIndex] = dryL + feedbackL * feedbackAmount;
            bufferR[writeIndex] = dryR + feedbackR * feedbackAmount;

            const float delayedL = readHermite4(bufferL, smoothedDelaySamplesL);
            const float delayedR = readHermite4(bufferR, smoothedDelaySamplesR);

            dampStateL += dampCoeff * (delayedL - dampStateL);
            dampStateR += dampCoeff * (delayedR - dampStateR);

            feedbackL = dampStateL;
            feedbackR = dampStateR;

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            const float wetL = dampStateL * duckGain;
            const float wetR = dampStateR * duckGain;

            bufL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufR)
                bufR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    // 4-Point Hermite Cubic Interpolation: C1-continuous, perfectly click-free and zipper-free
    inline float readHermite4(const std::vector<float>& buf, float dSamples) const noexcept
    {
        float rPos = static_cast<float>(writeIndex) - dSamples;
        while (rPos < 0.0f) rPos += static_cast<float>(maxDelaySamples);
        while (rPos >= static_cast<float>(maxDelaySamples)) rPos -= static_cast<float>(maxDelaySamples);

        const int i0 = static_cast<int>(rPos);
        const float frac = rPos - static_cast<float>(i0);

        const int im1 = (i0 - 1 + maxDelaySamples) % maxDelaySamples;
        const int i1  = (i0 + 1) % maxDelaySamples;
        const int i2  = (i0 + 2) % maxDelaySamples;

        const float ym1 = buf[im1];
        const float y0  = buf[i0];
        const float y1  = buf[i1];
        const float y2  = buf[i2];

        const float c0 = y0;
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    double currentSampleRate = 48000.0;
    int maxDelaySamples = 96032;
    int writeIndex = 0;

    std::vector<float> bufferL, bufferR;

    float targetDelaySamplesL = 12000.0f;
    float targetDelaySamplesR = 12000.0f;
    float smoothedDelaySamplesL = 12000.0f;
    float smoothedDelaySamplesR = 12000.0f;

    float feedbackAmount = 0.0f;
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    float dampCoeff = 0.8f;
    float dampStateL = 0.0f;
    float dampStateR = 0.0f;

    float duckAmount = 0.0f;
    float duckEnv = 0.0f;
    float wetMix = 0.5f;
};
