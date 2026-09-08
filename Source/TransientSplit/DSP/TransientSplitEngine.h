#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "TransientExtractor.h"
#include "FilterBank3.h"
#include "PreProcessors/MidSideSplitter.h"
#include "PreProcessors/DynamicSplitter.h"
#include "PreProcessors/SpectralHarmonicSplitter.h"
#include "PreProcessors/PhaseSpatialSplitter.h"
#include "EffectChain.h"
#include "Modulation/ModEngine.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <vector>

struct VisualizerPoint
{
    float input;
    float transient;
    float sustain;
    float output;
};

class TransientSplitEngine
{
public:
    static constexpr int VisualizerBufferSize = 1024;

    TransientSplitEngine()
    {
        visRingBuffer.assign(VisualizerBufferSize, { 0.0f, 0.0f, 0.0f, 0.0f });
        slotDestinations.fill(0); // Default: Bus 1 (Main Out)
        slotInputSources.fill(0); // Default: Stereo In
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        currentMaxBlockSize = std::max(64, maxBlockSize);

        extractor.prepare(currentSampleRate, currentMaxBlockSize);
        filterBank.prepare(currentSampleRate, currentMaxBlockSize);
        midSideSplitter.prepare(currentSampleRate, currentMaxBlockSize);
        dynamicSplitter.prepare(currentSampleRate, currentMaxBlockSize);
        spectralSplitter.prepare(currentSampleRate, currentMaxBlockSize);
        phaseSpatialSplitter.prepare(currentSampleRate, currentMaxBlockSize);

        transientChain.prepare(currentSampleRate, currentMaxBlockSize);
        sustainChain.prepare(currentSampleRate, currentMaxBlockSize);
        modEngine.prepare(currentSampleRate);

        dryBufferL.resize(currentMaxBlockSize, 0.0f);
        dryBufferR.resize(currentMaxBlockSize, 0.0f);
        transBufferL.resize(currentMaxBlockSize, 0.0f);
        transBufferR.resize(currentMaxBlockSize, 0.0f);
        sustBufferL.resize(currentMaxBlockSize, 0.0f);
        sustBufferR.resize(currentMaxBlockSize, 0.0f);

        filt1BufferL.resize(currentMaxBlockSize, 0.0f);
        filt1BufferR.resize(currentMaxBlockSize, 0.0f);
        filt2BufferL.resize(currentMaxBlockSize, 0.0f);
        filt2BufferR.resize(currentMaxBlockSize, 0.0f);
        filt3BufferL.resize(currentMaxBlockSize, 0.0f);
        filt3BufferR.resize(currentMaxBlockSize, 0.0f);

        leftBufferL.resize(currentMaxBlockSize, 0.0f);
        leftBufferR.resize(currentMaxBlockSize, 0.0f);
        rightBufferL.resize(currentMaxBlockSize, 0.0f);
        rightBufferR.resize(currentMaxBlockSize, 0.0f);
        midBufferL.resize(currentMaxBlockSize, 0.0f);
        midBufferR.resize(currentMaxBlockSize, 0.0f);
        sideBufferL.resize(currentMaxBlockSize, 0.0f);
        sideBufferR.resize(currentMaxBlockSize, 0.0f);

        dynHighBufferL.resize(currentMaxBlockSize, 0.0f);
        dynHighBufferR.resize(currentMaxBlockSize, 0.0f);
        dynLowBufferL.resize(currentMaxBlockSize, 0.0f);
        dynLowBufferR.resize(currentMaxBlockSize, 0.0f);

        harmonicBufferL.resize(currentMaxBlockSize, 0.0f);
        harmonicBufferR.resize(currentMaxBlockSize, 0.0f);
        noiseBufferL.resize(currentMaxBlockSize, 0.0f);
        noiseBufferR.resize(currentMaxBlockSize, 0.0f);

        inPhaseBufferL.resize(currentMaxBlockSize, 0.0f);
        inPhaseBufferR.resize(currentMaxBlockSize, 0.0f);
        spatialBufferL.resize(currentMaxBlockSize, 0.0f);
        spatialBufferR.resize(currentMaxBlockSize, 0.0f);

        for (int i = 0; i < 8; ++i)
        {
            modularSlotBufferL[i].resize(currentMaxBlockSize, 0.0f);
            modularSlotBufferR[i].resize(currentMaxBlockSize, 0.0f);
        }

        bus1AccumL.resize(currentMaxBlockSize, 0.0f);
        bus1AccumR.resize(currentMaxBlockSize, 0.0f);
        bus2AccumL.resize(currentMaxBlockSize, 0.0f);
        bus2AccumR.resize(currentMaxBlockSize, 0.0f);
        bus3AccumL.resize(currentMaxBlockSize, 0.0f);
        bus3AccumR.resize(currentMaxBlockSize, 0.0f);
        bus4AccumL.resize(currentMaxBlockSize, 0.0f);
        bus4AccumR.resize(currentMaxBlockSize, 0.0f);

        reset();
    }

