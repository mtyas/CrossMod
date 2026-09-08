#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <juce_audio_basics/juce_audio_basics.h>

enum class FilterBankType
{
    LowPass12 = 0,
    LowPass24,
    HighPass12,
    HighPass24,
    BandPass,
    PeakBell,
    Notch,
    LowShelf,
    HighShelf
};

struct FilterBankChannel
{
    FilterBankType type = FilterBankType::LowPass12;
    float freqHz = 1000.0f;
    float q = 0.707f;
    float gainDb = 0.0f;
    float drive = 1.0f;
    bool bypass = false;
    bool solo = false;
    bool mute = false;

    // ZDF SVF states for 2 cascade stages (for 12dB / 24dB)
    float s1_1L = 0.0f, s2_1L = 0.0f;
    float s1_1R = 0.0f, s2_1R = 0.0f;
    float s1_2L = 0.0f, s2_2L = 0.0f;
    float s1_2R = 0.0f, s2_2R = 0.0f;

    void reset()
    {
        s1_1L = 0.0f; s2_1L = 0.0f;
        s1_1R = 0.0f; s2_1R = 0.0f;
        s1_2L = 0.0f; s2_2L = 0.0f;
        s1_2R = 0.0f; s2_2R = 0.0f;
    }
};

class FilterBank3
{
public:
    FilterBank3()
    {
        channels[0].type = FilterBankType::LowPass24;
        channels[0].freqHz = 350.0f;
        channels[0].q = 0.707f;

        channels[1].type = FilterBankType::BandPass;
        channels[1].freqHz = 1500.0f;
        channels[1].q = 1.2f;

        channels[2].type = FilterBankType::HighPass24;
        channels[2].freqHz = 4500.0f;
        channels[2].q = 0.707f;
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        currentBlockSize = std::max(64, maxBlockSize);

        for (auto& ch : channels)
            ch.reset();
    }

    void reset()
    {
        for (auto& ch : channels)
            ch.reset();
    }

    void setFilterParameters(int bandIndex, FilterBankType type, float freqHz, float q, float gainDb, float drive, bool bypass, bool solo, bool mute)
    {
        if (bandIndex >= 0 && bandIndex < 3)
        {
            auto& ch = channels[bandIndex];
            ch.type = type;
            ch.freqHz = std::clamp(freqHz, 20.0f, 20000.0f);
            ch.q = std::clamp(q, 0.1f, 10.0f);
            ch.gainDb = std::clamp(gainDb, -18.0f, 18.0f);
            ch.drive = std::clamp(drive, 1.0f, 4.0f);
            ch.bypass = bypass;
            ch.solo = solo;
            ch.mute = mute;
        }
    }

    FilterBankChannel& getChannel(int bandIndex) noexcept
    {
        return channels[std::clamp(bandIndex, 0, 2)];
    }

    const FilterBankChannel& getChannel(int bandIndex) const noexcept
    {
        return channels[std::clamp(bandIndex, 0, 2)];
    }

    void process(const float* inL, const float* inR, int numSamples,
                 float* out1L, float* out1R,
                 float* out2L, float* out2R,
                 float* out3L, float* out3R)
    {
        if (numSamples <= 0 || inL == nullptr) return;

        const bool hasSolo = channels[0].solo || channels[1].solo || channels[2].solo;

        float* outL[3] = { out1L, out2L, out3L };
        float* outR[3] = { out1R, out2R, out3R };

        for (int b = 0; b < 3; ++b)
        {
            auto& ch = channels[b];

            // If muted or (someone is solo'd and this one isn't solo'd)
            if (ch.mute || (hasSolo && !ch.solo))
            {
                if (outL[b]) std::fill(outL[b], outL[b] + numSamples, 0.0f);
                if (outR[b]) std::fill(outR[b], outR[b] + numSamples, 0.0f);
                continue;
            }

            if (ch.bypass)
            {
                if (outL[b]) std::copy(inL, inL + numSamples, outL[b]);
                if (outR[b])
                {
                    if (inR) std::copy(inR, inR + numSamples, outR[b]);
                    else std::copy(inL, inL + numSamples, outR[b]);
                }
                continue;
            }

            // ZDF SVF processing for this band
            const double sr = currentSampleRate;
            const double w = std::clamp(static_cast<double>(ch.freqHz), 20.0, sr * 0.48);
            const float g = static_cast<float>(std::tan(3.141592653589793 * w / sr));
            const float R = 1.0f / (2.0f * ch.q);
            const float kGain = std::pow(10.0f, ch.gainDb * 0.05f);
            const float driveAmt = ch.drive;

            const bool is24dB = (ch.type == FilterBankType::LowPass24 || ch.type == FilterBankType::HighPass24);

            for (int i = 0; i < numSamples; ++i)
            {
                float xL = inL[i];
                float xR = inR ? inR[i] : xL;

                // Drive saturation
                if (driveAmt > 1.05f)
                {
                    xL = std::tanh(xL * driveAmt) / std::sqrt(driveAmt);
                    xR = std::tanh(xR * driveAmt) / std::sqrt(driveAmt);
                }

                // Stage 1
                float yL = processSVF(xL, ch.s1_1L, ch.s2_1L, g, R, ch.type, kGain);
                float yR = processSVF(xR, ch.s1_1R, ch.s2_1R, g, R, ch.type, kGain);

                // Stage 2 (for 24dB slopes)
                if (is24dB)
                {
                    yL = processSVF(yL, ch.s1_2L, ch.s2_2L, g, R, ch.type, kGain);
                    yR = processSVF(yR, ch.s1_2R, ch.s2_2R, g, R, ch.type, kGain);
                }

                if (outL[b]) outL[b][i] = yL;
                if (outR[b]) outR[b][i] = yR;
            }
        }
    }

