#pragma once

#include "ModulationEffect.h"
#include "DelayEffect.h"
#include "ReverbEffect.h"

enum class FxOrder
{
    Mod_Dly_Rvb = 0,
    Dly_Mod_Rvb = 1,
    Mod_Rvb_Dly = 2,
    Rvb_Dly_Mod = 3
};

class FxChain
{
public:
    FxChain() = default;

    void prepare(double sampleRate)
    {
        modEffect.prepare(sampleRate);
        delayEffect.prepare(sampleRate);
        reverbEffect.prepare(sampleRate);
    }

    void reset()
    {
        modEffect.reset();
        delayEffect.reset();
        reverbEffect.reset();
    }

    void setDelayBpmAndSync(double bpm, bool isSynced, int syncRateIndex)
    {
        delayEffect.setBpmAndSync(bpm, isSynced, syncRateIndex);
    }

    void process(float* left, float* right, int numSamples,
                 FxOrder order,
                 // Modulation parameters
                 ModFxType modType, float modRate, float modDepth, float modFeedback, float modMix,
                 // Delay parameters
                 DelayFxType delayType, float delayTime, float delayFeedback, float delayTone, float delayMix,
                 // Reverb parameters
                 ReverbFxType reverbType, float reverbDecay, float reverbDamping, float reverbTone, float reverbMix)
    {
        auto runMod = [&]() {
            modEffect.process(left, right, numSamples, modType, modRate, modDepth, modFeedback, modMix);
        };

        auto runDly = [&]() {
            delayEffect.process(left, right, numSamples, delayType, delayTime, delayFeedback, delayTone, delayMix);
        };

        auto runRvb = [&]() {
            reverbEffect.process(left, right, numSamples, reverbType, reverbDecay, reverbDamping, reverbTone, reverbMix);
        };

        switch (order)
        {
            case FxOrder::Mod_Dly_Rvb:
                runMod();
                runDly();
                runRvb();
                break;

            case FxOrder::Dly_Mod_Rvb:
                runDly();
                runMod();
                runRvb();
                break;

            case FxOrder::Mod_Rvb_Dly:
                runMod();
                runRvb();
                runDly();
                break;

            case FxOrder::Rvb_Dly_Mod:
                runRvb();
                runDly();
                runMod();
                break;
        }
    }

private:
    ModulationEffect modEffect;
    DelayEffect delayEffect;
    ReverbEffect reverbEffect;
};