    void reset()
    {
        extractor.reset();
        filterBank.reset();
        midSideSplitter.reset();
        dynamicSplitter.reset();
        spectralSplitter.reset();
        phaseSpatialSplitter.reset();
        transientChain.reset();
        sustainChain.reset();
        modEngine.reset();
        meterInput.store(0.0f);
        meterOutput.store(0.0f);
        meterTrans.store(0.0f);
        meterSust.store(0.0f);
    }

    void setEngineMode(int mode) noexcept { currentEngineMode = mode; }
    int getEngineMode() const noexcept { return currentEngineMode; }

    void setSplitParameters(float attackMs, float releaseMs, float sensitivity, float thresholdDb,
                            DetectionMode mode, float lookaheadMs)
    {
        baseAttackMs = attackMs;
        baseReleaseMs = releaseMs;
        baseSensitivity = sensitivity;
        baseThresholdDb = thresholdDb;
        currentDetectionMode = mode;
        currentLookaheadMs = lookaheadMs;
    }

    void setFilterParameters(int bandIndex, FilterBankType type, float freqHz, float q, float gainDb, float drive,
                             bool bypassed, bool solo, bool mute)
    {
        if (bandIndex >= 0 && bandIndex < 3)
        {
            baseFilterType[bandIndex] = type;
            baseFilterFreq[bandIndex] = freqHz;
            baseFilterQ[bandIndex] = q;
            baseFilterGain[bandIndex] = gainDb;
            baseFilterDrive[bandIndex] = drive;
            baseFilterBypass[bandIndex] = bypassed;
            baseFilterSolo[bandIndex] = solo;
            baseFilterMute[bandIndex] = mute;
        }
    }

    void setMidSideParameters(float widthPct, float balancePan) noexcept
    {
        midSideSplitter.setParameters(widthPct, balancePan);
    }

    void setDynamicSplitParameters(float threshDb, float attackMs, float releaseMs, float kneeDb) noexcept
    {
        dynamicSplitter.setParameters(threshDb, attackMs, releaseMs, kneeDb);
    }

    void setSpectralHarmonicParameters(float sens, float focus, float smoothingMs) noexcept
    {
        spectralSplitter.setParameters(sens, focus, smoothingMs);
    }

    void setPhaseSpatialParameters(float thresh, float windowMs, float spread) noexcept
    {
        phaseSpatialSplitter.setParameters(thresh, windowMs, spread);
    }

    void setChannelGains(float transDb, float sustDb) noexcept
    {
        transGainLinear = (transDb <= -59.5f) ? 0.0f : std::pow(10.0f, transDb * 0.05f);
        sustGainLinear  = (sustDb  <= -59.5f) ? 0.0f : std::pow(10.0f, sustDb  * 0.05f);
    }

    void setSlotInputSource(int slotIndex, int sourceChoice) noexcept
    {
        if (slotIndex >= 0 && slotIndex < 8)
            slotInputSources[slotIndex] = sourceChoice;
    }

    void setSlotRouting(int slotIndex, int destChoice) noexcept
    {
        if (slotIndex >= 0 && slotIndex < 8)
            slotDestinations[slotIndex] = destChoice;
    }

