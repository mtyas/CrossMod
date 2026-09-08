#pragma once

#include "ModLFO.h"
#include "ModEnvelopeFollower.h"
#include "ModTuringMachine.h"
#include "ModMatrix.h"
#include <array>

class ModEngine
{
public:
    ModEngine() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        lfo1.prepare(currentSampleRate);
        lfo2.prepare(currentSampleRate);
        lfo3.prepare(currentSampleRate);
        env1.prepare(currentSampleRate);
        env2.prepare(currentSampleRate);
        env3.prepare(currentSampleRate);
        turing1.prepare(currentSampleRate);
        turing2.prepare(currentSampleRate);
        reset();
    }

    void reset()
    {
        lfo1.reset();
        lfo2.reset();
        lfo3.reset();
        env1.reset();
        env2.reset();
        env3.reset();
        turing1.reset();
        turing2.reset();
        offsets.fill(0.0f);
    }

    void setBpm(double bpm)
    {
        lfo1.setBpm(bpm);
        lfo2.setBpm(bpm);
        lfo3.setBpm(bpm);
        turing1.setBpm(bpm);
        turing2.setBpm(bpm);
    }

    void syncToHostPosition(double ppqPosition, bool isPlaying)
    {
        lfo1.syncToHostPosition(ppqPosition);
        lfo2.syncToHostPosition(ppqPosition);
        lfo3.syncToHostPosition(ppqPosition);
        turing1.syncToHostPosition(ppqPosition, isPlaying);
        turing2.syncToHostPosition(ppqPosition, isPlaying);
    }

    ModLFO& getLFO1() noexcept { return lfo1; }
    ModLFO& getLFO2() noexcept { return lfo2; }
    ModLFO& getLFO3() noexcept { return lfo3; }
    ModEnvelopeFollower& getEnv1() noexcept { return env1; }
    ModEnvelopeFollower& getEnv2() noexcept { return env2; }
    ModEnvelopeFollower& getEnv3() noexcept { return env3; }
    ModTuringMachine& getTuring1() noexcept { return turing1; }
    ModTuringMachine& getTuring2() noexcept { return turing2; }
    ModMatrix& getMatrix() noexcept { return matrix; }

    void setMacros(float m1, float m2, float m3, float m4) noexcept
    {
        macro1 = std::clamp(m1, 0.0f, 1.0f);
        macro2 = std::clamp(m2, 0.0f, 1.0f);
        macro3 = std::clamp(m3, 0.0f, 1.0f);
        macro4 = std::clamp(m4, 0.0f, 1.0f);
    }

    void setXYPad1(float x, float y) noexcept
    {
        xyPad1X = std::clamp(x, -1.0f, 1.0f);
        xyPad1Y = std::clamp(y, -1.0f, 1.0f);
    }

    void setXYPad2(float x, float y) noexcept
    {
        xyPad2X = std::clamp(x, -1.0f, 1.0f);
        xyPad2Y = std::clamp(y, -1.0f, 1.0f);
    }

    void processBlock(const float* inL, const float* inR,
                      const float* transL, const float* transR,
                      const float* sustL, const float* sustR,
                      const float* f1L, const float* f1R,
                      const float* f2L, const float* f2R,
                      const float* f3L, const float* f3R,
                      const float* leftL, const float* leftR,
                      const float* rightL, const float* rightR,
                      const float* midL, const float* midR,
                      const float* sideL, const float* sideR,
                      const float* dynHighL, const float* dynHighR,
                      const float* dynLowL, const float* dynLowR,
                      const float* harmL, const float* harmR,
                      const float* noiseL, const float* noiseR,
                      const float* inPhaseL, const float* inPhaseR,
                      const float* spatialL, const float* spatialR,
                      int numSamples)
    {
        if (numSamples <= 0) return;

        float vLfo1 = 0.0f, vLfo2 = 0.0f, vLfo3 = 0.0f;
        float vEnv1 = 0.0f, vEnv2 = 0.0f, vEnv3 = 0.0f;
        float vTur1 = 0.0f, vTur2 = 0.0f;
        float transEnv = 0.0f, sustEnv = 0.0f;
        float f1Env = 0.0f, f2Env = 0.0f, f3Env = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            auto getSampleMono = [i](const float* bufL, const float* bufR, float fallback) -> float {
                if (bufL && bufR) return 0.5f * (bufL[i] + bufR[i]);
                if (bufL) return bufL[i];
                return fallback;
            };

            const float audioIn      = 0.5f * (inL[i] + (inR ? inR[i] : inL[i]));
            const float audioTrans   = getSampleMono(transL, transR, audioIn);
            const float audioSust    = getSampleMono(sustL, sustR, audioIn);
            const float audioF1      = getSampleMono(f1L, f1R, audioIn);
            const float audioF2      = getSampleMono(f2L, f2R, audioIn);
            const float audioF3      = getSampleMono(f3L, f3R, audioIn);
            const float audioLeft    = leftL ? leftL[i] : audioIn;
            const float audioRight   = rightL ? rightL[i] : audioIn;
            const float audioMid     = getSampleMono(midL, midR, audioIn);
            const float audioSide    = getSampleMono(sideL, sideR, audioIn);
            const float audioDynHigh = getSampleMono(dynHighL, dynHighR, audioIn);
            const float audioDynLow  = getSampleMono(dynLowL, dynLowR, audioIn);
            const float audioHarm    = getSampleMono(harmL, harmR, audioIn);
            const float audioNoise   = getSampleMono(noiseL, noiseR, audioIn);
            const float audioInPhase = getSampleMono(inPhaseL, inPhaseR, audioIn);
            const float audioSpatial = getSampleMono(spatialL, spatialR, audioIn);

            // 1. Advance LFOs
            vLfo1 = lfo1.getNextSample();
            vLfo2 = lfo2.getNextSample();
            vLfo3 = lfo3.getNextSample();

            // 2. Advance Envelope Followers
            auto getEnvSource = [&](ModEnvelopeFollower& env) -> float
            {
                switch (env.getSource())
                {
                    case EnvFollowerSource::Transient:   return audioTrans;
                    case EnvFollowerSource::Sustain:     return audioSust;
                    case EnvFollowerSource::Filter1:     return audioF1;
                    case EnvFollowerSource::Filter2:     return audioF2;
                    case EnvFollowerSource::Filter3:     return audioF3;
                    case EnvFollowerSource::Left:        return audioLeft;
                    case EnvFollowerSource::Right:       return audioRight;
                    case EnvFollowerSource::Mid:         return audioMid;
                    case EnvFollowerSource::Side:        return audioSide;
                    case EnvFollowerSource::DynamicHigh: return audioDynHigh;
                    case EnvFollowerSource::DynamicLow:  return audioDynLow;
                    case EnvFollowerSource::Harmonic:    return audioHarm;
                    case EnvFollowerSource::Noise:       return audioNoise;
                    case EnvFollowerSource::InPhase:     return audioInPhase;
                    case EnvFollowerSource::Spatial:     return audioSpatial;
                    default:                             return audioIn;
                }
            };

            vEnv1 = env1.processSample(getEnvSource(env1));
            vEnv2 = env2.processSample(getEnvSource(env2));
            vEnv3 = env3.processSample(getEnvSource(env3));

            // 3. Advance Turing Machines
            vTur1 = turing1.getNextSample();
            vTur2 = turing2.getNextSample();

            // 4. Track live audio levels
            transEnv = std::max(transEnv, std::abs(audioTrans));
            sustEnv  = std::max(sustEnv, std::abs(audioSust));
            f1Env    = std::max(f1Env, std::abs(audioF1));
            f2Env    = std::max(f2Env, std::abs(audioF2));
            f3Env    = std::max(f3Env, std::abs(audioF3));
        }

        // 5. Update array of source values (22 sources)
        std::array<float, ModMatrix::NumSources> srcVals = {
            0.0f,
            vLfo1,
            vLfo2,
            vLfo3,
            vEnv1,
            vEnv2,
            vEnv3,
            vTur1,
            vTur2,
            macro1,
            macro2,
            macro3,
            macro4,
            xyPad1X,
            xyPad1Y,
            xyPad2X,
            xyPad2Y,
            transEnv,
            sustEnv,
            f1Env,
            f2Env,
            f3Env
        };

        // 6. Compute destination offsets
        matrix.calculateOffsets(srcVals, offsets);
    }

    float getOffset(ModDest dest) const noexcept
    {
        const int idx = static_cast<int>(dest);
        if (idx >= 0 && idx < ModMatrix::NumDestinations)
            return offsets[idx];
        return 0.0f;
    }

    // Live state queries for UI visualizers
    float getLFO1Val() const noexcept { return lfo1.getCurrentValue(); }
    float getLFO2Val() const noexcept { return lfo2.getCurrentValue(); }
    float getLFO3Val() const noexcept { return lfo3.getCurrentValue(); }
    float getEnv1Val() const noexcept { return env1.getCurrentValue(); }
    float getEnv2Val() const noexcept { return env2.getCurrentValue(); }
    float getEnv3Val() const noexcept { return env3.getCurrentValue(); }
    float getTuring1Val() const noexcept { return turing1.getCurrentVoltage(); }
    float getTuring2Val() const noexcept { return turing2.getCurrentVoltage(); }
    uint32_t getTuring1Reg() const noexcept { return turing1.getShiftRegister(); }
    uint32_t getTuring2Reg() const noexcept { return turing2.getShiftRegister(); }
    int getTuring1Step() const noexcept { return turing1.getCurrentStep(); }
    int getTuring2Step() const noexcept { return turing2.getCurrentStep(); }
    float getSlotLiveOutput(int index) const noexcept { return matrix.getSlotLiveOutput(index); }
    float getMacro1() const noexcept { return macro1; }
    float getMacro2() const noexcept { return macro2; }
    float getMacro3() const noexcept { return macro3; }
    float getMacro4() const noexcept { return macro4; }
    float getXYPad1X() const noexcept { return xyPad1X; }
    float getXYPad1Y() const noexcept { return xyPad1Y; }
    float getXYPad2X() const noexcept { return xyPad2X; }
    float getXYPad2Y() const noexcept { return xyPad2Y; }

private:
    double currentSampleRate = 48000.0;

    ModLFO lfo1;
    ModLFO lfo2;
    ModLFO lfo3;
    ModEnvelopeFollower env1;
    ModEnvelopeFollower env2;
    ModEnvelopeFollower env3;
    ModTuringMachine turing1;
    ModTuringMachine turing2;

    float macro1 = 0.0f;
    float macro2 = 0.0f;
    float macro3 = 0.0f;
    float macro4 = 0.0f;

    float xyPad1X = 0.0f;
    float xyPad1Y = 0.0f;
    float xyPad2X = 0.0f;
    float xyPad2Y = 0.0f;

    ModMatrix matrix;
    std::array<float, ModMatrix::NumDestinations> offsets;
};
