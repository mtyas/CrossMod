#pragma once

#include <array>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include "CrossModVoice.h"
#include "LFO.h"
#include "ModMatrix.h"
#include "Effects/FxChain.h"
#include "../Parameters/ParameterIDs.h"

class CrossModEngine
{
public:
    static constexpr int NUM_VOICES = 16;

    CrossModEngine() = default;

    void prepare(double sampleRate, int maxBlockSize = 512)
    {
        currentSampleRate = sampleRate;
        for (auto& voice : voices)
        {
            voice.setSampleRate(sampleRate);
        }
        lfo1.setSampleRate(sampleRate);
        lfo2.setSampleRate(sampleRate);
        fxChain.prepare(sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& voice : voices)
        {
            voice.reset();
        }
        noteStack.clear();
        lfo1.resetPhase();
        lfo2.resetPhase();
        fxChain.reset();
        lastPlayedMidiNote = -1.0f;
    }

    void setBpm(double bpm)
    {
        currentBpm = bpm;
    }

    void setVoiceMode(VoiceMode mode)
    {
        if (currentVoiceMode != mode)
        {
            allNotesOff();
            noteStack.clear();
            currentVoiceMode = mode;
        }
    }

    void noteOn(int midiNote, float velocity)
    {
        if (velocity <= 0.001f)
        {
            noteOff(midiNote);
            return;
        }

        // LFO retriggering
        if (lfo1Retrig) lfo1.resetPhase();
        if (lfo2Retrig) lfo2.resetPhase();

        if (currentVoiceMode == VoiceMode::Mono)
        {
            bool isLegato = !noteStack.empty() && voices[0].isActive();
            removeFromNoteStack(midiNote);
            noteStack.push_back({ midiNote, velocity });

            voices[0].noteOn(midiNote, velocity, isLegato, currentGlide1Sec, currentGlide2Sec, lastPlayedMidiNote);
        }
        else if (currentVoiceMode == VoiceMode::MonoUnison)
        {
            bool isLegato = !noteStack.empty() && voices[0].isActive();
            removeFromNoteStack(midiNote);
            noteStack.push_back({ midiNote, velocity });

            constexpr int UNISON_COUNT = 8;
            for (int u = 0; u < UNISON_COUNT; ++u)
            {
                if (!isLegato && !voices[u].isActive())
                {
                    float phi = (float)u * (6.2831853f / 8.0f);
                    voices[u].setInitialPhase(phi, phi + 1.5707963f, phi * 0.5f);
                }
                voices[u].noteOn(midiNote, velocity, isLegato, currentGlide1Sec, currentGlide2Sec, lastPlayedMidiNote);
            }
        }
        else
        {
            // Polyphonic modes (PolyMultiMono, CyclicRing, SympatheticAll, RootDriver, ChaosDiffuse)
            int voiceIndex = findVoicePlayingNote(midiNote);
            if (voiceIndex < 0)
            {
                voiceIndex = findFreeVoiceIndex();
            }

            if (voiceIndex >= 0)
            {
                voices[voiceIndex].noteOn(midiNote, velocity, false, currentGlide1Sec, currentGlide2Sec, lastPlayedMidiNote);
                voiceAge[voiceIndex] = ++voiceAgeCounter;
            }
        }
        lastPlayedMidiNote = (float)midiNote;
    }

    void noteOff(int midiNote)
    {
        if (currentVoiceMode == VoiceMode::Mono)
        {
            removeFromNoteStack(midiNote);
            if (noteStack.empty())
            {
                voices[0].noteOff();
            }
            else
            {
                auto& lastNote = noteStack.back();
                voices[0].noteOn(lastNote.first, lastNote.second, true, currentGlide1Sec, currentGlide2Sec, lastPlayedMidiNote);
                lastPlayedMidiNote = (float)lastNote.first;
            }
        }
        else if (currentVoiceMode == VoiceMode::MonoUnison)
        {
            removeFromNoteStack(midiNote);
            if (noteStack.empty())
            {
                constexpr int UNISON_COUNT = 8;
                for (int u = 0; u < UNISON_COUNT; ++u)
                    voices[u].noteOff();
            }
            else
            {
                auto& lastNote = noteStack.back();
                constexpr int UNISON_COUNT = 8;
                for (int u = 0; u < UNISON_COUNT; ++u)
                    voices[u].noteOn(lastNote.first, lastNote.second, true, currentGlide1Sec, currentGlide2Sec, lastPlayedMidiNote);
                lastPlayedMidiNote = (float)lastNote.first;
            }
        }
        else
        {
            for (int i = 0; i < NUM_VOICES; ++i)
            {
                if (voices[i].isActive() && voices[i].getActiveMidiNote() == midiNote)
                {
                    voices[i].noteOff();
                }
            }
        }
    }

