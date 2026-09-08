#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "../VintageFilter.h"

enum class DelayFxType
{
    Tape = 0,
    BBD = 1,
    Digital = 2,
    PingPong = 3
};

class DelayEffect
{
public:
    DelayEffect() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        maxDelaySamples = (int)(currentSampleRate * 2.2); // 2.2 seconds buffer
        delayBufferL.assign(maxDelaySamples, 0.0f);
        delayBufferR.assign(maxDelaySamples, 0.0f);
        writeIndex = 0;

        smoothedDelaySamples = (float)(currentSampleRate * 0.35);
        targetDelaySamples = smoothedDelaySamples;

        filterStateL = 0.0f;
        filterStateR = 0.0f;
        flutterPhase1 = 0.0f;
        flutterPhase2 = 0.0f;
        currentMix = 0.0f;
    }

    void reset()
    {
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        writeIndex = 0;
        filterStateL = 0.0f;
        filterStateR = 0.0f;
        flutterPhase1 = 0.0f;
        flutterPhase2 = 0.0f;
        currentMix = 0.0f;
    }

    void setBpmAndSync(double bpm, bool isSynced, int syncRateIndex)
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
        syncEnabled = isSynced;
        syncIndex = syncRateIndex;
    }

    void process(float* left, float* right, int numSamples,
                 DelayFxType type, float timeSec, float feedback, float tone, float mix)
    {
        if (mix <= 0.0001f && currentMix <= 0.0001f)
            return;

        float effectiveTimeSec = timeSec;
        if (syncEnabled)
        {
            static const float beatDurations[] = {
                0.125f, 0.166667f, 0.25f, 0.375f, 0.333333f, 0.5f, 0.75f,
                0.666667f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 8.0f, 16.0f
            };
            int clampedIdx = std::clamp(syncIndex, 0, 14);
            float durationInBeats = beatDurations[clampedIdx];
            float beatSec = (float)(60.0 / currentBpm);
            effectiveTimeSec = durationInBeats * beatSec;
        }

        effectiveTimeSec = std::clamp(effectiveTimeSec, 0.01f, 1.8f);
        feedback = std::clamp(feedback, 0.0f, 0.95f);
        tone = std::clamp(tone, 0.0f, 1.0f);
        mix = std::clamp(mix, 0.0f, 1.0f);

        float newTarget = effectiveTimeSec * (float)currentSampleRate;
        targetDelaySamples = std::clamp(newTarget, 4.0f, (float)(maxDelaySamples - 16));

        // Tone filter cutoff
        float cutoffHz = 800.0f + tone * 16000.0f;
        if (type == DelayFxType::BBD)
        {
            cutoffHz = std::min(cutoffHz, 4000.0f); // BBD vintage analog roll-off
        }
        float filterCoeff = 1.0f - std::exp(-6.2831853f * cutoffHz / (float)currentSampleRate);

        // Smooth continuous time tracking (no sudden tap jumps or crossfade clicks)
        float timeInertiaSec = (type == DelayFxType::Tape || type == DelayFxType::BBD) ? 0.045f : 0.015f;
        float slewCoeff = 1.0f - std::exp(-1.0f / (float)(currentSampleRate * timeInertiaSec));

        float flutterInc1 = (float)(6.283185307179586 * 0.55 / currentSampleRate);
        float flutterInc2 = (float)(6.283185307179586 * 2.30 / currentSampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            currentMix += 0.005f * (mix - currentMix);

            // Smooth delay time
            smoothedDelaySamples += slewCoeff * (targetDelaySamples - smoothedDelaySamples);

            float flutterMod = 0.0f;
            if (type == DelayFxType::Tape)
            {
                flutterPhase1 += flutterInc1;
                if (flutterPhase1 >= 6.2831853f) flutterPhase1 -= 6.2831853f;

                flutterPhase2 += flutterInc2;
                if (flutterPhase2 >= 6.2831853f) flutterPhase2 -= 6.2831853f;

                // Independent continuous phases -> zero phase jump clicks!
                flutterMod = std::sin(flutterPhase1) * 2.5f + std::sin(flutterPhase2) * 0.8f;
            }

            float readDelay = std::clamp(smoothedDelaySamples + flutterMod, 4.0f, (float)(maxDelaySamples - 16));

            float wetL = readDelayCubic(delayBufferL, readDelay);
            float wetR = readDelayCubic(delayBufferR, readDelay);

            // Filter repeats
            filterStateL += filterCoeff * (wetL - filterStateL);
            filterStateR += filterCoeff * (wetR - filterStateR);
            float filteredL = filterStateL;
            float filteredR = filterStateR;

            // Saturation / Feedback processing
            float fbL = 0.0f;
            float fbR = 0.0f;

            switch (type)
            {
                case DelayFxType::Tape:
                    fbL = VintageFilter::fast_tanh(filteredL * 1.25f) * feedback;
                    fbR = VintageFilter::fast_tanh(filteredR * 1.25f) * feedback;
                    delayBufferL[writeIndex] = inL + fbL;
                    delayBufferR[writeIndex] = inR + fbR;
                    break;

                case DelayFxType::BBD:
                    fbL = VintageFilter::fast_tanh(filteredL * 1.45f) * (feedback * 0.98f);
                    fbR = VintageFilter::fast_tanh(filteredR * 1.45f) * (feedback * 0.98f);
                    delayBufferL[writeIndex] = inL + fbL;
                    delayBufferR[writeIndex] = inR + fbR;
                    break;

                case DelayFxType::Digital:
                    fbL = filteredL * feedback;
                    fbR = filteredR * feedback;
                    delayBufferL[writeIndex] = inL + fbL;
                    delayBufferR[writeIndex] = inR + fbR;
                    break;

                case DelayFxType::PingPong:
                    // True stereo alternating Ping-Pong:
                    // (L+R)/2 feeds Left; Left feeds Right; Right feeds Left!
                    fbL = VintageFilter::fast_tanh(filteredR * 1.15f) * feedback;
                    fbR = VintageFilter::fast_tanh(filteredL * 1.15f) * feedback;
                    delayBufferL[writeIndex] = (inL + inR) * 0.5f + fbL;
                    delayBufferR[writeIndex] = fbR;
                    break;
            }

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            left[i] = (1.0f - currentMix) * inL + currentMix * wetL;
            right[i] = (1.0f - currentMix) * inR + currentMix * wetR;
        }
    }

