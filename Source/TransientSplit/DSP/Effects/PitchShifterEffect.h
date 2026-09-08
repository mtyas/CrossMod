#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>

class PitchShifterEffect : public BaseEffect
{
public:
    PitchShifterEffect() = default;

    EffectType getType() const override { return EffectType::PitchShifter; }
    juce::String getName() const override { return "Pitch Shifter"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxDelaySamples = static_cast<int>(currentSampleRate * 0.5);

        delayBufferL.assign(maxDelaySamples, 0.0f);
        delayBufferR.assign(maxDelaySamples, 0.0f);

        reset();
    }

    void reset() override
    {
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        writeIndex = 0;
        phaseL = 0.0f;
        phaseR = 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Semitones (-24 to +24)
        const float semitones = -24.0f + std::clamp(p1, 0.0f, 1.0f) * 48.0f;

        // P2: Fine tune (-100 to +100 cents)
        const float cents = -100.0f + std::clamp(p2, 0.0f, 1.0f) * 200.0f;

        // P3: Window size / Grain Size (15ms to 120ms)
        const double windowMs = 15.0 + std::clamp(p3, 0.0f, 1.0f) * 105.0;
        windowSamples = static_cast<float>(windowMs * 0.001 * currentSampleRate);

        // P4: Feedback (0% to 75%)
        feedbackAmount = std::clamp(p4, 0.0f, 0.75f);

        // P5: Stereo Detune / Width (-50 cents to +50 cents spread)
        const float detuneCents = (-0.5f + std::clamp(p5, 0.0f, 1.0f)) * 100.0f;

        const float totalStL = semitones + ((cents - detuneCents * 0.5f) * 0.01f);
        const float totalStR = semitones + ((cents + detuneCents * 0.5f) * 0.01f);

        pitchRatioL = std::pow(2.0f, totalStL / 12.0f);
        pitchRatioR = std::pow(2.0f, totalStR / 12.0f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0 || maxDelaySamples <= 0) return;

        const float win = std::max(10.0f, windowSamples);
        const float rateL = (1.0f - pitchRatioL) / win;
        const float rateR = (1.0f - pitchRatioR) / win;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            delayBufferL[writeIndex] = dryL + feedbackL * feedbackAmount;
            delayBufferR[writeIndex] = dryR + feedbackR * feedbackAmount;

            phaseL += rateL;
            while (phaseL >= 1.0f) phaseL -= 1.0f;
            while (phaseL < 0.0f) phaseL += 1.0f;

            phaseR += rateR;
            while (phaseR >= 1.0f) phaseR -= 1.0f;
            while (phaseR < 0.0f) phaseR += 1.0f;

            auto getGrainTap = [this](const std::vector<float>& buf, float ph, float wSize) -> float
            {
                float ph2 = ph + 0.5f;
                if (ph2 >= 1.0f) ph2 -= 1.0f;

                float d1 = ph * wSize;
                float d2 = ph2 * wSize;

                auto readTap = [this, &buf](float d) -> float
                {
                    float rPos = static_cast<float>(writeIndex) - d;
                    while (rPos < 0.0f) rPos += static_cast<float>(maxDelaySamples);
                    while (rPos >= static_cast<float>(maxDelaySamples)) rPos -= static_cast<float>(maxDelaySamples);
                    int r0 = static_cast<int>(rPos);
                    int r1 = (r0 + 1) % maxDelaySamples;
                    float frac = rPos - static_cast<float>(r0);
                    return buf[r0] + frac * (buf[r1] - buf[r0]);
                };

                float tap1 = readTap(d1);
                float tap2 = readTap(d2);

                float w1 = 0.5f * (1.0f - std::cos(ph * 6.2831853f));
                float w2 = 0.5f * (1.0f - std::cos(ph2 * 6.2831853f));

                return tap1 * w1 + tap2 * w2;
            };

            const float wetL = getGrainTap(delayBufferL, phaseL, win);
            const float wetR = getGrainTap(delayBufferR, phaseR, win);

            feedbackL = wetL;
            feedbackR = wetR;

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;
    int maxDelaySamples = 24000;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writeIndex = 0;

    float phaseL = 0.0f;
    float phaseR = 0.0f;
    float pitchRatioL = 1.0f;
    float pitchRatioR = 1.0f;
    float windowSamples = 1920.0f;
    float feedbackAmount = 0.0f;
    float wetMix = 1.0f;

    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
};
