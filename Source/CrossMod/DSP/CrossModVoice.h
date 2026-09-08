#pragma once

#include <cmath>
#include <algorithm>
#include <cstdlib>
#include "Envelope.h"
#include "VintageFilter.h"
#include "ModMatrix.h"
#include "../Parameters/ParameterIDs.h"

class CrossModVoice
{
public:
    CrossModVoice() = default;

    void setSampleRate(double newSampleRate)
    {
        sampleRate = std::max(1.0, newSampleRate);
        ampEnv.setSampleRate(sampleRate);
        filterEnv.setSampleRate(sampleRate);
        modEnv.setSampleRate(sampleRate);
        filterL.setSampleRate(sampleRate);
        filterR.setSampleRate(sampleRate);
        reset();
    }

    void reset()
    {
        phase1 = 0.0f;
        phase2 = 0.0f;
        subPhase = 0.0f;
        lastOsc1Out = 0.0f;
        lastOsc2Out = 0.0f;
        ampEnv.reset();
        filterEnv.reset();
        modEnv.reset();
        filterL.reset();
        filterR.reset();
        currentMidiNote1 = 60.0f;
        currentMidiNote2 = 60.0f;
        targetMidiNote = 60.0f;
        hasPlayedNote = false;
        noteVelocity = 1.0f;
        randomSH = 0.0f;
        interVoiceInput = 0.0f;
        interVoiceFilterInput = 0.0f;
        interVoiceRingInput = 0.0f;
    }

    void setInitialPhase(float p1, float p2, float subP = 0.0f)
    {
        phase1 = p1;
        phase2 = p2;
        subPhase = subP;
    }

    void noteOn(int midiNote, float velocity, bool isLegato = false,
                float glideTime1 = 0.0f, float glideTime2 = 0.0f, float lastGlobalNote = -1.0f)
    {
        targetMidiNote = (float)midiNote;
        if (!isLegato || !ampEnv.isActive())
        {
            ampEnv.noteOn(velocity);
            filterEnv.noteOn(velocity);
            modEnv.noteOn(velocity);
            // Generate per-note random S&H value (-1 to +1)
            randomSH = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        }

        // Set starting pitch for Osc 1
        if (glideTime1 > 0.0005f)
        {
            if (!hasPlayedNote)
                currentMidiNote1 = (lastGlobalNote >= 0.0f) ? lastGlobalNote : (float)midiNote;
        }
        else
        {
            currentMidiNote1 = (float)midiNote;
        }

        // Set starting pitch for Osc 2
        if (glideTime2 > 0.0005f)
        {
            if (!hasPlayedNote)
                currentMidiNote2 = (lastGlobalNote >= 0.0f) ? lastGlobalNote : (float)midiNote;
        }
        else
        {
            currentMidiNote2 = (float)midiNote;
        }

        hasPlayedNote = true;
        noteVelocity = velocity;
        activeMidiNoteNumber = midiNote;
    }

