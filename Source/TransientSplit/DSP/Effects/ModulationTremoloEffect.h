#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <algorithm>

class ModulationTremoloEffect : public BaseEffect
{
public:
    ModulationTremoloEffect() = default;

    EffectType getType() const override { return EffectType::ModulationTremolo; }
    juce::String getName() const override { return "Tremolo / Auto-Pan"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        lfoPhase = 0.0f;
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free Hz, >0 = Synced subdivisions 1/32..2 Bars)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        float rateHz = 0.2f * std::pow(100.0f, std::clamp(p1, 0.0f, 1.0f));
        if (syncSec > 0.0)
            rateHz = static_cast<float>(1.0 / syncSec);

        lfoInc = static_cast<float>(6.283185307179586 * rateHz / currentSampleRate);

        // P2: Depth (0% to 100%)
        depth = std::clamp(p2, 0.0f, 1.0f);

        // P3: Shape (0.0 Sine, 0.5 Triangle, 1.0 Square)
        shape = std::clamp(p3, 0.0f, 1.0f);

        // P4: Stereo Phase (0 to 180 degrees)
        stereoPhase = std::clamp(p4, 0.0f, 1.0f) * 3.14159265f;

        wetMix = std::clamp(mix, 0.0f, 1.0f);
    }

    void process(float* bufL, float* bufR, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufL[i];
            const float dryR = bufR ? bufR[i] : dryL;

            lfoPhase += lfoInc;
            if (lfoPhase >= 6.2831853f) lfoPhase -= 6.2831853f;

            auto calcWave = [this](float ph) -> float
            {
                float normPhase = ph / 6.2831853f;
                normPhase -= std::floor(normPhase);

                // Duty cycle skew
                if (normPhase < dutyCycle)
                    normPhase = (normPhase / dutyCycle) * 0.5f;
                else
                    normPhase = 0.5f + ((normPhase - dutyCycle) / (1.0f - dutyCycle)) * 0.5f;

                const float s = std::sin(normPhase * 6.2831853f);
                if (shape <= 0.5f)
                {
                    const float tri = (std::asin(s) * 0.63661977f);
                    const float frac = shape * 2.0f;
                    return (1.0f - frac) * s + frac * tri;
                }
                else
                {
                    const float tri = (std::asin(s) * 0.63661977f);
                    const float sq = std::tanh(s * 6.0f);
                    const float frac = (shape - 0.5f) * 2.0f;
                    return (1.0f - frac) * tri + frac * sq;
                }
            };

            const float modL = 1.0f - depth * 0.5f * (1.0f - calcWave(lfoPhase));
            const float modR = 1.0f - depth * 0.5f * (1.0f - calcWave(lfoPhase + stereoPhase));

            const float wetL = dryL * modL;
            const float wetR = dryR * modR;

            bufL[i] = dryL * (1.0f - wetMix) + wetL * wetMix;
            if (bufR)
                bufR[i] = dryR * (1.0f - wetMix) + wetR * wetMix;
        }
    }

private:
    double currentSampleRate = 48000.0;
    float lfoPhase = 0.0f;
    float lfoInc = 0.0f;
    float depth = 0.8f;
    float shape = 0.0f;
    float stereoPhase = 0.0f;
    float dutyCycle = 0.5f;
    float wetMix = 1.0f;
};
