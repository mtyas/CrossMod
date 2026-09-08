#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

class ReverbSpringEffect : public BaseEffect
{
public:
    ReverbSpringEffect() = default;

    EffectType getType() const override { return EffectType::ReverbSpring; }
    juce::String getName() const override { return "Spring Reverb"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        const double scale = currentSampleRate / 44100.0;

        // 6 Allpass Dispersion Stages for characteristic spring chirp
        const int allpassLens[6] = { 157, 269, 383, 521, 677, 853 };
        for (int i = 0; i < 6; ++i)
        {
            dispersionL[i].assign(static_cast<int>(allpassLens[i] * scale) + 8, 0.0f);
            dispersionR[i].assign(static_cast<int>((allpassLens[i] + 23) * scale) + 8, 0.0f);
            dispIdxL[i] = 0;
            dispIdxR[i] = 0;
        }

        // Dual Spring Tanks
        tank1.assign(static_cast<int>(4219 * scale) + 64, 0.0f);
        tank2.assign(static_cast<int>(5179 * scale) + 64, 0.0f);

        reset();
    }

    void reset() override
    {
        for (int i = 0; i < 6; ++i)
        {
            std::fill(dispersionL[i].begin(), dispersionL[i].end(), 0.0f);
            std::fill(dispersionR[i].begin(), dispersionR[i].end(), 0.0f);
            dispIdxL[i] = 0;
            dispIdxR[i] = 0;
        }

        std::fill(tank1.begin(), tank1.end(), 0.0f);
        std::fill(tank2.begin(), tank2.end(), 0.0f);
        idxTank1 = 0; idxTank2 = 0;
        lp1 = 0.0f; lp2 = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Decay Time (0.3s to 15.0s -> loop gain 0.35 to 0.95)
        decay = std::clamp(p1, 0.0f, 1.0f);
        decayGain = 0.35f + decay * 0.60f;

        // P2: Chirp Dispersion (Allpass coefficient 0.4 to 0.88)
        chirpCoeff = 0.4f + std::clamp(p2, 0.0f, 1.0f) * 0.48f;

        // P3: Tension Resonance (Loop filter resonance peak)
        tensionReso = std::clamp(p3, 0.0f, 1.0f);

        // P4: Damping / Vintage Tone (600Hz to 12000Hz)
        const double cutoffHz = 600.0 * std::pow(20.0, std::clamp(p4, 0.0f, 1.0f));
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P5: Stereo Width (0% to 150%)
        stereoWidth = std::clamp(p5, 0.0f, 1.0f) * 1.5f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int s = 0; s < numSamples; ++s)
        {
            const float dryL = bufferL[s];
            const float dryR = bufferR ? bufferR[s] : dryL;

            // 6-stage Allpass Dispersion (Physical spring dispersion creates frequency-dependent delay)
            float chirpedL = dryL;
            float chirpedR = dryR;
            for (int i = 0; i < 6; ++i)
            {
                chirpedL = processAllpass(dispersionL[i], dispIdxL[i], chirpedL, chirpCoeff);
                chirpedR = processAllpass(dispersionR[i], dispIdxR[i], chirpedR, chirpCoeff);
            }

            // Dual Cross-Coupled Spring Delay Tanks
            float inT1 = chirpedL + lp2 * decayGain;
            float outT1 = tank1[idxTank1];
            tank1[idxTank1] = inT1;
            idxTank1 = (idxTank1 + 1) % static_cast<int>(tank1.size());

            // Tone damping with vintage tension resonance
            const float resoT1 = outT1 + std::tanh(outT1 * 2.0f) * (tensionReso * 0.15f);
            lp1 += dampCoeff * (resoT1 - lp1);

            float inT2 = chirpedR + lp1 * decayGain;
            float outT2 = tank2[idxTank2];
            tank2[idxTank2] = inT2;
            idxTank2 = (idxTank2 + 1) % static_cast<int>(tank2.size());

            const float resoT2 = outT2 + std::tanh(outT2 * 2.0f) * (tensionReso * 0.15f);
            lp2 += dampCoeff * (resoT2 - lp2);

            const float wetL = (outT1 - outT2 * 0.5f + chirpedL * 0.3f) * 0.9f;
            const float wetR = (outT2 - outT1 * 0.5f + chirpedR * 0.3f) * 0.9f;

            const float mid = 0.5f * (wetL + wetR);
            const float sideL = wetL - mid;
            const float sideR = wetR - mid;

            const float finalWetL = mid + sideL * stereoWidth;
            const float finalWetR = mid + sideR * stereoWidth;

            bufferL[s] = dryL * (1.0f - wetMix) + finalWetL * wetMix;
            if (bufferR)
                bufferR[s] = dryR * (1.0f - wetMix) + finalWetR * wetMix;
        }
    }

private:
    float processAllpass(std::vector<float>& buffer, int& index, float input, float coeff)
    {
        const float bufOut = buffer[index];
        const float output = -input * coeff + bufOut;
        buffer[index] = input + (output * coeff);
        index = (index + 1) % static_cast<int>(buffer.size());
        return output;
    }

    double currentSampleRate = 48000.0;
    float decay = 0.5f;
    float decayGain = 0.65f;
    float chirpCoeff = 0.7f;
    float tensionReso = 0.3f;
    float dampCoeff = 0.6f;
    float stereoWidth = 1.0f;
    float wetMix = 0.5f;

    std::array<std::vector<float>, 6> dispersionL, dispersionR;
    std::array<int, 6> dispIdxL { 0 }, dispIdxR { 0 };

    std::vector<float> tank1, tank2;
    int idxTank1 = 0, idxTank2 = 0;
    float lp1 = 0.0f, lp2 = 0.0f;
};
