#pragma once

#include "BaseEffect.h"

class DistortionEffect : public BaseEffect
{
public:
    DistortionEffect() = default;

    EffectType getType() const override { return EffectType::Distortion; }
    juce::String getName() const override { return "Distortion"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        dcPrevInL = 0.0f; dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f; dcPrevOutR = 0.0f;
        filterStateL = 0.0f;
        filterStateR = 0.0f;
    }

    void setParameters(float p1Drive, float p2Tone, float p3Mode, float p4Bias, float mixVal) override
    {
        driveGain = std::pow(10.0f, (std::clamp(p1Drive, 0.0f, 1.0f) * 42.0f) * 0.05f); // up to +42 dB
        toneCutoffHz = 400.0f + std::pow(std::clamp(p2Tone, 0.0f, 1.0f), 2.0f) * 14000.0f;
        distMode = std::clamp(static_cast<int>(std::round(p3Mode)), 0, 3);
        biasAmount = std::clamp(p4Bias, -0.8f, 0.8f);
        wetMix = std::clamp(mixVal, 0.0f, 1.0f);

        // Filter coeff
        const double w = 2.0 * 3.141592653589793 * (toneCutoffHz / currentSampleRate);
        filterAlpha = static_cast<float>(std::clamp(w / (w + 1.0), 0.001, 0.999));
    }

    void process(float* left, float* right, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left ? left[i] : 0.0f;
            const float dryR = right ? right[i] : 0.0f;

            // Apply drive and bias
            float xL = (dryL * driveGain) + biasAmount;
            float xR = (dryR * driveGain) + biasAmount;

            float wetL = 0.0f;
            float wetR = 0.0f;

            switch (distMode)
            {
                case 0: // Hard Clip with cubic smoothing
                    wetL = processHardClip(xL);
                    wetR = processHardClip(xR);
                    break;
                case 1: // Diode Asymmetric
                    wetL = processDiode(xL);
                    wetR = processDiode(xR);
                    break;
                case 2: // Foldback Distortion
                    wetL = processFoldback(xL);
                    wetR = processFoldback(xR);
                    break;
                case 3: // Sine Wavefolder
                    wetL = std::sin(xL * 1.5707963f);
                    wetR = std::sin(xR * 1.5707963f);
                    break;
                default:
                    wetL = std::tanh(xL);
                    wetR = std::tanh(xR);
                    break;
            }

            // High-pass DC Blocker (~2.5 Hz)
            const float dcR = 0.9995f;
            const float filteredDcL = wetL - dcPrevInL + dcR * dcPrevOutL;
            dcPrevInL = wetL; dcPrevOutL = filteredDcL;
            wetL = filteredDcL;

            const float filteredDcR = wetR - dcPrevInR + dcR * dcPrevOutR;
            dcPrevInR = wetR; dcPrevOutR = filteredDcR;
            wetR = filteredDcR;

            // Post Tone 1-Pole Low-Pass
            filterStateL += filterAlpha * (wetL - filterStateL);
            filterStateR += filterAlpha * (wetR - filterStateR);

            wetL = filterStateL;
            wetR = filterStateR;

            // Output level compensation
            const float comp = 1.0f / (1.0f + 0.3f * std::log10(driveGain + 1.0f));
            wetL *= comp;
            wetR *= comp;

            if (left) left[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (right) right[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

    EffectParamInfo getParamInfo(int paramIndex) const override
    {
        switch (paramIndex)
        {
            case 0: return { "Drive", "Drive", 0.5f, 0.0f, 1.0f };
            case 1: return { "Tone", "Tone", 0.7f, 0.0f, 1.0f };
            case 2: return { "Mode", "Type (0:Hard,1:Diode,2:Fold,3:Wave)", 0.0f, 0.0f, 3.0f };
            case 3: return { "Bias", "Bias / Asymmetry", 0.0f, -0.8f, 0.8f };
            default: return { "Param", "Param", 0.0f, 0.0f, 1.0f };
        }
    }

private:
    static float processHardClip(float x)
    {
        if (x > 1.0f) return 1.0f;
        if (x < -1.0f) return -1.0f;
        return x - (x * x * x) / 6.0f;
    }

    static float processDiode(float x)
    {
        if (x > 0.0f)
            return 1.0f - std::exp(-x * 2.0f);
        else
            return -0.5f * (1.0f - std::exp(x * 1.5f));
    }

    static float processFoldback(float x)
    {
        // Triangular foldback
        float v = x * 0.5f;
        while (v > 1.0f || v < -1.0f)
        {
            if (v > 1.0f) v = 2.0f - v;
            if (v < -1.0f) v = -2.0f - v;
        }
        return v * 1.5f;
    }

    float driveGain = 1.0f;
    float toneCutoffHz = 8000.0f;
    int distMode = 0;
    float biasAmount = 0.0f;
    float wetMix = 0.5f;

    float filterAlpha = 0.5f;
    float filterStateL = 0.0f;
    float filterStateR = 0.0f;

    float dcPrevInL = 0.0f; float dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f; float dcPrevOutR = 0.0f;
};