    void allNotesOff()
    {
        for (auto& voice : voices)
        {
            voice.noteOff();
        }
        noteStack.clear();
    }

    void setPitchBend(float normalizedBipolar)
    {
        pitchBend = std::clamp(normalizedBipolar, -1.0f, 1.0f);
    }

    void setModWheel(float normalized0to1)
    {
        modWheel = std::clamp(normalized0to1, 0.0f, 1.0f);
    }

    void setAftertouch(float normalized0to1)
    {
        aftertouch = std::clamp(normalized0to1, 0.0f, 1.0f);
    }

    void setModMatrixSlot(int slotIndex, ModSource src, ModDestination dest, float amount)
    {
        modMatrix.setSlot(slotIndex, src, dest, amount);
    }

    // Process a block of stereo audio
    void processBlock(juce::AudioBuffer<float>& buffer,
                      VoiceMode voiceMode, CrossModMode crossModMode,
                      float osc1GlideSec, float osc2GlideSec, float voiceCrossModDepth,
                      OscWaveform osc1Waveform, int osc1Coarse, float osc1Fine, float osc1Level,
                      OscWaveform osc2Waveform, int osc2Coarse, float osc2Fine, float osc2Level,
                      OscWaveform subOscWaveform, SubOscOctave subOscOctave, float subOscLevel,
                      float crossMod1to2, float crossMod2to1,
                      float filterCutoff, float filterResonance, FilterType filterType,
                      float filterDrive, float filterKeyTrack, float filterEnvAmt,
                      float ampA, float ampD, float ampS, float ampR,
                      float filtA, float filtD, float filtS, float filtR,
                      float modA, float modD, float modS, float modR,
                      float lfo1Rate, LFOShape lfo1Shape, bool lfo1Sync, int lfo1SyncRate, bool lfo1Retr,
                      float lfo2Rate, LFOShape lfo2Shape, bool lfo2Sync, int lfo2SyncRate, bool lfo2Retr,
                      float masterVolume, float masterPan, float stereoWidth, float vcaWarmth,
                      FxOrder fxOrder,
                      ModFxType modType, float modRate, float modDepth, float modFeedback, float modMix,
                      DelayFxType delayType, float delayTime, float delayFeedback, float delayTone, float delayMix,
                      bool delaySync, int delaySyncRate,
                      ReverbFxType reverbType, float reverbDecay, float reverbDamping, float reverbTone, float reverbMix)
    {
        if (currentVoiceMode != voiceMode)
        {
            allNotesOff();
            noteStack.clear();
            currentVoiceMode = voiceMode;
        }

        currentGlide1Sec = osc1GlideSec;
        currentGlide2Sec = osc2GlideSec;
        lfo1Retrig = lfo1Retr;
        lfo2Retrig = lfo2Retr;

        // Configure LFOs
        lfo1.setShape(lfo1Shape);
        lfo1.setRateHz(lfo1Rate);
        lfo1.setBpmAndSync(currentBpm, lfo1Sync, lfo1SyncRate);

        lfo2.setShape(lfo2Shape);
        lfo2.setRateHz(lfo2Rate);
        lfo2.setBpmAndSync(currentBpm, lfo2Sync, lfo2SyncRate);

        int numSamples = buffer.getNumSamples();
        float* channelDataL = buffer.getWritePointer(0);
        float* channelDataR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : nullptr;

        // Track active voice indices for voice-crossmod ring / unison
        std::vector<int> activeIndices;
        activeIndices.reserve(NUM_VOICES);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Collect currently active voices
            activeIndices.clear();
            if (voiceMode == VoiceMode::Mono)
            {
                if (voices[0].isActive())
                    activeIndices.push_back(0);
            }
            else if (voiceMode == VoiceMode::MonoUnison)
            {
                for (int i = 0; i < 8; ++i)
                {
                    if (voices[i].isActive())
                        activeIndices.push_back(i);
                }
            }
            else
            {
                for (int i = 0; i < NUM_VOICES; ++i)
                {
                    if (voices[i].isActive())
                        activeIndices.push_back(i);
                }
            }

            // Set up inter-voice coupling in poly cross-mod modes
            bool isCrossModPoly = (voiceMode == VoiceMode::CyclicRing ||
                                   voiceMode == VoiceMode::SympatheticAll ||
                                   voiceMode == VoiceMode::RootDriver ||
                                   voiceMode == VoiceMode::ChaosDiffuse);

            if (isCrossModPoly && activeIndices.size() > 1)
            {
                size_t M = activeIndices.size();
                switch (voiceMode)
                {
                    case VoiceMode::CyclicRing:
                    {
                        for (size_t k = 0; k < M; ++k)
                        {
                            size_t prevK = (k + M - 1) % M;
                            int prevVoiceIdx = activeIndices[prevK];
                            int thisVoiceIdx = activeIndices[k];
                            float prevSig = voices[prevVoiceIdx].getVoiceOutputForCoupling();
                            voices[thisVoiceIdx].setInterVoiceInput(prevSig * 0.5f, 0.0f, prevSig * 1.5f);
                        }
                        break;
                    }

                    case VoiceMode::SympatheticAll:
                    {
                        float sumOutputs = 0.0f;
                        for (size_t k = 0; k < M; ++k)
                        {
                            sumOutputs += voices[activeIndices[k]].getVoiceOutputForCoupling();
                        }
                        float invOther = 1.0f / (float)(M > 1 ? M - 1 : 1);
                        for (size_t k = 0; k < M; ++k)
                        {
                            int thisVoiceIdx = activeIndices[k];
                            float selfOut = voices[thisVoiceIdx].getVoiceOutputForCoupling();
                            float otherAvg = (sumOutputs - selfOut) * invOther;
                            voices[thisVoiceIdx].setInterVoiceInput(otherAvg * 0.4f, otherAvg * 1.8f, 0.0f);
                        }
                        break;
                    }

                    case VoiceMode::RootDriver:
                    {
                        size_t rootK = 0;
                        float lowestPitch = voices[activeIndices[0]].getTargetMidiNote();
                        for (size_t k = 1; k < M; ++k)
                        {
                            float p = voices[activeIndices[k]].getTargetMidiNote();
                            if (p < lowestPitch)
                            {
                                lowestPitch = p;
                                rootK = k;
                            }
                        }

                        int rootVoiceIdx = activeIndices[rootK];
                        float rootOut = voices[rootVoiceIdx].getVoiceOutputForCoupling();

                        float sumUpperSq = 0.0f;
                        for (size_t k = 0; k < M; ++k)
                        {
                            if (k != rootK)
                            {
                                int uIdx = activeIndices[k];
                                float uOut = voices[uIdx].getVoiceOutputForCoupling();
                                sumUpperSq += uOut * uOut;
                                voices[uIdx].setInterVoiceInput(rootOut * 2.2f, 0.0f, 0.0f);
                            }
                        }
                        float avgUpper2ndHarm = sumUpperSq / (float)(M > 1 ? M - 1 : 1);
                        voices[rootVoiceIdx].setInterVoiceInput(avgUpper2ndHarm * 0.35f, 0.0f, 0.0f);
                        break;
                    }

                    case VoiceMode::ChaosDiffuse:
                    {
                        for (size_t k = 0; k < M; ++k)
                        {
                            size_t prevK = (k + M - 1) % M;
                            size_t nextK = (k + 1) % M;
                            float vPrev = voices[activeIndices[prevK]].getVoiceOutputForCoupling();
                            float vNext = voices[activeIndices[nextK]].getVoiceOutputForCoupling();
                            float vThis = voices[activeIndices[k]].getVoiceOutputForCoupling();

                            float diff = vNext - vPrev;
                            float chaosSig = std::tanh(diff * 2.5f) + 0.35f * std::sin(vNext * 6.28f) - 0.2f * (vThis * vThis * vThis);
                            voices[activeIndices[k]].setInterVoiceInput(chaosSig * 1.4f, std::abs(chaosSig) * 0.6f, 0.0f);
                        }
                        break;
                    }

                    default: break;
                }
            }
            else
            {
                for (int idx : activeIndices)
                {
                    voices[idx].setInterVoiceInput(0.0f, 0.0f, 0.0f);
                }
            }

            // Evaluate global LFOs
            float lfo1Sample = lfo1.getNextSample(globalLfo1RateOffset);
            float lfo2Sample = lfo2.getNextSample(globalLfo2RateOffset);

            float sumL = 0.0f;
            float sumR = 0.0f;

            for (int idx : activeIndices)
            {
                auto& voice = voices[idx];

                // Gather modulation sources for this voice
                ModSourceValues sources;
                voice.fillModSources(sources, lfo1Sample, lfo2Sample, modWheel, pitchBend, aftertouch);

                // Evaluate modulation matrix for this voice
                ModDestinationOffsets offsets;
                modMatrix.evaluate(sources, offsets);

                // If in MonoUnison mode, apply unison detune and stereo spread
                if (voiceMode == VoiceMode::MonoUnison)
                {
                    float spreadFactor = ((float)idx - 3.5f) / 3.5f; // -1.0 to +1.0
                    float unisonDetuneSemis = spreadFactor * (voiceCrossModDepth * 12.0f); // up to +/- 12 semitones!
                    float unisonPanOffset = spreadFactor * 0.75f; // stereo width spread

                    offsets.osc1PitchSemis += unisonDetuneSemis;
                    offsets.osc2PitchSemis += unisonDetuneSemis;
                    offsets.vcaPan += unisonPanOffset;
                }

                // Update LFO rate modulation from matrix if any
                globalLfo1RateOffset = offsets.lfo1RateMod;
                globalLfo2RateOffset = offsets.lfo2RateMod;

                float vL = 0.0f;
                float vR = 0.0f;
                voice.renderNextSample(vL, vR, offsets,
                                       lfo1Sample, lfo2Sample, modWheel, pitchBend, aftertouch,
                                       osc1GlideSec, osc2GlideSec, stereoWidth,
                                       crossModMode,
                                       osc1Waveform, osc1Coarse, osc1Fine, osc1Level,
                                       osc2Waveform, osc2Coarse, osc2Fine, osc2Level,
                                       subOscWaveform, subOscOctave, subOscLevel,
                                       crossMod1to2, crossMod2to1,
                                       voiceCrossModDepth,
                                       filterCutoff, filterResonance, filterType,
                                       filterDrive, filterKeyTrack, filterEnvAmt,
                                       masterVolume, masterPan, vcaWarmth);

                if (voiceMode == VoiceMode::MonoUnison)
                {
                    vL *= 0.22f;
                    vR *= 0.22f;
                }

                sumL += vL;
                sumR += vR;
            }

            channelDataL[sample] += sumL;
            if (channelDataR)
            {
                channelDataR[sample] += sumR;
            }

            // Record Pre-FX stereo scope samples
            preFxScopeBufferL[scopeWriteIdx] = channelDataL[sample];
            preFxScopeBufferR[scopeWriteIdx] = (channelDataR != nullptr) ? channelDataR[sample] : channelDataL[sample];
            scopeWriteIdx = (scopeWriteIdx + 1) % SCOPE_BUFFER_SIZE;
        }

