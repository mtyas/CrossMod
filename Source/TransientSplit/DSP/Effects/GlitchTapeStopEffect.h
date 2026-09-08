#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

class GlitchTapeStopEffect : public BaseEffect
{
public:
    GlitchTapeStopEffect() = default;

    EffectType getType() const override { return EffectType::GlitchTapeStop; }
    juce::String getName() const override { return "Tape Stop Glitch"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxBufferSize = static_cast<int>(currentSampleRate * 3.0);
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
        tapeSpeed = 1.0f;
        isStopping = false;
        lpStateL = 0.0f;
        lpStateR = 0.0f;
    }

    void setBpm(double bpm) override
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync Trigger (0 = Free/P1, >0 = Synced Bars 1/2..4 Bars)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        double triggerSec = (240.0 / currentBpm) * (0.5 * std::pow(8.0, std::clamp(p1, 0.0f, 1.0f)));
        if (syncSec > 0.0)
            triggerSec = syncSec;

        triggerSamples = std::max(1024, static_cast<int>(triggerSec * currentSampleRate));

        // P2: Stop Time (100ms to 2000ms)
        const double stopSec = 0.1 + std::clamp(p2, 0.0f, 1.0f) * 1.9;
        stopDecay = static_cast<float>(std::exp(-1.0 / (stopSec * 0.35 * currentSampleRate)));

        // P3: Curve Slew
        curveExp = 1.0f + std::clamp(p3, 0.0f, 1.0f) * 2.0f;

        // P4: Lowpass Drop Filter
        const double cutoffHz = 400.0 + (1.0 - std::clamp(p4, 0.0f, 1.0f)) * 14000.0;
        lpCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0 || maxBufferSize <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            bufferL[writePos] = dryL;
            bufferR[writePos] = dryR;

            if (++timer >= triggerSamples)
            {
                timer = 0;
                isStopping = true;
                tapeSpeed = 1.0f;
                readPos = static_cast<float>(writePos);
            }

            float wetL = dryL;
            float wetR = dryR;

            if (isStopping)
            {
                tapeSpeed *= stopDecay;
                if (tapeSpeed < 0.02f)
                {
                    tapeSpeed += spinUpInc;
                    if (tapeSpeed >= 1.0f)
                    {
                        tapeSpeed = 1.0f;
                        isStopping = false;
                    }
                }

                readPos += std::pow(tapeSpeed, curveExp);
                while (readPos < 0.0f) readPos += static_cast<float>(maxBufferSize);
                while (readPos >= static_cast<float>(maxBufferSize)) readPos -= static_cast<float>(maxBufferSize);

                const int r0 = static_cast<int>(readPos);
                const int r1 = (r0 + 1) % maxBufferSize;
                const float frac = readPos - static_cast<float>(r0);

                float sL = bufferL[r0] + frac * (bufferL[r1] - bufferL[r0]);
                float sR = bufferR[r0] + frac * (bufferR[r1] - bufferR[r0]);

                lpStateL += lpCoeff * (sL - lpStateL);
                lpStateR += lpCoeff * (sR - lpStateR);

                wetL = lpStateL * tapeSpeed;
                wetR = lpStateR * tapeSpeed;
            }

            writePos = (writePos + 1) % maxBufferSize;

            bufL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufR)
                bufR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;
    double currentBpm = 120.0;

    int maxBufferSize = 144000;
    std::vector<float> bufferL;
    std::vector<float> bufferR;

    int writePos = 0;
    float readPos = 0.0f;
    int timer = 0;
    int triggerSamples = 48000;

    float stopDecay = 0.9995f;
    float curveExp = 1.0f;
    float lpCoeff = 0.8f;
    float spinUpInc = 0.001f;
    float wetMix = 1.0f;

    bool isStopping = false;
    float tapeSpeed = 1.0f;
    float lpStateL = 0.0f;
    float lpStateR = 0.0f;
};
