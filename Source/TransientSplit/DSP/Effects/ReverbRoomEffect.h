#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

class ReverbRoomEffect : public BaseEffect
{
public:
    ReverbRoomEffect() = default;

    EffectType getType() const override { return EffectType::ReverbRoom; }
    juce::String getName() const override { return "Room Reverb"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        const double scale = currentSampleRate / 44100.0;

        // 12-Tap Early Reflection delay line (~50ms max)
        erMaxSamples = static_cast<int>(currentSampleRate * 0.08) + 32;
        erBufferL.assign(erMaxSamples, 0.0f);
        erBufferR.assign(erMaxSamples, 0.0f);
        erWritePos = 0;

        // 4 Fast-Diffusing Room Tank loops (compact acoustic room delays)
        const int roomDelays[4] = { 647, 883, 1153, 1429 };
        const int roomDiffs[4]  = { 179, 263, 359, 449 };

        for (int i = 0; i < 4; ++i)
        {
            tankDel[i].assign(static_cast<int>(roomDelays[i] * scale) + 32, 0.0f);
            tankDiff[i].assign(static_cast<int>(roomDiffs[i] * scale) + 32, 0.0f);
            idxDel[i] = 0;
            idxDiff[i] = 0;
            lpFilt[i] = 0.0f;
        }

        reset();
    }

    void reset() override
    {
        std::fill(erBufferL.begin(), erBufferL.end(), 0.0f);
        std::fill(erBufferR.begin(), erBufferR.end(), 0.0f);
        erWritePos = 0;

        for (int i = 0; i < 4; ++i)
        {
            std::fill(tankDel[i].begin(), tankDel[i].end(), 0.0f);
            std::fill(tankDiff[i].begin(), tankDiff[i].end(), 0.0f);
            idxDel[i] = 0;
            idxDiff[i] = 0;
            lpFilt[i] = 0.0f;
        }
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Decay Time (0.1s to 3.5s -> loop gain 0.15 to 0.82)
        decayGain = 0.15f + std::clamp(p1, 0.0f, 1.0f) * 0.67f;

        // P2: Room Size (Small booth 0.4 to Studio live room 1.4)
        roomScale = 0.4f + std::clamp(p2, 0.0f, 1.0f) * 1.0f;

        // P3: Wall Absorption (1000Hz to 12000Hz damping)
        const double cutoffHz = 1000.0 * std::pow(12.0, 1.0f - std::clamp(p3, 0.0f, 1.0f));
        dampCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        // P4: Early Reflection Level (0.0 to 1.0)
        erLevel = std::clamp(p4, 0.0f, 1.0f);

        // P5: Stereo Width (0% to 150%)
        stereoWidth = std::clamp(p5, 0.0f, 1.0f) * 1.5f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0 || erMaxSamples <= 0) return;

        // Physical Early Reflection Tap Delays (in ms) and gains for Room
        const float erTapsMsL[6] = { 4.3f, 9.7f, 16.4f, 23.1f, 31.8f, 41.2f };
        const float erGainsL[6]  = { 0.75f, -0.62f, 0.51f, -0.42f, 0.35f, -0.28f };

        const float erTapsMsR[6] = { 5.6f, 11.2f, 18.1f, 25.4f, 34.2f, 43.7f };
        const float erGainsR[6]  = { -0.72f, 0.59f, -0.48f, 0.39f, -0.32f, 0.25f };

        int erTapSamplesL[6], erTapSamplesR[6];
        for (int t = 0; t < 6; ++t)
        {
            erTapSamplesL[t] = std::clamp(static_cast<int>(erTapsMsL[t] * roomScale * 0.001 * currentSampleRate), 1, erMaxSamples - 1);
            erTapSamplesR[t] = std::clamp(static_cast<int>(erTapsMsR[t] * roomScale * 0.001 * currentSampleRate), 1, erMaxSamples - 1);
        }

        for (int s = 0; s < numSamples; ++s)
        {
            const float dryL = bufferL[s];
            const float dryR = bufferR ? bufferR[s] : dryL;

            // Write into early reflection line
            erBufferL[erWritePos] = dryL;
            erBufferR[erWritePos] = dryR;

            // Compute discrete early reflections
            float erOutL = 0.0f;
            float erOutR = 0.0f;
            for (int t = 0; t < 6; ++t)
            {
                int rPosL = erWritePos - erTapSamplesL[t];
                if (rPosL < 0) rPosL += erMaxSamples;
                erOutL += erBufferL[rPosL] * erGainsL[t];

                int rPosR = erWritePos - erTapSamplesR[t];
                if (rPosR < 0) rPosR += erMaxSamples;
                erOutR += erBufferR[rPosR] * erGainsR[t];
            }

            erWritePos = (erWritePos + 1) % erMaxSamples;

            // Feed early reflections into diffuse room tank
            float tankInL = dryL * 0.5f + erOutL * 0.5f;
            float tankInR = dryR * 0.5f + erOutR * 0.5f;

            // 4-Branch Compact Room Tank
            float d1 = processAllpass(tankDiff[0], idxDiff[0], tankInL + lpFilt[3] * decayGain, 0.6f);
            float out1 = tankDel[0][idxDel[0]];
            tankDel[0][idxDel[0]] = d1;
            idxDel[0] = (idxDel[0] + 1) % static_cast<int>(tankDel[0].size());
            lpFilt[0] += dampCoeff * (out1 - lpFilt[0]);

            float d2 = processAllpass(tankDiff[1], idxDiff[1], tankInR + lpFilt[0] * decayGain, 0.6f);
            float out2 = tankDel[1][idxDel[1]];
            tankDel[1][idxDel[1]] = d2;
            idxDel[1] = (idxDel[1] + 1) % static_cast<int>(tankDel[1].size());
            lpFilt[1] += dampCoeff * (out2 - lpFilt[1]);

            float d3 = processAllpass(tankDiff[2], idxDiff[2], tankInL + lpFilt[1] * decayGain, 0.6f);
            float out3 = tankDel[2][idxDel[2]];
            tankDel[2][idxDel[2]] = d3;
            idxDel[2] = (idxDel[2] + 1) % static_cast<int>(tankDel[2].size());
            lpFilt[2] += dampCoeff * (out3 - lpFilt[2]);

            float d4 = processAllpass(tankDiff[3], idxDiff[3], tankInR + lpFilt[2] * decayGain, 0.6f);
            float out4 = tankDel[3][idxDel[3]];
            tankDel[3][idxDel[3]] = d4;
            idxDel[3] = (idxDel[3] + 1) % static_cast<int>(tankDel[3].size());
            lpFilt[3] += dampCoeff * (out4 - lpFilt[3]);

            const float lateL = (out1 - out3) * 0.7f;
            const float lateR = (out2 - out4) * 0.7f;

            // Sum discrete Early Reflections + Diffuse Room Body
            const float wetL = erOutL * erLevel * 0.6f + lateL * (1.0f - erLevel * 0.3f);
            const float wetR = erOutR * erLevel * 0.6f + lateR * (1.0f - erLevel * 0.3f);

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
    float decayGain = 0.5f;
    float roomScale = 1.0f;
    float dampCoeff = 0.6f;
    float erLevel = 0.7f;
    float stereoWidth = 1.0f;
    float wetMix = 0.5f;

    std::vector<float> erBufferL, erBufferR;
    int erMaxSamples = 3840;
    int erWritePos = 0;

    std::array<std::vector<float>, 4> tankDel, tankDiff;
    std::array<int, 4> idxDel { 0 }, idxDiff { 0 };
    std::array<float, 4> lpFilt { 0.0f };
};
