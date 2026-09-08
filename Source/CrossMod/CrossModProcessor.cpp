#include "CrossModProcessor.h"
#include "UI/CrossModEditor.h"

CrossModProcessor::CrossModProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "Parameters", ParameterFactory::createParameterLayout()),
      presetManager(apvts),
      factoryPresets(FactoryPresets::getPresets())
{
    presetManager.initialize();
    midiLearnManager.loadDefaultMapFromFile();
}

CrossModProcessor::~CrossModProcessor()
{
}

void CrossModProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
}

void CrossModProcessor::releaseResources()
{
    engine.reset();
}

bool CrossModProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // We only support stereo output and no inputs
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void CrossModProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // 1. Host BPM
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (position->getBpm().hasValue())
            {
                engine.setBpm(*position->getBpm());
            }
        }
    }

    // 2. Parse MIDI Events (including on-screen virtual keyboard)
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            engine.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        }
        else if (msg.isNoteOff())
        {
            engine.noteOff(msg.getNoteNumber());
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            engine.allNotesOff();
        }
        else if (msg.isPitchWheel())
        {
            float pb = juce::jmap((float)msg.getPitchWheelValue(), 0.0f, 16383.0f, -1.0f, 1.0f);
            engine.setPitchBend(pb);
        }
        else if (msg.isController())
        {
            if (msg.getControllerNumber() == 1) // Mod Wheel
            {
                engine.setModWheel(msg.getControllerValue() / 127.0f);
            }
            midiLearnManager.processMidiController(msg, apvts);
        }
        else if (msg.isChannelPressure())
        {
            engine.setAftertouch(msg.getChannelPressureValue() / 127.0f);
        }
    }

    // 3. Update Mod Matrix Slots from APVTS
    for (int i = 0; i < CrossModIDs::NUM_MOD_SLOTS; ++i)
    {
        auto srcParam = apvts.getRawParameterValue(CrossModIDs::modSlotSource(i).getParamID());
        auto dstParam = apvts.getRawParameterValue(CrossModIDs::modSlotDest(i).getParamID());
        auto amtParam = apvts.getRawParameterValue(CrossModIDs::modSlotAmount(i).getParamID());

        if (srcParam && dstParam && amtParam)
        {
            auto src = static_cast<ModSource>((int)srcParam->load());
            auto dst = static_cast<ModDestination>((int)dstParam->load());
            float amt = amtParam->load();
            engine.setModMatrixSlot(i, src, dst, amt);
        }
    }

    // 4. Read Synthesis Parameters
    auto getVal = [this](const juce::ParameterID& pid) -> float
    {
        if (auto* p = apvts.getRawParameterValue(pid.getParamID()))
            return p->load();
        return 0.0f;
    };

    auto voiceMode = static_cast<VoiceMode>((int)getVal(CrossModIDs::voiceMode));
    float osc1Glide = getVal(CrossModIDs::osc1Glide);
    float osc2Glide = getVal(CrossModIDs::osc2Glide);
    float voiceCrossMod = getVal(CrossModIDs::voiceCrossMod);

    auto osc1Waveform = static_cast<OscWaveform>((int)getVal(CrossModIDs::osc1Waveform));
    int osc1Coarse = (int)getVal(CrossModIDs::osc1Coarse);
    float osc1Fine = getVal(CrossModIDs::osc1Fine);
    float osc1Level = getVal(CrossModIDs::osc1Level);

    auto crossModMode = static_cast<CrossModMode>((int)getVal(CrossModIDs::crossModMode));
    float crossMod1to2 = getVal(CrossModIDs::crossMod1to2);
    float crossMod2to1 = getVal(CrossModIDs::crossMod2to1);

    auto osc2Waveform = static_cast<OscWaveform>((int)getVal(CrossModIDs::osc2Waveform));
    int osc2Coarse = (int)getVal(CrossModIDs::osc2Coarse);
    float osc2Fine = getVal(CrossModIDs::osc2Fine);
    float osc2Level = getVal(CrossModIDs::osc2Level);

    auto subOscWaveform = static_cast<OscWaveform>((int)getVal(CrossModIDs::subOscWaveform));
    auto subOscOctave = static_cast<SubOscOctave>((int)getVal(CrossModIDs::subOscOctave));
    float subOscLevel = getVal(CrossModIDs::subOscLevel);

    float filterCutoff = getVal(CrossModIDs::filterCutoff);
    float filterResonance = getVal(CrossModIDs::filterResonance);
    auto filterType = static_cast<FilterType>((int)getVal(CrossModIDs::filterType));
    float filterDrive = getVal(CrossModIDs::filterDrive);
    float filterKeyTrack = getVal(CrossModIDs::filterKeyTrack);
    float filterEnvAmt = getVal(CrossModIDs::filterEnvAmt);

    float ampA = getVal(CrossModIDs::ampAttack);
    float ampD = getVal(CrossModIDs::ampDecay);
    float ampS = getVal(CrossModIDs::ampSustain);
    float ampR = getVal(CrossModIDs::ampRelease);

    float filtA = getVal(CrossModIDs::filterAttack);
    float filtD = getVal(CrossModIDs::filterDecay);
    float filtS = getVal(CrossModIDs::filterSustain);
    float filtR = getVal(CrossModIDs::filterRelease);

    float modA = getVal(CrossModIDs::modAttack);
    float modD = getVal(CrossModIDs::modDecay);
    float modS = getVal(CrossModIDs::modSustain);
    float modR = getVal(CrossModIDs::modRelease);

    float lfo1Rate = getVal(CrossModIDs::lfo1Rate);
    auto lfo1Shape = static_cast<LFOShape>((int)getVal(CrossModIDs::lfo1Shape));
    bool lfo1Sync = getVal(CrossModIDs::lfo1Sync) > 0.5f;
    int lfo1SyncRate = (int)getVal(CrossModIDs::lfo1SyncRate);
    bool lfo1Retr = getVal(CrossModIDs::lfo1Retrig) > 0.5f;

    float lfo2Rate = getVal(CrossModIDs::lfo2Rate);
    auto lfo2Shape = static_cast<LFOShape>((int)getVal(CrossModIDs::lfo2Shape));
    bool lfo2Sync = getVal(CrossModIDs::lfo2Sync) > 0.5f;
    int lfo2SyncRate = (int)getVal(CrossModIDs::lfo2SyncRate);
    bool lfo2Retr = getVal(CrossModIDs::lfo2Retrig) > 0.5f;

    float masterVolume = getVal(CrossModIDs::masterVolume);
    float masterPan = getVal(CrossModIDs::masterPan);
    float stereoWidth = getVal(CrossModIDs::stereoWidth);
    float vcaWarmth = getVal(CrossModIDs::vcaSaturation);

    auto fxRoutingOrder = static_cast<FxOrder>((int)getVal(CrossModIDs::fxRoutingOrder));

    auto modType = static_cast<ModFxType>((int)getVal(CrossModIDs::modFxType));
    float modRate = getVal(CrossModIDs::modFxRate);
    float modDepth = getVal(CrossModIDs::modFxDepth);
    float modFeedback = getVal(CrossModIDs::modFxFeedback);
    float modMix = getVal(CrossModIDs::modFxMix);

    auto delayType = static_cast<DelayFxType>((int)getVal(CrossModIDs::delayFxType));
    float delayTime = getVal(CrossModIDs::delayFxTime);
    float delayFeedback = getVal(CrossModIDs::delayFxFeedback);
    float delayTone = getVal(CrossModIDs::delayFxTone);
    float delayMix = getVal(CrossModIDs::delayFxMix);
    bool delaySync = getVal(CrossModIDs::delayFxSync) > 0.5f;
    int delaySyncRate = (int)getVal(CrossModIDs::delayFxSyncRate);

    auto reverbType = static_cast<ReverbFxType>((int)getVal(CrossModIDs::reverbFxType));
    float reverbDecay = getVal(CrossModIDs::reverbFxDecay);
    float reverbDamping = getVal(CrossModIDs::reverbFxDamping);
    float reverbTone = getVal(CrossModIDs::reverbFxTone);
    float reverbMix = getVal(CrossModIDs::reverbFxMix);

    // Update envelopes on voices
    engine.updateVoiceEnvelopeParameters(ampA, ampD, ampS, ampR,
                                         filtA, filtD, filtS, filtR,
                                         modA, modD, modS, modR);

    // 5. Render Block
    engine.processBlock(buffer,
                        voiceMode, crossModMode,
                        osc1Glide, osc2Glide, voiceCrossMod,
                        osc1Waveform, osc1Coarse, osc1Fine, osc1Level,
                        osc2Waveform, osc2Coarse, osc2Fine, osc2Level,
                        subOscWaveform, subOscOctave, subOscLevel,
                        crossMod1to2, crossMod2to1,
                        filterCutoff, filterResonance, filterType,
                        filterDrive, filterKeyTrack, filterEnvAmt,
                        ampA, ampD, ampS, ampR,
                        filtA, filtD, filtS, filtR,
                        modA, modD, modS, modR,
                        lfo1Rate, lfo1Shape, lfo1Sync, lfo1SyncRate, lfo1Retr,
                        lfo2Rate, lfo2Shape, lfo2Sync, lfo2SyncRate, lfo2Retr,
                        masterVolume, masterPan, stereoWidth, vcaWarmth,
                        fxRoutingOrder,
                        modType, modRate, modDepth, modFeedback, modMix,
                        delayType, delayTime, delayFeedback, delayTone, delayMix,
                        delaySync, delaySyncRate,
                        reverbType, reverbDecay, reverbDamping, reverbTone, reverbMix);
}

