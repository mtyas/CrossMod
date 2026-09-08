#include "TransientSplitProcessor.h"
#include "UI/TransientSplitEditor.h"

TransientSplitProcessor::TransientSplitProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Out 1 (Main)", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Out 2 (Aux 1)", juce::AudioChannelSet::stereo(), false)
                     .withOutput("Out 3 (Aux 2)", juce::AudioChannelSet::stereo(), false)
                     .withOutput("Out 4 (Aux 3)", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, &undoManager, "Parameters", ParameterFactory::createParameterLayout()),
      presetManager(apvts)
{
}

TransientSplitProcessor::~TransientSplitProcessor() = default;

void TransientSplitProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
    updateEngineParameters();
    setLatencySamples(engine.getExtractor().getLatencySamples());
}

void TransientSplitProcessor::releaseResources()
{
    engine.reset();
}

bool TransientSplitProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut && mainIn != juce::AudioChannelSet::mono())
        return false;

    for (int b = 1; b < layouts.outputBuses.size(); ++b)
    {
        const auto& auxOut = layouts.outputBuses[b];
        if (!auxOut.isDisabled() && auxOut != juce::AudioChannelSet::mono() && auxOut != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void TransientSplitProcessor::updateEngineParameters()
{
    auto& mod = engine.getModEngine();

    // 0. Engine Mode
    const int engMode = static_cast<int>(apvts.getRawParameterValue(IDs::engineMode.getParamID())->load());
    engine.setEngineMode(engMode);

    // 1. Modulators: 3 LFOs
    auto updateLfo = [&](ModLFO& lfo, const juce::ParameterID& waveId, const juce::ParameterID& rateId,
                         const juce::ParameterID& syncId, const juce::ParameterID& subdivId,
                         const juce::ParameterID& smoothId, ModDest rateModDest)
    {
        const int w = static_cast<int>(apvts.getRawParameterValue(waveId.getParamID())->load());
        float r = apvts.getRawParameterValue(rateId.getParamID())->load();
        const bool s = apvts.getRawParameterValue(syncId.getParamID())->load() > 0.5f;
        const int div = static_cast<int>(apvts.getRawParameterValue(subdivId.getParamID())->load());
        float sm = apvts.getRawParameterValue(smoothId.getParamID())->load();
        r = std::clamp(r + mod.getOffset(rateModDest), 0.01f, 50.0f);
        lfo.setParameters(static_cast<LfoWaveform>(w), r, s, div, sm);
    };

    updateLfo(mod.getLFO1(), IDs::lfo1Wave, IDs::lfo1Rate, IDs::lfo1Sync, IDs::lfo1Subdiv, IDs::lfo1Smooth, ModDest::LFO1_Rate);
    updateLfo(mod.getLFO2(), IDs::lfo2Wave, IDs::lfo2Rate, IDs::lfo2Sync, IDs::lfo2Subdiv, IDs::lfo2Smooth, ModDest::LFO2_Rate);
    updateLfo(mod.getLFO3(), IDs::lfo3Wave, IDs::lfo3Rate, IDs::lfo3Sync, IDs::lfo3Subdiv, IDs::lfo3Smooth, ModDest::LFO3_Rate);

    // 2. Modulators: 3 Envelope Followers
    auto updateEnv = [&](ModEnvelopeFollower& env, const juce::ParameterID& srcId, const juce::ParameterID& attId,
                         const juce::ParameterID& relId, const juce::ParameterID& gainId)
    {
        const int src = static_cast<int>(apvts.getRawParameterValue(srcId.getParamID())->load());
        const float att = apvts.getRawParameterValue(attId.getParamID())->load();
        const float rel = apvts.getRawParameterValue(relId.getParamID())->load();
        const float g   = apvts.getRawParameterValue(gainId.getParamID())->load();
        env.setParameters(static_cast<EnvFollowerSource>(src), att, rel, g);
    };

    updateEnv(mod.getEnv1(), IDs::env1Source, IDs::env1Attack, IDs::env1Release, IDs::env1Gain);
    updateEnv(mod.getEnv2(), IDs::env2Source, IDs::env2Attack, IDs::env2Release, IDs::env2Gain);
    updateEnv(mod.getEnv3(), IDs::env3Source, IDs::env3Attack, IDs::env3Release, IDs::env3Gain);

    // 3. Modulators: 2 Turing Machines
    auto updateTuring = [&](ModTuringMachine& tur, const juce::ParameterID& probId, const juce::ParameterID& lenId,
                            const juce::ParameterID& syncId, const juce::ParameterID& subdivId,
                            const juce::ParameterID& rateId, const juce::ParameterID& glideId, ModDest probMod)
    {
        float p = apvts.getRawParameterValue(probId.getParamID())->load();
        const int len = static_cast<int>(apvts.getRawParameterValue(lenId.getParamID())->load());
        const bool s = apvts.getRawParameterValue(syncId.getParamID())->load() > 0.5f;
        const int div = static_cast<int>(apvts.getRawParameterValue(subdivId.getParamID())->load());
        const float r = apvts.getRawParameterValue(rateId.getParamID())->load();
        float g = apvts.getRawParameterValue(glideId.getParamID())->load();
        p = std::clamp(p + mod.getOffset(probMod), 0.0f, 1.0f);
        tur.setParameters(p, len, s, div, r, g);
    };

    updateTuring(mod.getTuring1(), IDs::turing1Prob, IDs::turing1Length, IDs::turing1Sync, IDs::turing1Subdiv, IDs::turing1Rate, IDs::turing1Glide, ModDest::Turing1_Prob);
    updateTuring(mod.getTuring2(), IDs::turing2Prob, IDs::turing2Length, IDs::turing2Sync, IDs::turing2Subdiv, IDs::turing2Rate, IDs::turing2Glide, ModDest::Turing2_Prob);

    // 4. 4 Macros
    const float m1 = apvts.getRawParameterValue(IDs::macro1.getParamID())->load();
    const float m2 = apvts.getRawParameterValue(IDs::macro2.getParamID())->load();
    const float m3 = apvts.getRawParameterValue(IDs::macro3.getParamID())->load();
    const float m4 = apvts.getRawParameterValue(IDs::macro4.getParamID())->load();
    mod.setMacros(m1, m2, m3, m4);

    // 5. 2 XY Pads
    const float xy1X = apvts.getRawParameterValue(IDs::xy1_x.getParamID())->load();
    const float xy1Y = apvts.getRawParameterValue(IDs::xy1_y.getParamID())->load();
    mod.setXYPad1(xy1X, xy1Y);

    const float xy2X = apvts.getRawParameterValue(IDs::xy2_x.getParamID())->load();
    const float xy2Y = apvts.getRawParameterValue(IDs::xy2_y.getParamID())->load();
    mod.setXYPad2(xy2X, xy2Y);

    // 6. Mod Matrix Routings (8 slots)
    for (int i = 0; i < ModMatrix::NumRoutingSlots; ++i)
    {
        const int src = static_cast<int>(apvts.getRawParameterValue(IDs::getModSlotSource(i).getParamID())->load());
        const int dst = static_cast<int>(apvts.getRawParameterValue(IDs::getModSlotDest(i).getParamID())->load());
        const float depth = apvts.getRawParameterValue(IDs::getModSlotDepth(i).getParamID())->load();
        mod.getMatrix().setSlot(i, static_cast<ModSource>(src), static_cast<ModDest>(dst), depth);
    }

    // 7. Pre-Processors: Transient Splitter Controls
    float attackMs    = apvts.getRawParameterValue(IDs::transientAttack.getParamID())->load();
    float releaseMs   = apvts.getRawParameterValue(IDs::transientRelease.getParamID())->load();
    float sensitivity = apvts.getRawParameterValue(IDs::transientSensitivity.getParamID())->load();
    float threshDb    = apvts.getRawParameterValue(IDs::transientThreshold.getParamID())->load();
    const int modeIdx       = static_cast<int>(apvts.getRawParameterValue(IDs::detectionMode.getParamID())->load());
    const float lookahead   = apvts.getRawParameterValue(IDs::lookahead.getParamID())->load();
    const float transGain   = apvts.getRawParameterValue(IDs::transientGain.getParamID())->load();
    const float sustGain    = apvts.getRawParameterValue(IDs::sustainGain.getParamID())->load();

    attackMs    = std::clamp(attackMs + mod.getOffset(ModDest::SplitAttack) * 50.0f, 1.0f, 100.0f);
    releaseMs   = std::clamp(releaseMs + mod.getOffset(ModDest::SplitRelease) * 200.0f, 10.0f, 500.0f);
    sensitivity = std::clamp(sensitivity + mod.getOffset(ModDest::SplitSensitivity), 0.0f, 1.0f);
    threshDb    = std::clamp(threshDb + mod.getOffset(ModDest::SplitThreshold) * 30.0f, -60.0f, 0.0f);

    engine.setSplitParameters(attackMs, releaseMs, sensitivity, threshDb,
                              static_cast<DetectionMode>(modeIdx), lookahead);
    engine.setChannelGains(transGain, sustGain);

    // 8. Pre-Processors: 3-Band ZDF Filter Bank
    for (int b = 0; b < 3; ++b)
    {
        const int typeIdx = static_cast<int>(apvts.getRawParameterValue(IDs::getFilterType(b).getParamID())->load());
        float freq  = apvts.getRawParameterValue(IDs::getFilterFreq(b).getParamID())->load();
        float q     = apvts.getRawParameterValue(IDs::getFilterQ(b).getParamID())->load();
        float gain  = apvts.getRawParameterValue(IDs::getFilterGain(b).getParamID())->load();
        float drive = apvts.getRawParameterValue(IDs::getFilterDrive(b).getParamID())->load();
        const bool bypass = apvts.getRawParameterValue(IDs::getFilterBypass(b).getParamID())->load() > 0.5f;
        const bool solo   = apvts.getRawParameterValue(IDs::getFilterSolo(b).getParamID())->load() > 0.5f;
        const bool mute   = apvts.getRawParameterValue(IDs::getFilterMute(b).getParamID())->load() > 0.5f;

        const int baseFiltDst = 5 + b * 4;
        freq  = std::clamp(freq * std::pow(2.0f, mod.getOffset(static_cast<ModDest>(baseFiltDst + 0)) * 2.0f), 20.0f, 20000.0f);
        q     = std::clamp(q + mod.getOffset(static_cast<ModDest>(baseFiltDst + 1)) * 5.0f, 0.1f, 18.0f);
        gain  = std::clamp(gain + mod.getOffset(static_cast<ModDest>(baseFiltDst + 2)) * 12.0f, -24.0f, 24.0f);
        drive = std::clamp(drive + mod.getOffset(static_cast<ModDest>(baseFiltDst + 3)) * 2.0f, 1.0f, 10.0f);

        engine.setFilterParameters(b, static_cast<FilterBankType>(typeIdx), freq, q, gain, drive, bypass, solo, mute);
    }

    // 8b. Pre-Processors: Mid/Side & Stereo
    const float msWidth = apvts.getRawParameterValue(IDs::midSideWidth.getParamID())->load();
    const float msBal   = apvts.getRawParameterValue(IDs::midSideBalance.getParamID())->load();
    engine.setMidSideParameters(msWidth, msBal);

    // 8c. Pre-Processors: Dynamic Splitter
    const float dynThresh = apvts.getRawParameterValue(IDs::dynSplitThreshold.getParamID())->load();
    const float dynAtt    = apvts.getRawParameterValue(IDs::dynSplitAttack.getParamID())->load();
    const float dynRel    = apvts.getRawParameterValue(IDs::dynSplitRelease.getParamID())->load();
    const float dynKnee   = apvts.getRawParameterValue(IDs::dynSplitKnee.getParamID())->load();
    engine.setDynamicSplitParameters(dynThresh, dynAtt, dynRel, dynKnee);

    // 8d. Pre-Processors: Spectral & Harmonic
    const float specSens  = apvts.getRawParameterValue(IDs::spectralHarmonicSens.getParamID())->load();
    const float specFocus = apvts.getRawParameterValue(IDs::spectralFocus.getParamID())->load();
    const float specSmooth = apvts.getRawParameterValue(IDs::spectralSmoothing.getParamID())->load();
    engine.setSpectralHarmonicParameters(specSens, specFocus, specSmooth);

    // 8e. Pre-Processors: Phase & Spatial
    const float phaseThresh = apvts.getRawParameterValue(IDs::phaseSpatialThresh.getParamID())->load();
    const float phaseWindow = apvts.getRawParameterValue(IDs::phaseSpatialWindow.getParamID())->load();
    const float phaseSpread = apvts.getRawParameterValue(IDs::phaseSpatialSpread.getParamID())->load();
    engine.setPhaseSpatialParameters(phaseThresh, phaseWindow, phaseSpread);

    // 9. Master Controls
    float masterBal          = apvts.getRawParameterValue(IDs::masterBalance.getParamID())->load();
    masterBal                = std::clamp(masterBal + mod.getOffset(ModDest::Master_Balance), -1.0f, 1.0f);
    float masterDryWet       = apvts.getRawParameterValue(IDs::masterDryWet.getParamID())->load();
    masterDryWet             = std::clamp(masterDryWet + mod.getOffset(ModDest::Master_DryWet), 0.0f, 1.0f);
    float masterGain         = apvts.getRawParameterValue(IDs::masterGain.getParamID())->load();
    masterGain               = std::clamp(masterGain + mod.getOffset(ModDest::Master_Gain) * 12.0f, -24.0f, 24.0f);
    const bool limiterOn     = apvts.getRawParameterValue(IDs::masterLimiter.getParamID())->load() > 0.5f;

    engine.setMasterParameters(masterBal, masterDryWet, masterGain, limiterOn);

    // 10. Transient Chain Slots (Slots 1..4)
    auto& tChain = engine.getTransientChain();
    for (int i = 0; i < 4; ++i)
    {
        const int src       = static_cast<int>(apvts.getRawParameterValue(IDs::getTransSlotSrc(i).getParamID())->load());
        const int typeIdx   = static_cast<int>(apvts.getRawParameterValue(IDs::getTransSlotType(i).getParamID())->load());
        const bool bypass   = apvts.getRawParameterValue(IDs::getTransSlotBypass(i).getParamID())->load() > 0.5f;
        float p1            = apvts.getRawParameterValue(IDs::getTransSlotP1(i).getParamID())->load();
        float p2            = apvts.getRawParameterValue(IDs::getTransSlotP2(i).getParamID())->load();
        float p3            = apvts.getRawParameterValue(IDs::getTransSlotP3(i).getParamID())->load();
        float p4            = apvts.getRawParameterValue(IDs::getTransSlotP4(i).getParamID())->load();
        float p5            = apvts.getRawParameterValue(IDs::getTransSlotP5(i).getParamID())->load();
        float mix           = apvts.getRawParameterValue(IDs::getTransSlotMix(i).getParamID())->load();
        const float inGain  = apvts.getRawParameterValue(IDs::getTransSlotInGain(i).getParamID())->load();
        const float outGain = apvts.getRawParameterValue(IDs::getTransSlotOutGain(i).getParamID())->load();
        const int dest      = static_cast<int>(apvts.getRawParameterValue(IDs::getTransSlotDest(i).getParamID())->load());

        // Mod Matrix offsets: Slot 1..8 start at ModDest index 17
        const int baseDst = 17 + i * 6;
        p1  = std::clamp(p1 + mod.getOffset(static_cast<ModDest>(baseDst + 0)), 0.0f, 1.0f);
        p2  = std::clamp(p2 + mod.getOffset(static_cast<ModDest>(baseDst + 1)), 0.0f, 1.0f);
        p3  = std::clamp(p3 + mod.getOffset(static_cast<ModDest>(baseDst + 2)), 0.0f, 1.0f);
        p4  = std::clamp(p4 + mod.getOffset(static_cast<ModDest>(baseDst + 3)), 0.0f, 1.0f);
        p5  = std::clamp(p5 + mod.getOffset(static_cast<ModDest>(baseDst + 4)), 0.0f, 1.0f);
        mix = std::clamp(mix + mod.getOffset(static_cast<ModDest>(baseDst + 5)), 0.0f, 1.0f);

        tChain.setSlotType(i, static_cast<EffectType>(typeIdx));
        tChain.setSlotBypass(i, bypass);
        tChain.setSlotParameters(i, p1, p2, p3, p4, p5, mix);
        tChain.setSlotLevels(i, inGain, outGain);
        engine.setSlotInputSource(i, src);
        engine.setSlotRouting(i, dest);
    }

    // 11. Sustain Chain Slots (Slots 5..8)
    auto& sChain = engine.getSustainChain();
    for (int i = 0; i < 4; ++i)
    {
        const int src       = static_cast<int>(apvts.getRawParameterValue(IDs::getSustSlotSrc(i).getParamID())->load());
        const int typeIdx   = static_cast<int>(apvts.getRawParameterValue(IDs::getSustSlotType(i).getParamID())->load());
        const bool bypass   = apvts.getRawParameterValue(IDs::getSustSlotBypass(i).getParamID())->load() > 0.5f;
        float p1            = apvts.getRawParameterValue(IDs::getSustSlotP1(i).getParamID())->load();
        float p2            = apvts.getRawParameterValue(IDs::getSustSlotP2(i).getParamID())->load();
        float p3            = apvts.getRawParameterValue(IDs::getSustSlotP3(i).getParamID())->load();
        float p4            = apvts.getRawParameterValue(IDs::getSustSlotP4(i).getParamID())->load();
        float p5            = apvts.getRawParameterValue(IDs::getSustSlotP5(i).getParamID())->load();
        float mix           = apvts.getRawParameterValue(IDs::getSustSlotMix(i).getParamID())->load();
        const float inGain  = apvts.getRawParameterValue(IDs::getSustSlotInGain(i).getParamID())->load();
        const float outGain = apvts.getRawParameterValue(IDs::getSustSlotOutGain(i).getParamID())->load();
        const int dest      = static_cast<int>(apvts.getRawParameterValue(IDs::getSustSlotDest(i).getParamID())->load());

        const int baseDst = 41 + i * 6;
        p1  = std::clamp(p1 + mod.getOffset(static_cast<ModDest>(baseDst + 0)), 0.0f, 1.0f);
        p2  = std::clamp(p2 + mod.getOffset(static_cast<ModDest>(baseDst + 1)), 0.0f, 1.0f);
        p3  = std::clamp(p3 + mod.getOffset(static_cast<ModDest>(baseDst + 2)), 0.0f, 1.0f);
        p4  = std::clamp(p4 + mod.getOffset(static_cast<ModDest>(baseDst + 3)), 0.0f, 1.0f);
        p5  = std::clamp(p5 + mod.getOffset(static_cast<ModDest>(baseDst + 4)), 0.0f, 1.0f);
        mix = std::clamp(mix + mod.getOffset(static_cast<ModDest>(baseDst + 5)), 0.0f, 1.0f);

        sChain.setSlotType(i, static_cast<EffectType>(typeIdx));
        sChain.setSlotBypass(i, bypass);
        sChain.setSlotParameters(i, p1, p2, p3, p4, p5, mix);
        sChain.setSlotLevels(i, inGain, outGain);
        engine.setSlotInputSource(4 + i, src);
        engine.setSlotRouting(4 + i, dest);
    }
}

void TransientSplitProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (pos->getBpm().hasValue())
                engine.setHostBpm(*pos->getBpm());

            if (pos->getPpqPosition().hasValue())
            {
                const bool isPlaying = pos->getIsPlaying();
                engine.syncToHostPosition(*pos->getPpqPosition(), isPlaying);
            }
        }
    }

    updateEngineParameters();

    auto mainBusBuffer = getBusBuffer(buffer, false, 0);
    auto aux1BusBuffer = (getBusCount(false) > 1 && getBus(false, 1)->isEnabled()) ? getBusBuffer(buffer, false, 1) : juce::AudioBuffer<float>();
    auto aux2BusBuffer = (getBusCount(false) > 2 && getBus(false, 2)->isEnabled()) ? getBusBuffer(buffer, false, 2) : juce::AudioBuffer<float>();
    auto aux3BusBuffer = (getBusCount(false) > 3 && getBus(false, 3)->isEnabled()) ? getBusBuffer(buffer, false, 3) : juce::AudioBuffer<float>();

    engine.process(mainBusBuffer,
                   aux1BusBuffer.getNumChannels() > 0 ? &aux1BusBuffer : nullptr,
                   aux2BusBuffer.getNumChannels() > 0 ? &aux2BusBuffer : nullptr,
                   aux3BusBuffer.getNumChannels() > 0 ? &aux3BusBuffer : nullptr);
}

juce::AudioProcessorEditor* TransientSplitProcessor::createEditor()
{
    return new TransientSplitEditor(*this);
}

int TransientSplitProcessor::getNumPrograms()
{
    return presetManager.getNumPresets();
}

int TransientSplitProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void TransientSplitProcessor::setCurrentProgram(int index)
{
    currentProgramIndex = index;
    presetManager.loadPreset(index);
}

const juce::String TransientSplitProcessor::getProgramName(int index)
{
    return presetManager.getPresetName(index);
}

void TransientSplitProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/)
{
}

void TransientSplitProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TransientSplitProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

// JUCE Plugin Factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TransientSplitProcessor();
}