        // 6. Master / Global Modulation Matrix Evaluation for FX & Voice Cross-Mod
        ModSourceValues masterSources;
        if (!activeIndices.empty())
        {
            voices[activeIndices.back()].fillModSources(masterSources,
                                                        lfo1.getLastOutput(), lfo2.getLastOutput(),
                                                        modWheel, pitchBend, aftertouch);
        }
        else
        {
            masterSources.lfo1 = lfo1.getLastOutput();
            masterSources.lfo2 = lfo2.getLastOutput();
            masterSources.modWheel = modWheel;
            masterSources.pitchBend = pitchBend;
            masterSources.aftertouch = aftertouch;
        }

        ModDestinationOffsets masterOffsets;
        modMatrix.evaluate(masterSources, masterOffsets);

        // Apply matrix offsets to all FX parameters
        float effModRate = std::clamp(modRate + masterOffsets.modFxRate, 0.05f, 15.0f);
        float effModDepth = std::clamp(modDepth + masterOffsets.modFxDepth, 0.0f, 1.0f);
        float effModFeedback = std::clamp(modFeedback + masterOffsets.modFxFeedback, 0.0f, 0.95f);
        float effModMix = std::clamp(modMix + masterOffsets.modFxMix, 0.0f, 1.0f);

        float effDelayTime = std::clamp(delayTime + masterOffsets.delayFxTime, 0.01f, 1.8f);
        float effDelayFeedback = std::clamp(delayFeedback + masterOffsets.delayFxFeedback, 0.0f, 0.95f);
        float effDelayTone = std::clamp(delayTone + masterOffsets.delayFxTone, 0.0f, 1.0f);
        float effDelayMix = std::clamp(delayMix + masterOffsets.delayFxMix, 0.0f, 1.0f);

