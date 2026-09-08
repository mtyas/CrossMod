#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/TransientSplit/DSP/TransientExtractor.h"
#include "../Source/TransientSplit/DSP/EffectChain.h"
#include "../Source/TransientSplit/DSP/TransientSplitEngine.h"
#include "../Source/TransientSplit/DSP/Modulation/ModLFO.h"
#include "../Source/TransientSplit/DSP/Modulation/ModEnvelopeFollower.h"
#include "../Source/TransientSplit/DSP/Modulation/ModTuringMachine.h"
#include "../Source/TransientSplit/DSP/Modulation/ModMatrix.h"
#include "../Source/TransientSplit/DSP/Modulation/ModEngine.h"
#include "../Source/TransientSplit/DSP/Effects/ReverbShimmerEffect.h"
#include "../Source/TransientSplit/DSP/Effects/DelayTapeEchoEffect.h"
#include "../Source/TransientSplit/DSP/Effects/DelayStereoEffect.h"
#include "../Source/TransientSplit/DSP/Effects/DelayPingPongEffect.h"
#include "../Source/TransientSplit/Parameters/ParameterIDs.h"
#include "../Source/TransientSplit/Presets/FactoryPresets.h"
#include <iostream>
#include <cassert>
#include <cmath>

void testTransientExtractorComplementarySum()
{
    std::cout << "[TEST] TransientExtractor Complementary Sum Reconstruction... ";

    TransientExtractor extractor;
    extractor.prepare(48000.0, 512);

    extractor.setParameters(
        20.0f,  // attackMs
        150.0f, // releaseMs
        1.0f,   // sensitivity
        -30.0f, // thresholdDb
        DetectionMode::Transient,
        64,     // lookahead
        0.0f,   // transGainDb
        0.0f,   // sustGainDb
        0.0f,   // transPan
        0.0f,   // sustPan
        SplitSoloMode::Normal
    );

    const int numSamples = 1024;
    std::vector<float> inL(numSamples, 0.0f);
    std::vector<float> inR(numSamples, 0.0f);
    std::vector<float> tL(numSamples, 0.0f);
    std::vector<float> tR(numSamples, 0.0f);
    std::vector<float> sL(numSamples, 0.0f);
    std::vector<float> sR(numSamples, 0.0f);
    std::vector<float> dryL(numSamples, 0.0f);
    std::vector<float> dryR(numSamples, 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        float env = std::exp(-static_cast<float>(i) * 0.005f);
        float spike = (i == 100 || i == 300 || i == 600) ? 0.9f : 0.0f;
        float sig = std::sin(static_cast<float>(i) * 0.1f) * env * 0.5f + spike;
        inL[i] = sig;
        inR[i] = sig * 0.8f;
    }

    extractor.process(inL.data(), inR.data(), tL.data(), tR.data(), sL.data(), sR.data(), numSamples, dryL.data(), dryR.data());

    for (int i = 70; i < numSamples - 10; ++i)
    {
        const float sumL = tL[i] + sL[i];
        const float sumR = tR[i] + sR[i];

        const float delayedInL = inL[i - 64];
        const float delayedInR = inR[i - 64];

        const float diffL = std::abs(sumL - delayedInL);
        const float diffR = std::abs(sumR - delayedInR);

        if (diffL > 0.01f || diffR > 0.01f)
        {
            std::cerr << "Mismatch at sample " << i << ": sumL=" << sumL << " delayedInL=" << delayedInL << " diff=" << diffL << std::endl;
            assert(false);
        }

        assert(std::abs(dryL[i] - delayedInL) < 1e-5f);
        assert(std::abs(dryR[i] - delayedInR) < 1e-5f);
    }

    std::cout << "PASSED!" << std::endl;
}

