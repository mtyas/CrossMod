#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

enum class ModFxType
{
    Chorus = 0,
    Flanger = 1,
    Phaser = 2,
    Ensemble = 3
};

class ModulationEffect
{
public:
    ModulationEffect() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        bufferSize = (int)(currentSampleRate * 0.1); // 100ms max buffer
        delayBufferL.assign(bufferSize, 0.0f);
        delayBufferR.assign(bufferSize, 0.0f);
        writeIndex = 0;

        // Reset phaser allpass states
        for (int i = 0; i < 6; ++i)
        {
            apL[i] = 0.0f;
            apR[i] = 0.0f;
        }
        lastPhaserFeedbackL = 0.0f;
        lastPhaserFeedbackR = 0.0f;
        lastFlangerFeedbackL = 0.0f;
        lastFlangerFeedbackR = 0.0f;
    }

    void reset()
    {
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        for (int i = 0; i < 6; ++i)
        {
            apL[i] = 0.0f;
            apR[i] = 0.0f;
        }
        lastPhaserFeedbackL = 0.0f;
        lastPhaserFeedbackR = 0.0f;
        lastFlangerFeedbackL = 0.0f;
        lastFlangerFeedbackR = 0.0f;
        lfoPhase = 0.0f;
        lfoPhase2 = 0.0f;
    }

    void process(float* left, float* right, int numSamples,
                 ModFxType type, float rateHz, float depth, float feedback, float mix)
    {
        if (mix <= 0.001f)
            return;

        rateHz = std::clamp(rateHz, 0.05f, 15.0f);
        depth = std::clamp(depth, 0.0f, 1.0f);
        feedback = std::clamp(feedback, 0.0f, 0.95f);
        mix = std::clamp(mix, 0.0f, 1.0f);

        float lfoInc = (float)(6.283185307179586 * rateHz / currentSampleRate);
        float lfoIncFast = (float)(6.283185307179586 * (rateHz * 5.8f) / currentSampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            // Advance LFO
            lfoPhase += lfoInc;
            if (lfoPhase >= 6.2831853f) lfoPhase -= 6.2831853f;

            lfoPhase2 += lfoIncFast;
            if (lfoPhase2 >= 6.2831853f) lfoPhase2 -= 6.2831853f;

            float wetL = 0.0f;
            float wetR = 0.0f;

            switch (type)
            {
                case ModFxType::Chorus:
                {
                    // Delay range ~15ms to ~30ms, quadrature LFO for stereo spread
                    float modL = 0.5f + 0.5f * std::sin(lfoPhase);
                    float modR = 0.5f + 0.5f * std::cos(lfoPhase);

                    float delaySecL = 0.015f + depth * 0.015f * modL;
                    float delaySecR = 0.015f + depth * 0.015f * modR;

                    // Write to buffer with subtle feedback
                    delayBufferL[writeIndex] = inL + lastFlangerFeedbackL * (feedback * 0.4f);
                    delayBufferR[writeIndex] = inR + lastFlangerFeedbackR * (feedback * 0.4f);

                    wetL = readDelayInterpolated(delayBufferL, delaySecL * (float)currentSampleRate);
                    wetR = readDelayInterpolated(delayBufferR, delaySecR * (float)currentSampleRate);

                    lastFlangerFeedbackL = wetL;
                    lastFlangerFeedbackR = wetR;
                    break;
                }

                case ModFxType::Flanger:
                {
                    // Short delay ~0.5ms to 5.0ms with resonant feedback
                    float modL = 0.5f + 0.5f * std::sin(lfoPhase);
                    float modR = 0.5f + 0.5f * std::sin(lfoPhase + 1.57f);

                    float delaySecL = 0.0005f + depth * 0.0045f * modL;
                    float delaySecR = 0.0005f + depth * 0.0045f * modR;

                    delayBufferL[writeIndex] = inL + lastFlangerFeedbackL * feedback;
                    delayBufferR[writeIndex] = inR + lastFlangerFeedbackR * feedback;

                    wetL = readDelayInterpolated(delayBufferL, delaySecL * (float)currentSampleRate);
                    wetR = readDelayInterpolated(delayBufferR, delaySecR * (float)currentSampleRate);

                    lastFlangerFeedbackL = std::tanh(wetL);
                    lastFlangerFeedbackR = std::tanh(wetR);
                    break;
                }

                case ModFxType::Phaser:
                {
                    // 6-stage Allpass cascade with resonant feedback loop
                    float lfo = 0.5f + 0.5f * std::sin(lfoPhase);
                    float fc = 200.0f + depth * 3500.0f * lfo;
                    float w = 3.14159265f * fc / (float)currentSampleRate;
                    float alpha = (std::tan(w) - 1.0f) / (std::tan(w) + 1.0f);

                    float xL = inL + lastPhaserFeedbackL * feedback;
                    float xR = inR + lastPhaserFeedbackR * feedback;

                    for (int s = 0; s < 6; ++s)
                    {
                        float yL = alpha * xL + apL[s];
                        apL[s] = xL - alpha * yL;
                        xL = yL;

                        float yR = alpha * xR + apR[s];
                        apR[s] = xR - alpha * yR;
                        xR = yR;
                    }

                    wetL = xL;
                    wetR = xR;
                    lastPhaserFeedbackL = std::tanh(wetL);
                    lastPhaserFeedbackR = std::tanh(wetR);
                    break;
                }

                case ModFxType::Ensemble:
                {
                    // Dual-frequency LFO modulation (Solina / Dimension D)
                    float modL = 0.5f + 0.35f * std::sin(lfoPhase) + 0.15f * std::sin(lfoPhase2);
                    float modR = 0.5f + 0.35f * std::cos(lfoPhase) + 0.15f * std::cos(lfoPhase2);

                    float delaySecL = 0.010f + depth * 0.012f * modL;
                    float delaySecR = 0.010f + depth * 0.012f * modR;

                    delayBufferL[writeIndex] = inL;
                    delayBufferR[writeIndex] = inR;

                    wetL = readDelayInterpolated(delayBufferL, delaySecL * (float)currentSampleRate);
                    wetR = readDelayInterpolated(delayBufferR, delaySecR * (float)currentSampleRate);
                    break;
                }
            }

            writeIndex = (writeIndex + 1) % bufferSize;

            left[i] = (1.0f - mix) * inL + mix * wetL;
            right[i] = (1.0f - mix) * inR + mix * wetR;
        }
    }

private:
    inline float readDelayInterpolated(const std::vector<float>& buffer, float delaySamples) const
    {
        float readPos = (float)writeIndex - delaySamples;
        while (readPos < 0.0f) readPos += (float)bufferSize;
        while (readPos >= (float)bufferSize) readPos -= (float)bufferSize;

        int i0 = (int)readPos;
        int i1 = (i0 + 1) % bufferSize;
        float frac = readPos - (float)i0;

        return buffer[i0] + frac * (buffer[i1] - buffer[i0]);
    }

    double currentSampleRate = 44100.0;
    int bufferSize = 4410;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writeIndex = 0;

    float lfoPhase = 0.0f;
    float lfoPhase2 = 0.0f;

    float apL[6] { 0.0f };
    float apR[6] { 0.0f };
    float lastPhaserFeedbackL = 0.0f;
    float lastPhaserFeedbackR = 0.0f;
    float lastFlangerFeedbackL = 0.0f;
    float lastFlangerFeedbackR = 0.0f;
};
