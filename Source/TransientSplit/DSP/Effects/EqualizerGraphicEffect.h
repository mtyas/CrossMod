#pragma once

#include "BaseEffect.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

class EqualizerGraphicEffect : public BaseEffect
{
public:
    EqualizerGraphicEffect() = default;

    EffectType getType() const override { return EffectType::EqualizerGraphic; }
    juce::String getName() const override { return "Graphic 5-Band EQ"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        juce::dsp::ProcessSpec spec { currentSampleRate, static_cast<juce::uint32>(maxBlockSize), 2 };

        b1Filter.prepare(spec);
        b2Filter.prepare(spec);
        b3Filter.prepare(spec);
        b4Filter.prepare(spec);
        b5Filter.prepare(spec);

        reset();
    }

    void reset() override
    {
        b1Filter.reset();
        b2Filter.reset();
        b3Filter.reset();
        b4Filter.reset();
        b5Filter.reset();
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // 5 Fixed Musical Bands: 100Hz, 350Hz, 1.2kHz, 4kHz, 10kHz Air (-15dB to +15dB)
        g1 = -15.0f + std::clamp(p1, 0.0f, 1.0f) * 30.0f;
        g2 = -15.0f + std::clamp(p2, 0.0f, 1.0f) * 30.0f;
        g3 = -15.0f + std::clamp(p3, 0.0f, 1.0f) * 30.0f;
        g4 = -15.0f + std::clamp(p4, 0.0f, 1.0f) * 30.0f;
        g5 = -15.0f + std::clamp(p5, 0.0f, 1.0f) * 30.0f;

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

        b1Filter.process(context);
        b2Filter.process(context);
        b3Filter.process(context);
        b4Filter.process(context);
        b5Filter.process(context);

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

        *b1Filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, 100.0f, 0.707f, std::pow(10.0f, g1 * 0.05f));

        *b2Filter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, 350.0f, 1.0f, std::pow(10.0f, g2 * 0.05f));

        *b3Filter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, 1200.0f, 1.0f, std::pow(10.0f, g3 * 0.05f));

        *b4Filter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, 4000.0f, 1.0f, std::pow(10.0f, g4 * 0.05f));

        *b5Filter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, 10000.0f, 0.707f, std::pow(10.0f, g5 * 0.05f));
    }

    float g1 = 0.0f, g2 = 0.0f, g3 = 0.0f, g4 = 0.0f, g5 = 0.0f;
    float wetMix = 1.0f;

    using FilterType = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    FilterType b1Filter;
    FilterType b2Filter;
    FilterType b3Filter;
    FilterType b4Filter;
    FilterType b5Filter;
};