        float effReverbDecay = std::clamp(reverbDecay + masterOffsets.reverbFxDecay, 0.1f, 10.0f);
        float effReverbDamping = std::clamp(reverbDamping + masterOffsets.reverbFxDamping, 0.0f, 1.0f);
        float effReverbTone = std::clamp(reverbTone + masterOffsets.reverbFxTone, 0.0f, 1.0f);
        float effReverbMix = std::clamp(reverbMix + masterOffsets.reverbFxMix, 0.0f, 1.0f);

        // Run Master FX Chain
        fxChain.setDelayBpmAndSync(currentBpm, delaySync, delaySyncRate);
        if (channelDataR != nullptr)
        {
            fxChain.process(channelDataL, channelDataR, numSamples,
                            fxOrder,
                            modType, effModRate, effModDepth, effModFeedback, effModMix,
                            delayType, effDelayTime, effDelayFeedback, effDelayTone, effDelayMix,
                            reverbType, effReverbDecay, effReverbDamping, effReverbTone, effReverbMix);

            lastMasterL = channelDataL[numSamples - 1];
            lastMasterR = channelDataR[numSamples - 1];

            // Record Post-FX stereo scope samples
            int recStart = (scopeWriteIdx - numSamples + SCOPE_BUFFER_SIZE) % SCOPE_BUFFER_SIZE;
            for (int s = 0; s < numSamples; ++s)
            {
                int idx = (recStart + s) % SCOPE_BUFFER_SIZE;
                postFxScopeBufferL[idx] = channelDataL[s];
                postFxScopeBufferR[idx] = channelDataR[s];
            }
        }
        else
        {
            std::vector<float> dummyR(numSamples, 0.0f);
            fxChain.process(channelDataL, dummyR.data(), numSamples,
                            fxOrder,
                            modType, effModRate, effModDepth, effModFeedback, effModMix,
                            delayType, effDelayTime, effDelayFeedback, effDelayTone, effDelayMix,
                            reverbType, effReverbDecay, effReverbDamping, effReverbTone, effReverbMix);

            lastMasterL = channelDataL[numSamples - 1];
            lastMasterR = lastMasterL;

            int recStart = (scopeWriteIdx - numSamples + SCOPE_BUFFER_SIZE) % SCOPE_BUFFER_SIZE;
            for (int s = 0; s < numSamples; ++s)
            {
                int idx = (recStart + s) % SCOPE_BUFFER_SIZE;
                postFxScopeBufferL[idx] = channelDataL[s];
                postFxScopeBufferR[idx] = channelDataL[s];
            }
        }
    }

    void updateVoiceEnvelopeParameters(float ampA, float ampD, float ampS, float ampR,
                                       float filtA, float filtD, float filtS, float filtR,
                                       float modA, float modD, float modS, float modR)
    {
        for (auto& voice : voices)
        {
            voiceSetEnvelopes(voice, ampA, ampD, ampS, ampR,
                              filtA, filtD, filtS, filtR,
                              modA, modD, modS, modR);
        }
    }

    // Voice activity mask for UI LEDs
    uint32_t getActiveVoiceMask() const
    {
        uint32_t mask = 0;
        for (int i = 0; i < NUM_VOICES; ++i)
        {
            if (voices[i].isActive())
                mask |= (1u << i);
        }
        return mask;
    }

    float getLastOutputL() const { return lastMasterL; }
    float getLastOutputR() const { return lastMasterR; }
    float getLfo1Output() const { return lfo1.getLastOutput(); }
    float getLfo2Output() const { return lfo2.getLastOutput(); }

    void copyPreFxScope(float* destL, float* destR, int numSamples) const
    {
        for (int i = 0; i < numSamples; ++i)
        {
            int idx = (scopeWriteIdx - numSamples + i + SCOPE_BUFFER_SIZE) % SCOPE_BUFFER_SIZE;
            destL[i] = preFxScopeBufferL[idx];
            destR[i] = preFxScopeBufferR[idx];
        }
    }

    void copyPostFxScope(float* destL, float* destR, int numSamples) const
    {
        for (int i = 0; i < numSamples; ++i)
        {
            int idx = (scopeWriteIdx - numSamples + i + SCOPE_BUFFER_SIZE) % SCOPE_BUFFER_SIZE;
            destL[i] = postFxScopeBufferL[idx];
            destR[i] = postFxScopeBufferR[idx];
        }
    }

