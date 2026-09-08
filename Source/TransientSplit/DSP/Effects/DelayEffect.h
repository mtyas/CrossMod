#pragma once

#include "BaseEffect.h"

class DelayEffect : public BaseEffect
{
public:
    DelayEffect() = default;

    EffectType getType() const override { return EffectType::Delay; }
    juce::String getName() const override { return "Stereo Delay"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxDelaySamples = static_cast<int>(currentSampleRate * 2.5) + 32; // 2.5 seconds buffer
        bufferL.assign(maxDelaySamples, 0.0f);
        bufferR.assign(maxDelaySamples, 0.0f);
        writeIndex = 0;
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
    }

    void setParameters(float p1Time, float p2Feedback, float p3Damp, float p4PingPong, float mixVal) override
    {
        timeParam = std::clamp(p1Time, 0.0f, 1.0f);
        feedbackAmount = std::clamp(p2Feedback, 0.0f, 0.98f);
        dampParam = std::clamp(p3Damp, 0.0f, 1.0f);
        pingPongAmount = std::clamp(p4PingPong, 0.0f, 1.0f);
        wetMix = std::clamp(mixVal, 0.0f, 1.0f);

        // Feedback Damping LP filter: 800Hz to 16kHz
        const double cutoff = 800.0 + dampParam * 15200.0;
        const double w = 2.0 * 3.141592653589793 * (cutoff / currentSampleRate);
        dampAlpha = static_cast<float>(std::clamp(w / (w + 1.0), 0.001, 0.999));
    }

    void process(float* left, float* right, int numSamples) override
    {
        if (numSamples <= 0 || maxDelaySamples <= 0) return;

        // Calculate delay time in samples
        float delayTimeSec = 0.02f + timeParam * 0.98f; // 20ms to 1000ms
        float targetDelaySamples = delayTimeSec * static_cast<float>(currentSampleRate);
        targetDelaySamples = std::clamp(targetDelaySamples, 10.0f, static_cast<float>(maxDelaySamples - 10));

        // Smooth delay time interpolation
        const float smoothCoeff = 0.002f;

        for (int i = 0; i < numSamples; ++i)
        {
            smoothedDelaySamples += smoothCoeff * (targetDelaySamples - smoothedDelaySamples);

            const float dryL = left ? left[i] : 0.0f;
            const float dryR = right ? right[i] : 0.0f;

            // Compute delay times for L and R (ping pong offsets R by 1.5x or swaps channels)
            const float delayL = smoothedDelaySamples;
            const float delayR = smoothedDelaySamples * (1.0f + 0.33f * pingPongAmount);

            // Read from delay buffers with linear interpolation
            float delayedL = readBuffer(bufferL, writeIndex - delayL);
            float delayedR = readBuffer(bufferR, writeIndex - delayR);

            // Damping low-pass filter in feedback path
            dampL += dampAlpha * (delayedL - dampL);
            dampR += dampAlpha * (delayedR - dampR);
            delayedL = dampL;
            delayedR = dampR;

            // Ping-pong cross-feed
            float fbInL = dryL + (delayedL * (1.0f - pingPongAmount) + delayedR * pingPongAmount) * feedbackAmount;
            float fbInR = dryR + (delayedR * (1.0f - pingPongAmount) + delayedL * pingPongAmount) * feedbackAmount;

            // Soft-clip in feedback path for analog warmth & overload protection
            fbInL = std::tanh(fbInL);
            fbInR = std::tanh(fbInR);

            bufferL[writeIndex] = fbInL;
            bufferR[writeIndex] = fbInR;

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            if (left) left[i] = dryL * (1.0f - wetMix) + delayedL * wetMix;
            if (right) right[i] = dryR * (1.0f - wetMix) + delayedR * wetMix;
        }
    }

    EffectParamInfo getParamInfo(int paramIndex) const override
    {
        switch (paramIndex)
        {
            case 0: return { "Time", "Delay Time", 0.35f, 0.0f, 1.0f };
            case 1: return { "Feedback", "Feedback", 0.45f, 0.0f, 0.98f };
            case 2: return { "Color", "HF Damping", 0.7f, 0.0f, 1.0f };
            case 3: return { "PingPong", "Ping-Pong / Spread", 0.3f, 0.0f, 1.0f };
            default: return { "Param", "Param", 0.0f, 0.0f, 1.0f };
        }
    }

private:
    float readBuffer(const std::vector<float>& buf, float samplePos) const
    {
        const int size = static_cast<int>(buf.size());
        while (samplePos < 0.0f) samplePos += size;
        while (samplePos >= size) samplePos -= size;

        const int i0 = static_cast<int>(samplePos);
        const int i1 = (i0 + 1) % size;
        const float frac = samplePos - static_cast<float>(i0);

        return buf[i0] + frac * (buf[i1] - buf[i0]);
    }

    float timeParam = 0.35f;
    float feedbackAmount = 0.45f;
    float dampParam = 0.7f;
    float pingPongAmount = 0.3f;
    float wetMix = 0.4f;

    float smoothedDelaySamples = 4800.0f;
    float dampAlpha = 0.5f;
    float dampL = 0.0f; float dampR = 0.0f;
    float feedbackL = 0.0f; float feedbackR = 0.0f;

    std::vector<float> bufferL;
    std::vector<float> bufferR;
    int maxDelaySamples = 0;
    int writeIndex = 0;
};