juce::AudioProcessorEditor* CrossModProcessor::createEditor()
{
    return new CrossModEditor(*this);
}

int CrossModProcessor::getNumPrograms()
{
    return presetManager.getNumPresets();
}

int CrossModProcessor::getCurrentProgram()
{
    return presetManager.getCurrentPresetIndex();
}

void CrossModProcessor::setCurrentProgram(int index)
{
    presetManager.loadPreset(index);
}

const juce::String CrossModProcessor::getProgramName(int index)
{
    return presetManager.getPresetName(index);
}

void CrossModProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void CrossModProcessor::loadPreset(int presetIndex)
{
    presetManager.loadPreset(presetIndex);
}

int CrossModProcessor::getNumFactoryPresets() const
{
    return presetManager.getNumPresets();
}

juce::String CrossModProcessor::getFactoryPresetName(int index) const
{
    return presetManager.getPresetName(index);
}

void CrossModProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
    {
        midiLearnManager.saveToXml(*xml);
        copyXmlToBinary(*xml, destData);
    }
}

void CrossModProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        midiLearnManager.loadFromXml(*xmlState);
    }
}

void CrossModProcessor::savePresetToFile(const juce::File& file)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
    {
        xml->writeTo(file);
    }
}

bool CrossModProcessor::loadPresetFromFile(const juce::File& file)
{
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        return true;
    }
    return false;
}

// JUCE Plugin Filter Factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrossModProcessor();
}
