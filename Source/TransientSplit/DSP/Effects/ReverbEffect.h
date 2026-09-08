#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

enum class ReverbAlgorithm
{
    Room = 0,
    Hall = 1,
    Plate = 2,
    Spring = 3,
    Shimmer = 4
};

class ReverbEffect : public BaseEffect
{
public:
    ReverbEffect() = default;

    EffectType getType() const override { return EffectType::Reverb; }
    juce::String getName() const override { return "Multi-Model Reverb"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        const double scale = currentSampleRate / 44100.0;

        const int combTunings[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        const int allpassTunings[4] = { 225, 341, 441, 556 };
        const int springChirpTunings[3] = { 142, 287, 439 };

        for (int i = 0; i < 8; ++i)
        {
            const int len = static_cast<int>(combTunings[i] * scale) + 4;
            combBuffersL[i].assign(len, 0.0f);
            combBuffersR[i].assign(len + 23, 0.0f);
            combIndicesL[i] = 0;
            combIndicesR[i] = 0;
            combFilterL[i] = 0.0f;
            combFilterR[i] = 0.0f;
        }

        for (int i = 0; i < 4; ++i)
        {
            const int len = static_cast<int>(allpassTunings[i] * scale) + 4;
            allpassBuffersL[i].assign(len, 0.0f);
            allpassBuffersR[i].assign(len + 17, 0.0f);
            allpassIndicesL[i] = 0;
            allpassIndicesR[i] = 0;
        }

        for (int i = 0; i < 3; ++i)
        {
            const int len = static_cast<int>(springChirpTunings[i] * scale) + 4;
            springBuffersL[i].assign(len, 0.0f);
            springBuffersR[i].assign(len + 11, 0.0f);
            springIndicesL[i] = 0;
            springIndicesR[i] = 0;
        }

        preDelayMaxSamples = static_cast<int>(currentSampleRate * 0.25) + 16;
        preDelayBufferL.assign(preDelayMaxSamples, 0.0f);
        preDelayBufferR.assign(preDelayMaxSamples, 0.0f);
        preDelayWrite = 0;

        shimmerBufferL.assign(4096, 0.0f);
        shimmerBufferR.assign(4096, 0.0f);

        reset();
    }

    void reset() override
    {
        for (int i = 0; i < 8; ++i)
        {
            std::fill(combBuffersL[i].begin(), combBuffersL[i].end(), 0.0f);
            std::fill(combBuffersR[i].begin(), combBuffersR[i].end(), 0.0f);
            combIndicesL[i] = 0;
            combIndicesR[i] = 0;
            combFilterL[i] = 0.0f;
            combFilterR[i] = 0.0f;
        }

        for (int i = 0; i < 4; ++i)
        {
            std::fill(allpassBuffersL[i].begin(), allpassBuffersL[i].end(), 0.0f);
            std::fill(allpassBuffersR[i].begin(), allpassBuffersR[i].end(), 0.0f);
            allpassIndicesL[i] = 0;
            allpassIndicesR[i] = 0;
        }

        for (int i = 0; i < 3; ++i)
        {
            std::fill(springBuffersL[i].begin(), springBuffersL[i].end(), 0.0f);
            std::fill(springBuffersR[i].begin(), springBuffersR[i].end(), 0.0f);
            springIndicesL[i] = 0;
            springIndicesR[i] = 0;
        }

        std::fill(preDelayBufferL.begin(), preDelayBufferL.end(), 0.0f);
        std::fill(preDelayBufferR.begin(), preDelayBufferR.end(), 0.0f);
        std::fill(shimmerBufferL.begin(), shimmerBufferL.end(), 0.0f);
        std::fill(shimmerBufferR.begin(), shimmerBufferR.end(), 0.0f);

        preDelayWrite = 0;
        shimmerWrite = 0;
        shimmerPhase = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float mix) override
    {
        // P1: Decay / Room Size (0.1 to 1.0)
        roomSize = std::clamp(p1, 0.1f, 0.98f);

        // P2: Damping / High Cut
        damp = std::clamp(p2, 0.05f, 0.9f);

        // P3: Algorithm (0: Room, 1: Hall, 2: Plate, 3: Spring, 4: Shimmer)
        const int algoIdx = std::clamp(static_cast<int>(std::floor(p3 * 4.99f)), 0, 4);
        algorithm = static_cast<ReverbAlgorithm>(algoIdx);

        // P4: Pre-delay (0ms to 150ms)
        const double preDelayMs = std::clamp(p4, 0.0f, 1.0f) * 150.0;
        preDelaySamples = static_cast<int>(preDelayMs * 0.001 * currentSampleRate);

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        // Model adjustments
        switch (algorithm)
        {
            case ReverbAlgorithm::Room:
                feedbackMultiplier = roomSize * 0.75f;
                dampMultiplier = damp * 0.8f;
                break;
            case ReverbAlgorithm::Hall:
                feedbackMultiplier = roomSize * 0.92f;
                dampMultiplier = damp * 0.4f;
                break;
            case ReverbAlgorithm::Plate:
                feedbackMultiplier = roomSize * 0.88f;
                dampMultiplier = damp * 0.2f; // Bright metallic plate
                break;
            case ReverbAlgorithm::Spring:
                feedbackMultiplier = roomSize * 0.82f;
                dampMultiplier = damp * 0.6f;
                break;
            case ReverbAlgorithm::Shimmer:
                feedbackMultiplier = roomSize * 0.94f;
                dampMultiplier = damp * 0.3f;
                break;
        }
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int s = 0; s < numSamples; ++s)
        {
            const float dryL = bufferL[s];
            const float dryR = bufferR ? bufferR[s] : dryL;

            // Pre-delay buffer write
            preDelayBufferL[preDelayWrite] = dryL;
            preDelayBufferR[preDelayWrite] = dryR;

            int preDelayRead = preDelayWrite - preDelaySamples;
            if (preDelayRead < 0) preDelayRead += preDelayMaxSamples;

            float inputL = preDelayBufferL[preDelayRead] * 0.02f;
            float inputR = preDelayBufferR[preDelayRead] * 0.02f;

            // Optional Shimmer feedback octave pitch injection
            if (algorithm == ReverbAlgorithm::Shimmer)
            {
                shimmerBufferL[shimmerWrite] = inputL + lastWetL * 0.3f;
                shimmerBufferR[shimmerWrite] = inputR + lastWetR * 0.3f;

                shimmerPhase += 0.5f; // Double speed = +1 Octave pitch up
                if (shimmerPhase >= 2048.0f) shimmerPhase -= 2048.0f;

                const int r0 = static_cast<int>(shimmerPhase);
                inputL += shimmerBufferL[r0] * 0.35f;
                inputR += shimmerBufferR[r0] * 0.35f;

                shimmerWrite = (shimmerWrite + 1) % 4096;
            }

            // Spring dispersion allpass cascade
            if (algorithm == ReverbAlgorithm::Spring)
            {
                for (int i = 0; i < 3; ++i)
                {
                    inputL = processAllpass(springBuffersL[i], springIndicesL[i], inputL, 0.7f);
                    inputR = processAllpass(springBuffersR[i], springIndicesR[i], inputR, 0.7f);
                }
            }

            preDelayWrite = (preDelayWrite + 1) % preDelayMaxSamples;

            float outL = 0.0f;
            float outR = 0.0f;

            // 8 Parallel feedback comb filters
            for (int i = 0; i < 8; ++i)
            {
                outL += processComb(combBuffersL[i], combIndicesL[i], combFilterL[i], inputL);
                outR += processComb(combBuffersR[i], combIndicesR[i], combFilterR[i], inputR);
            }

            // 4 Series allpass diffusers
            for (int i = 0; i < 4; ++i)
            {
                outL = processAllpass(allpassBuffersL[i], allpassIndicesL[i], outL, 0.5f);
                outR = processAllpass(allpassBuffersR[i], allpassIndicesR[i], outR, 0.5f);
            }

            lastWetL = outL;
            lastWetR = outR;

            bufferL[s] = dryL * (1.0f - wetMix) + outL * wetMix * 2.5f;
            if (bufferR)
                bufferR[s] = dryR * (1.0f - wetMix) + outR * wetMix * 2.5f;
        }
    }

private:
    float processComb(std::vector<float>& buffer, int& index, float& filterStore, float input)
    {
        const float output = buffer[index];
        filterStore = (output * (1.0f - dampMultiplier)) + (filterStore * dampMultiplier);
        buffer[index] = input + (filterStore * feedbackMultiplier);

        index = (index + 1) % static_cast<int>(buffer.size());
        return output;
    }

