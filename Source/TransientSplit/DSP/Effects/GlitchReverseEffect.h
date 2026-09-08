#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

class GlitchReverseEffect : public BaseEffect
{
public:
    GlitchReverseEffect() = default;

    EffectType getType() const override { return EffectType::GlitchReverse; }
    juce::String getName() const override { return "Reverse Slice"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxBufferSize = static_cast<int>(currentSampleRate * 2.0);
        bufferL.assign(maxBufferSize, 0.0f);
        bufferR.assign(maxBufferSize, 0.0f);
        reset();
    }

    void reset() override
    {
        std::fill(bufferL.begin(), bufferL.end(), 0.0f);
        std::fill(bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;
        readPos = 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
    }

    void setBpm(double bpm) override
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free/P1, >0 = Synced 1/16..2 Bars)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        double sliceSec = 0.0;
        if (syncSec > 0.0)
        {
            sliceSec = syncSec;
        }
        else
        {
            const int lenIdx = std::clamp(static_cast<int>(std::floor(p1 * 4.99f)), 0, 4);
            double beats = 0.25;
            switch (lenIdx)
            {
                case 0: beats = 0.25; break; // 1/16
                case 1: beats = 0.5;  break; // 1/8
                case 2: beats = 1.0;  break; // 1/4
                case 3: beats = 2.0;  break; // 1/2
                case 4: beats = 4.0;  break; // 1 Bar
            }
            sliceSec = (60.0 / currentBpm) * beats;
        }
        sliceSamples = std::max(128, static_cast<int>(sliceSec * currentSampleRate));

        // P2: Crossfade Window
        fadeFraction = 0.05f + std::clamp(p2, 0.0f, 1.0f) * 0.25f;

        // P3: Jitter Timing
        jitterAmount = std::clamp(p3, 0.0f, 1.0f);

        // P4: Feedback (0% to 70%)
        feedbackAmount = std::clamp(p4, 0.0f, 0.7f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0 || maxBufferSize <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            bufferL[writePos] = dryL + feedbackL * feedbackAmount;
            bufferR[writePos] = dryR + feedbackR * feedbackAmount;

            if (++timer >= sliceSamples)
            {
                timer = 0;
                const float jitter = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * jitterAmount * 0.3f;
                sliceStartPos = writePos;
                readPos = static_cast<float>(sliceStartPos + sliceSamples) * (1.0f + jitter);
            }

            const float step = (dirBlend < 0.5f) ? -1.0f : (dirBlend > 0.8f ? 1.0f : -1.0f);
            readPos += step;
            while (readPos < 0.0f) readPos += static_cast<float>(maxBufferSize);
            while (readPos >= static_cast<float>(maxBufferSize)) readPos -= static_cast<float>(maxBufferSize);

            const int r0 = static_cast<int>(readPos);
            const int r1 = (r0 + 1) % maxBufferSize;
            const float frac = readPos - static_cast<float>(r0);

            const float phase = static_cast<float>(timer) / static_cast<float>(sliceSamples);
            float env = 1.0f;
            if (phase < fadeFraction)
                env = phase / fadeFraction;
            else if (phase > (1.0f - fadeFraction))
                env = (1.0f - phase) / fadeFraction;

            const float wetL = (bufferL[r0] + frac * (bufferL[r1] - bufferL[r0])) * env;
            const float wetR = (bufferR[r0] + frac * (bufferR[r1] - bufferR[r0])) * env;

            feedbackL = wetL;
            feedbackR = wetR;

            writePos = (writePos + 1) % maxBufferSize;

            bufL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufR)
                bufR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;
    double currentBpm = 120.0;

    int maxBufferSize = 96000;
    std::vector<float> bufferL;
    std::vector<float> bufferR;

    int writePos = 0;
    float readPos = 0.0f;
    int sliceStartPos = 0;
    int sliceSamples = 6000;
    int timer = 0;

    float fadeFraction = 0.1f;
    float jitterAmount = 0.0f;
    float feedbackAmount = 0.0f;
    float dirBlend = 0.0f;
    float wetMix = 1.0f;

    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
};