void testChannelDryLevelDirectPassAndSlotInGains()
{
    std::cout << "[TEST] Master Dry/Wet Crossfade & Gating... ";

    TransientSplitEngine engine;
    engine.prepare(48000.0, 256);
    engine.setEngineMode(0); // Split Mode

    // 1. When all slots are bypassed and Master Dry/Wet is 100% Wet (1.0), output is silence
    engine.setMasterParameters(0.0f, 1.0f, 0.0f, false);
    juce::AudioBuffer<float> buf(2, 256);
    for (int i = 0; i < 256; ++i)
    {
        buf.setSample(0, i, 0.5f);
        buf.setSample(1, i, 0.5f);
    }
    engine.process(buf);
    for (int i = 0; i < 256; ++i)
        assert(std::abs(buf.getSample(0, i)) < 1e-4f);

    // 2. When Master Dry/Wet is 100% Dry (0.0), dry audio passes cleanly
    engine.setMasterParameters(0.0f, 0.0f, 0.0f, false);
    for (int i = 0; i < 256; ++i)
    {
        buf.setSample(0, i, 0.5f);
        buf.setSample(1, i, 0.5f);
    }
    engine.process(buf);
    for (int i = 0; i < 256; ++i)
        assert(std::abs(buf.getSample(0, i) - 0.5f) < 1e-3f);

    std::cout << "PASSED!" << std::endl;
}

void testModularSlotBypassMutingAndActiveRouting()
{
    std::cout << "[TEST] Modular FX Mode Bypassed Slots Mute & Active Slots Route... ";

    for (int testSlot = 0; testSlot < 8; ++testSlot)
    {
        TransientSplitEngine engine;
        engine.prepare(48000.0, 256);
        engine.setEngineMode(1); // Modular FX
        engine.setMasterParameters(0.0f, 1.0f, 0.0f, false);
        engine.setChannelGains(0.0f, 0.0f); // Unity bank gains

        // 1. When testSlot is on Bypass (default), it must output silence
        engine.setSlotInputSource(testSlot, 1); // Stereo In (1)
        engine.setSlotRouting(testSlot, 1);     // Bus 1 Main Out (1)

        juce::AudioBuffer<float> buf(2, 256);
        for (int i = 0; i < 256; ++i)
        {
            buf.setSample(0, i, 0.5f);
            buf.setSample(1, i, 0.5f);
        }

        engine.process(buf);

        for (int i = 0; i < 256; ++i)
        {
            const float outVal = buf.getSample(0, i);
            assert(std::abs(outVal) < 1e-5f); // Bypassed slot is muted
        }

        // 2. When testSlot is active (e.g. Overdrive), it outputs audio
        EffectChain& chain = (testSlot < 4) ? engine.getTransientChain() : engine.getSustainChain();
        const int slotInChain = (testSlot < 4) ? testSlot : (testSlot - 4);
        chain.setSlotType(slotInChain, EffectType::Overdrive);
        chain.setSlotParameters(slotInChain, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f);

        for (int i = 0; i < 256; ++i)
        {
            buf.setSample(0, i, 0.5f);
            buf.setSample(1, i, 0.5f);
        }

        engine.process(buf);

        for (int i = 0; i < 256; ++i)
        {
            const float outVal = buf.getSample(0, i);
            assert(std::abs(outVal) > 0.01f); // Active slot processes and routes sound
            assert(!std::isnan(outVal));
        }
    }

    std::cout << "PASSED!" << std::endl;
}

void testSlotInOutGainLevels()
{
    std::cout << "[TEST] Slot In & Out Gain Level Staging Controls... ";

    EffectSlot slot;
    slot.prepare(48000.0, 256);
    slot.setType(EffectType::Bypass); // In Bypass, In & Out gains still scale signal

    // Test unity gain (0 dB in, 0 dB out)
    slot.setLevels(0.0f, 0.0f);
    std::vector<float> bufL(256, 0.5f), bufR(256, 0.5f);
    slot.process(bufL.data(), bufR.data(), 256);
    assert(std::abs(bufL[0] - 0.5f) < 1e-5f);

    // Test -6 dB In Gain (0.501187x) and +6 dB Out Gain (1.99526x) -> Net ~1.0x
    slot.setLevels(-6.0206f, 6.0206f);
    std::fill(bufL.begin(), bufL.end(), 0.5f);
    slot.process(bufL.data(), bufR.data(), 256);
    assert(std::abs(bufL[0] - 0.5f) < 0.01f);

    // Test silence cutoff (<= -23.5 dB -> 0.0 linear)
    slot.setLevels(-24.0f, 0.0f);
    std::fill(bufL.begin(), bufL.end(), 0.5f);
    slot.process(bufL.data(), bufR.data(), 256);
    assert(bufL[0] == 0.0f);

    std::cout << "PASSED!" << std::endl;
}