    void noteOff()
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
        modEnv.noteOff();
    }

    bool isActive() const
    {
        return ampEnv.isActive();
    }

    int getActiveMidiNote() const { return activeMidiNoteNumber; }
    float getTargetMidiNote() const { return targetMidiNote; }

    void setEnvelopes(float ampA, float ampD, float ampS, float ampR,
                      float filtA, float filtD, float filtS, float filtR,
                      float modA, float modD, float modS, float modR)
    {
        ampEnv.setParameters(ampA, ampD, ampS, ampR);
        filterEnv.setParameters(filtA, filtD, filtS, filtR);
        modEnv.setParameters(modA, modD, modS, modR);
    }

    void setInterVoiceInput(float oscCoupling, float filterCoupling = 0.0f, float ringCoupling = 0.0f)
    {
        targetInterVoiceInput = oscCoupling;
        targetInterVoiceFilterInput = filterCoupling;
        targetInterVoiceRingInput = ringCoupling;
    }

    float getVoiceOutputForCoupling() const
    {
        // Provide the voice's dual-oscillator mix for coupling into the next voice
        return 0.5f * (lastOsc1Out + lastOsc2Out);
    }

    // PolyBLEP residual function for bandlimited step reconstruction
    static inline float polyBLEP(float t, float dt) noexcept
    {
        if (dt <= 1e-7f) return 0.0f;
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    static inline float evaluateWaveform(OscWaveform shape, float phase, float dt = 0.01f) noexcept
    {
        constexpr float invTwoPi = 0.15915494309189535f;
        float p = phase * invTwoPi;
        p -= std::floor(p);

        switch (shape)
        {
            case OscWaveform::Sine:
                return std::sin(phase);

            case OscWaveform::Triangle:
            {
                float tri = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
                return tri;
            }

            case OscWaveform::Saw:
            {
                float saw = 2.0f * p - 1.0f;
                saw -= polyBLEP(p, dt);
                return saw;
            }

            case OscWaveform::Square:
            {
                float sq = (p < 0.5f) ? 1.0f : -1.0f;
                sq += polyBLEP(p, dt);
                float pShift = p + 0.5f;
                if (pShift >= 1.0f) pShift -= 1.0f;
                sq -= polyBLEP(pShift, dt);
                return sq;
            }

            default:
                return std::sin(phase);
        }
    }

    void renderNextSample(float& outL, float& outR,
                          const ModDestinationOffsets& offsets,
                          float globalLfo1, float globalLfo2,
                          float modWheel, float pitchBend, float aftertouch,
                          float glideTime1Base, float glideTime2Base,
                          float stereoWidthBase,
                          // Parameter values
                          CrossModMode crossModMode,
                          OscWaveform osc1Waveform, int osc1Coarse, float osc1Fine, float osc1Level,
                          OscWaveform osc2Waveform, int osc2Coarse, float osc2Fine, float osc2Level,
                          OscWaveform subOscWaveform, SubOscOctave subOscOctave, float subOscLevelBase,
                          float crossMod1to2Base, float crossMod2to1Base,
                          float voiceCrossModDepthBase,
                          float filterCutoffBase, float filterResonanceBase, FilterType filterType,
                          float filterDrive, float filterKeyTrack, float filterEnvAmt,
                          float vcaLevelBase, float vcaPanBase, float vcaWarmth)
    {
        if (!ampEnv.isActive())
        {
            lastOsc1Out = 0.0f;
            lastOsc2Out = 0.0f;
            interVoiceInput = 0.0f;
            targetInterVoiceInput = 0.0f;
            interVoiceFilterInput = 0.0f;
            targetInterVoiceFilterInput = 0.0f;
            interVoiceRingInput = 0.0f;
            targetInterVoiceRingInput = 0.0f;
            outL = 0.0f;
            outR = 0.0f;
            return;
        }

        // Smooth inter-voice inputs to eliminate clicks when voices are added/removed in poly cross-mod
        interVoiceInput += 0.08f * (targetInterVoiceInput - interVoiceInput);
        interVoiceFilterInput += 0.08f * (targetInterVoiceFilterInput - interVoiceFilterInput);
        interVoiceRingInput += 0.08f * (targetInterVoiceRingInput - interVoiceRingInput);

        // 1. Independent Portamento / Glide in pitch for Osc 1 and Osc 2
        float effGlide1 = std::clamp(glideTime1Base + offsets.osc1Glide, 0.0f, 2.5f);
        if (effGlide1 > 0.0005f)
        {
            float glideSamples1 = effGlide1 * (float)sampleRate;
            float glideRate1 = 1.0f - std::exp(-1.0f / (glideSamples1 * 0.2f));
            if (std::abs(targetMidiNote - currentMidiNote1) > 0.001f)
                currentMidiNote1 += glideRate1 * (targetMidiNote - currentMidiNote1);
            else
                currentMidiNote1 = targetMidiNote;
        }
        else
        {
            currentMidiNote1 = targetMidiNote;
        }

        float effGlide2 = std::clamp(glideTime2Base + offsets.osc2Glide, 0.0f, 2.5f);
        if (effGlide2 > 0.0005f)
        {
            float glideSamples2 = effGlide2 * (float)sampleRate;
            float glideRate2 = 1.0f - std::exp(-1.0f / (glideSamples2 * 0.2f));
            if (std::abs(targetMidiNote - currentMidiNote2) > 0.001f)
                currentMidiNote2 += glideRate2 * (targetMidiNote - currentMidiNote2);
            else
                currentMidiNote2 = targetMidiNote;
        }
        else
        {
            currentMidiNote2 = targetMidiNote;
        }

        // 2. Advance Envelopes
        float envAmpVal = ampEnv.getNextSample();
        float envFilterVal = filterEnv.getNextSample();
        float envModVal = modEnv.getNextSample();

        // 3. Compute Pitch for Oscillator 1, Oscillator 2 and Sub Oscillator
        float pitchBendSemitones = pitchBend * 2.0f; // Standard +/- 2 semitone pitch wheel
        float semi1 = currentMidiNote1 + (float)osc1Coarse + (osc1Fine * 0.01f)
                      + pitchBendSemitones + offsets.osc1PitchSemis;
        float semi2 = currentMidiNote2 + (float)osc2Coarse + (osc2Fine * 0.01f)
                      + pitchBendSemitones + offsets.osc2PitchSemis;

        float freq1 = 440.0f * std::exp2((semi1 - 69.0f) * 0.08333333333f);
        float freq2 = 440.0f * std::exp2((semi2 - 69.0f) * 0.08333333333f);
        freq1 = std::clamp(freq1, 5.0f, 20000.0f);
        freq2 = std::clamp(freq2, 5.0f, 20000.0f);

        constexpr float twoPi = 6.283185307179586f;
        float phaseInc1 = (twoPi * freq1) / (float)sampleRate;
        float phaseInc2 = (twoPi * freq2) / (float)sampleRate;
        float dt1 = freq1 / (float)sampleRate;
        float dt2 = freq2 / (float)sampleRate;

        // Sub Oscillator generation (isolated from cross-mod path)
        float subSemi = currentMidiNote1 - (subOscOctave == SubOscOctave::OctaveMinus1 ? 12.0f : 24.0f) + pitchBendSemitones;
        float subFreq = 440.0f * std::exp2((subSemi - 69.0f) * 0.08333333333f);
        subFreq = std::clamp(subFreq, 2.0f, 20000.0f);
        float subPhaseInc = (twoPi * subFreq) / (float)sampleRate;
        float subDt = subFreq / (float)sampleRate;
        subPhase += subPhaseInc;
        if (subPhase >= twoPi) subPhase -= twoPi;
        float subOscSample = evaluateWaveform(subOscWaveform, subPhase, subDt);

        // 4. Cross Modulation Indexes
        constexpr float maxModIndex = 12.0f;
        float k1to2 = std::clamp(crossMod1to2Base + offsets.crossMod1to2, 0.0f, 1.0f) * maxModIndex;
        float k2to1 = std::clamp(crossMod2to1Base + offsets.crossMod2to1, 0.0f, 1.0f) * maxModIndex;
        float kInterVoice = std::clamp(voiceCrossModDepthBase + offsets.voiceCrossMod, 0.0f, 1.0f) * maxModIndex;

        // 5. Generate Oscillator Samples with 2x Oversampling across selected CrossModMode
        float osc1Sample = 0.0f;
        float osc2Sample = 0.0f;

        for (int step = 0; step < 2; ++step)
        {
            float currentOsc1 = 0.0f;
            float currentOsc2 = 0.0f;

            float boundedCoupling = std::clamp(interVoiceInput, -1.5f, 1.5f);

            switch (crossModMode)
            {
                case CrossModMode::FM: // 1. Analog Exponential Frequency Modulation
                {
                    float expTerm1 = std::clamp((k2to1 * lastOsc2Out + kInterVoice * boundedCoupling) * 0.35f, -6.0f, 6.0f);
                    float expTerm2 = std::clamp((k1to2 * lastOsc1Out) * 0.35f, -6.0f, 6.0f);
                    float modFreq1 = freq1 * std::exp2(expTerm1 * 1.44269504f); // 2^expTerm
                    float modFreq2 = freq2 * std::exp2(expTerm2 * 1.44269504f);
                    modFreq1 = std::clamp(modFreq1, 0.1f, (float)(sampleRate * 0.48));
                    modFreq2 = std::clamp(modFreq2, 0.1f, (float)(sampleRate * 0.48));

                    phase1 += (twoPi * modFreq1 / (float)sampleRate) * 0.5f;
                    if (phase1 >= twoPi) phase1 -= twoPi;

                    phase2 += (twoPi * modFreq2 / (float)sampleRate) * 0.5f;
                    if (phase2 >= twoPi) phase2 -= twoPi;

                    currentOsc1 = evaluateWaveform(osc1Waveform, phase1, dt1);
                    currentOsc2 = evaluateWaveform(osc2Waveform, phase2, dt2);
                    break;
                }

                case CrossModMode::PhaseMod: // 2. Yamaha DX-style true Phase Modulation
                {
                    phase1 += phaseInc1 * 0.5f;
                    if (phase1 >= twoPi) phase1 -= twoPi;

                    phase2 += phaseInc2 * 0.5f;
                    if (phase2 >= twoPi) phase2 -= twoPi;

                    float modToOsc1 = k2to1 * lastOsc2Out + kInterVoice * boundedCoupling;
                    currentOsc1 = evaluateWaveform(osc1Waveform, phase1 + modToOsc1, dt1);

                    float modToOsc2 = k1to2 * currentOsc1;
                    currentOsc2 = evaluateWaveform(osc2Waveform, phase2 + modToOsc2, dt2);
                    break;
                }

                case CrossModMode::ThroughZeroFM: // 3. True Through-Zero Frequency Modulation
                {
                    float instFreq1 = freq1 + (freq1 * std::clamp(k2to1 * lastOsc2Out + kInterVoice * boundedCoupling, -15.0f, 15.0f) * 0.4f);
                    float instFreq2 = freq2 + (freq2 * std::clamp(k1to2 * lastOsc1Out, -15.0f, 15.0f) * 0.4f);

                    float inc1 = (twoPi * instFreq1 / (float)sampleRate) * 0.5f;
                    float inc2 = (twoPi * instFreq2 / (float)sampleRate) * 0.5f;

                    phase1 += inc1;
                    while (phase1 >= twoPi) phase1 -= twoPi;
                    while (phase1 < 0.0f)   phase1 += twoPi;

                    phase2 += inc2;
                    while (phase2 >= twoPi) phase2 -= twoPi;
                    while (phase2 < 0.0f)   phase2 += twoPi;

                    currentOsc1 = evaluateWaveform(osc1Waveform, phase1, dt1);
                    currentOsc2 = evaluateWaveform(osc2Waveform, phase2, dt2);
                    break;
                }

                case CrossModMode::AM: // 4. Amplitude Modulation
                {
                    phase1 += phaseInc1 * 0.5f;
                    if (phase1 >= twoPi) phase1 -= twoPi;

                    phase2 += phaseInc2 * 0.5f;
                    if (phase2 >= twoPi) phase2 -= twoPi;

                    float raw1 = evaluateWaveform(osc1Waveform, phase1, dt1);
                    float raw2 = evaluateWaveform(osc2Waveform, phase2, dt2);

                    float amFactor1 = std::clamp(1.0f + (k2to1 * 0.15f * lastOsc2Out) + (kInterVoice * 0.15f * boundedCoupling), 0.0f, 2.0f);
                    float amFactor2 = std::clamp(1.0f + (k1to2 * 0.15f * raw1), 0.0f, 2.0f);

                    currentOsc1 = raw1 * amFactor1;
                    currentOsc2 = raw2 * amFactor2;
                    break;
                }

                case CrossModMode::RingMod: // 5. Four-Quadrant Balanced Ring Modulation
                {
                    phase1 += phaseInc1 * 0.5f;
                    if (phase1 >= twoPi) phase1 -= twoPi;

                    phase2 += phaseInc2 * 0.5f;
                    if (phase2 >= twoPi) phase2 -= twoPi;

                    float raw1 = evaluateWaveform(osc1Waveform, phase1, dt1);
                    float raw2 = evaluateWaveform(osc2Waveform, phase2, dt2);

                    float ringAmount1 = std::clamp(k2to1 * 0.15f + kInterVoice * 0.15f, 0.0f, 1.0f);
                    float ringAmount2 = std::clamp(k1to2 * 0.15f, 0.0f, 1.0f);

                    float ringSig = raw1 * raw2 * 2.0f;
                    if (std::abs(boundedCoupling) > 1e-5f)
                        ringSig += raw1 * boundedCoupling * 1.5f;

                    currentOsc1 = (1.0f - ringAmount1) * raw1 + ringAmount1 * ringSig;
                    currentOsc2 = (1.0f - ringAmount2) * raw2 + ringAmount2 * ringSig;
                    break;
                }
            }

            if (std::abs(interVoiceRingInput) > 1e-5f)
            {
                float boundedRingMod = std::clamp(interVoiceRingInput, -1.0f, 1.0f);
                float rAmt = std::clamp(voiceCrossModDepthBase + offsets.voiceCrossMod, 0.0f, 1.0f);
                currentOsc1 = (1.0f - rAmt) * currentOsc1 + rAmt * (currentOsc1 * boundedRingMod);
            }

            if (!std::isfinite(currentOsc1)) currentOsc1 = 0.0f;
            if (!std::isfinite(currentOsc2)) currentOsc2 = 0.0f;

            lastOsc1Out = currentOsc1;
            lastOsc2Out = currentOsc2;

            osc1Sample += currentOsc1 * 0.5f;
            osc2Sample += currentOsc2 * 0.5f;
        }

        // 6. Stereo Oscillator Mixer & Panning (Osc 1 Left, Osc 2 Right, Sub Center)
        float effectiveWidth = std::clamp(stereoWidthBase + offsets.stereoWidth, 0.0f, 1.0f);
        float angle1 = (1.0f - effectiveWidth) * 0.7853981633974483f; // when w=0 -> pi/4 (center), when w=1 -> 0 (hard L)
        float angle2 = (1.0f + effectiveWidth) * 0.7853981633974483f; // when w=0 -> pi/4 (center), when w=1 -> pi/2 (hard R)

        float osc1L = std::cos(angle1);
        float osc1R = std::sin(angle1);
        float osc2L = std::cos(angle2);
        float osc2R = std::sin(angle2);

        float lev1 = std::clamp(osc1Level + offsets.osc1Level, 0.0f, 1.0f);
        float lev2 = std::clamp(osc2Level + offsets.osc2Level, 0.0f, 1.0f);
        float effSubLevel = std::clamp(subOscLevelBase + offsets.subOscLevel, 0.0f, 1.0f);

        float inL = (osc1Sample * lev1 * osc1L) + (osc2Sample * lev2 * osc2L) + (subOscSample * effSubLevel * 0.7071f);
        float inR = (osc1Sample * lev1 * osc1R) + (osc2Sample * lev2 * osc2R) + (subOscSample * effSubLevel * 0.7071f);

        // 7. Dual VCF Filter (Stereo Left & Right with exact 1V/Oct Key Tracking at 0.5)
        float keyTrackOctaves = ((targetMidiNote - 60.0f) / 12.0f) * (filterKeyTrack * 2.0f);
        float envCutoffOffset = envFilterVal * filterEnvAmt * 5.0f; // up to 5 octaves
        float totalCutoffOctaves = keyTrackOctaves + envCutoffOffset + offsets.filterCutoffOctaves
                                   + (interVoiceFilterInput * (kInterVoice * 0.25f));

        float effectiveCutoff = filterCutoffBase * std::exp2(totalCutoffOctaves);
        effectiveCutoff = std::clamp(effectiveCutoff, 15.0f, 20000.0f);

        float effectiveResonance = std::clamp(filterResonanceBase + offsets.filterResonance, 0.0f, 1.0f);
        filterL.setParameters(effectiveCutoff, effectiveResonance, filterType, filterDrive);
        filterR.setParameters(effectiveCutoff, effectiveResonance, filterType, filterDrive);
        float filteredL = filterL.processSample(inL);
        float filteredR = filterR.processSample(inR);

        // 8. VCA (Amplifier) & Warmth Saturation
        float velScale = 0.25f + 0.75f * noteVelocity;
        float effectiveVcaLevel = std::clamp(vcaLevelBase + offsets.vcaLevel, 0.0f, 1.0f);
        float vcaGain = envAmpVal * velScale * effectiveVcaLevel * 0.40f;

        float satL = filteredL;
        float satR = filteredR;
        if (vcaWarmth > 0.001f)
        {
            float warmDrive = 1.0f + vcaWarmth * 2.5f;
            satL = VintageFilter::fast_tanh(filteredL * warmDrive);
            satR = VintageFilter::fast_tanh(filteredR * warmDrive);
        }
        float vcaOutL = satL * vcaGain;
        float vcaOutR = satR * vcaGain;

        // 9. Stereo Master Pan / Balance
        float effectivePan = std::clamp(vcaPanBase + offsets.vcaPan, -1.0f, 1.0f);
        if (std::abs(effectivePan) < 0.001f)
        {
            outL = vcaOutL;
            outR = vcaOutR;
        }
        else
        {
            float panAngle = (effectivePan + 1.0f) * 0.7853981633974483f;
            outL = vcaOutL * std::cos(panAngle) * 1.41421356f;
            outR = vcaOutR * std::sin(panAngle) * 1.41421356f;
        }
    }

    // Populate the per-voice modulation sources for the matrix
    void fillModSources(ModSourceValues& sources, float globalLfo1, float globalLfo2,
                        float modWheel, float pitchBend, float aftertouch) const
    {
        sources.lfo1 = globalLfo1;
        sources.lfo2 = globalLfo2;
        sources.ampEnv = ampEnv.getCurrentLevel();
        sources.filterEnv = filterEnv.getCurrentLevel();
        sources.modEnv = modEnv.getCurrentLevel();
        sources.randomSH = randomSH;
        sources.velocity = noteVelocity;
        sources.modWheel = modWheel;
        sources.pitchBend = pitchBend;
        sources.aftertouch = aftertouch;
        // KeyTrack source in octaves relative to Middle C (60)
        sources.keyTrack = (targetMidiNote - 60.0f) / 12.0f;
    }

private:
    double sampleRate = 44100.0;
    Envelope ampEnv;
    Envelope filterEnv;
    Envelope modEnv;
    VintageFilter filterL;
    VintageFilter filterR;

    float phase1 = 0.0f;
    float phase2 = 0.0f;
    float subPhase = 0.0f;
    float lastOsc1Out = 0.0f;
    float lastOsc2Out = 0.0f;

    float currentMidiNote1 = 60.0f;
    float currentMidiNote2 = 60.0f;
    float targetMidiNote = 60.0f;
    bool hasPlayedNote = false;
    float noteVelocity = 1.0f;
    int activeMidiNoteNumber = -1;
    float randomSH = 0.0f;
    float interVoiceInput = 0.0f;
    float targetInterVoiceInput = 0.0f;
    float interVoiceFilterInput = 0.0f;
    float targetInterVoiceFilterInput = 0.0f;
    float interVoiceRingInput = 0.0f;
    float targetInterVoiceRingInput = 0.0f;
};

