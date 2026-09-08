#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <algorithm>

enum class ReverbFxType
{
    Plate = 0,
    Room = 1,
    Hall = 2
};

class ReverbEffect
{
public:
    ReverbEffect() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        reverb.setSampleRate(currentSampleRate);
        reverb.reset();

        preDelayBufferL.assign((int)(currentSampleRate * 0.1), 0.0f);
        preDelayBufferR.assign((int)(currentSampleRate * 0.1), 0.0f);
        preDelayWrite = 0;

        scratchL.assign(4096, 0.0f);
        scratchR.assign(4096, 0.0f);
    }

    void reset()
    {
        reverb.reset();
        std::fill(preDelayBufferL.begin(), preDelayBufferL.end(), 0.0f);
        std::fill(preDelayBufferR.begin(), preDelayBufferR.end(), 0.0f);
        preDelayWrite = 0;
        filterStateL = 0.0f;
        filterStateR = 0.0f;
    }

    void process(float* left, float* right, int numSamples,
                 ReverbFxType type, float decaySec, float damping, float tone, float mix)
    {
        if (mix <= 0.001f)
            return;

        decaySec = std::clamp(decaySec, 0.1f, 10.0f);
        damping = std::clamp(damping, 0.0f, 1.0f);
        tone = std::clamp(tone, 0.0f, 1.0f);
        mix = std::clamp(mix, 0.0f, 1.0f);

        juce::Reverb::Parameters params;

        switch (type)
        {
            case ReverbFxType::Plate:
                // Fast diffusion, bright, wide
                params.roomSize = std::clamp(0.4f + decaySec * 0.06f, 0.3f, 0.95f);
                params.damping = damping * 0.4f; // less damping = brighter plate
                params.width = 1.0f;
                params.dryLevel = 0.0f; // we handle wet/dry mix externally
                params.wetLevel = 1.0f;
                params.freezeMode = 0.0f;
                break;

            case ReverbFxType::Room:
                // Moderate size, warm damping
                params.roomSize = std::clamp(0.2f + decaySec * 0.05f, 0.2f, 0.75f);
                params.damping = 0.3f + damping * 0.5f;
                params.width = 0.7f;
                params.dryLevel = 0.0f;
                params.wetLevel = 1.0f;
                params.freezeMode = 0.0f;
                break;

            case ReverbFxType::Hall:
                // Large, lush, deep
                params.roomSize = std::clamp(0.6f + decaySec * 0.04f, 0.5f, 0.98f);
                params.damping = 0.2f + damping * 0.4f;
                params.width = 1.0f;
                params.dryLevel = 0.0f;
                params.wetLevel = 1.0f;
                params.freezeMode = 0.0f;
                break;
        }

        reverb.setParameters(params);

        // Pre-delay for Hall and Plate
        int preDelaySamples = 0;
        if (type == ReverbFxType::Hall)
            preDelaySamples = (int)(currentSampleRate * 0.04); // 40ms
        else if (type == ReverbFxType::Plate)
            preDelaySamples = (int)(currentSampleRate * 0.01); // 10ms

        if (scratchL.size() < (size_t)numSamples)
        {
            scratchL.resize((size_t)numSamples);
            scratchR.resize((size_t)numSamples);
        }


        int bufLen = (int)preDelayBufferL.size();
        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            preDelayBufferL[preDelayWrite] = inL;
            preDelayBufferR[preDelayWrite] = inR;

            int readPos = preDelayWrite - preDelaySamples;
            if (readPos < 0) readPos += bufLen;

            scratchL[i] = preDelayBufferL[readPos];
            scratchR[i] = preDelayBufferR[readPos];

            preDelayWrite = (preDelayWrite + 1) % bufLen;
        }

        // Run through stereo reverb engine
        reverb.processStereo(scratchL.data(), scratchR.data(), numSamples);

        // Tone filter (1-pole lowpass/high-frequency tilt on wet)
        float cutoff = 1000.0f + tone * 15000.0f;
        float alpha = 1.0f - std::exp(-6.2831853f * cutoff / (float)currentSampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            filterStateL += alpha * (scratchL[i] - filterStateL);
            filterStateR += alpha * (scratchR[i] - filterStateR);

            float wetL = filterStateL;
            float wetR = filterStateR;

            left[i] = (1.0f - mix) * left[i] + mix * wetL;
            right[i] = (1.0f - mix) * right[i] + mix * wetR;
        }
    }

private:
    double currentSampleRate = 44100.0;
    juce::Reverb reverb;
    std::vector<float> preDelayBufferL;
    std::vector<float> preDelayBufferR;
    int preDelayWrite = 0;

    std::vector<float> scratchL;
    std::vector<float> scratchR;

    float filterStateL = 0.0f;
    float filterStateR = 0.0f;
};