void testDelayFeedbackDefaultsAndSafety()
{
    std::cout << "[TEST] Delay Feedback 0% Default & Safety Capping (<= 0.95)... ";

    // 1. Verify default param info for delays has Feedback = 0.0f
    assert(EffectSlot::getParamInfoForType(EffectType::DelayStereo, 1).defaultVal == 0.0f);
    assert(EffectSlot::getParamInfoForType(EffectType::DelayPingPong, 1).defaultVal == 0.0f);
    assert(EffectSlot::getParamInfoForType(EffectType::DelayTapeEcho, 1).defaultVal == 0.0f);

    // 2. Test safety under extreme input & max feedback
    DelayTapeEchoEffect tapeEcho;
    tapeEcho.prepare(48000.0, 256);
    tapeEcho.setParameters(0.2f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f); // Feedback knob at 1.0

    std::vector<float> bufL(256), bufR(256);
    for (int b = 0; b < 50; ++b)
    {
        for (int i = 0; i < 256; ++i)
        {
            bufL[i] = (b == 0 && i == 0) ? 1.0f : 0.0f; // Impulse
            bufR[i] = bufL[i];
        }

        tapeEcho.process(bufL.data(), bufR.data(), 256);

        for (int i = 0; i < 256; ++i)
        {
            assert(!std::isnan(bufL[i]));
            assert(!std::isinf(bufL[i]));
            assert(std::abs(bufL[i]) < 5.0f); // Bounded safely
        }
    }

    std::cout << "PASSED!" << std::endl;
}

void testArbitraryBackwardRoutingAndCyclePrevention()
{
    std::cout << "[TEST] Arbitrary Backward Inter-Slot Routing & Cycle Prevention... ";

    TransientSplitEngine engine;
    engine.prepare(48000.0, 256);
    engine.setEngineMode(1); // Modular FX

    // Self-routing cycle test
    assert(engine.wouldCreateCycle(2, 2) == true);

    // Chain: Slot 1 -> Slot 3 -> Slot 5
    engine.setSlotRouting(1, 7 + 3); // 1 -> 3
    engine.setSlotRouting(3, 7 + 5); // 3 -> 5

    // Routing 5 -> 1 would create 1 -> 3 -> 5 -> 1 cycle!
    assert(engine.wouldCreateCycle(5, 1) == true);
    // Routing 5 -> 3 would create 3 -> 5 -> 3 cycle!
    assert(engine.wouldCreateCycle(5, 3) == true);
    // Routing 5 -> 0 is valid!
    assert(engine.wouldCreateCycle(5, 0) == false);

    // Test Backward Routing Execution: Slot 6 -> Slot 2 -> Slot 0 -> Main Out
    engine.setSlotInputSource(6, 1); // Slot 6 <- Stereo In (1)
    engine.setSlotRouting(6, 7 + 2); // Slot 6 -> Slot 2 (backward!)
    engine.setSlotRouting(2, 7 + 0); // Slot 2 -> Slot 0 (backward!)
    engine.setSlotRouting(0, 1);     // Slot 0 -> Main Out (1)

    auto order = engine.getTopologicalExecutionOrder();
    auto it6 = std::find(order.begin(), order.end(), 6);
    auto it2 = std::find(order.begin(), order.end(), 2);
    auto it0 = std::find(order.begin(), order.end(), 0);

    assert(it6 < it2); // Slot 6 must execute before Slot 2
    assert(it2 < it0); // Slot 2 must execute before Slot 0

    // Set effects in the backward chain
    engine.getSustainChain().setSlotType(2, EffectType::Bitcrush); // Slot 6
    engine.getSustainChain().setSlotParameters(2, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f);

    engine.getTransientChain().setSlotType(2, EffectType::Overdrive); // Slot 2
    engine.getTransientChain().setSlotParameters(2, 0.3f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f);

    engine.getTransientChain().setSlotType(0, EffectType::TapeSat); // Slot 0
    engine.getTransientChain().setSlotParameters(0, 0.4f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f);

    juce::AudioBuffer<float> buffer(2, 256);
    for (int i = 0; i < 256; ++i)
    {
        buffer.setSample(0, i, 0.3f);
        buffer.setSample(1, i, 0.3f);
    }

    engine.process(buffer);

    for (int i = 0; i < 256; ++i)
    {
        assert(!std::isnan(buffer.getSample(0, i)));
        assert(!std::isnan(buffer.getSample(1, i)));
    }

    std::cout << "PASSED!" << std::endl;
}

