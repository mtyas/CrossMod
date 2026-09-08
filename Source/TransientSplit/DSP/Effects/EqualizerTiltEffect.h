#pragma once

#include "BaseEffect.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

class EqualizerTiltEffect : public BaseEffect
{
public:
    EqualizerTiltEffect() = default;

    EffectType getType() const override { return EffectType::EqualizerTilt; }
    juce::String getName() const override { return "Tilt EQ"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        juce::dsp::ProcessSpec spec { currentSampleRate, static_cast<juce::uint32>(maxBlockSize), 2 };

        tiltLowFilter.prepare(spec);
        tiltHighFilter.prepare(spec);
        airFilter.prepare(spec);
        subFilter.prepare(spec);
        midFilter.prepare(spec);

        reset();
    }

    void reset() override
    {
        tiltLowFilter.reset();
        tiltHighFilter.reset();
        airFilter.reset();
        subFilter.reset();
        midFilter.reset();
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Tilt Slope (-15dB to +15dB)
        tiltSlope = -15.0f + std::clamp(p1, 0.0f, 1.0f) * 30.0f;

        // P2: Pivot Frequency (300Hz to 3000Hz)
        pivotFreq = 300.0f * std::pow(10.0f, std::clamp(p2, 0.0f, 1.0f));

        // P3: Air Boost (0dB to +12dB, 12kHz shelf)
        airGainDb = std::clamp(p3, 0.0f, 1.0f) * 12.0f;

        // P4: Sub Warmth (0dB to +12dB, 60Hz shelf)
        subGainDb = std::clamp(p4, 0.0f, 1.0f) * 12.0f;

        // P5: Mid Contour (-10dB to +10dB at 1.5kHz)
        midGainDb = -10.0f + std::clamp(p5, 0.0f, 1.0f) * 20.0f;

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

        tiltLowFilter.process(context);
        tiltHighFilter.process(context);
        airFilter.process(context);
        subFilter.process(context);
        midFilter.process(context);

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

        const float lowSlopeDb = -tiltSlope;
        const float highSlopeDb = tiltSlope;

        *tiltLowFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, pivotFreq * 0.7f, 0.5f, std::pow(10.0f, lowSlopeDb * 0.05f));

        *tiltHighFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, pivotFreq * 1.3f, 0.5f, std::pow(10.0f, highSlopeDb * 0.05f));

        *airFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, 12000.0f, 0.707f, std::pow(10.0f, airGainDb * 0.05f));

        *subFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, 60.0f, 0.707f, std::pow(10.0f, subGainDb * 0.05f));

        *midFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, 1500.0f, 1.0f, std::pow(10.0f, midGainDb * 0.05f));
    }

    float tiltSlope = 0.0f;
    float pivotFreq = 1000.0f;
    float airGainDb = 0.0f;
    float subGainDb = 0.0f;
    float midGainDb = 0.0f;
    float wetMix = 1.0f;

    using FilterType = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    FilterType tiltLowFilter;
    FilterType tiltHighFilter;
    FilterType airFilter;
    FilterType subFilter;
    FilterType midFilter;
};
