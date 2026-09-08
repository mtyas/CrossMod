#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>

class DelayPingPongEffect : public BaseEffect
{
public:
    DelayPingPongEffect() = default;

    EffectType getType() const override { return EffectType::DelayPingPong; }
    juce::String getName() const override { return "Ping-Pong Delay"; }

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
        dampL = 0.0f;
        dampR = 0.0f;
        modPhase = 0.0f;
        smoothedDelaySamples = targetDelaySamples;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free ms, >0 = Synced subdivisions 1/32..1 Bar)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        double timeSec = 0.01 + std::clamp(p1, 0.0f, 1.0f) * 1.19;
        if (syncSec > 0.0)
            timeSec = syncSec;

        targetDelaySamples = static_cast<float>(timeSec * currentSampleRate);
        if (smoothedDelaySamples <= 0.0f)
            smoothedDelaySamples = targetDelaySamples;

        // P2: Feedback (0% to 95%)
        feedbackAmount = std::clamp(p2, 0.0f, 0.95f);

        // P3: Cross Width (Stereo Separation)
        crossWidth = std::clamp(p3, 0.0f, 1.0f);

        // P4: Damping Filter (800Hz to 18000Hz)
        const double cutoffHz = 800.0 * std::pow(22.5, std::clamp(p4, 0.0f, 1.0f));
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0 || maxDelaySamples <= 0) return;

        const float smoothAlpha = static_cast<float>(1.0 - std::exp(-6.2831853 * 4.0 / currentSampleRate));

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            // Smooth delay parameter slewing
            smoothedDelaySamples += smoothAlpha * (targetDelaySamples - smoothedDelaySamples);

            modPhase += modInc;
            if (modPhase >= 6.2831853f) modPhase -= 6.2831853f;
            const float modL = std::sin(modPhase) * modDepthSamples;
            const float modR = std::sin(modPhase + 1.57f) * modDepthSamples;

            bufferL[writeIndex] = dryL + feedbackR * feedbackAmount;
            bufferR[writeIndex] = dryR + feedbackL * feedbackAmount;

            const float delayedL = readHermite4(bufferL, smoothedDelaySamples + modL);
            const float delayedR = readHermite4(bufferR, smoothedDelaySamples + modR);

            dampL += dampCoeff * (delayedL - dampL);
            dampR += dampCoeff * (delayedR - dampR);

            feedbackL = dampL;
            feedbackR = dampR;

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            const float mid = 0.5f * (dampL + dampR);
            const float sideL = dampL - mid;
            const float sideR = dampR - mid;

            const float wetL = mid + sideL * crossWidth;
            const float wetR = mid + sideR * crossWidth;

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

    float targetDelaySamples = 12000.0f;
    float smoothedDelaySamples = 12000.0f;

    float feedbackAmount = 0.0f;
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    float dampCoeff = 0.8f;
    float dampL = 0.0f;
    float dampR = 0.0f;

    float crossWidth = 1.0f;
    float modPhase = 0.0f;
    float modInc = 0.0f;
    float modDepthSamples = 0.0f;
    float wetMix = 0.5f;
};