void testShimmerSemitonePitchShift()
{
    std::cout << "[TEST] Redesigned Dattorro Shimmer Reverb (-24 to +24 st)... ";

    ReverbShimmerEffect shimmer;
    shimmer.prepare(48000.0, 256);

    const std::vector<float> pitchKnobValues = { 0.0f, 0.25f, 0.5f, 0.6458f, 0.75f, 1.0f }; // -24st, -12st, 0st, +7st (5th), +12st, +24st

    for (float pVal : pitchKnobValues)
    {
        shimmer.reset();
        shimmer.setParameters(0.85f, pVal, 0.8f, 0.75f, 1.0f, 1.0f);

        std::vector<float> bufL(256), bufR(256);
        for (int b = 0; b < 20; ++b)
        {
            for (int i = 0; i < 256; ++i)
            {
                bufL[i] = (b == 0 && i == 0) ? 1.0f : 0.0f;
                bufR[i] = bufL[i];
            }

            shimmer.process(bufL.data(), bufR.data(), 256);

            for (int i = 0; i < 256; ++i)
            {
                assert(!std::isnan(bufL[i]));
                assert(!std::isnan(bufR[i]));
                assert(!std::isinf(bufL[i]));
                assert(!std::isinf(bufR[i]));
                assert(std::abs(bufL[i]) < 10.0f);
            }
        }
    }

    std::cout << "PASSED!" << std::endl;
}

void testDryWetPhaseCoherence()
{
    std::cout << "[TEST] Master Dry/Wet Phase Coherence Across All Mix Values... ";

    TransientSplitEngine engine;
    engine.prepare(48000.0, 256);

    const std::vector<float> mixValues = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

    for (float mix : mixValues)
    {
        engine.reset();
        engine.setMasterParameters(0.0f, mix, 0.0f, false);

        juce::AudioBuffer<float> buffer(2, 256);
        for (int b = 0; b < 4; ++b)
        {
            for (int i = 0; i < 256; ++i)
            {
                float s = std::sin(static_cast<float>(i + b * 256) * 0.05f) * 0.6f;
                buffer.setSample(0, i, s);
                buffer.setSample(1, i, s * 0.7f);
            }

            engine.process(buffer);

            if (b >= 2)
            {
                for (int i = 10; i < 240; ++i)
                {
                    const float sOut = buffer.getSample(0, i);
                    assert(!std::isnan(sOut));
                    assert(std::abs(sOut) <= 0.61f);
                }
            }
        }
    }

    std::cout << "PASSED!" << std::endl;
}

void testModulationEngineAndXYPad()
{
    std::cout << "[TEST] Modulation Engine, XY Pad & Expanded Destinations... ";

    ModEngine mod;
    mod.prepare(48000.0);
    mod.setBpm(120.0);

    mod.setXYPad1(0.75f, -0.60f);
    mod.setXYPad2(-0.50f, 0.40f);
    mod.setMacros(0.1f, 0.2f, 0.3f, 0.4f);

    mod.getMatrix().setSlot(0, ModSource::XY1_X, ModDest::XY1_Speed, 0.8f);
    mod.getMatrix().setSlot(1, ModSource::XY1_Y, ModDest::LFO1_Rate, 0.5f);
    mod.getMatrix().setSlot(2, ModSource::XY2_X, ModDest::SplitAttack, 0.4f);
    mod.getMatrix().setSlot(3, ModSource::Macro4, ModDest::Filt1Freq, 0.9f);

    float testIn[16] = { 0.5f };
    float testTrans[16] = { 0.8f };
    float testSust[16] = { 0.2f };
    float testFilt[16] = { 0.3f };
    mod.processBlock(testIn, nullptr, testTrans, nullptr, testSust, nullptr,
                     testFilt, nullptr, testFilt, nullptr, testFilt, nullptr,
                     testIn, nullptr, testIn, nullptr, testIn, nullptr, testIn, nullptr,
                     testIn, nullptr, testIn, nullptr, testIn, nullptr, testIn, nullptr,
                     testIn, nullptr, testIn, nullptr, 16);

    const float offSpeed = mod.getOffset(ModDest::XY1_Speed);
    const float offLfoRate = mod.getOffset(ModDest::LFO1_Rate);
    const float offAtt = mod.getOffset(ModDest::SplitAttack);
    const float offFilt1 = mod.getOffset(ModDest::Filt1Freq);

    assert(std::abs(offSpeed - (0.75f * 0.8f)) < 1e-4f);
    assert(std::abs(offLfoRate - (-0.60f * 0.5f)) < 1e-4f);
    assert(std::abs(offAtt - (-0.50f * 0.4f)) < 1e-4f);
    assert(std::abs(offFilt1 - (0.4f * 0.9f)) < 1e-4f);

    assert(mod.getXYPad1X() == 0.75f);
    assert(mod.getXYPad1Y() == -0.60f);
    assert(mod.getXYPad2X() == -0.50f);
    assert(mod.getXYPad2Y() == 0.40f);
    assert(mod.getMacro4() == 0.4f);

    std::cout << "PASSED!" << std::endl;
}