    bool wouldCreateCycle(int sourceSlot, int targetSlot) const
    {
        if (sourceSlot < 0 || sourceSlot >= 8 || targetSlot < 0 || targetSlot >= 8)
            return false;
        if (sourceSlot == targetSlot)
            return true;

        std::array<std::vector<int>, 8> adj;
        for (int s = 0; s < 8; ++s)
        {
            const int inSrc = slotInputSources[s];
            if (inSrc >= 17 && inSrc <= 24)
            {
                int upSlot = inSrc - 17;
                if (upSlot >= 0 && upSlot < 8 && upSlot != s)
                    adj[upSlot].push_back(s);
            }

            const int dest = slotDestinations[s];
            if (dest >= 7 && dest <= 14) // Dest Choice for slots 1..8
            {
                int target = dest - 7;
                if (target >= 0 && target < 8 && target != s)
                    adj[s].push_back(target);
            }
            else if (dest == 6 && s < 7) // Next slot
            {
                adj[s].push_back(s + 1);
            }
        }

        adj[sourceSlot].push_back(targetSlot);

        std::array<bool, 8> visited {};
        std::array<bool, 8> inStack {};

        auto dfs = [&](auto& self, int u) -> bool
        {
            visited[u] = true;
            inStack[u] = true;
            for (int v : adj[u])
            {
                if (!visited[v] && self(self, v))
                    return true;
                if (inStack[v])
                    return true;
            }
            inStack[u] = false;
            return false;
        };

        for (int i = 0; i < 8; ++i)
        {
            if (!visited[i] && dfs(dfs, i))
                return true;
        }
        return false;
    }

    std::vector<int> getTopologicalExecutionOrder() const
    {
        std::array<std::vector<int>, 8> adj;
        std::array<int, 8> inDegree {};

        for (int s = 0; s < 8; ++s)
        {
            const int inSrc = slotInputSources[s];
            if (inSrc >= 17 && inSrc <= 24)
            {
                int upSlot = inSrc - 17;
                if (upSlot >= 0 && upSlot < 8 && upSlot != s)
                {
                    adj[upSlot].push_back(s);
                    inDegree[s]++;
                }
            }

            const int dest = slotDestinations[s];
            if (dest >= 7 && dest <= 14)
            {
                int target = dest - 7;
                if (target >= 0 && target < 8 && target != s)
                {
                    adj[s].push_back(target);
                    inDegree[target]++;
                }
            }
            else if (dest == 6 && s < 7)
            {
                adj[s].push_back(s + 1);
                inDegree[s + 1]++;
            }
        }

        std::vector<int> order;
        std::vector<int> q;
        for (int i = 0; i < 8; ++i)
        {
            if (inDegree[i] == 0)
                q.push_back(i);
        }

        while (!q.empty())
        {
            int u = q.front();
            q.erase(q.begin());
            order.push_back(u);

            for (int v : adj[u])
            {
                if (--inDegree[v] == 0)
                    q.push_back(v);
            }
        }

        if (order.size() < 8)
        {
            for (int i = 0; i < 8; ++i)
            {
                if (std::find(order.begin(), order.end(), i) == order.end())
                    order.push_back(i);
            }
        }
        return order;
    }

    void setMasterParameters(float balance, float dryWet, float gainDb, bool limiter) noexcept
    {
        masterBalance = std::clamp(balance, -1.0f, 1.0f);
        masterDryWet = std::clamp(dryWet, 0.0f, 1.0f);
        masterGainLinear = std::pow(10.0f, gainDb * 0.05f);
        enableLimiter = limiter;
    }

    void setHostBpm(double bpm) { modEngine.setBpm(bpm); }
    void syncToHostPosition(double ppqPosition, bool isPlaying) { modEngine.syncToHostPosition(ppqPosition, isPlaying); }

