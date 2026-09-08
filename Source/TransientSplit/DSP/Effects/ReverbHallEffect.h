#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

class ReverbHallEffect : public BaseEffect
{
public:
    ReverbHallEffect() = default;

    EffectType getType() const override { return EffectType::ReverbHall; }
    juce::String getName() const override { return "Hall Reverb"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        const double scale = currentSampleRate / 44100.0;

        // 4 Input Air Dispersion Diffusers
        const int inDiffLens[4] = { 347, 491, 769, 1061 };
        for (int i = 0; i < 4; ++i)
        {
            inDiffL[i].assign(static_cast<int>(inDiffLens[i] * scale) + 16, 0.0f);
            inDiffR[i].assign(static_cast<int>(inDiffLens[i] * scale) + 16, 0.0f);
            inDiffIdxL[i] = 0;
            inDiffIdxR[i] = 0;
        }

        // 4 Vast Cavernous Hall Tank Delays (Prime lengths 7841 to 13873 samples)
        tankDel1.assign(static_cast<int>(7841 * scale) + 64, 0.0f);
        tankDel2.assign(static_cast<int>(9643 * scale) + 64, 0.0f);
        tankDel3.assign(static_cast<int>(11597 * scale) + 64, 0.0f);
        tankDel4.assign(static_cast<int>(13873 * scale) + 64, 0.0f);

        tankDiff1.assign(static_cast<int>(2153 * scale) + 64, 0.0f);
        tankDiff2.assign(static_cast<int>(2879 * scale) + 64, 0.0f);
        tankDiff3.assign(static_cast<int>(3761 * scale) + 64, 0.0f);
        tankDiff4.assign(static_cast<int>(4831 * scale) + 64, 0.0f);

        preDelayMaxSamples = static_cast<int>(currentSampleRate * 0.40) + 32;
        preDelayL.assign(preDelayMaxSamples, 0.0f);
        preDelayR.assign(preDelayMaxSamples, 0.0f);

        reset();
    }

    void reset() override
    {
        for (int i = 0; i < 4; ++i)
        {
            std::fill(inDiffL[i].begin(), inDiffL[i].end(), 0.0f);
            std::fill(inDiffR[i].begin(), inDiffR[i].end(), 0.0f);
            inDiffIdxL[i] = 0;
            inDiffIdxR[i] = 0;
        }

        std::fill(tankDel1.begin(), tankDel1.end(), 0.0f);
        std::fill(tankDel2.begin(), tankDel2.end(), 0.0f);
        std::fill(tankDel3.begin(), tankDel3.end(), 0.0f);
        std::fill(tankDel4.begin(), tankDel4.end(), 0.0f);

        std::fill(tankDiff1.begin(), tankDiff1.end(), 0.0f);
        std::fill(tankDiff2.begin(), tankDiff2.end(), 0.0f);
        std::fill(tankDiff3.begin(), tankDiff3.end(), 0.0f);
        std::fill(tankDiff4.begin(), tankDiff4.end(), 0.0f);

        std::fill(preDelayL.begin(), preDelayL.end(), 0.0f);
        std::fill(preDelayR.begin(), preDelayR.end(), 0.0f);

        idxDel1 = 0; idxDel2 = 0; idxDel3 = 0; idxDel4 = 0;
        idxDiff1 = 0; idxDiff2 = 0; idxDiff3 = 0; idxDiff4 = 0;
        preDelayWrite = 0;

        lp1 = 0.0f; lp2 = 0.0f; lp3 = 0.0f; lp4 = 0.0f;
        lp2_1 = 0.0f; lp2_2 = 0.0f; lp2_3 = 0.0f; lp2_4 = 0.0f;
        modPhase1 = 0.0f; modPhase2 = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Decay Time (1.0s to 45.0s -> loop gain 0.50 to 0.988)
        decayGain = 0.50f + std::clamp(p1, 0.0f, 1.0f) * 0.488f;

        // P2: Hall Scale (Scale factor on volumetric injection)
        hallScale = 0.7f + std::clamp(p2, 0.0f, 1.0f) * 0.5f;

        // P3: Air Absorption Damping (350Hz to 14000Hz air absorption roll-off)
        const double cutoffHz = 350.0 * std::pow(40.0, std::clamp(p3, 0.0f, 1.0f));
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P4: Pre-Delay (0ms to 350ms)
        const double preMs = std::clamp(p4, 0.0f, 1.0f) * 350.0;
        preDelaySamples = std::clamp(static_cast<int>(preMs * 0.001 * currentSampleRate), 0, preDelayMaxSamples - 2);

        // P5: Spatial Envelopment (0% to 150%)
        stereoWidth = std::clamp(p5, 0.0f, 1.0f) * 1.5f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0 || preDelayMaxSamples <= 0) return;

        const float modInc1 = static_cast<float>(6.2831853 * 0.38 / currentSampleRate);
        const float modInc2 = static_cast<float>(6.2831853 * 0.61 / currentSampleRate);