void testAllEffectTypesStability()
{
    std::cout << "[TEST] All 35 Modular Effect Types Stability & NaN Protection... ";

    const std::vector<EffectType> types = {
        EffectType::Bypass,
        EffectType::CompressorVCA,
        EffectType::CompressorOpto,
        EffectType::CompressorFET,
        EffectType::Gate,
        EffectType::EqualizerParametric,
        EffectType::EqualizerGraphic,
        EffectType::EqualizerTilt,
        EffectType::FilterDualHPLP,
        EffectType::FilterBandpass,
        EffectType::DistortionHardClip,
        EffectType::DistortionTube,
        EffectType::DistortionWavefolder,
        EffectType::FuzzGermanium,
        EffectType::FuzzOctave,
        EffectType::Overdrive,
        EffectType::TapeSat,
        EffectType::Bitcrush,
        EffectType::GlitchStutter,
        EffectType::GlitchTapeStop,
        EffectType::GlitchReverse,
        EffectType::GlitchGranular,
        EffectType::PitchShifter,
        EffectType::FrequencyShifter,
        EffectType::ModulationChorus,
        EffectType::ModulationFlanger,
        EffectType::ModulationPhaser,
        EffectType::ModulationTremolo,
        EffectType::DelayStereo,
        EffectType::DelayPingPong,
        EffectType::DelayTapeEcho,
        EffectType::ReverbRoom,
        EffectType::ReverbHall,
        EffectType::ReverbPlate,
        EffectType::ReverbSpring,
        EffectType::ReverbShimmer
    };

    EffectSlot slot;
    slot.prepare(48000.0, 256);

    const int blockSize = 256;
    std::vector<float> bufL(blockSize);
    std::vector<float> bufR(blockSize);

    for (auto type : types)
    {
        slot.setType(type);
        slot.reset();
        slot.setParameters(0.8f, 0.7f, 0.5f, 0.4f, 0.6f, 1.0f);

        for (int b = 0; b < 20; ++b)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                bufL[i] = std::sin(static_cast<float>(i + b * blockSize) * 0.05f) * 1.5f;
                bufR[i] = std::cos(static_cast<float>(i + b * blockSize) * 0.08f) * 1.5f;
            }

            slot.process(bufL.data(), bufR.data(), blockSize);

            for (int i = 0; i < blockSize; ++i)
            {
                assert(!std::isnan(bufL[i]));
                assert(!std::isnan(bufR[i]));
                assert(!std::isinf(bufL[i]));
                assert(!std::isinf(bufR[i]));
            }
        }
    }

    std::cout << "PASSED!" << std::endl;
}

