#pragma once

#include <cmath>
#include <algorithm>

enum class EnvFollowerSource
{
    Input = 0,
    Transient = 1,
    Sustain = 2,
    Filter1 = 3,
    Filter2 = 4,
    Filter3 = 5,
    Left = 6,
    Right = 7,
    Mid = 8,
    Side = 9,
    DynamicHigh = 10,
    DynamicLow = 11,
    Harmonic = 12,
    Noise = 13,
    InPhase = 14,
    Spatial = 15
};

class ModEnvelopeFollower
{
public:
    ModEnvelopeFollower() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset()
    {
        envelope = 0.0f;
    }

    void setParameters(EnvFollowerSource src, float attackMs, float releaseMs, float sensitivity)
    {
        source = src;
        attackTimeMs = std::clamp(attackMs, 0.5f, 300.0f);
        releaseTimeMs = std::clamp(releaseMs, 5.0f, 1500.0f);
        gainLinear = std::clamp(sensitivity, 0.1f, 5.0f);

        const double attSec = attackTimeMs * 0.001;
        const double relSec = releaseTimeMs * 0.001;

        alphaAttack = static_cast<float>(std::exp(-1.0 / (attSec * currentSampleRate)));
        alphaRelease = static_cast<float>(std::exp(-1.0 / (relSec * currentSampleRate)));
    }

    EnvFollowerSource getSource() const noexcept { return source; }

    float processSample(float inputSample)
    {
        const float absIn = std::abs(inputSample) * gainLinear;

        if (absIn > envelope)
            envelope = alphaAttack * envelope + (1.0f - alphaAttack) * absIn;
        else
            envelope = alphaRelease * envelope + (1.0f - alphaRelease) * absIn;

        return std::clamp(envelope, 0.0f, 1.0f);
    }

    float getCurrentValue() const noexcept { return std::clamp(envelope, 0.0f, 1.0f); }

private:
    double currentSampleRate = 48000.0;
    EnvFollowerSource source = EnvFollowerSource::Input;

    float attackTimeMs = 10.0f;
    float releaseTimeMs = 120.0f;
    float gainLinear = 1.0f;

    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;
    float envelope = 0.0f;
};