    void process(juce::AudioBuffer<float>& mainBuffer,
                 juce::AudioBuffer<float>* aux1Buffer = nullptr,
                 juce::AudioBuffer<float>* aux2Buffer = nullptr,
                 juce::AudioBuffer<float>* aux3Buffer = nullptr)
    {
        const int numSamples = mainBuffer.getNumSamples();
        const int numChannels = mainBuffer.getNumChannels();
        if (numSamples <= 0) return;

        if (numSamples > currentMaxBlockSize)
            prepare(currentSampleRate, numSamples);

        const float* inL = mainBuffer.getReadPointer(0);
        const float* inR = (numChannels > 1) ? mainBuffer.getReadPointer(1) : inL;

        for (int i = 0; i < numSamples; ++i)
        {
            dryBufferL[i] = inL[i];
            dryBufferR[i] = inR[i];
        }

        // Apply Pre-Processor Modulations
        const float modAttack = modEngine.getOffset(ModDest::SplitAttack);
        const float modRelease = modEngine.getOffset(ModDest::SplitRelease);
        const float modSens = modEngine.getOffset(ModDest::SplitSensitivity);
        const float modThresh = modEngine.getOffset(ModDest::SplitThreshold);

        extractor.setParameters(
            std::clamp(baseAttackMs + modAttack * 50.0f, 1.0f, 100.0f),
            std::clamp(baseReleaseMs + modRelease * 200.0f, 10.0f, 500.0f),
            std::clamp(baseSensitivity + modSens, 0.0f, 1.0f),
            std::clamp(baseThresholdDb + modThresh * 20.0f, -60.0f, 0.0f),
            currentDetectionMode, static_cast<int>(currentLookaheadMs * 0.001f * currentSampleRate),
            0.0f, 0.0f, 0.0f, 0.0f, SplitSoloMode::Normal);

        // Run Transient Extractor
        extractor.process(inL, (numChannels > 1) ? inR : nullptr,
                          transBufferL.data(), transBufferR.data(),
                          sustBufferL.data(), sustBufferR.data(),
                          numSamples);

        // Apply Filter Bank Modulations
        const ModDest fFreqDests[3] = { ModDest::Filt1Freq, ModDest::Filt2Freq, ModDest::Filt3Freq };
        const ModDest fQDests[3]    = { ModDest::Filt1Q, ModDest::Filt2Q, ModDest::Filt3Q };
        const ModDest fGainDests[3] = { ModDest::Filt1Gain, ModDest::Filt2Gain, ModDest::Filt3Gain };
        const ModDest fDrDests[3]   = { ModDest::Filt1Drive, ModDest::Filt2Drive, ModDest::Filt3Drive };

        for (int b = 0; b < 3; ++b)
        {
            const float mFreq = modEngine.getOffset(fFreqDests[b]);
            const float mQ    = modEngine.getOffset(fQDests[b]);
            const float mGain = modEngine.getOffset(fGainDests[b]);
            const float mDr   = modEngine.getOffset(fDrDests[b]);

            const float moddedFreq = std::clamp(baseFilterFreq[b] * std::pow(2.0f, mFreq * 3.0f), 20.0f, 20000.0f);
            const float moddedQ    = std::clamp(baseFilterQ[b] + mQ * 5.0f, 0.1f, 10.0f);
            const float moddedGain = std::clamp(baseFilterGain[b] + mGain * 18.0f, -18.0f, 18.0f);
            const float moddedDr   = std::clamp(baseFilterDrive[b] + mDr * 2.0f, 1.0f, 4.0f);

            filterBank.setFilterParameters(b, baseFilterType[b], moddedFreq, moddedQ, moddedGain, moddedDr,
                                           baseFilterBypass[b], baseFilterSolo[b], baseFilterMute[b]);
        }

        // Run 3-Band ZDF Filter Bank
        filterBank.process(inL, (numChannels > 1) ? inR : nullptr, numSamples,
                           filt1BufferL.data(), filt1BufferR.data(),
                           filt2BufferL.data(), filt2BufferR.data(),
                           filt3BufferL.data(), filt3BufferR.data());

        // Run Mid/Side Splitter
        midSideSplitter.process(inL, (numChannels > 1) ? inR : nullptr,
                                leftBufferL.data(), leftBufferR.data(),
                                rightBufferL.data(), rightBufferR.data(),
                                midBufferL.data(), midBufferR.data(),
                                sideBufferL.data(), sideBufferR.data(),
                                numSamples);

        // Run Dynamic Splitter (Loud vs Quiet)
        dynamicSplitter.process(inL, (numChannels > 1) ? inR : nullptr,
                                dynHighBufferL.data(), dynHighBufferR.data(),
                                dynLowBufferL.data(), dynLowBufferR.data(),
                                numSamples);

        // Run Spectral & Harmonic Decomposition
        spectralSplitter.process(inL, (numChannels > 1) ? inR : nullptr,
                                 harmonicBufferL.data(), harmonicBufferR.data(),
                                 noiseBufferL.data(), noiseBufferR.data(),
                                 numSamples);

        // Run Phase & Spatial Splitting
        phaseSpatialSplitter.process(inL, (numChannels > 1) ? inR : nullptr,
                                     inPhaseBufferL.data(), inPhaseBufferR.data(),
                                     spatialBufferL.data(), spatialBufferR.data(),
                                     numSamples);

        // Clear Output Bus Accumulators
        std::fill(bus1AccumL.begin(), bus1AccumL.begin() + numSamples, 0.0f);
        std::fill(bus1AccumR.begin(), bus1AccumR.begin() + numSamples, 0.0f);
        std::fill(bus2AccumL.begin(), bus2AccumL.begin() + numSamples, 0.0f);
        std::fill(bus2AccumR.begin(), bus2AccumR.begin() + numSamples, 0.0f);
        std::fill(bus3AccumL.begin(), bus3AccumL.begin() + numSamples, 0.0f);
        std::fill(bus3AccumR.begin(), bus3AccumR.begin() + numSamples, 0.0f);
        std::fill(bus4AccumL.begin(), bus4AccumL.begin() + numSamples, 0.0f);
        std::fill(bus4AccumR.begin(), bus4AccumR.begin() + numSamples, 0.0f);

        // Populate root input feeds for each slot (choices 1..16)
        for (int s = 0; s < 8; ++s)
        {
            const int inChoice = slotInputSources[s];
            if (inChoice >= 1 && inChoice <= 16)
            {
                const float* rootL = inL;
                const float* rootR = inR;

                switch (inChoice)
                {
                    case 1: rootL = inL; rootR = inR; break;                                  // Stereo In
                    case 2: rootL = transBufferL.data(); rootR = transBufferR.data(); break;  // Attack
                    case 3: rootL = sustBufferL.data();  rootR = sustBufferR.data();  break;  // Body
                    case 4: rootL = filt1BufferL.data(); rootR = filt1BufferR.data(); break;  // Filter 1
                    case 5: rootL = filt2BufferL.data(); rootR = filt2BufferR.data(); break;  // Filter 2
                    case 6: rootL = filt3BufferL.data(); rootR = filt3BufferR.data(); break;  // Filter 3
                    case 7: rootL = leftBufferL.data();  rootR = leftBufferR.data();  break;  // Left (L)
                    case 8: rootL = rightBufferL.data(); rootR = rightBufferR.data(); break;  // Right (R)
                    case 9: rootL = midBufferL.data();   rootR = midBufferR.data();   break;  // Mid (M)
                    case 10: rootL = sideBufferL.data(); rootR = sideBufferR.data();  break;  // Side (S)
                    case 11: rootL = dynHighBufferL.data(); rootR = dynHighBufferR.data(); break; // Dynamic High
                    case 12: rootL = dynLowBufferL.data();  rootR = dynLowBufferR.data();  break; // Dynamic Low
                    case 13: rootL = harmonicBufferL.data(); rootR = harmonicBufferR.data(); break; // Harmonic
                    case 14: rootL = noiseBufferL.data();    rootR = noiseBufferR.data();    break; // Noise
                    case 15: rootL = inPhaseBufferL.data();  rootR = inPhaseBufferR.data();  break; // In-Phase
                    case 16: rootL = spatialBufferL.data();  rootR = spatialBufferR.data();  break; // Spatial
                }

                std::copy_n(rootL, numSamples, modularSlotBufferL[s].begin());
                std::copy_n(rootR, numSamples, modularSlotBufferR[s].begin());
            }
            else
            {
                // Disconnected (0) or Inter-slot (17..24, populated in topological execution order)
                std::fill_n(modularSlotBufferL[s].begin(), numSamples, 0.0f);
                std::fill_n(modularSlotBufferR[s].begin(), numSamples, 0.0f);
            }
        }

        // Advance Modulation Engine with audio envelope tracking
        modEngine.processBlock(inL, (numChannels > 1) ? inR : nullptr,
                               transBufferL.data(), transBufferR.data(),
                               sustBufferL.data(), sustBufferR.data(),
                               filt1BufferL.data(), filt1BufferR.data(),
                               filt2BufferL.data(), filt2BufferR.data(),
                               filt3BufferL.data(), filt3BufferR.data(),
                               leftBufferL.data(), leftBufferR.data(),
                               rightBufferL.data(), rightBufferR.data(),
                               midBufferL.data(), midBufferR.data(),
                               sideBufferL.data(), sideBufferR.data(),
                               dynHighBufferL.data(), dynHighBufferR.data(),
                               dynLowBufferL.data(), dynLowBufferR.data(),
                               harmonicBufferL.data(), harmonicBufferR.data(),
                               noiseBufferL.data(), noiseBufferR.data(),
                               inPhaseBufferL.data(), inPhaseBufferR.data(),
                               spatialBufferL.data(), spatialBufferR.data(),
                               numSamples);

        int bus1Feeds = 0, bus2Feeds = 0, bus3Feeds = 0, bus4Feeds = 0;
        auto execOrder = getTopologicalExecutionOrder();

        // Process each slot in topological dependency order
        for (int s : execOrder)
        {
            EffectSlot& slot = (s < 4) ? transientChain.getSlot(s) : sustainChain.getSlot(s - 4);

            // Pull input from upstream slot if configured as Inter-Slot (17..24)
            if (slotInputSources[s] >= 17 && slotInputSources[s] <= 24)
            {
                int upSlot = slotInputSources[s] - 17;
                if (upSlot >= 0 && upSlot < 8)
                {
                    std::copy_n(modularSlotBufferL[upSlot].begin(), numSamples, modularSlotBufferL[s].begin());
                    std::copy_n(modularSlotBufferR[upSlot].begin(), numSamples, modularSlotBufferR[s].begin());
                }
            }

            if (slot.getBypassed())
            {
                std::fill(modularSlotBufferL[s].begin(), modularSlotBufferL[s].begin() + numSamples, 0.0f);
                std::fill(modularSlotBufferR[s].begin(), modularSlotBufferR[s].begin() + numSamples, 0.0f);
                continue;
            }

            slot.process(modularSlotBufferL[s].data(), modularSlotBufferR[s].data(), numSamples);

            const int dest = slotDestinations[s];
            const float* outSlotL = modularSlotBufferL[s].data();
            const float* outSlotR = modularSlotBufferR[s].data();

            switch (dest)
            {
                case 0: // Disconnected
                    break;
                case 1: // Bus 1 (Main Out)
                    bus1Feeds++;
                    for (int i = 0; i < numSamples; ++i) { bus1AccumL[i] += outSlotL[i]; bus1AccumR[i] += outSlotR[i]; }
                    break;
                case 2: // Bus 2 (Aux 1 Out)
                    bus2Feeds++;
                    for (int i = 0; i < numSamples; ++i) { bus2AccumL[i] += outSlotL[i]; bus2AccumR[i] += outSlotR[i]; }
                    break;
                case 3: // Bus 3 (Aux 2 Out)
                    bus3Feeds++;
                    for (int i = 0; i < numSamples; ++i) { bus3AccumL[i] += outSlotL[i]; bus3AccumR[i] += outSlotR[i]; }
                    break;
                case 4: // Bus 4 (Aux 3 Out)
                    bus4Feeds++;
                    for (int i = 0; i < numSamples; ++i) { bus4AccumL[i] += outSlotL[i]; bus4AccumR[i] += outSlotR[i]; }
                    break;
                case 5: // All Buses (1+2+3+4)
                    bus1Feeds++; bus2Feeds++; bus3Feeds++; bus4Feeds++;
                    for (int i = 0; i < numSamples; ++i)
                    {
                        bus1AccumL[i] += outSlotL[i]; bus1AccumR[i] += outSlotR[i];
                        bus2AccumL[i] += outSlotL[i]; bus2AccumR[i] += outSlotR[i];
                        bus3AccumL[i] += outSlotL[i]; bus3AccumR[i] += outSlotR[i];
                        bus4AccumL[i] += outSlotL[i]; bus4AccumR[i] += outSlotR[i];
                    }
                    break;
                case 6: // Next Slot
                    if (s < 7)
                    {
                        for (int i = 0; i < numSamples; ++i)
                        {
                            modularSlotBufferL[s + 1][i] += outSlotL[i];
                            modularSlotBufferR[s + 1][i] += outSlotR[i];
                        }
                    }
                    else
                    {
                        bus1Feeds++;
                        for (int i = 0; i < numSamples; ++i) { bus1AccumL[i] += outSlotL[i]; bus1AccumR[i] += outSlotR[i]; }
                    }
                    break;
                default: // -> Slot 1..8 (7..14)
                    if (dest >= 7 && dest <= 14)
                    {
                        int targetSlot = dest - 7;
                        if (targetSlot != s && !wouldCreateCycle(s, targetSlot))
                        {
                            for (int i = 0; i < numSamples; ++i)
                            {
                                modularSlotBufferL[targetSlot][i] += outSlotL[i];
                                modularSlotBufferR[targetSlot][i] += outSlotR[i];
                            }
                        }
                        else
                        {
                            bus1Feeds++;
                            for (int i = 0; i < numSamples; ++i) { bus1AccumL[i] += outSlotL[i]; bus1AccumR[i] += outSlotR[i]; }
                        }
                    }
                    break;
            }
        }

        // Headroom normalization per bus
        auto normalizeBus = [numSamples](std::vector<float>& accL, std::vector<float>& accR, int count)
        {
            if (count > 1)
            {
                const float norm = 1.0f / std::sqrt(static_cast<float>(count));
                for (int i = 0; i < numSamples; ++i) { accL[i] *= norm; accR[i] *= norm; }
            }
        };

        normalizeBus(bus1AccumL, bus1AccumR, bus1Feeds);
        normalizeBus(bus2AccumL, bus2AccumR, bus2Feeds);
        normalizeBus(bus3AccumL, bus3AccumR, bus3Feeds);
        normalizeBus(bus4AccumL, bus4AccumR, bus4Feeds);

        const bool hasAuxBuses = (aux1Buffer != nullptr || aux2Buffer != nullptr || aux3Buffer != nullptr);

        // If no auxiliary DAW buses active, sum all 4 buses into Bus 1 for stereo listening
        if (!hasAuxBuses)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                bus1AccumL[i] += bus2AccumL[i] + bus3AccumL[i] + bus4AccumL[i];
                bus1AccumR[i] += bus2AccumR[i] + bus3AccumR[i] + bus4AccumR[i];
            }
        }

        float* outMainL = mainBuffer.getWritePointer(0);
        float* outMainR = (numChannels > 1) ? mainBuffer.getWritePointer(1) : outMainL;

        float peakIn = 0.0f;
        float peakOut = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float drySampleL = dryBufferL[i];
            const float drySampleR = dryBufferR[i];

            // Master Dry/Wet & Gain for Bus 1 (Main)
            float finalL = (drySampleL * (1.0f - masterDryWet) + bus1AccumL[i] * masterDryWet) * masterGainLinear;
            float finalR = (drySampleR * (1.0f - masterDryWet) + bus1AccumR[i] * masterDryWet) * masterGainLinear;

            if (enableLimiter)
            {
                finalL = std::tanh(finalL);
                finalR = std::tanh(finalR);
            }

            outMainL[i] = finalL;
            if (numChannels > 1) outMainR[i] = finalR;

            peakIn = std::max(peakIn, (std::abs(drySampleL) + std::abs(drySampleR)) * 0.5f);
            peakOut = std::max(peakOut, (std::abs(finalL) + std::abs(finalR)) * 0.5f);

            if ((i % 4) == 0)
            {
                int wIdx = visWriteIndex.load(std::memory_order_relaxed);
                visRingBuffer[wIdx] = {
                    (drySampleL + drySampleR) * 0.5f,
                    (transBufferL[i] + transBufferR[i]) * 0.5f,
                    (sustBufferL[i] + sustBufferR[i]) * 0.5f,
                    (finalL + finalR) * 0.5f
                };
                visWriteIndex.store((wIdx + 1) % VisualizerBufferSize, std::memory_order_relaxed);
            }
        }

        // Render Aux Buses if DAW requested them
        auto renderAuxBus = [numSamples, this](juce::AudioBuffer<float>* buf, const std::vector<float>& accL, const std::vector<float>& accR)
        {
            if (buf == nullptr) return;
            float* wL = buf->getWritePointer(0);
            float* wR = (buf->getNumChannels() > 1) ? buf->getWritePointer(1) : wL;
            for (int i = 0; i < numSamples; ++i)
            {
                float aL = accL[i] * masterGainLinear;
                float aR = accR[i] * masterGainLinear;
                if (enableLimiter) { aL = std::tanh(aL); aR = std::tanh(aR); }
                wL[i] = aL;
                if (wR) wR[i] = aR;
            }
        };

        renderAuxBus(aux1Buffer, bus2AccumL, bus2AccumR);
        renderAuxBus(aux2Buffer, bus3AccumL, bus3AccumR);
        renderAuxBus(aux3Buffer, bus4AccumL, bus4AccumR);

        meterInput.store(peakIn, std::memory_order_relaxed);
        meterOutput.store(peakOut, std::memory_order_relaxed);
    }

    TransientExtractor& getExtractor() noexcept { return extractor; }
    FilterBank3& getFilterBank() noexcept { return filterBank; }
    EffectChain& getTransientChain() noexcept { return transientChain; }
    EffectChain& getSustainChain() noexcept { return sustainChain; }
    ModEngine& getModEngine() noexcept { return modEngine; }

    float getInputMeterLevel() const noexcept { return meterInput.load(std::memory_order_relaxed); }
    float getOutputMeterLevel() const noexcept { return meterOutput.load(std::memory_order_relaxed); }
    float getTransMeterLevel() const noexcept { return meterTrans.load(std::memory_order_relaxed); }
    float getSustMeterLevel() const noexcept { return meterSust.load(std::memory_order_relaxed); }

    const std::vector<VisualizerPoint>& getVisualizerPoints() const noexcept { return visRingBuffer; }
    void getVisualizerSnapshot(std::vector<VisualizerPoint>& outSnapshot) const { outSnapshot = visRingBuffer; }
    int getVisualizerWriteIndex() const noexcept { return visWriteIndex.load(std::memory_order_relaxed); }

