#pragma once

#include "BaseEffect.h"

class TapeEffect : public BaseEffect
{
public:
    TapeEffect() = default;

    EffectType getType() const override { return EffectType::TapeSat; }
    juce::String getName() const override { return "Tape Saturation"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        modBufferSize = static_cast<int>(currentSampleRate * 0.05) + 32; // 50ms buffer for wow/flutter
        modBufferL.assign(modBufferSize, 0.0f);
        modBufferR.assign(modBufferSize, 0.0f);
        writeIndex = 0;
        reset();
    }

    void reset() override
    {
        std::fill(modBufferL.begin(), modBufferL.end(), 0.0f);
        std::fill(modBufferR.begin(), modBufferR.end(), 0.0f);
        writeIndex = 0;
        wowPhase = 0.0f;
        flutterPhase = 0.0f;
        lpStateL = 0.0f; lpStateR = 0.0f;
        hpBumpL = 0.0f; hpBumpR = 0.0f;
        dcPrevInL = 0.0f; dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f; dcPrevOutR = 0.0f;
    }

    void setParameters(float p1Drive, float p2Warmth, float p3Flutter, float p4Age, float p5Bias, float mixVal) override
    {
        tapeSaturation = std::clamp(p1Drive, 0.0f, 1.0f);
        warmthAmount = std::clamp(p2Warmth, 0.0f, 1.0f);
        flutterAmount = std::clamp(p3Flutter, 0.0f, 1.0f);
        tapeAge = std::clamp(p4Age, 0.0f, 1.0f);
        headBias = std::clamp(p5Bias, 0.0f, 1.0f);
        wetMix = std::clamp(mixVal, 0.0f, 1.0f);

        // Saturation drive
        driveGain = 1.0f + tapeSaturation * 5.0f * (1.0f + headBias * 0.5f);

        // Tape head HF roll-off (Warmth: 3.5kHz for dark vintage tape up to 20kHz for open tape)
        const double cutoff = 3500.0 + (1.0 - warmthAmount * 0.75) * 16500.0 - tapeAge * 2000.0;
        const double wLp = 2.0 * 3.141592653589793 * (std::clamp(cutoff, 2000.0, 20000.0) / currentSampleRate);
        lpAlpha = static_cast<float>(std::clamp(wLp / (wLp + 1.0), 0.001, 0.999));

        // Low end head-bump filter (~60-90Hz)
        const double wBump = 2.0 * 3.141592653589793 * (80.0 / currentSampleRate);
        bumpAlpha = static_cast<float>(std::clamp(wBump / (wBump + 1.0), 0.001, 0.999));
    }

