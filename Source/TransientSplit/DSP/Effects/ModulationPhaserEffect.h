#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <array>
#include <algorithm>

class ModulationPhaserEffect : public BaseEffect
{
public:
    ModulationPhaserEffect() = default;

    EffectType getType() const override { return EffectType::ModulationPhaser; }
    juce::String getName() const override { return "4-Stage Phaser"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        lfoPhase = 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
        allpassStateL.fill(0.0f);
        allpassStateR.fill(0.0f);
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P5: Tempo Sync (0 = Free Hz, >0 = Synced subdivisions 1/16..4 Bars)
        const double syncSec = TempoSyncHelper::getSubdivisionSeconds(p5, currentBpm);
        float rateHz = 0.05f + std::clamp(p1, 0.0f, 1.0f) * 7.95f;
        if (syncSec > 0.0)
            rateHz = static_cast<float>(1.0 / syncSec);

        lfoInc = static_cast<float>(6.283185307179586 * rateHz / currentSampleRate);

        // P2: Depth (0% to 100%)
        depth = std::clamp(p2, 0.0f, 1.0f);

        // P3: Center Range (200Hz to 4000Hz)
        centerFreq = 200.0f * std::pow(20.0f, std::clamp(p3, 0.0f, 1.0f));

        // P4: Feedback (0% to 88%)
        feedbackAmount = std::clamp(p4, 0.0f, 0.88f);

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

            const float modL = (std::sin(lfoPhase) + 1.0f) * 0.5f;
            const float modR = (std::sin(lfoPhase + 1.5707963f) + 1.0f) * 0.5f;

            auto processPhaser = [this](float in, float mod, float& fbStore, std::array<float, 4>& states) -> float
            {
                const float currentF = centerFreq * std::pow(4.0f, mod * depth);
                const float w = static_cast<float>(3.141592653589793 * currentF / currentSampleRate);
                const float a1 = (1.0f - w) / (1.0f + w);

                float sig = in + fbStore * feedbackAmount;
                for (int s = 0; s < 4; ++s)
                {
                    float y = -a1 * sig + states[s];
                    states[s] = sig + a1 * y;
                    sig = y;
                }

                fbStore = sig;
                return (in + sig) * 0.5f;
            };

            const float wetL = processPhaser(dryL, modL, feedbackL, allpassStateL);
            const float wetR = processPhaser(dryR, modR, feedbackR, allpassStateR);

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
    float centerFreq = 800.0f;
    float feedbackAmount = 0.5f;
    float poleSpread = 1.0f;
    float wetMix = 1.0f;

    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
    std::array<float, 4> allpassStateL { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> allpassStateR { 0.0f, 0.0f, 0.0f, 0.0f };
};
