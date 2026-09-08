#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class BitcrushEffect : public BaseEffect
{
public:
    BitcrushEffect() = default;

    EffectType getType() const override { return EffectType::Bitcrush; }
    juce::String getName() const override { return "Bitcrusher & Decimator"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        samplePhase = 0.0f;
        heldL = 0.0f;
        heldR = 0.0f;
        filterStateL = 0.0f;
        filterStateR = 0.0f;
        preFiltL = 0.0f;
        preFiltR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Bit Depth (2 to 16 bits)
        bitDepth = 2.0f + std::clamp(p1, 0.0f, 1.0f) * 14.0f;

        // P2: Downsample Rate (48kHz down to 800Hz)
        targetRate = 800.0f + std::clamp(p2, 0.0f, 1.0f) * (static_cast<float>(currentSampleRate) - 800.0f);

        // P3: Drive / Jitter
        drive = 1.0f + std::clamp(p3, 0.0f, 1.0f) * 4.0f;

        // P4: Tone Lowpass Filter Cutoff (500Hz to 20kHz)
        const double cutoffHz = 500.0 * std::pow(40.0, std::clamp(p4, 0.0f, 1.0f));
        filterCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P5: Anti-Alias / Noise Blend
        antiAlias = std::clamp(p5, 0.0f, 1.0f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        sampleHoldInterval = static_cast<float>(currentSampleRate / targetRate);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        const float numLevels = std::pow(2.0f, bitDepth);
        const float invLevels = 1.0f / numLevels;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            // Pre-filtering for anti-aliasing control
            preFiltL += (0.1f + (1.0f - antiAlias) * 0.8f) * (dryL - preFiltL);
            preFiltR += (0.1f + (1.0f - antiAlias) * 0.8f) * (dryR - preFiltR);

            const float inL = (antiAlias > 0.05f) ? preFiltL : dryL;
            const float inR = (antiAlias > 0.05f) ? preFiltR : dryR;

            samplePhase += 1.0f;
            if (samplePhase >= sampleHoldInterval)
            {
                samplePhase -= sampleHoldInterval;

                // Quantization
                const float drivenL = inL * drive;
                const float drivenR = inR * drive;

                heldL = std::clamp(std::round(drivenL * numLevels) * invLevels, -1.0f, 1.0f);
                heldR = std::clamp(std::round(drivenR * numLevels) * invLevels, -1.0f, 1.0f);
            }

            // Post low-pass filtering
            filterStateL += filterCoeff * (heldL - filterStateL);
            filterStateR += filterCoeff * (heldR - filterStateR);

            bufferL[i] = dryL * (1.0f - wetMix) + filterStateL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + filterStateR * wetMix;
        }
    }

private:
    float bitDepth = 16.0f;
    float targetRate = 48000.0f;
    float drive = 1.0f;
    float filterCoeff = 0.9f;
    float antiAlias = 0.0f;
    float wetMix = 1.0f;

    float samplePhase = 0.0f;
    float sampleHoldInterval = 1.0f;
    float heldL = 0.0f;
    float heldR = 0.0f;
    float filterStateL = 0.0f;
    float filterStateR = 0.0f;
    float preFiltL = 0.0f;
    float preFiltR = 0.0f;
};
