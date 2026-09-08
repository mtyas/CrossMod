#pragma once

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "../Parameters/ParameterIDs.h"

class LFO
{
public:
    LFO() = default;

    void setSampleRate(double newSampleRate)
    {
        sampleRate = std::max(1.0, newSampleRate);
        updateIncrement();
    }

    void setRateHz(float rateHz)
    {
        currentRateHz = std::clamp(rateHz, 0.01f, 2000.0f);
        updateIncrement();
    }

    void setShape(LFOShape shape)
    {
        currentShape = shape;
    }

    void setBpmAndSync(double bpm, bool isSynced, int syncRateIndex)
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
        syncEnabled = isSynced;
        syncIndex = syncRateIndex;
        updateIncrement();
    }

    void resetPhase()
    {
        phase = 0.0f;
        generateNewRandomTarget();
        currentRandomValue = targetRandomValue;
    }

    static inline float polyBLEP(float t, float dt) noexcept
    {
        if (dt <= 1e-7f) return 0.0f;
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    inline float getNextSample(float rateModOffset = 0.0f)
    {
        // Apply rate modulation if any
        float effectiveRate = currentEffectiveRate;
        if (rateModOffset != 0.0f)
        {
            // 1V/Oct pitch tracking modulation
            effectiveRate *= std::exp2(rateModOffset * 2.0f);
            effectiveRate = std::clamp(effectiveRate, 0.001f, (float)(sampleRate * 0.48));
        }

        float inc = static_cast<float>(effectiveRate / sampleRate);
        phase += inc;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            generateNewRandomTarget();
        }

        float out = 0.0f;
        switch (currentShape)
        {
            case LFOShape::Sine:
                out = std::sin(phase * 6.283185307179586f);
                break;

            case LFOShape::Triangle:
                out = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                break;

            case LFOShape::SawUp:
                out = 2.0f * phase - 1.0f - polyBLEP(phase, inc);
                break;

            case LFOShape::SawDown:
                out = 1.0f - 2.0f * phase + polyBLEP(phase, inc);
                break;

            case LFOShape::Square:
            {
                float sq = (phase < 0.5f) ? 1.0f : -1.0f;
                sq += polyBLEP(phase, inc);
                float pShift = phase + 0.5f;
                if (pShift >= 1.0f) pShift -= 1.0f;
                sq -= polyBLEP(pShift, inc);
                out = sq;
                break;
            }

            case LFOShape::SampleAndHold:
                // Smooth slightly towards target to prevent audio clicks when S&H modulates filter/pitch
                currentRandomValue += 0.08f * (targetRandomValue - currentRandomValue);
                out = currentRandomValue;
                break;
        }

        lastOutput = out;
        return out;
    }

    float getLastOutput() const { return lastOutput; }

private:
    void generateNewRandomTarget()
    {
        targetRandomValue = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }

    void updateIncrement()
    {
        if (syncEnabled)
        {
            // Beats per second = BPM / 60
            // Convert sync rate index into quarter note multiplier
            // Indices: 0: 1/32 (0.125 beats), 1: 1/16T (0.1667 beats), 2: 1/16 (0.25), 3: 1/8T (0.333),
            // 4: 1/8 (0.5), 5: 1/4T (0.667), 6: 1/4 (1.0), 7: 1/2 (2.0), 8: 1 Bar (4.0), 9: 2 Bars (8.0), 10: 4 Bars (16.0)
            static const float beatDurations[] = {
                0.125f, 0.166667f, 0.25f, 0.375f, 0.333333f, 0.5f, 0.75f,
                0.666667f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 8.0f, 16.0f
            };
            int clampedIdx = std::clamp(syncIndex, 0, 14);
            float durationInBeats = beatDurations[clampedIdx];
            float bps = (float)(currentBpm / 60.0);
            currentEffectiveRate = bps / durationInBeats;
        }
        else
        {
            currentEffectiveRate = currentRateHz;
        }
    }

    double sampleRate = 44100.0;
    double currentBpm = 120.0;
    float currentRateHz = 1.5f;
    float currentEffectiveRate = 1.5f;
    bool syncEnabled = false;
    int syncIndex = 6;
    LFOShape currentShape = LFOShape::Sine;

    float phase = 0.0f;
    float targetRandomValue = 0.0f;
    float currentRandomValue = 0.0f;
    float lastOutput = 0.0f;
};
