#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class FilterBandpassEffect : public BaseEffect
{
public:
    FilterBandpassEffect() = default;

    EffectType getType() const override { return EffectType::FilterBandpass; }
    juce::String getName() const override { return "Bandpass & Notch Filter"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        s1_L = 0.0f; s2_L = 0.0f;
        s1_R = 0.0f; s2_R = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Center Frequency (40Hz to 14000Hz)
        centerFreqHz = 40.0f * std::pow(350.0f, std::clamp(p1, 0.0f, 1.0f));

        // P2: Resonance Q (0.0 to 1.0 -> self-oscillates at high Q)
        reso = std::clamp(p2, 0.0f, 1.0f);

        // P3: Notch Blend (0.0 = Pure Bandpass, 1.0 = Pure Notch)
        notchBlend = std::clamp(p3, 0.0f, 1.0f);

        // P4: Drive (1.0x to 4.5x)
        drive = 1.0f + std::clamp(p4, 0.0f, 1.0f) * 3.5f;

        // P5: Stereo Frequency Spread (-30% to +30%)
        stereoSpread = -0.3f + std::clamp(p5, 0.0f, 1.0f) * 0.6f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        updateCoeffs();
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            const float drivenL = std::tanh(dryL * drive) / std::sqrt(drive);
            const float drivenR = std::tanh(dryR * drive) / std::sqrt(drive);

            float bpOutL = 0.0f, notchOutL = 0.0f;
            float bpOutR = 0.0f, notchOutR = 0.0f;

            processZdfSVF(drivenL, s1_L, s2_L, g_L, R, bpOutL, notchOutL);
            processZdfSVF(drivenR, s1_R, s2_R, g_R, R, bpOutR, notchOutR);

            const float wetL = bpOutL * (1.0f - notchBlend) + notchOutL * notchBlend;
            const float wetR = bpOutR * (1.0f - notchBlend) + notchOutR * notchBlend;

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    void updateCoeffs()
    {
        const double sr = currentSampleRate;

        const double freqL = std::clamp(centerFreqHz * (1.0f - stereoSpread * 0.5f), 20.0f, static_cast<float>(sr * 0.48f));
        const double freqR = std::clamp(centerFreqHz * (1.0f + stereoSpread * 0.5f), 20.0f, static_cast<float>(sr * 0.48f));

        g_L = static_cast<float>(std::tan(3.141592653589793 * freqL / sr));
        g_R = static_cast<float>(std::tan(3.141592653589793 * freqR / sr));

        // Damping factor R (self-oscillation when reso -> 1.0)
        R = (1.0f - reso * 1.02f);
    }

    inline void processZdfSVF(float input, float& s1, float& s2, float g, float dampR, float& bpOut, float& notchOut) noexcept
    {
        const float satS1 = std::tanh(s1);

        const float denom = 1.0f + 2.0f * dampR * g + g * g;
        const float hp = (input - 2.0f * dampR * satS1 - g * satS1 - s2) / denom;
        const float bp = g * hp + satS1;
        const float lp = g * bp + s2;

        s1 = 2.0f * bp - satS1;
        s2 = 2.0f * lp - s2;

        if (std::abs(s1) < 1e-15f) s1 = 0.0f;
        if (std::abs(s2) < 1e-15f) s2 = 0.0f;

        bpOut = bp;
        notchOut = hp + lp;
    }

    double currentSampleRate = 48000.0;
    float centerFreqHz = 1000.0f;
    float reso = 0.3f;
    float notchBlend = 0.0f;
    float drive = 1.0f;
    float stereoSpread = 0.0f;
    float wetMix = 1.0f;

    float g_L = 0.1f, g_R = 0.1f, R = 1.0f;
    float s1_L = 0.0f, s2_L = 0.0f;
    float s1_R = 0.0f, s2_R = 0.0f;
};
