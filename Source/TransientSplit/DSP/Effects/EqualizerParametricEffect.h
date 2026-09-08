#pragma once

#include "BaseEffect.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

class EqualizerParametricEffect : public BaseEffect
{
public:
    EqualizerParametricEffect() = default;

    EffectType getType() const override { return EffectType::EqualizerParametric; }
    juce::String getName() const override { return "Parametric 3-Band EQ"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        juce::dsp::ProcessSpec spec { currentSampleRate, static_cast<juce::uint32>(maxBlockSize), 2 };

        lowFilter.prepare(spec);
        midFilter.prepare(spec);
        highFilter.prepare(spec);

        reset();
    }

    void reset() override
    {
        lowFilter.reset();
        midFilter.reset();
        highFilter.reset();
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Low Shelf Gain (-18dB to +18dB, 100Hz)
        lowGainDb = -18.0f + std::clamp(p1, 0.0f, 1.0f) * 36.0f;

        // P2: Mid Center Frequency (200Hz to 6000Hz)
        midFreq = 200.0f * std::pow(30.0f, std::clamp(p2, 0.0f, 1.0f));

        // P3: Mid Gain (-18dB to +18dB)
        midGainDb = -18.0f + std::clamp(p3, 0.0f, 1.0f) * 36.0f;

        // P4: Mid Q / Bandwidth (0.3 to 6.0)
        midQ = 0.3f + std::clamp(p4, 0.0f, 1.0f) * 5.7f;

        // P5: High Shelf Gain (-18dB to +18dB, 8500Hz)
        highGainDb = -18.0f + std::clamp(p5, 0.0f, 1.0f) * 36.0f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        updateCoefficients();
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        float* channels[2] = { bufferL, bufferR ? bufferR : bufferL };
        juce::AudioBuffer<float> temp(channels, bufferR ? 2 : 1, numSamples);
        juce::AudioBuffer<float> dryCopy;
        dryCopy.makeCopyOf(temp);

        juce::dsp::AudioBlock<float> block(temp);
        juce::dsp::ProcessContextReplacing<float> context(block);

        lowFilter.process(context);
        midFilter.process(context);
        highFilter.process(context);

        for (int ch = 0; ch < temp.getNumChannels(); ++ch)
        {
            float* out = (ch == 0) ? bufferL : bufferR;
            const float* wet = temp.getReadPointer(ch);
            const float* dry = dryCopy.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
                out[i] = dry[i] * (1.0f - wetMix) + wet[i] * wetMix;
        }
    }

private:
    void updateCoefficients()
    {
        const float sr = static_cast<float>(currentSampleRate);

        *lowFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, 100.0f, 0.707f, std::pow(10.0f, lowGainDb * 0.05f));

        *midFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, midFreq, midQ, std::pow(10.0f, midGainDb * 0.05f));

        *highFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, 8500.0f, 0.707f, std::pow(10.0f, highGainDb * 0.05f));
    }

    float lowGainDb = 0.0f;
    float midFreq = 1200.0f;
    float midGainDb = 0.0f;
    float midQ = 1.2f;
    float highGainDb = 0.0f;
    float wetMix = 1.0f;

    using FilterType = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    FilterType lowFilter;
    FilterType midFilter;
    FilterType highFilter;
};
