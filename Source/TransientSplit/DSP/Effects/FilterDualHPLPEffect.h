#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class FilterDualHPLPEffect : public BaseEffect
{
public:
    FilterDualHPLPEffect() = default;

    EffectType getType() const override { return EffectType::FilterDualHPLP; }
    juce::String getName() const override { return "Dual HP / LP Filter"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        s1_lpL = 0.0f; s2_lpL = 0.0f;
        s1_lpR = 0.0f; s2_lpR = 0.0f;
        s1_hpL = 0.0f; s2_hpL = 0.0f;
        s1_hpR = 0.0f; s2_hpR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: LP Cutoff (40Hz to 20000Hz)
        lpCutoffHz = 40.0f * std::pow(500.0f, std::clamp(p1, 0.0f, 1.0f));

        // P2: LP Resonance (0.0 to 1.0 -> self-oscillates above ~0.82)
        lpReso = std::clamp(p2, 0.0f, 1.0f);

        // P3: HP Cutoff (15Hz to 4000Hz)
        hpCutoffHz = 15.0f * std::pow(266.6f, std::clamp(p3, 0.0f, 1.0f));

        // P4: HP Resonance (0.0 to 1.0 -> self-oscillates above ~0.82)
        hpReso = std::clamp(p4, 0.0f, 1.0f);

        // P5: Drive Warmth (1.0x to 4.5x analog saturation)
        drive = 1.0f + std::clamp(p5, 0.0f, 1.0f) * 3.5f;

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

            // 1. Process High-Pass Filter Stage (ZDF State Variable Filter)
            const float hpOutL = processZdfSVF(dryL, s1_hpL, s2_hpL, g_hp, R_hp, true);
            const float hpOutR = processZdfSVF(dryR, s1_hpR, s2_hpR, g_hp, R_hp, true);

            // 2. Drive / Analog Saturation stage between HP and LP
            const float drivenL = std::tanh(hpOutL * drive) / std::sqrt(drive);
            const float drivenR = std::tanh(hpOutR * drive) / std::sqrt(drive);

            // 3. Process Low-Pass Filter Stage (ZDF State Variable Filter with self-oscillating non-linear feedback)
            const float lpOutL = processZdfSVF(drivenL, s1_lpL, s2_lpL, g_lp, R_lp, false);
            const float lpOutR = processZdfSVF(drivenR, s1_lpR, s2_lpR, g_lp, R_lp, false);

            bufferL[i] = dryL * (1.0f - wetMix) + lpOutL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + lpOutR * wetMix;
        }
    }

private:
    void updateCoeffs()
    {
        const double sr = currentSampleRate;

        // Bilinear transform pre-warping: g = tan(pi * fc / sr)
        const double w_lp = std::clamp(lpCutoffHz, 20.0f, static_cast<float>(sr * 0.48f));
        g_lp = static_cast<float>(std::tan(3.141592653589793 * w_lp / sr));

        // Damping factor R: when reso -> 1.0, R -> 0.0 and slightly negative for self-oscillation
        // R = 1.0 / (2 * Q). Q from 0.5 to 25.0+
        R_lp = (1.0f - lpReso * 1.02f);

        const double w_hp = std::clamp(hpCutoffHz, 15.0f, static_cast<float>(sr * 0.48f));
        g_hp = static_cast<float>(std::tan(3.141592653589793 * w_hp / sr));
        R_hp = (1.0f - hpReso * 1.02f);
    }

    // Zero-Delay Feedback State Variable Filter with Non-Linear Saturated Resonance for stable self-oscillation
    inline float processZdfSVF(float input, float& s1, float& s2, float g, float R, bool isHighPass) noexcept
    {
        // Non-linear saturating feedback term: prevents blowup and creates rich sinusoidal self-oscillation
        const float satS1 = std::tanh(s1);

        // ZDF SVF linear implicit solver with non-linear feedback
        const float denom = 1.0f + 2.0f * R * g + g * g;
        const float hp = (input - 2.0f * R * satS1 - g * satS1 - s2) / denom;
        const float bp = g * hp + satS1;
        const float lp = g * bp + s2;

        // Trapezoidal state integration
        s1 = 2.0f * bp - satS1;
        s2 = 2.0f * lp - s2;

        // Anti-denormal flushing
        if (std::abs(s1) < 1e-15f) s1 = 0.0f;
        if (std::abs(s2) < 1e-15f) s2 = 0.0f;

        return isHighPass ? hp : lp;
    }

    double currentSampleRate = 48000.0;
    float lpCutoffHz = 20000.0f;
    float lpReso = 0.2f;
    float hpCutoffHz = 20.0f;
    float hpReso = 0.2f;
    float drive = 1.0f;
    float wetMix = 1.0f;

    float g_lp = 1.0f, R_lp = 1.0f;
    float g_hp = 0.01f, R_hp = 1.0f;

    float s1_lpL = 0.0f, s2_lpL = 0.0f;
    float s1_lpR = 0.0f, s2_lpR = 0.0f;
    float s1_hpL = 0.0f, s2_hpL = 0.0f;
    float s1_hpR = 0.0f, s2_hpR = 0.0f;
};