    // Evaluate magnitude response for GUI curve display
    float getMagnitudeForFrequency(int bandIndex, double freq) const
    {
        if (bandIndex < 0 || bandIndex >= 3) return 1.0f;
        const auto& ch = channels[bandIndex];
        if (ch.bypass || ch.mute) return 1.0f;

        const double w = 2.0 * 3.141592653589793 * freq / currentSampleRate;
        const double wa = 2.0 * currentSampleRate * std::tan(w * 0.5);
        const double w0 = 2.0 * 3.141592653589793 * ch.freqHz;
        const double q = ch.q;
        const double A = std::pow(10.0, ch.gainDb / 40.0);

        // Analog prototype approximation magnitude
        const double s_mag = wa;
        const double denom = std::sqrt(std::pow(w0 * w0 - s_mag * s_mag, 2.0) + std::pow(s_mag * w0 / q, 2.0));
        if (denom < 1e-9) return 1.0f;

        double mag = 1.0;
        switch (ch.type)
        {
            case FilterBankType::LowPass12:
                mag = (w0 * w0) / denom;
                break;
            case FilterBankType::LowPass24:
                mag = std::pow((w0 * w0) / denom, 2.0);
                break;
            case FilterBankType::HighPass12:
                mag = (s_mag * s_mag) / denom;
                break;
            case FilterBankType::HighPass24:
                mag = std::pow((s_mag * s_mag) / denom, 2.0);
                break;
            case FilterBankType::BandPass:
                mag = (s_mag * w0 / q) / denom;
                break;
            case FilterBankType::Notch:
                mag = std::abs(w0 * w0 - s_mag * s_mag) / denom;
                break;
            case FilterBankType::PeakBell:
                mag = std::sqrt(std::pow(w0 * w0 - s_mag * s_mag, 2.0) + std::pow(s_mag * w0 * A / q, 2.0)) / denom;
                break;
            case FilterBankType::LowShelf:
                mag = (s_mag < w0) ? A : 1.0;
                break;
            case FilterBankType::HighShelf:
                mag = (s_mag > w0) ? A : 1.0;
                break;
        }

        return static_cast<float>(mag);
    }

private:
    inline float processSVF(float input, float& s1, float& s2, float g, float R, FilterBankType type, float kGain) noexcept
    {
        const float satS1 = std::tanh(s1);

        const float denom = 1.0f + 2.0f * R * g + g * g;
        const float hp = (input - 2.0f * R * satS1 - g * satS1 - s2) / denom;
        const float bp = g * hp + satS1;
        const float lp = g * bp + s2;

        s1 = 2.0f * bp - satS1;
        s2 = 2.0f * lp - s2;

        if (std::abs(s1) < 1e-15f) s1 = 0.0f;
        if (std::abs(s2) < 1e-15f) s2 = 0.0f;

        switch (type)
        {
            case FilterBankType::LowPass12:
            case FilterBankType::LowPass24:
                return lp;

            case FilterBankType::HighPass12:
            case FilterBankType::HighPass24:
                return hp;

            case FilterBankType::BandPass:
                return bp;

            case FilterBankType::Notch:
                return hp + lp;

            case FilterBankType::PeakBell:
                return hp + lp + bp * kGain;

            case FilterBankType::LowShelf:
                return hp + lp * kGain;

            case FilterBankType::HighShelf:
                return hp * kGain + lp;
        }

        return lp;
    }

    double currentSampleRate = 48000.0;
    int currentBlockSize = 512;
    std::array<FilterBankChannel, 3> channels;
};
