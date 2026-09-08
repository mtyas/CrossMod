#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

class ReverbShimmerEffect : public BaseEffect
{
public:
    ReverbShimmerEffect() = default;

    EffectType getType() const override { return EffectType::ReverbShimmer; }
    juce::String getName() const override { return "Shimmer Reverb"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        const double scale = currentSampleRate / 44100.0;

        // Dattorro input diffusers
        const int inDiffLens[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i)
        {
            inDiffL[i].assign(static_cast<int>(inDiffLens[i] * scale) + 4, 0.0f);
            inDiffR[i].assign(static_cast<int>(inDiffLens[i] * scale) + 4, 0.0f);
            inDiffIdxL[i] = 0;
            inDiffIdxR[i] = 0;
        }

        // Tank delays and diffusers
        tankDel1L.assign(static_cast<int>(4453 * scale) + 64, 0.0f);
        tankDel1R.assign(static_cast<int>(4217 * scale) + 64, 0.0f);
        tankDel2L.assign(static_cast<int>(3720 * scale) + 64, 0.0f);
        tankDel2R.assign(static_cast<int>(3163 * scale) + 64, 0.0f);

        tankDiff1L.assign(static_cast<int>(672 * scale) + 64, 0.0f);
        tankDiff1R.assign(static_cast<int>(908 * scale) + 64, 0.0f);
        tankDiff2L.assign(static_cast<int>(1800 * scale) + 64, 0.0f);
        tankDiff2R.assign(static_cast<int>(2656 * scale) + 64, 0.0f);

        shimmerBufferSize = static_cast<int>(currentSampleRate * 0.20) + 32; // 200ms grain buffer
        shimmerBufferL.assign(shimmerBufferSize, 0.0f);
        shimmerBufferR.assign(shimmerBufferSize, 0.0f);

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

        std::fill(tankDel1L.begin(), tankDel1L.end(), 0.0f);
        std::fill(tankDel1R.begin(), tankDel1R.end(), 0.0f);
        std::fill(tankDel2L.begin(), tankDel2L.end(), 0.0f);
        std::fill(tankDel2R.begin(), tankDel2R.end(), 0.0f);

        std::fill(tankDiff1L.begin(), tankDiff1L.end(), 0.0f);
        std::fill(tankDiff1R.begin(), tankDiff1R.end(), 0.0f);
        std::fill(tankDiff2L.begin(), tankDiff2L.end(), 0.0f);
        std::fill(tankDiff2R.begin(), tankDiff2R.end(), 0.0f);

        std::fill(shimmerBufferL.begin(), shimmerBufferL.end(), 0.0f);
        std::fill(shimmerBufferR.begin(), shimmerBufferR.end(), 0.0f);

        idxDel1L = 0; idxDel1R = 0;
        idxDel2L = 0; idxDel2R = 0;
        idxDiff1L = 0; idxDiff1R = 0;
        idxDiff2L = 0; idxDiff2R = 0;
        shimmerWrite = 0;

        tankLpL = 0.0f; tankLpR = 0.0f;
        pitchPhase = 0.0f;
        modPhase = 0.0f;
        lastWetL = 0.0f; lastWetR = 0.0f;
        shimmerToneStateL = 0.0f; shimmerToneStateR = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Decay Time (0.3 to 0.95 -> mapped to safe tank gain 0.35 to 0.78)
        decay = std::clamp(p1, 0.0f, 1.0f);
        decayGain = 0.35f + decay * 0.43f;

        // P2: Pitch Shift (-24.0 to +24.0 semitones)
        pitchSemitones = -24.0f + std::clamp(p2, 0.0f, 1.0f) * 48.0f;
        const float pitchRatio = std::pow(2.0f, pitchSemitones / 12.0f);

        const float windowSize = static_cast<float>(currentSampleRate * 0.060); // 60ms grain window
        pitchRate = (1.0f - pitchRatio) / std::max(10.0f, windowSize);

        // P3: Shimmer Mix (0% to 100%)
        shimmerMix = std::clamp(p3, 0.0f, 1.0f);

        // P4: Shimmer Tone (500Hz to 16000Hz)
        const double cutoffHz = 500.0 * std::pow(32.0, std::clamp(p4, 0.0f, 1.0f));
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P5: Stereo Width (0% to 100%)
        stereoWidth = std::clamp(p5, 0.0f, 1.0f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0 || shimmerBufferSize <= 0) return;

        const float windowSize = static_cast<float>(currentSampleRate * 0.060);
        const float modInc = static_cast<float>(6.2831853 * 0.7 / currentSampleRate); // 0.7 Hz slow tank modulation

        for (int s = 0; s < numSamples; ++s)
        {
            const float dryL = bufferL[s];
            const float dryR = bufferR ? bufferR[s] : dryL;

            // 1. Shimmer Pitch Shift Loop (fed from previous lush diffuse tank output)
            const float satLastL = std::tanh(lastWetL * 0.4f);
            const float satLastR = std::tanh(lastWetR * 0.4f);

            shimmerBufferL[shimmerWrite] = satLastL;
            shimmerBufferR[shimmerWrite] = satLastR;

            pitchPhase += pitchRate;
            while (pitchPhase >= 1.0f) pitchPhase -= 1.0f;
            while (pitchPhase < 0.0f) pitchPhase += 1.0f;

            float ph2 = pitchPhase + 0.5f;
            if (ph2 >= 1.0f) ph2 -= 1.0f;

            auto readGrain = [this](const std::vector<float>& buf, float d) -> float
            {
                float rPos = static_cast<float>(shimmerWrite) - d;
                while (rPos < 0.0f) rPos += static_cast<float>(shimmerBufferSize);
                while (rPos >= static_cast<float>(shimmerBufferSize)) rPos -= static_cast<float>(shimmerBufferSize);
                int r0 = static_cast<int>(rPos);
                int r1 = (r0 + 1) % shimmerBufferSize;
                float frac = rPos - static_cast<float>(r0);
                return buf[r0] + frac * (buf[r1] - buf[r0]);
            };

            const float w1 = 0.5f * (1.0f - std::cos(pitchPhase * 6.2831853f));
            const float w2 = 0.5f * (1.0f - std::cos(ph2 * 6.2831853f));

            const float shiftedL = readGrain(shimmerBufferL, pitchPhase * windowSize) * w1 + readGrain(shimmerBufferL, ph2 * windowSize) * w2;
            const float shiftedR = readGrain(shimmerBufferR, pitchPhase * windowSize) * w1 + readGrain(shimmerBufferR, ph2 * windowSize) * w2;

            shimmerToneStateL += dampCoeff * (shiftedL - shimmerToneStateL);
            shimmerToneStateR += dampCoeff * (shiftedR - shimmerToneStateR);

            shimmerWrite = (shimmerWrite + 1) % shimmerBufferSize;

            // Injected shimmer signal into input
            float inL = dryL + shimmerToneStateL * (shimmerMix * 0.35f);
            float inR = dryR + shimmerToneStateR * (shimmerMix * 0.35f);

            // 2. Dattorro 4-stage Input Diffusers
            const float inCoeff = 0.75f;
            for (int i = 0; i < 4; ++i)
            {
                inL = processAllpass(inDiffL[i], inDiffIdxL[i], inL, inCoeff);
                inR = processAllpass(inDiffR[i], inDiffIdxR[i], inR, inCoeff);
            }

            // 3. Modulated Tank Loops (Left & Right Cross-Coupled)
            modPhase += modInc;
            if (modPhase >= 6.2831853f) modPhase -= 6.2831853f;
            const float modL = std::sin(modPhase) * 8.0f;
            const float modR = std::cos(modPhase * 1.3f) * 8.0f;

            // Tank Left Branch
            float tankInL = inL + tankLpR * decayGain;
            float diff1L = processAllpassMod(tankDiff1L, idxDiff1L, tankInL, 0.7f, modL);
            float del1L = readDelay(tankDel1L, idxDel1L);
            tankDel1L[idxDel1L] = diff1L;
            idxDel1L = (idxDel1L + 1) % static_cast<int>(tankDel1L.size());

            tankLpL += dampCoeff * (del1L - tankLpL);
            float diff2L = processAllpass(tankDiff2L, idxDiff2L, tankLpL, 0.5f);
            float del2L = readDelay(tankDel2L, idxDel2L);
            tankDel2L[idxDel2L] = diff2L;
            idxDel2L = (idxDel2L + 1) % static_cast<int>(tankDel2L.size());

            // Tank Right Branch
            float tankInR = inR + tankLpL * decayGain;
            float diff1R = processAllpassMod(tankDiff1R, idxDiff1R, tankInR, 0.7f, modR);
            float del1R = readDelay(tankDel1R, idxDel1R);
            tankDel1R[idxDel1R] = diff1R;
            idxDel1R = (idxDel1R + 1) % static_cast<int>(tankDel1R.size());

            tankLpR += dampCoeff * (del1R - tankLpR);
            float diff2R = processAllpass(tankDiff2R, idxDiff2R, tankLpR, 0.5f);
            float del2R = readDelay(tankDel2R, idxDel2R);
            tankDel2R[idxDel2R] = diff2R;
            idxDel2R = (idxDel2R + 1) % static_cast<int>(tankDel2R.size());

            // 4. Tap Stereo Output with Decorrelated Spacing
            const float wetOutL = (del1L - diff2R + del2L * 0.7f) * 0.7f;
            const float wetOutR = (del1R - diff2L + del2R * 0.7f) * 0.7f;

            lastWetL = wetOutL;
            lastWetR = wetOutR;

            const float mid = 0.5f * (wetOutL + wetOutR);
            const float sideL = wetOutL - mid;
            const float sideR = wetOutR - mid;

            const float finalWetL = (mid + sideL * stereoWidth) * 1.6f;
            const float finalWetR = (mid + sideR * stereoWidth) * 1.6f;

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
        float readPos = static_cast<float>(index) - (16.0f + modOffset);
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

    float readDelay(const std::vector<float>& buffer, int index)
    {
        return buffer[index];
    }

    double currentSampleRate = 48000.0;
    float decay = 0.7f;
    float decayGain = 0.65f;
    float pitchSemitones = 12.0f;
    float pitchRate = -0.0004f;
    float shimmerMix = 0.6f;
    float dampCoeff = 0.75f;
    float stereoWidth = 1.0f;
    float wetMix = 0.5f;

    std::array<std::vector<float>, 4> inDiffL, inDiffR;
    std::array<int, 4> inDiffIdxL { 0 }, inDiffIdxR { 0 };

    std::vector<float> tankDiff1L, tankDiff1R, tankDiff2L, tankDiff2R;
    std::vector<float> tankDel1L, tankDel1R, tankDel2L, tankDel2R;
    int idxDiff1L = 0, idxDiff1R = 0, idxDiff2L = 0, idxDiff2R = 0;
    int idxDel1L = 0, idxDel1R = 0, idxDel2L = 0, idxDel2R = 0;

    float tankLpL = 0.0f, tankLpR = 0.0f;
    float modPhase = 0.0f;

    std::vector<float> shimmerBufferL, shimmerBufferR;
    int shimmerBufferSize = 9600;
    int shimmerWrite = 0;
    float pitchPhase = 0.0f;
    float lastWetL = 0.0f, lastWetR = 0.0f;
    float shimmerToneStateL = 0.0f, shimmerToneStateR = 0.0f;
};
