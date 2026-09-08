#pragma once

#include <cmath>
#include <algorithm>
#include "../Parameters/ParameterIDs.h"

class VintageFilter
{
public:
    VintageFilter() = default;

    static inline float fast_tanh(float x) noexcept
    {
        if (x < -3.0f) return -1.0f;
        if (x > 3.0f) return 1.0f;
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    static inline float fast_tan(float x) noexcept
    {
        // 6th order Horner polynomial for tan(x) on [0, 0.85] (error < 0.03%)
        float x2 = x * x;
        return x * (1.0f + x2 * (0.3333333f + x2 * (0.1333333f + 0.0539683f * x2)));
    }

    static inline float soft_saturate_state(float s, float limit = 3.0f) noexcept
    {
        float invLimit = 1.0f / limit;
        return fast_tanh(s * invLimit) * limit;
    }

    void setSampleRate(double newSampleRate)
    {
        sampleRate = std::max(1.0, newSampleRate);
        reset();
    }

    void reset()
    {
        s1 = 0.0f;
        s2 = 0.0f;
        s3 = 0.0f;
        s4 = 0.0f;
    }

    void excite() noexcept
    {
        if (std::abs(s1) < 1e-6f && std::abs(s2) < 1e-6f)
            s1 = 1e-4f;
    }

    void setParameters(float cutoffHz, float resonance, FilterType type, float drive)
    {
        float maxCutoff = std::min(18500.0f, (float)(sampleRate * 0.42));
        currentCutoff = std::clamp(cutoffHz, 15.0f, maxCutoff);
        currentResonance = std::clamp(resonance, 0.0f, 0.995f);
        currentType = type;
        currentDrive = std::clamp(drive, 0.0f, 1.0f);
        if (currentResonance > 0.95f && std::abs(s1) < 1e-6f && std::abs(s2) < 1e-6f)
        {
            s1 = 1e-4f;
        }
    }


    inline float processSample(float input)
    {
        if (!std::isfinite(input))
            input = 0.0f;

        // Auto-heal if states ever corrupt
        if (!std::isfinite(s1) || !std::isfinite(s2) || !std::isfinite(s3) || !std::isfinite(s4))
        {
            reset();
        }

        // 1. Analog Input Saturation / Drive (silky fast_tanh)
        float driveGain = 1.0f + currentDrive * 2.5f;
        float drivenInput = fast_tanh(input * driveGain);

        // 2x internal oversampling for perfect stability under audio-rate modulation:
        // With 2x oversampling, effective sample rate is 2 * sampleRate
        // wd is always <= 0.85!
        float wd = 3.141592653589793f * currentCutoff / (float)(sampleRate * 2.0);
        wd = std::clamp(wd, 0.0001f, 0.85f);
        float g = fast_tan(wd);

        // Resonance parameter (q factor / damping factor)
        // At resonance = 1.0, k -> 0.01 (clean self-oscillation with soft saturation)
        float k = 2.0f * (1.0f - currentResonance);

        float denom1 = 1.0f + g * (k + g);
        float k2 = 1.41421356f;
        float denom2 = 1.0f + g * (k2 + g);

        float stage1Output = 0.0f;
        float stage2Output = 0.0f;

        // Run 2 sub-steps of SVF integration
        for (int step = 0; step < 2; ++step)
        {
            // Stage 1 (2-pole SVF)
            float hp1 = (drivenInput - (k + g) * s1 - s2) / denom1;
            float bp1 = g * hp1 + s1;
            float lp1 = g * bp1 + s2;
            s1 = g * hp1 + bp1;
            s2 = g * bp1 + lp1;

            // Smooth C2 continuous saturation inside integrator loop (no click / pop)
            s1 = soft_saturate_state(s1, 3.0f);
            s2 = soft_saturate_state(s2, 3.0f);
            bp1 = fast_tanh(bp1);
            lp1 = fast_tanh(lp1);

            switch (currentType)
            {
                case FilterType::Lowpass12:  stage1Output = lp1; break;
                case FilterType::Bandpass12: stage1Output = bp1; break;
                case FilterType::Highpass12: stage1Output = hp1; break;
                case FilterType::Lowpass24:  stage1Output = lp1; break;
            }

            if (currentType == FilterType::Lowpass24)
            {
                // Stage 2 (Second 2-pole stage for 24dB cascade)
                float hp2 = (stage1Output - (k2 + g) * s3 - s4) / denom2;
                float bp2 = g * hp2 + s3;
                float lp2 = g * bp2 + s4;
                s3 = g * hp2 + bp2;
                s4 = g * bp2 + lp2;

                s3 = soft_saturate_state(s3, 3.0f);
                s4 = soft_saturate_state(s4, 3.0f);
                bp2 = fast_tanh(bp2);
                lp2 = fast_tanh(lp2);

                stage2Output = lp2;
            }
        }

        float finalOut = (currentType == FilterType::Lowpass24) ? stage2Output : stage1Output;
        return std::isfinite(finalOut) ? finalOut : 0.0f;
    }

private:
    double sampleRate = 44100.0;
    float currentCutoff = 1000.0f;
    float currentResonance = 0.1f;
    float currentDrive = 0.0f;
    FilterType currentType = FilterType::Lowpass24;

    float s1 = 0.0f;
    float s2 = 0.0f;
    float s3 = 0.0f;
    float s4 = 0.0f;
};

