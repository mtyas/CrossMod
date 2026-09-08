#pragma once

#include <cmath>
#include <algorithm>
#include <random>
#include <array>

enum class LfoWaveform
{
    Sine = 0,
    Triangle = 1,
    SawUp = 2,
    SawDown = 3,
    Square = 4,
    SampleAndHold = 5,
    RandomSmooth = 6
};

class ModLFO
{
public:
    static constexpr int ScopeBufferSize = 64;

    ModLFO()
    {
        scopeBuffer.fill(0.0f);
    }

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset()
    {
        phase = 0.0f;
        currentValue = 0.0f;
        shTarget = 0.0f;
        shCurrent = 0.0f;
        prevPhase = 0.0f;
        scopeBuffer.fill(0.0f);
        scopeWriteIndex = 0;
    }

    void setBpm(double bpm)
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
    }

    void setParameters(LfoWaveform wave, float rateParam, bool isSynced, int syncSubdiv, float smoothParam)
    {
        waveform = wave;
        syncEnabled = isSynced;
        syncIndex = syncSubdiv;

        // Ultra-deep smoothing: from 1ms up to 2000ms (2 seconds)
        const float s = std::clamp(smoothParam, 0.0f, 1.0f);
        if (s > 0.001f)
        {
            const double smoothSec = 0.001 * std::pow(2000.0, static_cast<double>(s));
            slewAlpha = static_cast<float>(std::exp(-1.0 / (smoothSec * currentSampleRate)));
        }
        else
        {
            slewAlpha = 0.0f; // Instant / bypass
        }

        if (syncEnabled)
        {
            const double beatsPerSec = currentBpm / 60.0;
            double beats = 1.0;
            switch (syncIndex)
            {
                case 0: beats = 0.125; break; // 1/32
                case 1: beats = 0.25;  break; // 1/16
                case 2: beats = 0.5;   break; // 1/8
                case 3: beats = 1.0;   break; // 1/4
                case 4: beats = 2.0;   break; // 1/2
                case 5: beats = 4.0;   break; // 1 Bar
                case 6: beats = 8.0;   break; // 2 Bars
                case 7: beats = 16.0;  break; // 4 Bars
                case 8: beats = 32.0;  break; // 8 Bars
                default: beats = 1.0;  break;
            }
            freqHz = static_cast<float>(beatsPerSec / beats);
        }
        else
        {
            // Direct Hz from parameter (0.01 Hz up to 50.0 Hz)
            freqHz = std::clamp(rateParam, 0.01f, 50.0f);
        }

        phaseInc = static_cast<float>(freqHz / currentSampleRate);
    }

    void syncToHostPosition(double ppqPosition)
    {
        if (syncEnabled && ppqPosition >= 0.0)
        {
            double beats = 1.0;
            switch (syncIndex)
            {
                case 0: beats = 0.125; break; // 1/32
                case 1: beats = 0.25;  break; // 1/16
                case 2: beats = 0.5;   break; // 1/8
                case 3: beats = 1.0;   break; // 1/4
                case 4: beats = 2.0;   break; // 1/2
                case 5: beats = 4.0;   break; // 1 Bar
                case 6: beats = 8.0;   break; // 2 Bars
                case 7: beats = 16.0;  break; // 4 Bars
                case 8: beats = 32.0;  break; // 8 Bars
                default: beats = 1.0;  break;
            }
            const double cyclePos = std::fmod(ppqPosition, beats) / beats;
            phase = static_cast<float>(cyclePos);
        }
    }

    float getNextSample()
    {
        prevPhase = phase;
        phase += phaseInc;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            shTarget = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
        }

        float raw = 0.0f;
        switch (waveform)
        {
            case LfoWaveform::Sine:
                raw = std::sin(phase * 6.283185307179586f);
                break;

            case LfoWaveform::Triangle:
                raw = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                break;

            case LfoWaveform::SawUp:
                raw = 2.0f * phase - 1.0f;
                break;

            case LfoWaveform::SawDown:
                raw = 1.0f - 2.0f * phase;
                break;

            case LfoWaveform::Square:
                raw = (phase < 0.5f) ? 1.0f : -1.0f;
                break;

            case LfoWaveform::SampleAndHold:
                raw = shTarget;
                break;

            case LfoWaveform::RandomSmooth:
                shCurrent += 0.04f * (shTarget - shCurrent);
                raw = shCurrent;
                break;
        }

        // Apply exponential time-constant smoothing (1ms to 2000ms)
        if (slewAlpha > 0.0f)
            currentValue = slewAlpha * currentValue + (1.0f - slewAlpha) * raw;
        else
            currentValue = raw;

        // Store into mini visualizer scope buffer periodically
        if (++sampleCounter >= 16)
        {
            sampleCounter = 0;
            scopeBuffer[scopeWriteIndex] = currentValue;
            scopeWriteIndex = (scopeWriteIndex + 1) % ScopeBufferSize;
        }

        return currentValue;
    }

    float getCurrentValue() const noexcept { return currentValue; }
    float getFreqHz() const noexcept { return freqHz; }
    float getPhase() const noexcept { return phase; }

    const std::array<float, ScopeBufferSize>& getScopeBuffer() const noexcept { return scopeBuffer; }
    int getScopeWriteIndex() const noexcept { return scopeWriteIndex; }

private:
    double currentSampleRate = 48000.0;
    double currentBpm = 120.0;

    LfoWaveform waveform = LfoWaveform::Sine;
    bool syncEnabled = false;
    int syncIndex = 7;
    float freqHz = 1.0f;
    float phaseInc = 0.0f;
    float phase = 0.0f;
    float prevPhase = 0.0f;
    float slewAlpha = 0.0f;

    float shTarget = 0.0f;
    float shCurrent = 0.0f;
    float currentValue = 0.0f;

    int sampleCounter = 0;
    int scopeWriteIndex = 0;
    std::array<float, ScopeBufferSize> scopeBuffer;
};
