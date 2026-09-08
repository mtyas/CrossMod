#pragma once

#include "Effects/BaseEffect.h"
#include "Effects/CompressorVCAEffect.h"
#include "Effects/CompressorOptoEffect.h"
#include "Effects/CompressorFETEffect.h"
#include "Effects/GateEffect.h"
#include "Effects/EqualizerParametricEffect.h"
#include "Effects/EqualizerGraphicEffect.h"
#include "Effects/EqualizerTiltEffect.h"
#include "Effects/FilterDualHPLPEffect.h"
#include "Effects/FilterBandpassEffect.h"
#include "Effects/DistortionHardClipEffect.h"
#include "Effects/DistortionTubeEffect.h"
#include "Effects/DistortionWavefolderEffect.h"
#include "Effects/FuzzGermaniumEffect.h"
#include "Effects/FuzzOctaveEffect.h"
#include "Effects/OverdriveEffect.h"
#include "Effects/TapeEffect.h"
#include "Effects/BitcrushEffect.h"
#include "Effects/GlitchStutterEffect.h"
#include "Effects/GlitchTapeStopEffect.h"
#include "Effects/GlitchReverseEffect.h"
#include "Effects/GlitchGranularEffect.h"
#include "Effects/PitchShifterEffect.h"
#include "Effects/FrequencyShifterEffect.h"
#include "Effects/ModulationChorusEffect.h"
#include "Effects/ModulationFlangerEffect.h"
#include "Effects/ModulationPhaserEffect.h"
#include "Effects/ModulationTremoloEffect.h"
#include "Effects/DelayStereoEffect.h"
#include "Effects/DelayPingPongEffect.h"
#include "Effects/DelayTapeEchoEffect.h"
#include "Effects/ReverbRoomEffect.h"
#include "Effects/ReverbHallEffect.h"
#include "Effects/ReverbPlateEffect.h"
#include "Effects/ReverbSpringEffect.h"
#include "Effects/ReverbShimmerEffect.h"
#include <array>
#include <memory>

class EffectSlot
{
public:
    EffectSlot()
    {
        // 1. Dynamics
        compVCA = std::make_unique<CompressorVCAEffect>();
        compOpto = std::make_unique<CompressorOptoEffect>();
        compFET = std::make_unique<CompressorFETEffect>();
        gate = std::make_unique<GateEffect>();

        // 2. EQ & Filter
        eqParam = std::make_unique<EqualizerParametricEffect>();
        eqGraph = std::make_unique<EqualizerGraphicEffect>();
        eqTilt = std::make_unique<EqualizerTiltEffect>();
        filterHPLP = std::make_unique<FilterDualHPLPEffect>();
        filterBP = std::make_unique<FilterBandpassEffect>();

        // 3. Distortion & Saturation
        distHard = std::make_unique<DistortionHardClipEffect>();
        distTube = std::make_unique<DistortionTubeEffect>();
        distWave = std::make_unique<DistortionWavefolderEffect>();
        fuzzGerm = std::make_unique<FuzzGermaniumEffect>();
        fuzzOct = std::make_unique<FuzzOctaveEffect>();
        overdrive = std::make_unique<OverdriveEffect>();
        tapeSat = std::make_unique<TapeEffect>();

        // 4. Lo-Fi & Glitch
        bitcrush = std::make_unique<BitcrushEffect>();
        glitchStutter = std::make_unique<GlitchStutterEffect>();
        glitchTapeStop = std::make_unique<GlitchTapeStopEffect>();
        glitchReverse = std::make_unique<GlitchReverseEffect>();
        glitchGranular = std::make_unique<GlitchGranularEffect>();

        // 5. Pitch & Modulation
        pitchShifter = std::make_unique<PitchShifterEffect>();
        freqShifter = std::make_unique<FrequencyShifterEffect>();
        modChorus = std::make_unique<ModulationChorusEffect>();
        modFlanger = std::make_unique<ModulationFlangerEffect>();
        modPhaser = std::make_unique<ModulationPhaserEffect>();
        modTremolo = std::make_unique<ModulationTremoloEffect>();

        // 6. Delay
        delayStereo = std::make_unique<DelayStereoEffect>();
        delayPingPong = std::make_unique<DelayPingPongEffect>();
        delayTapeEcho = std::make_unique<DelayTapeEchoEffect>();

        // 7. Reverb
        revRoom = std::make_unique<ReverbRoomEffect>();
        revHall = std::make_unique<ReverbHallEffect>();
        revPlate = std::make_unique<ReverbPlateEffect>();
        revSpring = std::make_unique<ReverbSpringEffect>();
        revShimmer = std::make_unique<ReverbShimmerEffect>();
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        compVCA->prepare(sampleRate, maxBlockSize);
        compOpto->prepare(sampleRate, maxBlockSize);
        compFET->prepare(sampleRate, maxBlockSize);
        gate->prepare(sampleRate, maxBlockSize);

        eqParam->prepare(sampleRate, maxBlockSize);
        eqGraph->prepare(sampleRate, maxBlockSize);
        eqTilt->prepare(sampleRate, maxBlockSize);
        filterHPLP->prepare(sampleRate, maxBlockSize);
        filterBP->prepare(sampleRate, maxBlockSize);

        distHard->prepare(sampleRate, maxBlockSize);
        distTube->prepare(sampleRate, maxBlockSize);
        distWave->prepare(sampleRate, maxBlockSize);
        fuzzGerm->prepare(sampleRate, maxBlockSize);
        fuzzOct->prepare(sampleRate, maxBlockSize);
        overdrive->prepare(sampleRate, maxBlockSize);
        tapeSat->prepare(sampleRate, maxBlockSize);

        bitcrush->prepare(sampleRate, maxBlockSize);
        glitchStutter->prepare(sampleRate, maxBlockSize);
        glitchTapeStop->prepare(sampleRate, maxBlockSize);
        glitchReverse->prepare(sampleRate, maxBlockSize);
        glitchGranular->prepare(sampleRate, maxBlockSize);

        pitchShifter->prepare(sampleRate, maxBlockSize);
        freqShifter->prepare(sampleRate, maxBlockSize);
        modChorus->prepare(sampleRate, maxBlockSize);
        modFlanger->prepare(sampleRate, maxBlockSize);
        modPhaser->prepare(sampleRate, maxBlockSize);
        modTremolo->prepare(sampleRate, maxBlockSize);

        delayStereo->prepare(sampleRate, maxBlockSize);
        delayPingPong->prepare(sampleRate, maxBlockSize);
        delayTapeEcho->prepare(sampleRate, maxBlockSize);

        revRoom->prepare(sampleRate, maxBlockSize);
        revHall->prepare(sampleRate, maxBlockSize);
        revPlate->prepare(sampleRate, maxBlockSize);
        revSpring->prepare(sampleRate, maxBlockSize);
        revShimmer->prepare(sampleRate, maxBlockSize);
    }

