#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>

class DelayTapeEchoEffect : public BaseEffect
{
public:
    DelayTapeEchoEffect() = default;

    EffectType getType() const override { return EffectType::DelayTapeEcho; }
    juce::String getName() const override { return "Tape Echo"; }

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
        flutterPhase = 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
        dampL = 0.0f;
        dampR = 0.0f;
        smoothedDelaySamples = targetDelaySamples;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free ms, >0 = Synced subdivisions 1/32..1 Bar)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        double timeSec = 0.02 + std::clamp(p1, 0.0f, 1.0f) * 0.98;
        if (syncSec > 0.0)
            timeSec = syncSec;

        targetDelaySamples = static_cast<float>(timeSec * currentSampleRate);
        if (smoothedDelaySamples <= 0.0f)
            smoothedDelaySamples = targetDelaySamples;

        // P2: Feedback (0% to 95% safe maximum)
        feedbackAmount = std::clamp(p2, 0.0f, 1.0f) * 0.95f;

        // P3: Wow & Flutter (0% to 100%)
        flutterDepth = std::clamp(p3, 0.0f, 1.0f) * static_cast<float>(currentSampleRate * 0.003);

        // P4: Tape Age Tone (500Hz to 12000Hz)
        const double cutoffHz = 500.0 + (1.0 - std::clamp(p4, 0.0f, 1.0f)) * 11500.0;
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0 || maxDelaySamples <= 0) return;

        const float smoothAlpha = static_cast<float>(1.0 - std::exp(-6.2831853 * 3.5 / currentSampleRate));

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            // Butter-smooth exponential delay parameter slewing (smooth analog tape speed transitions)
            smoothedDelaySamples += smoothAlpha * (targetDelaySamples - smoothedDelaySamples);

            const float satL = std::tanh((dryL + feedbackL * feedbackAmount) * tapeDrive);
            const float satR = std::tanh((dryR + feedbackR * feedbackAmount) * tapeDrive);

            bufferL[writeIndex] = satL;
            bufferR[writeIndex] = satR;

            flutterPhase += 0.0003f;
            if (flutterPhase >= 6.2831853f) flutterPhase -= 6.2831853f;

            const float flutter = (std::sin(flutterPhase * 3.1f) + 0.4f * std::sin(flutterPhase * 11.7f)) * flutterDepth;
            const float dSamples = smoothedDelaySamples + flutter;

            const float delayedL = readHermite4(bufferL, dSamples);
            const float delayedR = readHermite4(bufferR, dSamples * 1.015f);

            dampL += dampCoeff * (delayedL - dampL);
            dampR += dampCoeff * (delayedR - dampR);

            feedbackL = dampL;
            feedbackR = dampR;

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            bufL[i] = dryL * (1.0f - wetMix) + dampL * wetMix;
            if (bufR)
                bufR[i] = dryR * (1.0f - wetMix) + dampR * wetMix;
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

    float flutterPhase = 0.0f;
    float flutterDepth = 0.0f;

    float dampCoeff = 0.8f;
    float dampL = 0.0f;
    float dampR = 0.0f;

    float tapeDrive = 1.0f;
    float wetMix = 0.5f;
};