    void process(float* left, float* right, int numSamples) override
    {
        if (numSamples <= 0 || modBufferSize <= 0) return;

        const float wowInc = static_cast<float>(2.0 * 3.141592653589793 * 0.8 / currentSampleRate);     // 0.8 Hz wow
        const float flutterInc = static_cast<float>(2.0 * 3.141592653589793 * 9.5 / currentSampleRate); // 9.5 Hz flutter
        const float maxModDelaySamples = static_cast<float>(currentSampleRate * 0.0025); // 2.5ms max depth

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left ? left[i] : 0.0f;
            const float dryR = right ? right[i] : 0.0f;

            // 1. Magnetic Tape Saturation with polynomial hysteresis soft clipping
            float xL = dryL * driveGain;
            float xR = dryR * driveGain;

            // Tape bias / age asymmetry
            xL += tapeAge * 0.08f * (xL * xL);
            xR += tapeAge * 0.08f * (xR * xR);

            float satL = tapeHysteresis(xL, tapeSaturation);
            float satR = tapeHysteresis(xR, tapeSaturation);

            // 2. High-precision DC Blocker (~2.5 Hz)
            const float dcR = 0.9995f;
            const float filteredDcL = satL - dcPrevInL + dcR * dcPrevOutL;
            dcPrevInL = satL; dcPrevOutL = filteredDcL;
            satL = filteredDcL;

            const float filteredDcR = satR - dcPrevInR + dcR * dcPrevOutR;
            dcPrevInR = satR; dcPrevOutR = filteredDcR;
            satR = filteredDcR;

            // 3. Optional Wow & Flutter pitch modulation
            float wetL = satL;
            float wetR = satR;

            if (flutterAmount > 0.005f)
            {
                modBufferL[writeIndex] = satL;
                modBufferR[writeIndex] = satR;

                // Update wow/flutter LFO
                wowPhase += wowInc;
                if (wowPhase > 6.2831853f) wowPhase -= 6.2831853f;

                flutterPhase += flutterInc;
                if (flutterPhase > 6.2831853f) flutterPhase -= 6.2831853f;

                const float wow = std::sin(wowPhase) * 0.7f;
                const float flutter = std::sin(flutterPhase) * 0.3f + std::sin(flutterPhase * 1.618f) * 0.15f;
                const float modTotal = (wow + flutter) * flutterAmount;

                const float baseDelay = static_cast<float>(currentSampleRate * 0.002) * flutterAmount;
                const float currentDelay = std::max(0.0f, baseDelay + modTotal * maxModDelaySamples);

                const float flutterL = readInterpolated(modBufferL, writeIndex - currentDelay);
                const float flutterR = readInterpolated(modBufferR, writeIndex - currentDelay);

                writeIndex = (writeIndex + 1) % modBufferSize;

                wetL = satL * (1.0f - flutterAmount) + flutterL * flutterAmount;
                wetR = satR * (1.0f - flutterAmount) + flutterR * flutterAmount;
            }

            // 4. Low-end Head Bump (~80Hz resonant bass enhancement)
            hpBumpL += bumpAlpha * (wetL - hpBumpL);
            hpBumpR += bumpAlpha * (wetR - hpBumpR);
            wetL += hpBumpL * 0.25f * (1.0f - tapeAge * 0.5f);
            wetR += hpBumpR * 0.25f * (1.0f - tapeAge * 0.5f);

            // 5. Tape Head HF Roll-off
            lpStateL += lpAlpha * (wetL - lpStateL);
            lpStateR += lpAlpha * (wetR - lpStateR);
            wetL = lpStateL;
            wetR = lpStateR;

            // Gain trim
            const float trim = 1.0f / (1.0f + 0.3f * tapeSaturation);
            wetL *= trim;
            wetR *= trim;

            if (left) left[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (right) right[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

    EffectParamInfo getParamInfo(int paramIndex) const override
    {
        switch (paramIndex)
        {
            case 0: return { "Drive", "Tape Drive", 0.5f, 0.0f, 1.0f };
            case 1: return { "Flutter", "Wow & Flutter", 0.2f, 0.0f, 1.0f };
            case 2: return { "Speed", "Speed / Head Tone", 0.7f, 0.0f, 1.0f };
            case 3: return { "Age", "Tape Age / Bias", 0.2f, 0.0f, 1.0f };
            default: return { "Param", "Param", 0.0f, 0.0f, 1.0f };
        }
    }

private:
    static float tapeHysteresis(float x, float sat)
    {
        // Magnetic tape smooth sigmoid saturation
        const float k = 1.2f + sat * 1.5f;
        return x / std::sqrt(1.0f + (x * x * k * k) / (1.0f + std::abs(x)));
    }

    float readInterpolated(const std::vector<float>& buf, float samplePos) const
    {
        const int size = static_cast<int>(buf.size());
        while (samplePos < 0.0f) samplePos += size;
        while (samplePos >= size) samplePos -= size;

        const int i0 = static_cast<int>(samplePos);
        const int i1 = (i0 + 1) % size;
        const float frac = samplePos - static_cast<float>(i0);

        return buf[i0] + frac * (buf[i1] - buf[i0]);
    }

    float tapeSaturation = 0.5f;
    float warmthAmount = 0.5f;
    float flutterAmount = 0.2f;
    float tapeSpeed = 0.7f;
    float tapeAge = 0.2f;
    float headBias = 0.3f;
    float wetMix = 0.5f;

    float driveGain = 1.0f;
    float lpAlpha = 0.8f;
    float bumpAlpha = 0.02f;
    float lpStateL = 0.0f; float lpStateR = 0.0f;
    float hpBumpL = 0.0f; float hpBumpR = 0.0f;

    float dcPrevInL = 0.0f; float dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f; float dcPrevOutR = 0.0f;

    std::vector<float> modBufferL;
    std::vector<float> modBufferR;
    int modBufferSize = 0;
    int writeIndex = 0;

    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
};