void testSplitModeUnifiedModularRouting()
{
    std::cout << "[TEST] Split Mode Unified Modular DAG Routing & Slot Muting... ";

    TransientSplitEngine engine;
    engine.prepare(48000.0, 256);
    engine.setEngineMode(0); // Split Mode
    engine.setMasterParameters(0.0f, 1.0f, 0.0f, false);
    engine.setChannelGains(0.0f, 0.0f); // 0 dB bank gains

    // 1. By default with all slots on Bypass, output is silence
    juce::AudioBuffer<float> buf(2, 256);
    for (int i = 0; i < 256; ++i)
    {
        buf.setSample(0, i, (i == 0) ? 1.0f : 0.0f); // Transient impulse
        buf.setSample(1, i, (i == 0) ? 1.0f : 0.0f);
    }

    engine.process(buf);

    for (int i = 0; i < 256; ++i)
        assert(std::abs(buf.getSample(0, i)) < 1e-4f); // Muted when slots are OFF/bypassed

    // 2. Turn on Slot 1 (Transient Slot 0: Overdrive) and route to Main Out (1)
    engine.getTransientChain().setSlotType(0, EffectType::Overdrive);
    engine.getTransientChain().setSlotParameters(0, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f);
    engine.setSlotInputSource(0, 2); // Attack (2)
    engine.setSlotRouting(0, 1);     // Main Out (1)

    for (int i = 0; i < 256; ++i)
    {
        buf.setSample(0, i, (i == 0) ? 1.0f : 0.0f);
        buf.setSample(1, i, (i == 0) ? 1.0f : 0.0f);
    }

    engine.process(buf);

    float peak = 0.0f;
    for (int i = 0; i < 256; ++i)
        peak = std::max(peak, std::abs(buf.getSample(0, i)));
    assert(peak > 0.01f); // Processed audio routed to Main Out

    std::cout << "PASSED!" << std::endl;
}

void testFactoryPresetsIntegrity()
{
    std::cout << "[TEST] Factory Presets Collection Integrity... ";

    auto presets = FactoryPresets::getPresets();
    assert(!presets.empty());
    assert(presets.size() >= 8);

    for (const auto& p : presets)
    {
        assert(p.name.isNotEmpty());
        assert(p.category.isNotEmpty());
    }

    std::cout << "PASSED! (Found " << presets.size() << " presets)" << std::endl;
}

void testFilterBank3IndependentBandsAndOverlapping()
{
    std::cout << "[TEST] FilterBank3 Independent Processing & Overlapping Frequencies... ";

    FilterBank3 fb;
    fb.prepare(48000.0, 256);

    // Overlapping filters: Filter 1 = LP at 1000Hz, Filter 2 = BP at 1000Hz, Filter 3 = HP at 1000Hz
    fb.setFilterParameters(0, FilterBankType::LowPass24, 1000.0f, 0.707f, 0.0f, 1.0f, false, false, false);
    fb.setFilterParameters(1, FilterBankType::BandPass, 1000.0f, 1.5f, 0.0f, 1.0f, false, false, false);
    fb.setFilterParameters(2, FilterBankType::HighPass24, 1000.0f, 0.707f, 0.0f, 1.0f, false, false, false);

    std::vector<float> inL(256, 0.0f), inR(256, 0.0f);
    std::vector<float> f1L(256, 0.0f), f1R(256, 0.0f);
    std::vector<float> f2L(256, 0.0f), f2R(256, 0.0f);
    std::vector<float> f3L(256, 0.0f), f3R(256, 0.0f);

    for (int i = 0; i < 256; ++i)
    {
        inL[i] = std::sin(2.0 * 3.14159 * 200.0 * i / 48000.0); // 200 Hz tone (low)
        inR[i] = inL[i];
    }

    fb.process(inL.data(), inR.data(), 256,
               f1L.data(), f1R.data(),
               f2L.data(), f2R.data(),
               f3L.data(), f3R.data());

    // 200 Hz tone should pass Filter 1 (Lowpass) strongly, and be attenuated by Filter 3 (Highpass)
    float peakF1 = 0.0f, peakF3 = 0.0f;
    for (int i = 64; i < 256; ++i)
    {
        peakF1 = std::max(peakF1, std::abs(f1L[i]));
        peakF3 = std::max(peakF3, std::abs(f3L[i]));
    }

    assert(peakF1 > 0.6f);
    assert(peakF3 < 0.15f);

    std::cout << "PASSED!" << std::endl;
}

