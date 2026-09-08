#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

class GlitchStutterEffect : public BaseEffect
{
public:
    GlitchStutterEffect() = default;

    EffectType getType() const override { return EffectType::GlitchStutter; }
    juce::String getName() const override { return "Stutter Repeater"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxBufferSize = static_cast<int>(currentSampleRate * 2.0);
        bufferMemL.assign(maxBufferSize, 0.0f);
        bufferMemR.assign(maxBufferSize, 0.0f);
        reset();
    }

    void reset() override
    {
        std::fill(bufferMemL.begin(), bufferMemL.end(), 0.0f);
        std::fill(bufferMemR.begin(), bufferMemR.end(), 0.0f);
        writePos = 0;
        readPos = 0.0f;
        stutterActive = false;
        repeatsLeft = 0;
        feedbackStoreL = 0.0f;
        feedbackStoreR = 0.0f;
    }

    void setBpm(double bpm) override
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free/P1, >0 = Synced subdivisions 1/32..1/2)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        double sliceSec = 0.0;
        if (syncSec > 0.0)
        {
            sliceSec = syncSec;
        }
        else
        {
            const int divIdx = std::clamp(static_cast<int>(std::floor(p1 * 4.99f)), 0, 4);
            double beats = 0.25;
            switch (divIdx)
            {
                case 0: beats = 0.125; break; // 1/32
                case 1: beats = 0.25;  break; // 1/16
                case 2: beats = 0.5;   break; // 1/8
                case 3: beats = 1.0;   break; // 1/4
                case 4: beats = 2.0;   break; // 1/2
            }
            sliceSec = (60.0 / currentBpm) * beats;
        }
        sliceSamples = std::max(64, static_cast<int>(sliceSec * currentSampleRate));

        // P2: Trigger Probability (10% to 100%)
        triggerProb = 0.1f + std::clamp(p2, 0.0f, 1.0f) * 0.9f;

        // P3: Burst Repeats (1 to 16 repeats)
        maxRepeats = 1 + static_cast<int>(std::clamp(p3, 0.0f, 1.0f) * 15.0f);

        // P4: Feedback Decay (0% to 90%)
        decayFeedback = std::clamp(p4, 0.0f, 0.9f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0 || maxBufferSize <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            bufferMemL[writePos] = dryL;
            bufferMemR[writePos] = dryR;

            if (++sampleTimer >= sliceSamples)
            {
                sampleTimer = 0;
                const float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
                if (r < triggerProb && repeatsLeft <= 0)
                {
                    stutterActive = true;
                    repeatsLeft = maxRepeats;
                    sliceStartPos = writePos - sliceSamples;
                    if (sliceStartPos < 0) sliceStartPos += maxBufferSize;
                    readPos = static_cast<float>(sliceStartPos);
                }
            }

            float wetL = dryL;
            float wetR = dryR;

            if (stutterActive && repeatsLeft > 0)
            {
                const float speed = 1.0f - pitchDiveAmount * 0.5f * (1.0f - static_cast<float>(repeatsLeft) / static_cast<float>(maxRepeats + 1));
                readPos += speed;
                if (readPos >= static_cast<float>(sliceStartPos + sliceSamples))
                {
                    readPos = static_cast<float>(sliceStartPos);
                    repeatsLeft--;
                    if (repeatsLeft <= 0)
                        stutterActive = false;
                }

                while (readPos < 0.0f) readPos += static_cast<float>(maxBufferSize);
                while (readPos >= static_cast<float>(maxBufferSize)) readPos -= static_cast<float>(maxBufferSize);

                const int r0 = static_cast<int>(readPos);
                const int r1 = (r0 + 1) % maxBufferSize;
                const float frac = readPos - static_cast<float>(r0);

                const float decayGain = std::pow(decayFeedback, static_cast<float>(maxRepeats - repeatsLeft));
                wetL = (bufferMemL[r0] + frac * (bufferMemL[r1] - bufferMemL[r0])) * decayGain;
                wetR = (bufferMemR[r0] + frac * (bufferMemR[r1] - bufferMemR[r0])) * decayGain;
            }

            writePos = (writePos + 1) % maxBufferSize;

            bufferL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;
    double currentBpm = 120.0;

    int maxBufferSize = 96000;
    std::vector<float> bufferMemL;
    std::vector<float> bufferMemR;

    int writePos = 0;
    float readPos = 0.0f;
    int sliceStartPos = 0;
    int sliceSamples = 6000;
    int sampleTimer = 0;

    float triggerProb = 0.6f;
    int maxRepeats = 4;
    int repeatsLeft = 0;
    float decayFeedback = 0.8f;
    float pitchDiveAmount = 0.0f;
    float wetMix = 1.0f;
    bool stutterActive = false;

    float feedbackStoreL = 0.0f;
    float feedbackStoreR = 0.0f;
};