        for (int s = 0; s < numSamples; ++s)
        {
            const float dryL = bufferL[s];
            const float dryR = bufferR ? bufferR[s] : dryL;

            // Long Pre-delay
            preDelayL[preDelayWrite] = dryL;
            preDelayR[preDelayWrite] = dryR;

            int readPos = preDelayWrite - preDelaySamples;
            while (readPos < 0) readPos += preDelayMaxSamples;
            const float preL = preDelayL[readPos];
            const float preR = preDelayR[readPos];

            preDelayWrite = (preDelayWrite + 1) % preDelayMaxSamples;

            // 4-Stage Progressive Input Air Diffusers
            float inL = preL * hallScale;
            float inR = preR * hallScale;
            const float inCoeff = 0.75f;
            for (int i = 0; i < 4; ++i)
            {
                inL = processAllpass(inDiffL[i], inDiffIdxL[i], inL, inCoeff);
                inR = processAllpass(inDiffR[i], inDiffIdxR[i], inR, inCoeff);
            }

            // Air turbulence modulation
            modPhase1 += modInc1;
            if (modPhase1 >= 6.2831853f) modPhase1 -= 6.2831853f;
            modPhase2 += modInc2;
            if (modPhase2 >= 6.2831853f) modPhase2 -= 6.2831853f;

            const float mod1 = std::sin(modPhase1) * 14.0f;
            const float mod2 = std::cos(modPhase2) * 14.0f;

            // 4 Vast Cross-Coupled Tank Loops with 2-pole Air Absorption
            // Branch 1
            float tIn1 = inL + lp2_4 * decayGain;
            float dDiff1 = processAllpassMod(tankDiff1, idxDiff1, tIn1, 0.68f, mod1);
            float dOut1 = tankDel1[idxDel1];
            tankDel1[idxDel1] = dDiff1;
            idxDel1 = (idxDel1 + 1) % static_cast<int>(tankDel1.size());
            lp1 += dampCoeff * (dOut1 - lp1);
            lp2_1 += dampCoeff * (lp1 - lp2_1); // 2-pole air absorption

            // Branch 2
            float tIn2 = inR + lp2_1 * decayGain;
            float dDiff2 = processAllpassMod(tankDiff2, idxDiff2, tIn2, 0.68f, mod2);
            float dOut2 = tankDel2[idxDel2];
            tankDel2[idxDel2] = dDiff2;
            idxDel2 = (idxDel2 + 1) % static_cast<int>(tankDel2.size());
            lp2 += dampCoeff * (dOut2 - lp2);
            lp2_2 += dampCoeff * (lp2 - lp2_2);

            // Branch 3
            float tIn3 = inL + lp2_2 * decayGain;
            float dDiff3 = processAllpassMod(tankDiff3, idxDiff3, tIn3, 0.68f, -mod1);
            float dOut3 = tankDel3[idxDel3];
            tankDel3[idxDel3] = dDiff3;
            idxDel3 = (idxDel3 + 1) % static_cast<int>(tankDel3.size());
            lp3 += dampCoeff * (dOut3 - lp3);
            lp2_3 += dampCoeff * (lp3 - lp2_3);

            // Branch 4
            float tIn4 = inR + lp2_3 * decayGain;
            float dDiff4 = processAllpassMod(tankDiff4, idxDiff4, tIn4, 0.68f, -mod2);
            float dOut4 = tankDel4[idxDel4];
            tankDel4[idxDel4] = dDiff4;
            idxDel4 = (idxDel4 + 1) % static_cast<int>(tankDel4.size());
            lp4 += dampCoeff * (dOut4 - lp4);
            lp2_4 += dampCoeff * (lp4 - lp2_4);

            // Immense 3D Hall Spatial Tap Sum
            const float wetL = (dOut1 - dOut3 + dDiff2 * 0.7f - dDiff4 * 0.5f) * 0.65f;
            const float wetR = (dOut2 - dOut4 + dDiff1 * 0.7f - dDiff3 * 0.5f) * 0.65f;

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
        float readPos = static_cast<float>(index) - (20.0f + modOffset);
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
    float decayGain = 0.85f;
    float hallScale = 1.0f;
    float dampCoeff = 0.7f;
    int preDelaySamples = 0;
    float stereoWidth = 1.0f;
    float wetMix = 0.5f;

    std::array<std::vector<float>, 4> inDiffL, inDiffR;
    std::array<int, 4> inDiffIdxL { 0 }, inDiffIdxR { 0 };

    std::vector<float> tankDel1, tankDel2, tankDel3, tankDel4;
    std::vector<float> tankDiff1, tankDiff2, tankDiff3, tankDiff4;
    int idxDel1 = 0, idxDel2 = 0, idxDel3 = 0, idxDel4 = 0;
    int idxDiff1 = 0, idxDiff2 = 0, idxDiff3 = 0, idxDiff4 = 0;

    float lp1 = 0.0f, lp2 = 0.0f, lp3 = 0.0f, lp4 = 0.0f;
    float lp2_1 = 0.0f, lp2_2 = 0.0f, lp2_3 = 0.0f, lp2_4 = 0.0f;
    float modPhase1 = 0.0f, modPhase2 = 0.0f;

    std::vector<float> preDelayL, preDelayR;
    int preDelayMaxSamples = 19200;
    int preDelayWrite = 0;
};