private:
    void removeFromNoteStack(int midiNote)
    {
        noteStack.erase(
            std::remove_if(noteStack.begin(), noteStack.end(),
                           [midiNote](const std::pair<int, float>& p) { return p.first == midiNote; }),
            noteStack.end());
    }

    int findVoicePlayingNote(int midiNote)
    {
        for (int i = 0; i < NUM_VOICES; ++i)
        {
            if (voices[i].isActive() && voices[i].getActiveMidiNote() == midiNote)
                return i;
        }
        return -1;
    }

    int findFreeVoiceIndex()
    {
        // 1. Look for inactive voice
        for (int i = 0; i < NUM_VOICES; ++i)
        {
            if (!voices[i].isActive())
                return i;
        }

        // 2. Otherwise steal oldest active voice
        int oldestIdx = 0;
        uint64_t oldestAge = UINT64_MAX;
        for (int i = 0; i < NUM_VOICES; ++i)
        {
            if (voiceAge[i] < oldestAge)
            {
                oldestAge = voiceAge[i];
                oldestIdx = i;
            }
        }
        return oldestIdx;
    }

    void voiceSetEnvelopes(CrossModVoice& voice,
                           float ampA, float ampD, float ampS, float ampR,
                           float filtA, float filtD, float filtS, float filtR,
                           float modA, float modD, float modS, float modR);

    double currentSampleRate = 44100.0;
    double currentBpm = 120.0;
    VoiceMode currentVoiceMode = VoiceMode::PolyMultiMono;

    std::array<CrossModVoice, NUM_VOICES> voices;
    std::array<uint64_t, NUM_VOICES> voiceAge { 0 };
    uint64_t voiceAgeCounter = 0;

    std::vector<std::pair<int, float>> noteStack; // for mono legato
    float currentGlide1Sec = 0.0f;
    float currentGlide2Sec = 0.0f;
    float lastPlayedMidiNote = -1.0f;

    LFO lfo1;
    LFO lfo2;
    bool lfo1Retrig = true;
    bool lfo2Retrig = false;
    float globalLfo1RateOffset = 0.0f;
    float globalLfo2RateOffset = 0.0f;

    ModMatrix modMatrix;
    FxChain fxChain;

    float pitchBend = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;

    float lastMasterL = 0.0f;
    float lastMasterR = 0.0f;

    static constexpr int SCOPE_BUFFER_SIZE = 512;
    std::array<float, SCOPE_BUFFER_SIZE> preFxScopeBufferL { 0.0f };
    std::array<float, SCOPE_BUFFER_SIZE> preFxScopeBufferR { 0.0f };
    std::array<float, SCOPE_BUFFER_SIZE> postFxScopeBufferL { 0.0f };
    std::array<float, SCOPE_BUFFER_SIZE> postFxScopeBufferR { 0.0f };
    int scopeWriteIdx = 0;
};