private:
    inline float readDelayCubic(const std::vector<float>& buffer, float delaySamples) const noexcept
    {
        float readPos = (float)writeIndex - delaySamples;
        while (readPos < 0.0f) readPos += (float)maxDelaySamples;
        while (readPos >= (float)maxDelaySamples) readPos -= (float)maxDelaySamples;

        int i1 = (int)readPos;
        int i0 = (i1 - 1 + maxDelaySamples) % maxDelaySamples;
        int i2 = (i1 + 1) % maxDelaySamples;
        int i3 = (i1 + 2) % maxDelaySamples;

        float frac = readPos - (float)i1;

        float y0 = buffer[i0];
        float y1 = buffer[i1];
        float y2 = buffer[i2];
        float y3 = buffer[i3];

        // 4-point Hermite cubic interpolation (smooth C1 continuity)
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    double currentSampleRate = 44100.0;
    int maxDelaySamples = 97020;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writeIndex = 0;

    float smoothedDelaySamples = 15000.0f;
    float targetDelaySamples = 15000.0f;

    float filterStateL = 0.0f;
    float filterStateR = 0.0f;
    float flutterPhase1 = 0.0f;
    float flutterPhase2 = 0.0f;
    float currentMix = 0.0f;

    double currentBpm = 120.0;
    bool syncEnabled = false;
    int syncIndex = 8;
};