    void reset()
    {
        compVCA->reset();
        compOpto->reset();
        compFET->reset();
        gate->reset();

        eqParam->reset();
        eqGraph->reset();
        eqTilt->reset();
        filterHPLP->reset();
        filterBP->reset();

        distHard->reset();
        distTube->reset();
        distWave->reset();
        fuzzGerm->reset();
        fuzzOct->reset();
        overdrive->reset();
        tapeSat->reset();

        bitcrush->reset();
        glitchStutter->reset();
        glitchTapeStop->reset();
        glitchReverse->reset();
        glitchGranular->reset();

        pitchShifter->reset();
        freqShifter->reset();
        modChorus->reset();
        modFlanger->reset();
        modPhaser->reset();
        modTremolo->reset();

        delayStereo->reset();
        delayPingPong->reset();
        delayTapeEcho->reset();

        revRoom->reset();
        revHall->reset();
        revPlate->reset();
        revSpring->reset();
        revShimmer->reset();
    }

    void setBpm(double bpm)
    {
        compVCA->setBpm(bpm);
        compOpto->setBpm(bpm);
        compFET->setBpm(bpm);
        gate->setBpm(bpm);

        eqParam->setBpm(bpm);
        eqGraph->setBpm(bpm);
        eqTilt->setBpm(bpm);
        filterHPLP->setBpm(bpm);
        filterBP->setBpm(bpm);

        distHard->setBpm(bpm);
        distTube->setBpm(bpm);
        distWave->setBpm(bpm);
        fuzzGerm->setBpm(bpm);
        fuzzOct->setBpm(bpm);
        overdrive->setBpm(bpm);
        tapeSat->setBpm(bpm);

        bitcrush->setBpm(bpm);
        glitchStutter->setBpm(bpm);
        glitchTapeStop->setBpm(bpm);
        glitchReverse->setBpm(bpm);
        glitchGranular->setBpm(bpm);

        pitchShifter->setBpm(bpm);
        freqShifter->setBpm(bpm);
        modChorus->setBpm(bpm);
        modFlanger->setBpm(bpm);
        modPhaser->setBpm(bpm);
        modTremolo->setBpm(bpm);

        delayStereo->setBpm(bpm);
        delayPingPong->setBpm(bpm);
        delayTapeEcho->setBpm(bpm);

        revRoom->setBpm(bpm);
        revHall->setBpm(bpm);
        revPlate->setBpm(bpm);
        revSpring->setBpm(bpm);
        revShimmer->setBpm(bpm);
    }

