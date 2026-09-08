#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

enum class GlitchMode
{
    StutterRepeat = 0,
    TapeStop = 1,
    ReverseSlice = 2,
    GranularJitter = 3
};

class GlitchEffect : public BaseEffect
{
public:
    GlitchEffect() = default;

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxBufferSize = static_cast<int>(currentSampleRate * 2.0); // 2 second buffer

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
        tapeSpeed = 1.0f;
        stutterActive = false;
        stutterCounter = 0;
    }

    void setBpm(double bpm) override
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
    }

    void setParameters(float p1, float p2, float p3, float p4, float mix) override
    {
        // P1: Slice Division (0: 1/32, 1: 1/16, 2: 1/8, 3: 1/4, 4: 1/2)
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

        const double sliceSec = (60.0 / currentBpm) * beats;
        sliceSamples = std::max(64, static_cast<int>(sliceSec * currentSampleRate));

        // P2: Stutter Probability / Trigger threshold
        triggerProb = std::clamp(p2, 0.05f, 1.0f);

        // P3: Mode (0: Stutter, 1: Tape Stop, 2: Reverse, 3: Granular Jitter)
        const int modeIdx = std::clamp(static_cast<int>(std::floor(p3 * 3.99f)), 0, 3);
        mode = static_cast<GlitchMode>(modeIdx);

        // P4: Jitter / Degrade amount
        jitterAmount = std::clamp(p4, 0.0f, 1.0f);

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0 || maxBufferSize <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            // Always write incoming audio into circular buffer
            bufferMemL[writePos] = dryL;
            bufferMemR[writePos] = dryR;

            // Trigger / Slice Boundary Check
            if (++sampleTimer >= sliceSamples)
            {
                sampleTimer = 0;
                const float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
                stutterActive = (r < triggerProb);

                if (stutterActive)
                {
                    sliceStartPos = writePos - sliceSamples;
                    if (sliceStartPos < 0) sliceStartPos += maxBufferSize;
                    readPos = static_cast<float>(sliceStartPos);
                    tapeSpeed = 1.0f;
                }
            }

            float wetL = dryL;
            float wetR = dryR;

            if (stutterActive)
            {
                switch (mode)
                {
                    case GlitchMode::StutterRepeat:
                    {
                        readPos += 1.0f;
                        if (readPos >= static_cast<float>(sliceStartPos + sliceSamples))
                            readPos = static_cast<float>(sliceStartPos);
                        break;
                    }

                    case GlitchMode::TapeStop:
                    {
                        tapeSpeed = std::max(0.01f, tapeSpeed * 0.9998f);
                        readPos += tapeSpeed;
                        break;
                    }

                    case GlitchMode::ReverseSlice:
                    {
                        readPos -= 1.0f;
                        if (readPos < static_cast<float>(sliceStartPos))
                            readPos = static_cast<float>(sliceStartPos + sliceSamples - 1);
                        break;
                    }

                    case GlitchMode::GranularJitter:
                    {
                        const float jitter = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * jitterAmount * 30.0f;
                        readPos += 1.0f + jitter;
                        if (readPos >= static_cast<float>(sliceStartPos + sliceSamples))
                            readPos = static_cast<float>(sliceStartPos);
                        break;
                    }
                }

                // Wrap read position in circular buffer
                while (readPos < 0.0f) readPos += static_cast<float>(maxBufferSize);
                while (readPos >= static_cast<float>(maxBufferSize)) readPos -= static_cast<float>(maxBufferSize);

                const int r0 = static_cast<int>(readPos);
                const int r1 = (r0 + 1) % maxBufferSize;
                const float frac = readPos - static_cast<float>(r0);

                wetL = bufferMemL[r0] + frac * (bufferMemL[r1] - bufferMemL[r0]);
                wetR = bufferMemR[r0] + frac * (bufferMemR[r1] - bufferMemR[r0]);
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

    GlitchMode mode = GlitchMode::StutterRepeat;
    float triggerProb = 0.5f;
    float jitterAmount = 0.0f;
    float wetMix = 1.0f;

    bool stutterActive = false;
    int stutterCounter = 0;
    float tapeSpeed = 1.0f;
};