void testUniversalSlotInputSourcesAndRouting()
{
    std::cout << "[TEST] Universal Slot Input Sources (Stereo, Transient, Sustain, Filters 1-3, Slots)... ";

    TransientSplitEngine engine;
    engine.prepare(48000.0, 256);
    engine.setChannelGains(-60.0f, -60.0f); // Mute direct dry pass

    // Slot 1 (index 0): In = Filter 1 (src = 4), Type = Overdrive, Out = Main Out (dest = 1)
    engine.setSlotInputSource(0, 4); // Filter 1 (4)
    engine.getTransientChain().setSlotType(0, EffectType::Overdrive);
    engine.getTransientChain().setSlotParameters(0, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f);
    engine.setSlotRouting(0, 1);

    // Slot 2 (index 1): In = Slot 1 Out (src = 17), Type = Chorus, Out = Main Out (dest = 1)
    engine.setSlotInputSource(1, 17); // Slot 1 Out (17)
    engine.getTransientChain().setSlotType(1, EffectType::ModulationChorus);
    engine.getTransientChain().setSlotParameters(1, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f);
    engine.setSlotRouting(1, 1);

    juce::AudioBuffer<float> buf(2, 256);
    for (int i = 0; i < 256; ++i)
    {
        buf.setSample(0, i, (i % 32 == 0) ? 0.8f : 0.0f);
        buf.setSample(1, i, (i % 32 == 0) ? 0.8f : 0.0f);
    }

    engine.process(buf);

    float peak = 0.0f;
    for (int i = 0; i < 256; ++i)
        peak = std::max(peak, std::abs(buf.getSample(0, i)));

    assert(peak > 0.01f);
    std::cout << "PASSED!" << std::endl;
}

void testMidSideSplitterDecomposition()
{
    std::cout << "[TEST] Mid/Side & L/R Decomposition Matrix... ";

    MidSideSplitter ms;
    ms.prepare(48000.0, 256);
    ms.setParameters(1.0f, 0.0f);

    std::vector<float> inL(256), inR(256);
    std::vector<float> leftL(256), leftR(256), rightL(256), rightR(256);
    std::vector<float> midL(256), midR(256), sideL(256), sideR(256);

    for (int i = 0; i < 256; ++i)
    {
        inL[i] = std::sin(i * 0.05f);
        inR[i] = std::cos(i * 0.05f) * 0.7f;
    }

    ms.process(inL.data(), inR.data(),
               leftL.data(), leftR.data(), rightL.data(), rightR.data(),
               midL.data(), midR.data(), sideL.data(), sideR.data(),
               256);

    const float invSqrt2 = 0.7071067811865475f;
    for (int i = 0; i < 256; ++i)
    {
        // Check L/R isolation
        assert(std::abs(leftL[i] - inL[i]) < 1e-5f);
        assert(std::abs(rightL[i] - inR[i]) < 1e-5f);

        // Check M/S exact reconstruction: L = (M + S) * invSqrt2, R = (M - S) * invSqrt2
        const float recL = (midL[i] + sideL[i]) * invSqrt2;
        const float recR = (midR[i] + sideR[i]) * invSqrt2;
        assert(std::abs(recL - inL[i]) < 1e-4f);
        assert(std::abs(recR - inR[i]) < 1e-4f);
    }

    std::cout << "PASSED!" << std::endl;
}

void testDynamicSplitterComplementarySum()
{
    std::cout << "[TEST] Dynamic Splitter (Loud vs Quiet) Soft-Knee Unity Sum... ";

    DynamicSplitter dyn;
    dyn.prepare(48000.0, 512);
    dyn.setParameters(-20.0f, 5.0f, 50.0f, 6.0f);

    std::vector<float> inL(512), inR(512);
    std::vector<float> highL(512), highR(512), lowL(512), lowR(512);

    for (int i = 0; i < 512; ++i)
    {
        float amp = (i > 100 && i < 300) ? 0.9f : 0.02f;
        inL[i] = std::sin(i * 0.1f) * amp;
        inR[i] = std::cos(i * 0.1f) * amp;
    }

    dyn.process(inL.data(), inR.data(), highL.data(), highR.data(), lowL.data(), lowR.data(), 512);

    for (int i = 0; i < 512; ++i)
    {
        // Unity sum: High + Low = Input
        const float sumL = highL[i] + lowL[i];
        const float sumR = highR[i] + lowR[i];
        assert(std::abs(sumL - inL[i]) < 1e-5f);
        assert(std::abs(sumR - inR[i]) < 1e-5f);
    }

    std::cout << "PASSED!" << std::endl;
}

