#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

class ReverbPlateEffect : public BaseEffect
{
public:
    ReverbPlateEffect() = default;

    EffectType getType() const override { return EffectType::ReverbPlate; }
    juce::String getName() const override { return "Plate Reverb"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        const double scale = currentSampleRate / 44100.0;

        // 4 Fast Steel Plate Input Diffusers (EMT-140 Instant Dense Diffusion)
        const int inDiffLens[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i)
        {
            inDiff[i].assign(static_cast<int>(inDiffLens[i] * scale) + 16, 0.0f);
            inDiffIdx[i] = 0;
        }

        // Dual Asymmetric Steel Plate Delay & Diffusion Loops (Dattorro EMT-140 Topology)
        tankDelL.assign(static_cast<int>(4453 * scale) + 32, 0.0f);
        tankDelR.assign(static_cast<int>(3720 * scale) + 32, 0.0f);
        tankDiff1L.assign(static_cast<int>(908 * scale) + 32, 0.0f);
        tankDiff1R.assign(static_cast<int>(672 * scale) + 32, 0.0f);
        tankDiff2L.assign(static_cast<int>(1800 * scale) + 32, 0.0f);
        tankDiff2R.assign(static_cast<int>(2656 * scale) + 32, 0.0f);

        preDelayMaxSamples = static_cast<int>(currentSampleRate * 0.15) + 32;
        preDelayL.assign(preDelayMaxSamples, 0.0f);
        preDelayR.assign(preDelayMaxSamples, 0.0f);

        reset();
    }

