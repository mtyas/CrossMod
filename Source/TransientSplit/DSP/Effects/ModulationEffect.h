#pragma once

#include "BaseEffect.h"

class ModulationEffect : public BaseEffect
{
public:
    ModulationEffect() = default;

    EffectType getType() const override { return EffectType::Modulation; }
    juce::String getName() const override { return "Modulation FX"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        maxDelaySamples = static_cast<int>(currentSampleRate * 0.05) + 32; // 50ms buffer
        delayBufL.assign(maxDelaySamples, 0.0f);
        delayBufR.assign(maxDelaySamples, 0.0f);
        writeIndex = 0;
        reset();
    }

    void reset() override
    {
        std::fill(delayBufL.begin(), delayBufL.end(), 0.0f);
        std::fill(delayBufR.begin(), delayBufR.end(), 0.0f);
        writeIndex = 0;
        lfoPhase = 0.0f;
        flangerFbL = 0.0f; flangerFbR = 0.0f;
        phaserFbL = 0.0f; phaserFbR = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            apStateL[i] = 0.0f;
            apStateR[i] = 0.0f;
        }
    }

    void setParameters(float p1Rate, float p2Depth, float p3Type, float p4Feedback, float mixVal) override
    {
        modRateHz = 0.1f + std::pow(std::clamp(p1Rate, 0.0f, 1.0f), 2.0f) * 11.9f; // 0.1 Hz to 12 Hz
        modDepth = std::clamp(p2Depth, 0.0f, 1.0f);
        modMode = std::clamp(static_cast<int>(std::round(p3Type)), 0, 4);
        modFeedback = std::clamp(p4Feedback, 0.0f, 0.95f);
        wetMix = std::clamp(mixVal, 0.0f, 1.0f);
    }

    void process(float* left, float* right, int numSamples) override
    {
        if (numSamples <= 0 || maxDelaySamples <= 0) return;

        const float lfoInc = static_cast<float>(2.0 * 3.141592653589793 * modRateHz / currentSampleRate);

        for (int s = 0; s < numSamples; ++s)
        {
            const float dryL = left ? left[s] : 0.0f;
            const float dryR = right ? right[s] : 0.0f;

            lfoPhase += lfoInc;
            if (lfoPhase > 6.2831853f) lfoPhase -= 6.2831853f;

            // Quadrature LFO for rich stereo spread
            const float lfoL = std::sin(lfoPhase);
            const float lfoR = std::cos(lfoPhase);

            float wetL = 0.0f;
            float wetR = 0.0f;

            switch (modMode)
            {
                case 0: // CHORUS
                {
                    const float centerDelay = static_cast<float>(currentSampleRate * 0.015); // 15ms center
                    const float modDepthSamples = static_cast<float>(currentSampleRate * 0.006) * modDepth; // +/- 6ms

                    delayBufL[writeIndex] = dryL;
                    delayBufR[writeIndex] = dryR;

                    const float dL = centerDelay + lfoL * modDepthSamples;
                    const float dR = centerDelay + lfoR * modDepthSamples;

                    wetL = readBuffer(delayBufL, writeIndex - dL);
                    wetR = readBuffer(delayBufR, writeIndex - dR);
                    break;
                }

                case 1: // FLANGER
                {
                    const float centerDelay = static_cast<float>(currentSampleRate * 0.003); // 3ms center
                    const float modDepthSamples = static_cast<float>(currentSampleRate * 0.0025) * modDepth;

                    delayBufL[writeIndex] = dryL + flangerFbL * modFeedback;
                    delayBufR[writeIndex] = dryR + flangerFbR * modFeedback;

                    const float dL = centerDelay + (lfoL * 0.5f + 0.5f) * modDepthSamples;
                    const float dR = centerDelay + (lfoR * 0.5f + 0.5f) * modDepthSamples;

                    wetL = readBuffer(delayBufL, writeIndex - dL);
                    wetR = readBuffer(delayBufR, writeIndex - dR);

                    flangerFbL = std::tanh(wetL);
                    flangerFbR = std::tanh(wetR);
                    break;
                }

                case 2: // PHASER (4-Stage All-Pass Filter Cascade)
                {
                    // LFO modulates all-pass break frequency (200Hz to 4000Hz)
                    const float minF = 200.0f;
                    const float maxF = 4000.0f;
                    const float phFreqL = minF + (maxF - minF) * (lfoL * 0.5f + 0.5f) * modDepth;
                    const float phFreqR = minF + (maxF - minF) * (lfoR * 0.5f + 0.5f) * modDepth;

                    const float wL = static_cast<float>(std::tan(3.14159265 * phFreqL / currentSampleRate));
                    const float aL = (wL - 1.0f) / (wL + 1.0f);

                    const float wR = static_cast<float>(std::tan(3.14159265 * phFreqR / currentSampleRate));
                    const float aR = (wR - 1.0f) / (wR + 1.0f);

                    float xL = dryL + phaserFbL * modFeedback * 0.85f;
                    float xR = dryR + phaserFbR * modFeedback * 0.85f;

                    for (int i = 0; i < 4; ++i)
                    {
                        const float outApL = aL * xL + apStateL[i];
                        apStateL[i] = xL - aL * outApL;
                        xL = outApL;

                        const float outApR = aR * xR + apStateR[i];
                        apStateR[i] = xR - aR * outApR;
                        xR = outApR;
                    }

                    phaserFbL = xL;
                    phaserFbR = xR;

                    wetL = xL;
                    wetR = xR;
                    break;
                }

                case 3: // TREMOLO
                {
                    const float depthScale = modDepth * 0.95f;
                    const float modAmpL = 1.0f - depthScale * (0.5f * (1.0f + lfoL));
                    const float modAmpR = 1.0f - depthScale * (0.5f * (1.0f + lfoR));
                    wetL = dryL * modAmpL;
                    wetR = dryR * modAmpR;
                    break;
                }

                case 4: // VIBRATO
                {
                    const float centerDelay = static_cast<float>(currentSampleRate * 0.005); // 5ms
                    const float depthSamples = static_cast<float>(currentSampleRate * 0.003) * modDepth;

                    delayBufL[writeIndex] = dryL;
                    delayBufR[writeIndex] = dryR;

                    wetL = readBuffer(delayBufL, writeIndex - (centerDelay + lfoL * depthSamples));
                    wetR = readBuffer(delayBufR, writeIndex - (centerDelay + lfoR * depthSamples));
                    break;
                }
            }

            writeIndex = (writeIndex + 1) % maxDelaySamples;

            if (left) left[s] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (right) right[s] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

    EffectParamInfo getParamInfo(int paramIndex) const override
    {
        switch (paramIndex)
        {
            case 0: return { "Rate", "Speed / Rate", 0.35f, 0.0f, 1.0f };
            case 1: return { "Depth", "Mod Depth", 0.6f, 0.0f, 1.0f };
            case 2: return { "Type", "Mode (Chorus,Flanger,Phaser,Trem,Vib)", 0.0f, 0.0f, 4.0f };
            case 3: return { "Feedback", "Feedback / Spread", 0.4f, 0.0f, 1.0f };
            default: return { "Param", "Param", 0.0f, 0.0f, 1.0f };
        }
    }

private:
    float readBuffer(const std::vector<float>& buf, float samplePos) const
    {
        const int size = static_cast<int>(buf.size());
        while (samplePos < 0.0f) samplePos += size;
        while (samplePos >= size) samplePos -= size;

        const int i0 = static_cast<int>(samplePos);
        const int i1 = (i0 + 1) % size;
        const float frac = samplePos - static_cast<float>(i0);

        return buf[i0] + frac * (buf[i1] - buf[i0]);
    }

    float modRateHz = 1.2f;
    float modDepth = 0.6f;
    int modMode = 0;
    float modFeedback = 0.4f;
    float wetMix = 0.5f;

    float lfoPhase = 0.0f;
    std::vector<float> delayBufL;
    std::vector<float> delayBufR;
    int maxDelaySamples = 0;
    int writeIndex = 0;

    float flangerFbL = 0.0f; float flangerFbR = 0.0f;
    float phaserFbL = 0.0f; float phaserFbR = 0.0f;
    float apStateL[4] = {0}; float apStateR[4] = {0};
};