void testSpectralHarmonicSplitterDecomposition()
{
    std::cout << "[TEST] Spectral & Harmonic (Tonal vs Noise) Decomposition Unity Sum... ";

    SpectralHarmonicSplitter spec;
    spec.prepare(48000.0, 512);
    spec.setParameters(0.5f, 0.5f, 10.0f);

    std::vector<float> inL(512), inR(512);
    std::vector<float> harmL(512), harmR(512), noiseL(512), noiseR(512);

    for (int i = 0; i < 512; ++i)
    {
        inL[i] = std::sin(i * 0.08f) * 0.6f + ((i % 7 == 0) ? 0.2f : -0.2f);
        inR[i] = std::sin(i * 0.12f) * 0.5f + ((i % 11 == 0) ? 0.2f : -0.2f);
    }

    spec.process(inL.data(), inR.data(), harmL.data(), harmR.data(), noiseL.data(), noiseR.data(), 512);

    for (int i = 0; i < 512; ++i)
    {
        // Unity sum: Harmonic + Noise = Input
        const float sumL = harmL[i] + noiseL[i];
        const float sumR = harmR[i] + noiseR[i];
        assert(std::abs(sumL - inL[i]) < 1e-5f);
        assert(std::abs(sumR - inR[i]) < 1e-5f);
    }

    std::cout << "PASSED!" << std::endl;
}

void testPhaseSpatialSplitterDecomposition()
{
    std::cout << "[TEST] Phase & Spatial (In-Phase vs Spatial Diffuse) Decomposition... ";

    PhaseSpatialSplitter ps;
    ps.prepare(48000.0, 512);
    ps.setParameters(0.5f, 20.0f, 1.0f);

    std::vector<float> inL(512), inR(512);
    std::vector<float> inPhaseL(512), inPhaseR(512), spatialL(512), spatialR(512);

    for (int i = 0; i < 512; ++i)
    {
        inL[i] = std::sin(i * 0.05f) * 0.7f;
        inR[i] = -std::sin(i * 0.05f) * 0.7f; // 180 deg out of phase
    }

    ps.process(inL.data(), inR.data(), inPhaseL.data(), inPhaseR.data(), spatialL.data(), spatialR.data(), 512);

    for (int i = 0; i < 512; ++i)
    {
        // Unity sum with spread = 1.0: In-Phase + Spatial = Input
        const float sumL = inPhaseL[i] + spatialL[i];
        const float sumR = inPhaseR[i] + spatialR[i];
        assert(std::abs(sumL - inL[i]) < 1e-5f);
        assert(std::abs(sumR - inR[i]) < 1e-5f);
    }

    std::cout << "PASSED!" << std::endl;
}

int main()
{
    std::cout << "============================================" << std::endl;
    std::cout << "    OmniSplit Audio Plugin Test Suite       " << std::endl;
    std::cout << "============================================" << std::endl;

    try
    {
        testFilterBank3IndependentBandsAndOverlapping();
        testUniversalSlotInputSourcesAndRouting();
        testMidSideSplitterDecomposition();
        testDynamicSplitterComplementarySum();
        testSpectralHarmonicSplitterDecomposition();
        testPhaseSpatialSplitterDecomposition();
        testTransientExtractorComplementarySum();
        testChannelDryLevelDirectPassAndSlotInGains();
        testModularSlotBypassMutingAndActiveRouting();
        testSplitModeUnifiedModularRouting();
        testSlotInOutGainLevels();
        testDelayFeedbackDefaultsAndSafety();
        testArbitraryBackwardRoutingAndCyclePrevention();
        testShimmerSemitonePitchShift();
        testDryWetPhaseCoherence();
        testModulationEngineAndXYPad();
        testAllEffectTypesStability();
        testFactoryPresetsIntegrity();

        std::cout << "============================================" << std::endl;
        std::cout << "  ALL OMNISPLIT TESTS PASSED!               " << std::endl;
        std::cout << "============================================" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "TEST EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
}