    void setType(EffectType newType)
    {
        currentType = newType;
    }

    EffectType getType() const noexcept { return currentType; }

    juce::String getName() const
    {
        return getEffectName(currentType);
    }

    static juce::String getEffectName(EffectType type)
    {
        switch (type)
        {
            case EffectType::Bypass:                return "Bypass";
            case EffectType::CompressorVCA:         return "VCA Comp";
            case EffectType::CompressorOpto:        return "Opto Comp";
            case EffectType::CompressorFET:         return "FET Comp";
            case EffectType::Gate:                  return "Noise Gate";
            case EffectType::EqualizerParametric:   return "Param EQ";
            case EffectType::EqualizerGraphic:      return "Graphic EQ";
            case EffectType::EqualizerTilt:         return "Tilt EQ";
            case EffectType::FilterDualHPLP:        return "HP/LP Filter";
            case EffectType::FilterBandpass:        return "BP/Notch";
            case EffectType::DistortionHardClip:    return "Hard Clip";
            case EffectType::DistortionTube:        return "Tube Sat";
            case EffectType::DistortionWavefolder:  return "Wavefolder";
            case EffectType::FuzzGermanium:         return "Germ Fuzz";
            case EffectType::FuzzOctave:            return "Octave Fuzz";
            case EffectType::Overdrive:             return "Overdrive";
            case EffectType::TapeSat:               return "Tape Sat";
            case EffectType::Bitcrush:              return "Bitcrush";
            case EffectType::GlitchStutter:         return "Stutter";
            case EffectType::GlitchTapeStop:        return "Tape Stop";
            case EffectType::GlitchReverse:         return "Reverse";
            case EffectType::GlitchGranular:        return "Granular";
            case EffectType::PitchShifter:          return "Pitch Shift";
            case EffectType::FrequencyShifter:      return "Bode Shift";
            case EffectType::ModulationChorus:      return "Chorus";
            case EffectType::ModulationFlanger:     return "Flanger";
            case EffectType::ModulationPhaser:      return "Phaser";
            case EffectType::ModulationTremolo:     return "Tremolo";
            case EffectType::DelayStereo:           return "Stereo Delay";
            case EffectType::DelayPingPong:         return "Ping-Pong";
            case EffectType::DelayTapeEcho:         return "Tape Echo";
            case EffectType::ReverbRoom:            return "Room Reverb";
            case EffectType::ReverbHall:            return "Hall Reverb";
            case EffectType::ReverbPlate:           return "Plate Reverb";
            case EffectType::ReverbSpring:          return "Spring Reverb";
            case EffectType::ReverbShimmer:         return "Shimmer";
            default:                                return "Effect";
        }
    }

    void setBypassed(bool bypass) noexcept { isBypassed = bypass; }
    bool getBypassed() const noexcept { return isBypassed; }

    void setLevels(float inGainDb, float outGainDb) noexcept
    {
        inGainLinear = (inGainDb <= -23.5f) ? 0.0f : std::pow(10.0f, inGainDb * 0.05f);
        outGainLinear = (outGainDb <= -23.5f) ? 0.0f : std::pow(10.0f, outGainDb * 0.05f);
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix)
    {
        auto* eff = getActiveEffect();
        if (eff != nullptr)
            eff->setParameters(p1, p2, p3, p4, p5, mix);
    }