    void reset() override
    {
        for (int i = 0; i < 4; ++i)
        {
            std::fill(inDiff[i].begin(), inDiff[i].end(), 0.0f);
            inDiffIdx[i] = 0;
        }

        std::fill(tankDelL.begin(), tankDelL.end(), 0.0f);
        std::fill(tankDelR.begin(), tankDelR.end(), 0.0f);
        std::fill(tankDiff1L.begin(), tankDiff1L.end(), 0.0f);
        std::fill(tankDiff1R.begin(), tankDiff1R.end(), 0.0f);
        std::fill(tankDiff2L.begin(), tankDiff2L.end(), 0.0f);
        std::fill(tankDiff2R.begin(), tankDiff2R.end(), 0.0f);

        std::fill(preDelayL.begin(), preDelayL.end(), 0.0f);
        std::fill(preDelayR.begin(), preDelayR.end(), 0.0f);

        idxDelL = 0; idxDelR = 0;
        idxDiff1L = 0; idxDiff1R = 0;
        idxDiff2L = 0; idxDiff2R = 0;
        preDelayWrite = 0;

        lpL = 0.0f; lpR = 0.0f;
        hpStateL = 0.0f; hpStateR = 0.0f;
        lastInL = 0.0f; lastInR = 0.0f;
        modPhase = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Decay Time (0.3s to 20.0s -> loop gain 0.30 to 0.96)
        decayGain = 0.30f + std::clamp(p1, 0.0f, 1.0f) * 0.66f;

        // P2: Plate Density / Tension (0.5 to 0.78 allpass gain)
        diffGain = 0.50f + std::clamp(p2, 0.0f, 1.0f) * 0.28f;

        // P3: Plate Sparkle / Brightness (2500Hz to 18000Hz steel shimmer filter)
        const double cutoffHz = 2500.0 * std::pow(7.2, std::clamp(p3, 0.0f, 1.0f));
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P4: Bass Cut Filter (High-pass filter from 60Hz to 500Hz to prevent plate low-end mud)
        const double hpCutoffHz = 60.0 + std::clamp(p4, 0.0f, 1.0f) * 440.0;
        hpCoeff = static_cast<float>(std::exp(-6.2831853 * hpCutoffHz / currentSampleRate));

        // P5: Stereo Width (0% to 150%)
        stereoWidth = std::clamp(p5, 0.0f, 1.0f) * 1.5f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        const float modInc = static_cast<float>(6.2831853 * 1.25 / currentSampleRate);

        for (int s = 0; s < numSamples; ++s)
        {
            const float dryL = bufferL[s];
            const float dryR = bufferR ? bufferR[s] : dryL;

            // Plate High-Pass filter on input (EMT-140 standard bass cut)
            hpStateL = hpCoeff * hpStateL + hpCoeff * (dryL - lastInL);
            lastInL = dryL;
            hpStateR = hpCoeff * hpStateR + hpCoeff * (dryR - lastInR);
            lastInR = dryR;

            float inMono = (hpStateL + hpStateR) * 0.5f;

            // 4-Stage High-Gain Input Allpass Diffuser (Instant dense reflection onset)
            inMono = processAllpass(inDiff[0], inDiffIdx[0], inMono, 0.75f);
            inMono = processAllpass(inDiff[1], inDiffIdx[1], inMono, 0.75f);
            inMono = processAllpass(inDiff[2], inDiffIdx[2], inMono, 0.625f);
            inMono = processAllpass(inDiff[3], inDiffIdx[3], inMono, 0.625f);

            // Modulated Plate flexural dispersion
            modPhase += modInc;
            if (modPhase >= 6.2831853f) modPhase -= 6.2831853f;
            const float modL = std::sin(modPhase) * 6.0f;
            const float modR = std::cos(modPhase) * 6.0f;

            // Left Tank Branch (Dattorro EMT-140 Topology)
            float tInL = inMono + lpR * decayGain;
            tInL = processAllpassMod(tankDiff1L, idxDiff1L, tInL, -diffGain, modL);
            float dOutL = tankDelL[idxDelL];
            tankDelL[idxDelL] = tInL;
            idxDelL = (idxDelL + 1) % static_cast<int>(tankDelL.size());
            lpL += dampCoeff * (dOutL - lpL);
            lpL = processAllpass(tankDiff2L, idxDiff2L, lpL, diffGain);

            // Right Tank Branch
            float tInR = inMono + lpL * decayGain;
            tInR = processAllpassMod(tankDiff1R, idxDiff1R, tInR, -diffGain, modR);
            float dOutR = tankDelR[idxDelR];
            tankDelR[idxDelR] = tInR;
            idxDelR = (idxDelR + 1) % static_cast<int>(tankDelR.size());
            lpR += dampCoeff * (dOutR - lpR);
            lpR = processAllpass(tankDiff2R, idxDiff2R, lpR, diffGain);

            // Dual Asymmetric Steel Plate Pickup Taps (Crisp, bright, glistening plate sparkle)
            const float wetL = (lpL * 0.6f + dOutR * 0.5f - dOutL * 0.3f);
            const float wetR = (lpR * 0.6f + dOutL * 0.5f - dOutR * 0.3f);

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

    float processAllpassMod(std::vector<float>& buffer, int& index, float input, float coeff, float modOffset)
    {
        const int size = static_cast<int>(buffer.size());
        float readPos = static_cast<float>(index) - (12.0f + modOffset);
        while (readPos < 0.0f) readPos += static_cast<float>(size);
        while (readPos >= static_cast<float>(size)) readPos -= static_cast<float>(size);

        const int r0 = static_cast<int>(readPos);
        const int r1 = (r0 + 1) % size;
        const float frac = readPos - static_cast<float>(r0);
        const float bufOut = buffer[r0] + frac * (buffer[r1] - buffer[r0]);

        const float output = -input * coeff + bufOut;
        buffer[index] = input + (output * coeff);
        index = (index + 1) % size;
        return output;
    }

    double currentSampleRate = 48000.0;
    float decayGain = 0.7f;
    float diffGain = 0.65f;
    float dampCoeff = 0.8f;
    float hpCoeff = 0.98f;
    float stereoWidth = 1.0f;
    float wetMix = 0.5f;

    std::array<std::vector<float>, 4> inDiff;
    std::array<int, 4> inDiffIdx { 0 };

    std::vector<float> tankDelL, tankDelR;
    std::vector<float> tankDiff1L, tankDiff1R, tankDiff2L, tankDiff2R;
    int idxDelL = 0, idxDelR = 0;
    int idxDiff1L = 0, idxDiff1R = 0;
    int idxDiff2L = 0, idxDiff2R = 0;

    float lpL = 0.0f, lpR = 0.0f;
    float hpStateL = 0.0f, hpStateR = 0.0f;
    float lastInL = 0.0f, lastInR = 0.0f;
    float modPhase = 0.0f;

    std::vector<float> preDelayL, preDelayR;
    int preDelayMaxSamples = 7200;
    int preDelayWrite = 0;
};