private:
    double currentSampleRate = 48000.0;
    int currentMaxBlockSize = 512;
    int currentEngineMode = 0;

    TransientExtractor extractor;
    FilterBank3 filterBank;
    MidSideSplitter midSideSplitter;
    DynamicSplitter dynamicSplitter;
    SpectralHarmonicSplitter spectralSplitter;
    PhaseSpatialSplitter phaseSpatialSplitter;

    EffectChain transientChain;
    EffectChain sustainChain;
    ModEngine modEngine;

    float baseAttackMs = 15.0f;
    float baseReleaseMs = 80.0f;
    float baseSensitivity = 0.5f;
    float baseThresholdDb = -24.0f;
    DetectionMode currentDetectionMode = DetectionMode::Transient;
    float currentLookaheadMs = 0.0f;

    std::array<FilterBankType, 3> baseFilterType = { FilterBankType::LowPass12, FilterBankType::BandPass, FilterBankType::HighPass12 };
    std::array<float, 3> baseFilterFreq = { 250.0f, 1500.0f, 5000.0f };
    std::array<float, 3> baseFilterQ = { 0.707f, 1.0f, 0.707f };
    std::array<float, 3> baseFilterGain = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> baseFilterDrive = { 1.0f, 1.0f, 1.0f };
    std::array<bool, 3> baseFilterBypass = { false, false, false };
    std::array<bool, 3> baseFilterSolo = { false, false, false };
    std::array<bool, 3> baseFilterMute = { false, false, false };

    float transGainLinear = 1.0f;
    float sustGainLinear = 1.0f;

    std::array<int, 8> slotInputSources;
    std::array<int, 8> slotDestinations;

    float masterBalance = 0.0f;
    float masterDryWet = 1.0f;
    float masterGainLinear = 1.0f;
    bool enableLimiter = true;

    std::vector<float> dryBufferL, dryBufferR;
    std::vector<float> transBufferL, transBufferR;
    std::vector<float> sustBufferL, sustBufferR;
    std::vector<float> filt1BufferL, filt1BufferR;
    std::vector<float> filt2BufferL, filt2BufferR;
    std::vector<float> filt3BufferL, filt3BufferR;
    std::vector<float> leftBufferL, leftBufferR;
    std::vector<float> rightBufferL, rightBufferR;
    std::vector<float> midBufferL, midBufferR;
    std::vector<float> sideBufferL, sideBufferR;
    std::vector<float> dynHighBufferL, dynHighBufferR;
    std::vector<float> dynLowBufferL, dynLowBufferR;
    std::vector<float> harmonicBufferL, harmonicBufferR;
    std::vector<float> noiseBufferL, noiseBufferR;
    std::vector<float> inPhaseBufferL, inPhaseBufferR;
    std::vector<float> spatialBufferL, spatialBufferR;
    std::array<std::vector<float>, 8> modularSlotBufferL;
    std::array<std::vector<float>, 8> modularSlotBufferR;

    std::vector<float> bus1AccumL, bus1AccumR;
    std::vector<float> bus2AccumL, bus2AccumR;
    std::vector<float> bus3AccumL, bus3AccumR;
    std::vector<float> bus4AccumL, bus4AccumR;

    std::atomic<float> meterInput { 0.0f };
    std::atomic<float> meterOutput { 0.0f };
    std::atomic<float> meterTrans { 0.0f };
    std::atomic<float> meterSust { 0.0f };

    std::vector<VisualizerPoint> visRingBuffer;
    std::atomic<int> visWriteIndex { 0 };
};