    float processAllpass(std::vector<float>& buffer, int& index, float input, float feedback)
    {
        const float bufOut = buffer[index];
        const float output = -input + bufOut;
        buffer[index] = input + (bufOut * feedback);

        index = (index + 1) % static_cast<int>(buffer.size());
        return output;
    }

    double currentSampleRate = 48000.0;

    ReverbAlgorithm algorithm = ReverbAlgorithm::Room;
    float roomSize = 0.5f;
    float damp = 0.5f;
    float wetMix = 0.35f;

    float feedbackMultiplier = 0.75f;
    float dampMultiplier = 0.5f;

    int preDelaySamples = 0;
    int preDelayMaxSamples = 16;
    int preDelayWrite = 0;
    std::vector<float> preDelayBufferL;
    std::vector<float> preDelayBufferR;

    std::array<std::vector<float>, 8> combBuffersL;
    std::array<std::vector<float>, 8> combBuffersR;
    std::array<int, 8> combIndicesL { 0 };
    std::array<int, 8> combIndicesR { 0 };
    std::array<float, 8> combFilterL { 0.0f };
    std::array<float, 8> combFilterR { 0.0f };

    std::array<std::vector<float>, 4> allpassBuffersL;
    std::array<std::vector<float>, 4> allpassBuffersR;
    std::array<int, 4> allpassIndicesL { 0 };
    std::array<int, 4> allpassIndicesR { 0 };

    std::array<std::vector<float>, 3> springBuffersL;
    std::array<std::vector<float>, 3> springBuffersR;
    std::array<int, 3> springIndicesL { 0 };
    std::array<int, 3> springIndicesR { 0 };

    std::vector<float> shimmerBufferL;
    std::vector<float> shimmerBufferR;
    int shimmerWrite = 0;
    float shimmerPhase = 0.0f;
    float lastWetL = 0.0f;
    float lastWetR = 0.0f;
};