    void process(float* left, float* right, int numSamples)
    {
        if (numSamples <= 0) return;

        if (inGainLinear != 1.0f)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                left[i] *= inGainLinear;
                if (right) right[i] *= inGainLinear;
            }
        }

        if (!isBypassed && currentType != EffectType::Bypass)
        {
            auto* eff = getActiveEffect();
            if (eff != nullptr)
                eff->process(left, right, numSamples);
        }

        if (outGainLinear != 1.0f)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                left[i] *= outGainLinear;
                if (right) right[i] *= outGainLinear;
            }
        }
    }

    BaseEffect* getActiveEffect()
    {
        switch (currentType)
        {
            case EffectType::CompressorVCA:     return compVCA.get();
            case EffectType::CompressorOpto:    return compOpto.get();
            case EffectType::CompressorFET:     return compFET.get();
            case EffectType::Gate:              return gate.get();

            case EffectType::EqualizerParametric: return eqParam.get();
            case EffectType::EqualizerGraphic:    return eqGraph.get();
            case EffectType::EqualizerTilt:       return eqTilt.get();
            case EffectType::FilterDualHPLP:      return filterHPLP.get();
            case EffectType::FilterBandpass:      return filterBP.get();

            case EffectType::DistortionHardClip:   return distHard.get();
            case EffectType::DistortionTube:       return distTube.get();
            case EffectType::DistortionWavefolder: return distWave.get();
            case EffectType::FuzzGermanium:        return fuzzGerm.get();
            case EffectType::FuzzOctave:           return fuzzOct.get();
            case EffectType::Overdrive:            return overdrive.get();
            case EffectType::TapeSat:              return tapeSat.get();

            case EffectType::Bitcrush:          return bitcrush.get();
            case EffectType::GlitchStutter:     return glitchStutter.get();
            case EffectType::GlitchTapeStop:    return glitchTapeStop.get();
            case EffectType::GlitchReverse:     return glitchReverse.get();
            case EffectType::GlitchGranular:    return glitchGranular.get();

            case EffectType::PitchShifter:      return pitchShifter.get();
            case EffectType::FrequencyShifter:  return freqShifter.get();
            case EffectType::ModulationChorus:  return modChorus.get();
            case EffectType::ModulationFlanger: return modFlanger.get();
            case EffectType::ModulationPhaser:  return modPhaser.get();
            case EffectType::ModulationTremolo: return modTremolo.get();

            case EffectType::DelayStereo:       return delayStereo.get();
            case EffectType::DelayPingPong:     return delayPingPong.get();
            case EffectType::DelayTapeEcho:     return delayTapeEcho.get();

            case EffectType::ReverbRoom:        return revRoom.get();
            case EffectType::ReverbHall:        return revHall.get();
            case EffectType::ReverbPlate:       return revPlate.get();
            case EffectType::ReverbSpring:      return revSpring.get();
            case EffectType::ReverbShimmer:     return revShimmer.get();

            case EffectType::Bypass:
            default:                            return nullptr;
        }
    }

    static EffectParamInfo getParamInfoForType(EffectType type, int paramIndex)
    {
        switch (type)
        {
            // 1. Dynamics
            case EffectType::CompressorVCA:
                switch (paramIndex)
                {
                    case 0: return { "Thresh", "dB", 0.55f, 0.0f, 1.0f };
                    case 1: return { "Ratio", ":1", 0.25f, 0.0f, 1.0f };
                    case 2: return { "Attack", "ms", 0.2f, 0.0f, 1.0f };
                    case 3: return { "Release", "ms", 0.3f, 0.0f, 1.0f };
                    case 4: return { "Knee / Gain", "dB", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::CompressorOpto:
                switch (paramIndex)
                {
                    case 0: return { "Peak Reduct", "%", 0.45f, 0.0f, 1.0f };
                    case 1: return { "Gain", "dB", 0.35f, 0.0f, 1.0f };
                    case 2: return { "Response", "%", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Warmth", "%", 0.2f, 0.0f, 1.0f };
                    case 4: return { "HF Focus", "%", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::CompressorFET:
                switch (paramIndex)
                {
                    case 0: return { "Drive", "dB", 0.4f, 0.0f, 1.0f };
                    case 1: return { "Ratio / All", "", 0.3f, 0.0f, 1.0f };
                    case 2: return { "Fast Attack", "us", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Fast Release", "ms", 0.5f, 0.0f, 1.0f };
                    case 4: return { "Punch Level", "%", 0.5f, 0.0f, 1.0f };
                }
                break;

            case EffectType::Gate:
                switch (paramIndex)
                {
                    case 0: return { "Threshold", "dB", 0.3f, 0.0f, 1.0f };
                    case 1: return { "Attack", "ms", 0.1f, 0.0f, 1.0f };
                    case 2: return { "Release", "ms", 0.3f, 0.0f, 1.0f };
                    case 3: return { "Floor", "dB", 0.2f, 0.0f, 1.0f };
                    case 4: return { "Hysteresis", "dB", 0.25f, 0.0f, 1.0f };
                }
                break;

            // 2. EQ & Filter
            case EffectType::EqualizerParametric:
                switch (paramIndex)
                {
                    case 0: return { "Low Gain", "dB", 0.5f, 0.0f, 1.0f };
                    case 1: return { "Mid Freq", "Hz", 0.5f, 0.0f, 1.0f };
                    case 2: return { "Mid Gain", "dB", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Mid Q Width", "Q", 0.2f, 0.0f, 1.0f };
                    case 4: return { "High Gain", "dB", 0.5f, 0.0f, 1.0f };
                }
                break;

            case EffectType::EqualizerGraphic:
                switch (paramIndex)
                {
                    case 0: return { "100 Hz", "dB", 0.5f, 0.0f, 1.0f };
                    case 1: return { "350 Hz", "dB", 0.5f, 0.0f, 1.0f };
                    case 2: return { "1.2 kHz", "dB", 0.5f, 0.0f, 1.0f };
                    case 3: return { "4.0 kHz", "dB", 0.5f, 0.0f, 1.0f };
                    case 4: return { "10 kHz Air", "dB", 0.5f, 0.0f, 1.0f };
                }
                break;

            case EffectType::EqualizerTilt:
                switch (paramIndex)
                {
                    case 0: return { "Tilt Slope", "dB", 0.5f, 0.0f, 1.0f };
                    case 1: return { "Pivot Freq", "Hz", 0.5f, 0.0f, 1.0f };
                    case 2: return { "Air Boost", "dB", 0.0f, 0.0f, 1.0f };
                    case 3: return { "Sub Warmth", "dB", 0.0f, 0.0f, 1.0f };
                    case 4: return { "Mid Contour", "dB", 0.5f, 0.0f, 1.0f };
                }
                break;

            case EffectType::FilterDualHPLP:
                switch (paramIndex)
                {
                    case 0: return { "LP Cutoff", "Hz", 1.0f, 0.0f, 1.0f };
                    case 1: return { "LP Reso", "Q", 0.2f, 0.0f, 1.0f };
                    case 2: return { "HP Cutoff", "Hz", 0.0f, 0.0f, 1.0f };
                    case 3: return { "HP Reso", "Q", 0.2f, 0.0f, 1.0f };
                    case 4: return { "Drive Warmth", "dB", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::FilterBandpass:
                switch (paramIndex)
                {
                    case 0: return { "Center Freq", "Hz", 0.5f, 0.0f, 1.0f };
                    case 1: return { "Resonance Q", "", 0.3f, 0.0f, 1.0f };
                    case 2: return { "Notch Depth", "%", 0.0f, 0.0f, 1.0f };
                    case 3: return { "Drive", "%", 0.0f, 0.0f, 1.0f };
                    case 4: return { "Stereo Spread", "%", 0.5f, 0.0f, 1.0f };
                }
                break;

            // 3. Distortion & Saturation
            case EffectType::DistortionHardClip:
                switch (paramIndex)
                {
                    case 0: return { "Drive", "%", 0.4f, 0.0f, 1.0f };
                    case 1: return { "Tone", "Hz", 0.7f, 0.0f, 1.0f };
                    case 2: return { "Asymmetry", "%", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Bass Boost", "dB", 0.0f, 0.0f, 1.0f };
                    case 4: return { "Out Level", "%", 0.6f, 0.0f, 1.0f };
                }
                break;

            case EffectType::DistortionTube:
                switch (paramIndex)
                {
                    case 0: return { "Drive", "%", 0.35f, 0.0f, 1.0f };
                    case 1: return { "Bias Warmth", "%", 0.4f, 0.0f, 1.0f };
                    case 2: return { "Sag Dynamic", "%", 0.3f, 0.0f, 1.0f };
                    case 3: return { "High Cut", "Hz", 0.8f, 0.0f, 1.0f };
                    case 4: return { "Out Level", "%", 0.6f, 0.0f, 1.0f };
                }
                break;

            case EffectType::DistortionWavefolder:
                switch (paramIndex)
                {
                    case 0: return { "Drive", "%", 0.3f, 0.0f, 1.0f };
                    case 1: return { "Fold Stages", "", 0.4f, 0.0f, 1.0f };
                    case 2: return { "Asymmetry", "%", 0.0f, 0.0f, 1.0f };
                    case 3: return { "Tone Filter", "Hz", 0.8f, 0.0f, 1.0f };
                    case 4: return { "Fold Warmth", "%", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::FuzzGermanium:
                switch (paramIndex)
                {
                    case 0: return { "Fuzz Gain", "%", 0.55f, 0.0f, 1.0f };
                    case 1: return { "Voltage Bias", "%", 0.8f, 0.0f, 1.0f };
                    case 2: return { "Tone", "Hz", 0.65f, 0.0f, 1.0f };
                    case 3: return { "Body", "%", 0.5f, 0.0f, 1.0f };
                    case 4: return { "Out Level", "%", 0.6f, 0.0f, 1.0f };
                }
                break;

            case EffectType::FuzzOctave:
                switch (paramIndex)
                {
                    case 0: return { "Fuzz Gain", "%", 0.6f, 0.0f, 1.0f };
                    case 1: return { "Octave Blend", "%", 0.7f, 0.0f, 1.0f };
                    case 2: return { "Tone", "Hz", 0.6f, 0.0f, 1.0f };
                    case 3: return { "Rectify Peak", "%", 0.5f, 0.0f, 1.0f };
                    case 4: return { "Out Level", "%", 0.6f, 0.0f, 1.0f };
                }
                break;

            case EffectType::Overdrive:
                switch (paramIndex)
                {
                    case 0: return { "Drive", "%", 0.4f, 0.0f, 1.0f };
                    case 1: return { "Tone Focus", "Hz", 0.6f, 0.0f, 1.0f };
                    case 2: return { "Mid Push", "%", 0.4f, 0.0f, 1.0f };
                    case 3: return { "Warmth", "%", 0.5f, 0.0f, 1.0f };
                    case 4: return { "Out Level", "%", 0.6f, 0.0f, 1.0f };
                }
                break;

            case EffectType::TapeSat:
                switch (paramIndex)
                {
                    case 0: return { "Tape Drive", "%", 0.4f, 0.0f, 1.0f };
                    case 1: return { "Warmth", "%", 0.5f, 0.0f, 1.0f };
                    case 2: return { "Flutter", "%", 0.2f, 0.0f, 1.0f };
                    case 3: return { "Tape Age", "%", 0.15f, 0.0f, 1.0f };
                    case 4: return { "Head Sat Bias", "%", 0.3f, 0.0f, 1.0f };
                }
                break;

            // 4. Lo-Fi & Glitch
            case EffectType::Bitcrush:
                switch (paramIndex)
                {
                    case 0: return { "Bit Depth", "bit", 0.9f, 0.0f, 1.0f };
                    case 1: return { "Downsample", "kHz", 0.85f, 0.0f, 1.0f };
                    case 2: return { "Jitter Drive", "%", 0.0f, 0.0f, 1.0f };
                    case 3: return { "Post LP Tone", "Hz", 0.8f, 0.0f, 1.0f };
                    case 4: return { "Anti-Alias", "%", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::GlitchStutter:
                switch (paramIndex)
                {
                    case 0: return { "Slice Div", "", 0.25f, 0.0f, 1.0f };
                    case 1: return { "Trigger Prob", "%", 0.6f, 0.0f, 1.0f };
                    case 2: return { "Burst Count", "", 0.3f, 0.0f, 1.0f };
                    case 3: return { "Decay Feedb", "%", 0.8f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::GlitchTapeStop:
                switch (paramIndex)
                {
                    case 0: return { "Trigger Rate", "bar", 0.4f, 0.0f, 1.0f };
                    case 1: return { "Stop Time", "ms", 0.5f, 0.0f, 1.0f };
                    case 2: return { "Curve Slew", "", 0.3f, 0.0f, 1.0f };
                    case 3: return { "LP Drop", "Hz", 0.6f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "bar", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::GlitchReverse:
                switch (paramIndex)
                {
                    case 0: return { "Slice Length", "", 0.25f, 0.0f, 1.0f };
                    case 1: return { "Crossfade", "%", 0.3f, 0.0f, 1.0f };
                    case 2: return { "Jitter Time", "%", 0.1f, 0.0f, 1.0f };
                    case 3: return { "Feedback", "%", 0.0f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::GlitchGranular:
                switch (paramIndex)
                {
                    case 0: return { "Grain Size", "ms", 0.3f, 0.0f, 1.0f };
                    case 1: return { "Density Jit", "%", 0.4f, 0.0f, 1.0f };
                    case 2: return { "Pitch Drift", "%", 0.2f, 0.0f, 1.0f };
                    case 3: return { "Spread Width", "%", 0.5f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            // 5. Pitch & Modulation
            case EffectType::PitchShifter:
                switch (paramIndex)
                {
                    case 0: return { "Semitones", "st", 0.5f, 0.0f, 1.0f };
                    case 1: return { "Fine Cents", "ct", 0.5f, 0.0f, 1.0f };
                    case 2: return { "Grain Window", "ms", 0.4f, 0.0f, 1.0f };
                    case 3: return { "Feedback", "%", 0.0f, 0.0f, 1.0f };
                    case 4: return { "Stereo Detune", "ct", 0.5f, 0.0f, 1.0f };
                }
                break;

            case EffectType::FrequencyShifter:
                switch (paramIndex)
                {
                    case 0: return { "Coarse Shift", "Hz", 0.5f, 0.0f, 1.0f };
                    case 1: return { "Fine Shift", "Hz", 0.5f, 0.0f, 1.0f };
                    case 2: return { "Stereo Mode", "", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Feedback", "%", 0.0f, 0.0f, 1.0f };
                    case 4: return { "Filter Tone", "Hz", 0.85f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ModulationChorus:
                switch (paramIndex)
                {
                    case 0: return { "Rate", "Hz", 0.25f, 0.0f, 1.0f };
                    case 1: return { "Depth", "%", 0.6f, 0.0f, 1.0f };
                    case 2: return { "Stereo Width", "deg", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Pre-Delay", "ms", 0.2f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ModulationFlanger:
                switch (paramIndex)
                {
                    case 0: return { "Rate", "Hz", 0.2f, 0.0f, 1.0f };
                    case 1: return { "Depth", "%", 0.7f, 0.0f, 1.0f };
                    case 2: return { "Manual Delay", "ms", 0.3f, 0.0f, 1.0f };
                    case 3: return { "Feedback Res", "%", 0.75f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ModulationPhaser:
                switch (paramIndex)
                {
                    case 0: return { "Rate", "Hz", 0.2f, 0.0f, 1.0f };
                    case 1: return { "Depth", "%", 0.75f, 0.0f, 1.0f };
                    case 2: return { "Center Range", "Hz", 0.4f, 0.0f, 1.0f };
                    case 3: return { "Feedback Res", "%", 0.5f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ModulationTremolo:
                switch (paramIndex)
                {
                    case 0: return { "Rate", "Hz", 0.35f, 0.0f, 1.0f };
                    case 1: return { "Depth", "%", 0.7f, 0.0f, 1.0f };
                    case 2: return { "Wave Shape", "", 0.0f, 0.0f, 1.0f };
                    case 3: return { "Stereo Phase", "deg", 0.0f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            // 6. Delay
            case EffectType::DelayStereo:
                switch (paramIndex)
                {
                    case 0: return { "Delay Time", "ms", 0.35f, 0.0f, 1.0f };
                    case 1: return { "Feedback", "%", 0.0f, 0.0f, 1.0f };
                    case 2: return { "Stereo Offset", "%", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Damp Filter", "Hz", 0.8f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::DelayPingPong:
                switch (paramIndex)
                {
                    case 0: return { "Delay Time", "ms", 0.35f, 0.0f, 1.0f };
                    case 1: return { "Feedback", "%", 0.0f, 0.0f, 1.0f };
                    case 2: return { "Cross Width", "%", 1.0f, 0.0f, 1.0f };
                    case 3: return { "Damp Filter", "Hz", 0.8f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::DelayTapeEcho:
                switch (paramIndex)
                {
                    case 0: return { "Echo Time", "ms", 0.35f, 0.0f, 1.0f };
                    case 1: return { "Feedback", "%", 0.0f, 0.0f, 1.0f };
                    case 2: return { "Wow & Flutter", "%", 0.35f, 0.0f, 1.0f };
                    case 3: return { "Tape Age Tone", "%", 0.3f, 0.0f, 1.0f };
                    case 4: return { "Tempo Sync", "div", 0.0f, 0.0f, 1.0f };
                }
                break;

            // 7. Reverb
            case EffectType::ReverbRoom:
                switch (paramIndex)
                {
                    case 0: return { "Decay Time", "s", 0.4f, 0.0f, 1.0f };
                    case 1: return { "Room Size", "%", 0.6f, 0.0f, 1.0f };
                    case 2: return { "Absorption", "%", 0.5f, 0.0f, 1.0f };
                    case 3: return { "Early Bounces", "%", 0.7f, 0.0f, 1.0f };
                    case 4: return { "Stereo Width", "%", 1.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ReverbHall:
                switch (paramIndex)
                {
                    case 0: return { "Decay Time", "s", 0.75f, 0.0f, 1.0f };
                    case 1: return { "Hall Scale", "%", 0.8f, 0.0f, 1.0f };
                    case 2: return { "Air Damping", "Hz", 0.4f, 0.0f, 1.0f };
                    case 3: return { "Pre-Delay", "ms", 0.15f, 0.0f, 1.0f };
                    case 4: return { "Envelopment", "%", 1.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ReverbPlate:
                switch (paramIndex)
                {
                    case 0: return { "Decay Time", "s", 0.6f, 0.0f, 1.0f };
                    case 1: return { "Steel Density", "%", 0.75f, 0.0f, 1.0f };
                    case 2: return { "Plate Sparkle", "%", 0.8f, 0.0f, 1.0f };
                    case 3: return { "Bass Cut", "Hz", 0.2f, 0.0f, 1.0f };
                    case 4: return { "Pickup Width", "%", 1.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ReverbSpring:
                switch (paramIndex)
                {
                    case 0: return { "Decay Time", "s", 0.55f, 0.0f, 1.0f };
                    case 1: return { "Chirp Dispers", "%", 0.7f, 0.0f, 1.0f };
                    case 2: return { "Tension Reso", "%", 0.4f, 0.0f, 1.0f };
                    case 3: return { "Damping", "Hz", 0.5f, 0.0f, 1.0f };
                    case 4: return { "Stereo Width", "%", 1.0f, 0.0f, 1.0f };
                }
                break;

            case EffectType::ReverbShimmer:
                switch (paramIndex)
                {
                    case 0: return { "Decay Time", "s", 0.75f, 0.0f, 1.0f };
                    case 1: return { "Pitch Shift", "st", 0.75f, 0.0f, 1.0f };
                    case 2: return { "Shimmer Mix", "%", 0.7f, 0.0f, 1.0f };
                    case 3: return { "Shimmer Tone", "Hz", 0.75f, 0.0f, 1.0f };
                    case 4: return { "Stereo Width", "%", 1.0f, 0.0f, 1.0f };
                }
                break;

            default:
                break;
        }

        return { "Param", "", 0.0f, 0.0f, 1.0f };
    }

private:
    EffectType currentType = EffectType::Bypass;
    bool isBypassed = false;
    float inGainLinear = 1.0f;
    float outGainLinear = 1.0f;

    // 1. Dynamics
    std::unique_ptr<CompressorVCAEffect> compVCA;
    std::unique_ptr<CompressorOptoEffect> compOpto;
    std::unique_ptr<CompressorFETEffect> compFET;
    std::unique_ptr<GateEffect> gate;

    // 2. EQ & Filter
    std::unique_ptr<EqualizerParametricEffect> eqParam;
    std::unique_ptr<EqualizerGraphicEffect> eqGraph;
    std::unique_ptr<EqualizerTiltEffect> eqTilt;
    std::unique_ptr<FilterDualHPLPEffect> filterHPLP;
    std::unique_ptr<FilterBandpassEffect> filterBP;

    // 3. Distortion & Saturation
    std::unique_ptr<DistortionHardClipEffect> distHard;
    std::unique_ptr<DistortionTubeEffect> distTube;
    std::unique_ptr<DistortionWavefolderEffect> distWave;
    std::unique_ptr<FuzzGermaniumEffect> fuzzGerm;
    std::unique_ptr<FuzzOctaveEffect> fuzzOct;
    std::unique_ptr<OverdriveEffect> overdrive;
    std::unique_ptr<TapeEffect> tapeSat;

    // 4. Lo-Fi & Glitch
    std::unique_ptr<BitcrushEffect> bitcrush;
    std::unique_ptr<GlitchStutterEffect> glitchStutter;
    std::unique_ptr<GlitchTapeStopEffect> glitchTapeStop;
    std::unique_ptr<GlitchReverseEffect> glitchReverse;
    std::unique_ptr<GlitchGranularEffect> glitchGranular;

    // 5. Pitch & Modulation
    std::unique_ptr<PitchShifterEffect> pitchShifter;
    std::unique_ptr<FrequencyShifterEffect> freqShifter;
    std::unique_ptr<ModulationChorusEffect> modChorus;
    std::unique_ptr<ModulationFlangerEffect> modFlanger;
    std::unique_ptr<ModulationPhaserEffect> modPhaser;
    std::unique_ptr<ModulationTremoloEffect> modTremolo;

    // 6. Delay
    std::unique_ptr<DelayStereoEffect> delayStereo;
    std::unique_ptr<DelayPingPongEffect> delayPingPong;
    std::unique_ptr<DelayTapeEchoEffect> delayTapeEcho;

    // 7. Reverb
    std::unique_ptr<ReverbRoomEffect> revRoom;
    std::unique_ptr<ReverbHallEffect> revHall;
    std::unique_ptr<ReverbPlateEffect> revPlate;
    std::unique_ptr<ReverbSpringEffect> revSpring;
    std::unique_ptr<ReverbShimmerEffect> revShimmer;
};

class EffectChain
{
public:
    static constexpr int NumSlots = 4;

    EffectChain() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        for (auto& slot : slots)
            slot.prepare(sampleRate, maxBlockSize);
    }

    void reset()
    {
        for (auto& slot : slots)
            slot.reset();
    }

    void setBpm(double bpm)
    {
        for (auto& slot : slots)
            slot.setBpm(bpm);
    }

    void setSlotType(int slotIndex, EffectType type)
    {
        if (slotIndex >= 0 && slotIndex < NumSlots)
            slots[slotIndex].setType(type);
    }

    void setSlotBypass(int slotIndex, bool bypass)
    {
        if (slotIndex >= 0 && slotIndex < NumSlots)
            slots[slotIndex].setBypassed(bypass);
    }

    void setSlotParameters(int slotIndex, float p1, float p2, float p3, float p4, float p5, float mix)
    {
        if (slotIndex >= 0 && slotIndex < NumSlots)
            slots[slotIndex].setParameters(p1, p2, p3, p4, p5, mix);
    }

    void setSlotLevels(int slotIndex, float inGainDb, float outGainDb)
    {
        if (slotIndex >= 0 && slotIndex < NumSlots)
            slots[slotIndex].setLevels(inGainDb, outGainDb);
    }

    void process(float* bufferL, float* bufferR, int numSamples)
    {
        for (auto& slot : slots)
        {
            slot.process(bufferL, bufferR, numSamples);
        }
    }

    EffectSlot& getSlot(int index)
    {
        return slots[std::clamp(index, 0, NumSlots - 1)];
    }

private:
    std::array<EffectSlot, NumSlots> slots;
};
