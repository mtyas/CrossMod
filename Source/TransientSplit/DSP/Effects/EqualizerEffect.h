#pragma once

#include "BaseEffect.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

enum class EqMode
{
    Parametric5Band = 0,
    Graphic = 1,
    Tilt = 2
};

class EqualizerEffect : public BaseEffect
{
public:
    EqualizerEffect() = default;

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        juce::dsp::ProcessSpec spec { currentSampleRate, static_cast<juce::uint32>(maxBlockSize), 2 };

        lowShelfFilter.prepare(spec);
        mid1Filter.prepare(spec);
        mid2Filter.prepare(spec);
        mid3Filter.prepare(spec);
        highShelfFilter.prepare(spec);
        tiltLowFilter.prepare(spec);
        tiltHighFilter.prepare(spec);

        reset();
    }

    void reset() override
    {
        lowShelfFilter.reset();
        mid1Filter.reset();
        mid2Filter.reset();
        mid3Filter.reset();
        highShelfFilter.reset();
        tiltLowFilter.reset();
        tiltHighFilter.reset();
    }

    void setParameters(float p1, float p2, float p3, float p4, float mix) override
    {
        // P1: Mode (0: 5-Band, 1: Graphic, 2: Tilt)
        const int modeInt = std::clamp(static_cast<int>(std::floor(p1 * 2.99f)), 0, 2);
        mode = static_cast<EqMode>(modeInt);

        // P2: Low Band / Tilt Slope (-15dB to +15dB)
        lowGainDb = -15.0f + std::clamp(p2, 0.0f, 1.0f) * 30.0f;

        // P3: Mid Gain (-15dB to +15dB)
        midGainDb = -15.0f + std::clamp(p3, 0.0f, 1.0f) * 30.0f;

        // P4: High Band (-15dB to +15dB)
        highGainDb = -15.0f + std::clamp(p4, 0.0f, 1.0f) * 30.0f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        updateCoefficients();
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        // Prepare temporary JUCE buffer
        float* channels[2] = { bufferL, bufferR ? bufferR : bufferL };
        juce::AudioBuffer<float> temp(channels, bufferR ? 2 : 1, numSamples);
        juce::AudioBuffer<float> dryCopy;
        dryCopy.makeCopyOf(temp);

        juce::dsp::AudioBlock<float> block(temp);
        juce::dsp::ProcessContextReplacing<float> context(block);

        if (mode == EqMode::Parametric5Band || mode == EqMode::Graphic)
        {
            lowShelfFilter.process(context);
            mid1Filter.process(context);
            mid2Filter.process(context);
            mid3Filter.process(context);
            highShelfFilter.process(context);
        }
        else if (mode == EqMode::Tilt)
        {
            tiltLowFilter.process(context);
            tiltHighFilter.process(context);
        }

        // Mix Wet/Dry
        for (int ch = 0; ch < temp.getNumChannels(); ++ch)
        {
            float* out = (ch == 0) ? bufferL : bufferR;
            const float* wet = temp.getReadPointer(ch);
            const float* dry = dryCopy.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                out[i] = dry[i] * (1.0f - wetMix) + wet[i] * wetMix;
            }
        }
    }

private:
    void updateCoefficients()
    {
        const float sr = static_cast<float>(currentSampleRate);

        if (mode == EqMode::Parametric5Band || mode == EqMode::Graphic)
        {
            // Low Shelf (100 Hz)
            *lowShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sr, 100.0f, 0.707f, std::pow(10.0f, lowGainDb * 0.05f));

            // Low-Mid Peak (350 Hz)
            *mid1Filter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sr, 350.0f, 1.0f, std::pow(10.0f, (lowGainDb * 0.4f + midGainDb * 0.6f) * 0.05f));

            // Mid Peak (1200 Hz)
            *mid2Filter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sr, 1200.0f, 1.2f, std::pow(10.0f, midGainDb * 0.05f));

            // High-Mid Peak (3500 Hz)
            *mid3Filter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sr, 3500.0f, 1.0f, std::pow(10.0f, (midGainDb * 0.4f + highGainDb * 0.6f) * 0.05f));

            // High Shelf (8500 Hz)
            *highShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sr, 8500.0f, 0.707f, std::pow(10.0f, highGainDb * 0.05f));
        }
        else if (mode == EqMode::Tilt)
        {
            // Tilt around 1kHz pivot: boost highs/cut lows or boost lows/cut highs
            const float tiltSlope = lowGainDb; // -15 to +15 dB
            const float lowSlopeDb = -tiltSlope;
            const float highSlopeDb = tiltSlope;

            *tiltLowFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sr, 800.0f, 0.5f, std::pow(10.0f, lowSlopeDb * 0.05f));

            *tiltHighFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sr, 1200.0f, 0.5f, std::pow(10.0f, highSlopeDb * 0.05f));
        }
    }

    double currentSampleRate = 48000.0;

    EqMode mode = EqMode::Parametric5Band;
    float lowGainDb = 0.0f;
    float midGainDb = 0.0f;
    float highGainDb = 0.0f;
    float wetMix = 1.0f;

    using FilterType = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;

    FilterType lowShelfFilter;
    FilterType mid1Filter;
    FilterType mid2Filter;
    FilterType mid3Filter;
    FilterType highShelfFilter;
    FilterType tiltLowFilter;
    FilterType tiltHighFilter;
};
