#include "CrossModEditor.h"

CrossModEditor::CrossModEditor(CrossModProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      modMatrixComponent(p.getAPVTS()),
      fxSectionComponent(p.getAPVTS(), p.getMidiLearnManager())
{
    setLookAndFeel(&vintageLaf);
    auto& apvts = processor.getAPVTS();
    auto& mlm = processor.getMidiLearnManager();

    // 1. Header Presets
    updatePresetList();
    presetBox.onChange = [this]() {
        int id = presetBox.getSelectedId() - 1;
        if (id >= 0 && id < processor.getPresetManager().getNumPresets())
        {
            processor.getPresetManager().loadPreset(id);
        }
    };
    addAndMakeVisible(presetBox);

    prevPresetBtn.onClick = [this]() {
        int cur = processor.getPresetManager().getCurrentPresetIndex();
        if (cur > 0)
        {
            processor.getPresetManager().loadPreset(cur - 1);
            presetBox.setSelectedId(cur, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(prevPresetBtn);

    nextPresetBtn.onClick = [this]() {
        int cur = processor.getPresetManager().getCurrentPresetIndex();
        if (cur < processor.getPresetManager().getNumPresets() - 1)
        {
            processor.getPresetManager().loadPreset(cur + 1);
            presetBox.setSelectedId(cur + 2, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(nextPresetBtn);

    // Save / Load / Directory Presets
    savePresetBtn.setTooltip("Save current sound into presets folder (or overwrite)");
    savePresetBtn.onClick = [this]() {
        auto presetsDir = processor.getPresetManager().getPresetsDirectory();
        juce::String currentName = processor.getPresetManager().getCurrentPresetName();
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Preset into Library",
            presetsDir.getChildFile(currentName + ".crossmod"),
            "*.crossmod");
        auto flags = juce::FileBrowserComponent::saveMode
                   | juce::FileBrowserComponent::canSelectFiles
                   | juce::FileBrowserComponent::warnAboutOverwriting;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                if (!file.hasFileExtension(".crossmod"))
                    file = file.withFileExtension(".crossmod");
                processor.getPresetManager().saveCurrentPresetToFile(file);
                updatePresetList();
            }
        });
    };
    addAndMakeVisible(savePresetBtn);

    loadPresetBtn.setTooltip("Load a preset file from disk into the library");
    loadPresetBtn.onClick = [this]() {
        auto presetsDir = processor.getPresetManager().getPresetsDirectory();
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load CrossMod Preset",
            presetsDir,
            "*.crossmod;*.xml");
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                processor.getPresetManager().loadPresetFromFile(file, true);
                updatePresetList();
            }
        });
    };
    addAndMakeVisible(loadPresetBtn);

    openFolderBtn.setTooltip("Open Presets folder in Windows Explorer");
    openFolderBtn.onClick = [this]() {
        processor.getPresetManager().openPresetsFolderInExplorer();
    };
    addAndMakeVisible(openFolderBtn);

    // Undo / Redo
    undoBtn.setTooltip("Undo parameter change");
    undoBtn.onClick = [this]() { processor.getUndoManager().undo(); };
    addAndMakeVisible(undoBtn);

    redoBtn.setTooltip("Redo parameter change");
    redoBtn.onClick = [this]() { processor.getUndoManager().redo(); };
    addAndMakeVisible(redoBtn);

    // MIDI Learn button in header
    midiBtn.setTooltip("View or manage MIDI Learn mappings");
    midiBtn.onClick = [this]() { showMidiMenu(); };
    addAndMakeVisible(midiBtn);

    // 2. Voice & Polyphony Section
    auto voiceModes = ParameterFactory::getVoiceModeChoices();
    for (int i = 0; i < voiceModes.size(); ++i)
        voiceModeBox.addItem(voiceModes[i], i + 1);
    addAndMakeVisible(voiceModeBox);
    voiceModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::voiceMode.getParamID(), voiceModeBox);

    glide1Knob.init(*this, apvts, mlm, CrossModIDs::osc1Glide, "GLIDE 1", VintageLookAndFeel::Ivory, " s");
    glide2Knob.init(*this, apvts, mlm, CrossModIDs::osc2Glide, "GLIDE 2", VintageLookAndFeel::Ivory, " s");
    voiceXModKnob.init(*this, apvts, mlm, CrossModIDs::voiceCrossMod, "VOICE X-MOD", VintageLookAndFeel::Crimson, "");

    // 3. Oscillator 1 Controls
    auto oscWaveforms = ParameterFactory::getOscWaveformChoices();
    for (int i = 0; i < oscWaveforms.size(); ++i)
        osc1WaveformBox.addItem(oscWaveforms[i], i + 1);
    addAndMakeVisible(osc1WaveformBox);
    osc1WaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::osc1Waveform.getParamID(), osc1WaveformBox);

    osc1CoarseKnob.init(*this, apvts, mlm, CrossModIDs::osc1Coarse, "COARSE", VintageLookAndFeel::Amber, " st");
    osc1FineKnob.init(*this, apvts, mlm, CrossModIDs::osc1Fine, "FINE", VintageLookAndFeel::Amber, " ct");
    osc1LevelKnob.init(*this, apvts, mlm, CrossModIDs::osc1Level, "LEVEL", VintageLookAndFeel::Amber, "");

    // Sub Oscillator Controls
    for (int i = 0; i < oscWaveforms.size(); ++i)
        subOscWaveformBox.addItem(oscWaveforms[i], i + 1);
    addAndMakeVisible(subOscWaveformBox);
    subOscWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::subOscWaveform.getParamID(), subOscWaveformBox);

    auto subOctaves = ParameterFactory::getSubOscOctaveChoices();
    for (int i = 0; i < subOctaves.size(); ++i)
        subOscOctaveBox.addItem(subOctaves[i], i + 1);
    addAndMakeVisible(subOscOctaveBox);
    subOscOctaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::subOscOctave.getParamID(), subOscOctaveBox);

    subOscLevelKnob.init(*this, apvts, mlm, CrossModIDs::subOscLevel, "SUB LEVEL", VintageLookAndFeel::Amber, "");

    // 4. Cross-Modulation Center
    auto crossModModes = ParameterFactory::getCrossModModeChoices();
    for (int i = 0; i < crossModModes.size(); ++i)
        crossModModeBox.addItem(crossModModes[i], i + 1);
    addAndMakeVisible(crossModModeBox);
    crossModModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::crossModMode.getParamID(), crossModModeBox);

    crossMod1to2Knob.init(*this, apvts, mlm, CrossModIDs::crossMod1to2, "1 -> 2 DEPTH", VintageLookAndFeel::Crimson, "");
    crossMod2to1Knob.init(*this, apvts, mlm, CrossModIDs::crossMod2to1, "2 -> 1 DEPTH", VintageLookAndFeel::Crimson, "");
    addAndMakeVisible(visualizer);

    // 5. Oscillator 2 Controls
    for (int i = 0; i < oscWaveforms.size(); ++i)
        osc2WaveformBox.addItem(oscWaveforms[i], i + 1);
    addAndMakeVisible(osc2WaveformBox);
    osc2WaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::osc2Waveform.getParamID(), osc2WaveformBox);

    osc2CoarseKnob.init(*this, apvts, mlm, CrossModIDs::osc2Coarse, "COARSE", VintageLookAndFeel::Cyan, " st");
    osc2FineKnob.init(*this, apvts, mlm, CrossModIDs::osc2Fine, "FINE", VintageLookAndFeel::Cyan, " ct");
    osc2LevelKnob.init(*this, apvts, mlm, CrossModIDs::osc2Level, "LEVEL", VintageLookAndFeel::Cyan, "");

    // 6. Filter Controls
    filterCutoffKnob.init(*this, apvts, mlm, CrossModIDs::filterCutoff, "CUTOFF", VintageLookAndFeel::Emerald, " Hz");
    filterResoKnob.init(*this, apvts, mlm, CrossModIDs::filterResonance, "RESONANCE", VintageLookAndFeel::Emerald, "");
    filterDriveKnob.init(*this, apvts, mlm, CrossModIDs::filterDrive, "DRIVE", VintageLookAndFeel::Emerald, "");
    filterKeyTrackKnob.init(*this, apvts, mlm, CrossModIDs::filterKeyTrack, "TRACK", VintageLookAndFeel::Emerald, "");
    filterEnvAmtKnob.init(*this, apvts, mlm, CrossModIDs::filterEnvAmt, "ENV AMT", VintageLookAndFeel::Emerald, "");

    auto filterTypes = ParameterFactory::getFilterTypeChoices();
    for (int i = 0; i < filterTypes.size(); ++i)
        filterTypeBox.addItem(filterTypes[i], i + 1);
    addAndMakeVisible(filterTypeBox);
    filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::filterType.getParamID(), filterTypeBox);

    // 7. VCA & Master Controls
    masterVolumeKnob.init(*this, apvts, mlm, CrossModIDs::masterVolume, "MASTER", VintageLookAndFeel::Gold, "");
    masterPanKnob.init(*this, apvts, mlm, CrossModIDs::masterPan, "PAN", VintageLookAndFeel::Gold, "");
    stereoWidthKnob.init(*this, apvts, mlm, CrossModIDs::stereoWidth, "WIDTH", VintageLookAndFeel::Gold, "");
    vcaWarmthKnob.init(*this, apvts, mlm, CrossModIDs::vcaSaturation, "WARMTH", VintageLookAndFeel::Gold, "");

    // 8. LFO 1 & 2 Controls
    lfo1RateKnob.init(*this, apvts, mlm, CrossModIDs::lfo1Rate, "RATE", VintageLookAndFeel::Ivory, " Hz");
    auto lfoShapes = ParameterFactory::getLFOShapeChoices();
    for (int i = 0; i < lfoShapes.size(); ++i)
        lfo1ShapeBox.addItem(lfoShapes[i], i + 1);
    addAndMakeVisible(lfo1ShapeBox);
    lfo1ShapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::lfo1Shape.getParamID(), lfo1ShapeBox);
    addAndMakeVisible(lfo1SyncBtn);
    lfo1SyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, CrossModIDs::lfo1Sync.getParamID(), lfo1SyncBtn);
    lfo1SyncBtn.onClick = [this]() {
        updateLfo1RateAttachment(lfo1SyncBtn.getToggleState());
    };

    lfo2RateKnob.init(*this, apvts, mlm, CrossModIDs::lfo2Rate, "RATE", VintageLookAndFeel::Ivory, " Hz");
    for (int i = 0; i < lfoShapes.size(); ++i)
        lfo2ShapeBox.addItem(lfoShapes[i], i + 1);
    addAndMakeVisible(lfo2ShapeBox);
    lfo2ShapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::lfo2Shape.getParamID(), lfo2ShapeBox);
    addAndMakeVisible(lfo2SyncBtn);
    lfo2SyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, CrossModIDs::lfo2Sync.getParamID(), lfo2SyncBtn);
    lfo2SyncBtn.onClick = [this]() {
        updateLfo2RateAttachment(lfo2SyncBtn.getToggleState());
    };

    bool initLfo1Sync = apvts.getRawParameterValue(CrossModIDs::lfo1Sync.getParamID())->load() > 0.5f;
    lastLfo1Synced = initLfo1Sync;
    updateLfo1RateAttachment(initLfo1Sync);

    bool initLfo2Sync = apvts.getRawParameterValue(CrossModIDs::lfo2Sync.getParamID())->load() > 0.5f;
    lastLfo2Synced = initLfo2Sync;
    updateLfo2RateAttachment(initLfo2Sync);

    // 9. Envelopes (Amp, Filter, Mod)
    ampAttackKnob.init(*this, apvts, mlm, CrossModIDs::ampAttack, "ATTACK", VintageLookAndFeel::Gold, " s");
    ampDecayKnob.init(*this, apvts, mlm, CrossModIDs::ampDecay, "DECAY", VintageLookAndFeel::Gold, " s");
    ampSustainKnob.init(*this, apvts, mlm, CrossModIDs::ampSustain, "SUSTAIN", VintageLookAndFeel::Gold, "");
    ampReleaseKnob.init(*this, apvts, mlm, CrossModIDs::ampRelease, "RELEASE", VintageLookAndFeel::Gold, " s");

    filtAttackKnob.init(*this, apvts, mlm, CrossModIDs::filterAttack, "ATTACK", VintageLookAndFeel::Emerald, " s");
    filtDecayKnob.init(*this, apvts, mlm, CrossModIDs::filterDecay, "DECAY", VintageLookAndFeel::Emerald, " s");
    filtSustainKnob.init(*this, apvts, mlm, CrossModIDs::filterSustain, "SUSTAIN", VintageLookAndFeel::Emerald, "");
    filtReleaseKnob.init(*this, apvts, mlm, CrossModIDs::filterRelease, "RELEASE", VintageLookAndFeel::Emerald, " s");

    modAttackKnob.init(*this, apvts, mlm, CrossModIDs::modAttack, "ATTACK", VintageLookAndFeel::Ivory, " s");
    modDecayKnob.init(*this, apvts, mlm, CrossModIDs::modDecay, "DECAY", VintageLookAndFeel::Ivory, " s");
    modSustainKnob.init(*this, apvts, mlm, CrossModIDs::modSustain, "SUSTAIN", VintageLookAndFeel::Ivory, "");
    modReleaseKnob.init(*this, apvts, mlm, CrossModIDs::modRelease, "RELEASE", VintageLookAndFeel::Ivory, " s");

    // 10. Modulation Matrix
    addAndMakeVisible(modMatrixComponent);

    // 11. Studio Multi-FX Section
    addAndMakeVisible(fxSectionComponent);

    setSize(1060, 830);
    startTimerHz(30);
}

CrossModEditor::~CrossModEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void CrossModEditor::showMidiMenu()
{
    auto& mlm = processor.getMidiLearnManager();
    auto mappings = mlm.getAllMappings();

    juce::PopupMenu menu;
    menu.addSectionHeader("ACTIVE MIDI CC MAPPINGS (" + juce::String(mappings.size()) + ")");

    if (mappings.empty())
    {
        menu.addItem(1, "No Active Mappings (Right-click any knob to Learn)", false, false);
    }
    else
    {
        for (size_t i = 0; i < mappings.size(); ++i)
        {
            const auto& m = mappings[i];
            juce::String chStr = (m.channel == 0) ? "Omni" : ("Ch " + juce::String(m.channel));
            auto* p = processor.getAPVTS().getParameter(m.paramID);
            juce::String pName = (p != nullptr) ? p->getName(25) : m.paramID;
            menu.addItem((int)(10 + i), pName + " -> CC " + juce::String(m.cc) + " (" + chStr + ")", false, false);
        }
    }

    menu.addSeparator();
    menu.addItem(100, "Save Mappings as Default");
    menu.addItem(101, "Load Default Mappings");
    menu.addItem(102, "Clear All MIDI Mappings");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&midiBtn),
        [this, &mlm](int result)
        {
            if (result == 100)
            {
                mlm.saveDefaultMapToFile();
            }
            else if (result == 101)
            {
                mlm.loadDefaultMapFromFile();
                repaint();
            }
            else if (result == 102)
            {
                mlm.clearAllMappings();
                repaint();
            }
        });
}

void CrossModEditor::updatePresetList()
{
    presetBox.clear();
    auto& pm = processor.getPresetManager();
    pm.rescanPresets();
    int count = pm.getNumPresets();
    for (int i = 0; i < count; ++i)
        presetBox.addItem(pm.getPresetName(i), i + 1);

    presetBox.setSelectedId(pm.getCurrentPresetIndex() + 1, juce::dontSendNotification);
}

void CrossModEditor::updateLfo1RateAttachment(bool isSynced)
{
    auto& apvts = processor.getAPVTS();
    auto& mlm = processor.getMidiLearnManager();
    lfo1RateKnob.attachment.reset();
    if (isSynced)
    {
        auto syncChoices = ParameterFactory::getSyncRateChoices();
        lfo1RateKnob.slider->setRange(0, syncChoices.size() - 1, 1);
        lfo1RateKnob.slider->textFromValueFunction = [](double v) {
            auto choices = ParameterFactory::getSyncRateChoices();
            int idx = std::clamp((int)std::round(v), 0, choices.size() - 1);
            return choices[idx];
        };
        lfo1RateKnob.slider->valueFromTextFunction = [](const juce::String& text) {
            auto choices = ParameterFactory::getSyncRateChoices();
            for (int i = 0; i < choices.size(); ++i)
                if (choices[i].equalsIgnoreCase(text.trim()))
                    return (double)i;
            return 8.0;
        };
        lfo1RateKnob.slider->setTextValueSuffix("");
        lfo1RateKnob.slider->setParamInfo(CrossModIDs::lfo1SyncRate.getParamID(),
                                          apvts.getParameter(CrossModIDs::lfo1SyncRate.getParamID()),
                                          &mlm);
        lfo1RateKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, CrossModIDs::lfo1SyncRate.getParamID(), *lfo1RateKnob.slider);
        lfo1RateKnob.label->setText("SYNC RATE", juce::dontSendNotification);
    }
    else
    {
        auto* param = apvts.getParameter(CrossModIDs::lfo1Rate.getParamID());
        auto range = param->getNormalisableRange();
        lfo1RateKnob.slider->setRange(range.start, range.end, range.interval);
        lfo1RateKnob.slider->textFromValueFunction = nullptr;
        lfo1RateKnob.slider->valueFromTextFunction = nullptr;
        lfo1RateKnob.slider->setTextValueSuffix(" Hz");
        lfo1RateKnob.slider->setParamInfo(CrossModIDs::lfo1Rate.getParamID(), param, &mlm);
        lfo1RateKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, CrossModIDs::lfo1Rate.getParamID(), *lfo1RateKnob.slider);
        lfo1RateKnob.label->setText("RATE", juce::dontSendNotification);
    }
}

void CrossModEditor::updateLfo2RateAttachment(bool isSynced)
{
    auto& apvts = processor.getAPVTS();
    auto& mlm = processor.getMidiLearnManager();
    lfo2RateKnob.attachment.reset();
    if (isSynced)
    {
        auto syncChoices = ParameterFactory::getSyncRateChoices();
        lfo2RateKnob.slider->setRange(0, syncChoices.size() - 1, 1);
        lfo2RateKnob.slider->textFromValueFunction = [](double v) {
            auto choices = ParameterFactory::getSyncRateChoices();
            int idx = std::clamp((int)std::round(v), 0, choices.size() - 1);
            return choices[idx];
        };
        lfo2RateKnob.slider->valueFromTextFunction = [](const juce::String& text) {
            auto choices = ParameterFactory::getSyncRateChoices();
            for (int i = 0; i < choices.size(); ++i)
                if (choices[i].equalsIgnoreCase(text.trim()))
                    return (double)i;
            return 5.0;
        };
        lfo2RateKnob.slider->setTextValueSuffix("");
        lfo2RateKnob.slider->setParamInfo(CrossModIDs::lfo2SyncRate.getParamID(),
                                          apvts.getParameter(CrossModIDs::lfo2SyncRate.getParamID()),
                                          &mlm);
        lfo2RateKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, CrossModIDs::lfo2SyncRate.getParamID(), *lfo2RateKnob.slider);
        lfo2RateKnob.label->setText("SYNC RATE", juce::dontSendNotification);
    }
    else
    {
        auto* param = apvts.getParameter(CrossModIDs::lfo2Rate.getParamID());
        auto range = param->getNormalisableRange();
        lfo2RateKnob.slider->setRange(range.start, range.end, range.interval);
        lfo2RateKnob.slider->textFromValueFunction = nullptr;
        lfo2RateKnob.slider->valueFromTextFunction = nullptr;
        lfo2RateKnob.slider->setTextValueSuffix(" Hz");
        lfo2RateKnob.slider->setParamInfo(CrossModIDs::lfo2Rate.getParamID(), param, &mlm);
        lfo2RateKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, CrossModIDs::lfo2Rate.getParamID(), *lfo2RateKnob.slider);
        lfo2RateKnob.label->setText("RATE", juce::dontSendNotification);
    }
}

void CrossModEditor::timerCallback()
{
    activeVoiceMask = processor.getEngine().getActiveVoiceMask();
    peakMeterL = std::abs(processor.getEngine().getLastOutputL());
    peakMeterR = std::abs(processor.getEngine().getLastOutputR());

    auto& apvts = processor.getAPVTS();
    auto getVal = [&apvts](const juce::ParameterID& pid) {
        if (auto* p = apvts.getRawParameterValue(pid.getParamID()))
            return p->load();
        return 0.0f;
    };

    bool lfo1Synced = getVal(CrossModIDs::lfo1Sync) > 0.5f;
    if (lfo1Synced != lastLfo1Synced)
    {
        lastLfo1Synced = lfo1Synced;
        updateLfo1RateAttachment(lfo1Synced);
    }

    bool lfo2Synced = getVal(CrossModIDs::lfo2Sync) > 0.5f;
    if (lfo2Synced != lastLfo2Synced)
    {
        lastLfo2Synced = lfo2Synced;
        updateLfo2RateAttachment(lfo2Synced);
    }

    fxSectionComponent.updateSyncState();

    visualizer.setParams(getVal(CrossModIDs::crossMod1to2),
                         getVal(CrossModIDs::crossMod2to1),
                         getVal(CrossModIDs::osc1Coarse),
                         getVal(CrossModIDs::osc2Coarse),
                         getVal(CrossModIDs::osc1Fine),
                         getVal(CrossModIDs::osc2Fine),
                         getVal(CrossModIDs::voiceCrossMod));

    std::array<float, 128> preBufL;
    std::array<float, 128> preBufR;
    std::array<float, 128> postBufL;
    std::array<float, 128> postBufR;
    processor.getEngine().copyPreFxScope(preBufL.data(), preBufR.data(), 128);
    processor.getEngine().copyPostFxScope(postBufL.data(), postBufR.data(), 128);
    visualizer.setAudioWaveforms(preBufL.data(), preBufR.data(), postBufL.data(), postBufR.data(), 128);

    undoBtn.setEnabled(processor.getUndoManager().canUndo());
    redoBtn.setEnabled(processor.getUndoManager().canRedo());

    repaint();
}

void CrossModEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Walnut Wood End Cheeks (Left and Right)
    float woodW = 16.0f;
    auto leftWood = bounds.removeFromLeft(woodW);
    auto rightWood = bounds.removeFromRight(woodW);

    juce::ColourGradient woodGradL(juce::Colour(0xff432616), leftWood.getTopLeft(),
                                  juce::Colour(0xff22120a), leftWood.getBottomRight(), false);
    g.setGradientFill(woodGradL);
    g.fillRect(leftWood);

    juce::ColourGradient woodGradR(juce::Colour(0xff432616), rightWood.getTopRight(),
                                  juce::Colour(0xff22120a), rightWood.getBottomLeft(), false);
    g.setGradientFill(woodGradR);
    g.fillRect(rightWood);

    // Wood seams
    g.setColour(juce::Colour(0xff120904));
    g.drawVerticalLine((int)leftWood.getRight(), 0.0f, bounds.getBottom());
    g.drawVerticalLine((int)rightWood.getX(), 0.0f, bounds.getBottom());

    // 2. Main Dark Slate Brushed Chassis
    juce::ColourGradient metalGrad(juce::Colour(0xff22252c), bounds.getTopLeft(),
                                  juce::Colour(0xff17191d), bounds.getBottomRight(), false);
    g.setGradientFill(metalGrad);
    g.fillRect(bounds);

    g.setColour(juce::Colour(0x30ffffff));
    g.drawHorizontalLine(0, bounds.getX(), bounds.getRight());

    // 3. Header Silkscreen
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffe5a823)); // Gold
    g.drawText("CROSSMOD", (int)bounds.getX() + 18, 10, 136, 24, juce::Justification::left);

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff29b6f6)); // Azure/Cyan brand accent
    g.drawText("by mtyas", (int)bounds.getX() + 150, 14, 70, 18, juce::Justification::left);

    g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
    g.setColour(juce::Colour(0xff9ea5b1));
    g.drawText("ADVANCED CROSS-MODULATION SYNTHESIZER", (int)bounds.getX() + 18, 32, 280, 14, juce::Justification::left);

    // Voice Activity LEDs
    float ledStartX = bounds.getRight() - 175.0f;
    float ledY = 22.0f;
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8c94a0));
    g.drawText("VOICES", (int)ledStartX - 45, (int)ledY - 3, 40, 12, juce::Justification::right);

    for (int v = 0; v < 16; ++v)
    {
        float lx = ledStartX + v * 9.8f;
        bool active = (activeVoiceMask & (1u << v)) != 0;
        g.setColour(active ? juce::Colour(0xff2ecc71) : juce::Colour(0xff182b1e));
        g.fillEllipse(lx, ledY, 6.0f, 6.0f);
        if (active)
        {
            g.setColour(juce::Colour(0x80ffffff));
            g.drawEllipse(lx, ledY, 6.0f, 6.0f, 0.8f);
        }
    }

    // Module Card Containers Helper
    auto drawCard = [&g](float x, float y, float w, float h, const juce::String& title, juce::Colour accent) {
        auto r = juce::Rectangle<float>(x, y, w, h);
        g.setColour(juce::Colour(0xff1b1d22));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(juce::Colour(0xff32363f));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);

        // Header tab
        g.setColour(accent.withAlpha(0.25f));
        g.fillRoundedRectangle(r.getX(), r.getY(), r.getWidth(), 20.0f, 6.0f);
        g.fillRect(r.getX(), r.getY() + 14.0f, r.getWidth(), 6.0f);

        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.setColour(accent);
        g.drawText(title, (int)r.getX() + 8, (int)r.getY() + 2, (int)r.getWidth() - 16, 16, juce::Justification::centredLeft);
    };

    float innerX = bounds.getX() + 10.0f;

    // ROW 1: Sound Generation
    int r1Y = 50;
    int r1H = 214;
    drawCard(innerX, r1Y, 140, r1H, "OSCILLATOR 1 (L)", juce::Colour(0xfff39c12));       // Amber
    drawCard(innerX + 145, r1Y, 105, r1H, "SUB OSC", juce::Colour(0xffe67e22));          // Orange
    drawCard(innerX + 255, r1Y, 245, r1H, "CROSS-MODULATION", juce::Colour(0xffe74c3c));  // Crimson
    drawCard(innerX + 505, r1Y, 140, r1H, "OSCILLATOR 2 (R)", juce::Colour(0xff29b6f6));   // Cyan
    drawCard(innerX + 650, r1Y, 205, r1H, "RESONANT VCF", juce::Colour(0xff2ecc71));       // Emerald
    drawCard(innerX + 860, r1Y, 148, r1H, "VCA & MASTER", juce::Colour(0xfff1c40f));       // Gold

    // Metering in VCA card
    float meterX = innerX + 860 + 110.0f;
    float meterY = r1Y + 32.0f;
    float meterW = 10.0f;
    float meterH = 166.0f;

    g.setColour(juce::Colour(0xff121417));
    g.fillRect(meterX, meterY, meterW * 2.0f + 4.0f, meterH);

    float valL = juce::jlimit(0.0f, 1.0f, peakMeterL * 1.5f);
    float valR = juce::jlimit(0.0f, 1.0f, peakMeterR * 1.5f);

    auto drawMeterBar = [&g, meterY, meterH](float x, float val) {
        float h = meterH * val;
        juce::ColourGradient meterGrad(juce::Colour(0xffe74c3c), x, meterY,
                                      juce::Colour(0xff2ecc71), x, meterY + meterH, false);
        meterGrad.addColour(0.3, juce::Colour(0xfff1c40f));
        g.setGradientFill(meterGrad);
        g.fillRect(x, meterY + meterH - h, 9.0f, h);
    };

    drawMeterBar(meterX + 1.0f, valL);
    drawMeterBar(meterX + meterW + 3.0f, valR);

    // ROW 2: Voice, LFOs & Envelopes
    int r2Y = 272;
    int r2H = 222;
    drawCard(innerX, r2Y, 205, r2H, "VOICE & GLIDE", juce::Colour(0xffe0dcd3));
    drawCard(innerX + 211, r2Y, 155, r2H, "DUAL LFOs", juce::Colour(0xffe0dcd3));

    int envBaseX = innerX + 372;
    int envCardW = 208;
    drawCard(envBaseX, r2Y, envCardW, r2H, "AMP ENVELOPE", juce::Colour(0xfff1c40f));
    drawCard(envBaseX + envCardW + 6, r2Y, envCardW, r2H, "FILTER ENVELOPE", juce::Colour(0xff2ecc71));
    drawCard(envBaseX + (envCardW + 6) * 2, r2Y, envCardW, r2H, "MOD ENVELOPE", juce::Colour(0xffe0dcd3));
}

void CrossModEditor::resized()
{
    auto bounds = getLocalBounds();
    int woodW = 16;
    int innerX = woodW + 10;

    // Header Controls (Preset selector & History & MIDI)
    prevPresetBtn.setBounds(innerX + 225, 12, 22, 24);
    presetBox.setBounds(innerX + 250, 12, 160, 24);
    nextPresetBtn.setBounds(innerX + 413, 12, 22, 24);
    savePresetBtn.setBounds(innerX + 441, 12, 44, 24);
    loadPresetBtn.setBounds(innerX + 489, 12, 44, 24);
    openFolderBtn.setBounds(innerX + 537, 12, 34, 24);
    undoBtn.setBounds(innerX + 577, 12, 44, 24);
    redoBtn.setBounds(innerX + 625, 12, 44, 24);
    midiBtn.setBounds(innerX + 673, 12, 44, 24);

    // ==========================================
    // ROW 1: SOUND GENERATION & FILTERING
    // ==========================================
    int r1Y = 50;

    // 1. Oscillator 1 (w: 140)
    int o1X = innerX;
    osc1WaveformBox.setBounds(o1X + 10, r1Y + 24, 120, 22);
    osc1CoarseKnob.setBounds(o1X + 6, r1Y + 48, 62, 76);
    osc1FineKnob.setBounds(o1X + 72, r1Y + 48, 62, 76);
    osc1LevelKnob.setBounds(o1X + 39, r1Y + 128, 62, 76);

    // 2. Sub Oscillator (w: 105)
    int subX = innerX + 145;
    subOscWaveformBox.setBounds(subX + 8, r1Y + 24, 89, 22);
    subOscOctaveBox.setBounds(subX + 8, r1Y + 50, 89, 22);
    subOscLevelKnob.setBounds(subX + 21, r1Y + 82, 63, 122);

    // 3. Cross-Modulation Center (w: 245)
    int cmX = innerX + 255;
    crossModModeBox.setBounds(cmX + 32, r1Y + 24, 180, 22);
    crossMod1to2Knob.setBounds(cmX + 6, r1Y + 50, 72, 145);
    visualizer.setBounds(cmX + 80, r1Y + 50, 85, 145);
    crossMod2to1Knob.setBounds(cmX + 167, r1Y + 50, 72, 145);

    // 4. Oscillator 2 (w: 140)
    int o2X = innerX + 505;
    osc2WaveformBox.setBounds(o2X + 10, r1Y + 24, 120, 22);
    osc2CoarseKnob.setBounds(o2X + 6, r1Y + 48, 62, 76);
    osc2FineKnob.setBounds(o2X + 72, r1Y + 48, 62, 76);
    osc2LevelKnob.setBounds(o2X + 39, r1Y + 128, 62, 76);

    // 5. Filter (VCF) (w: 205)
    int fX = innerX + 650;
    filterTypeBox.setBounds(fX + 10, r1Y + 24, 135, 22);
    filterCutoffKnob.setBounds(fX + 6, r1Y + 50, 62, 76);
    filterResoKnob.setBounds(fX + 71, r1Y + 50, 62, 76);
    filterDriveKnob.setBounds(fX + 136, r1Y + 50, 62, 76);
    filterKeyTrackKnob.setBounds(fX + 38, r1Y + 128, 62, 76);
    filterEnvAmtKnob.setBounds(fX + 105, r1Y + 128, 62, 76);

    // 5. VCA & Master (w: 148)
    int vcaX = innerX + 860;
    masterVolumeKnob.setBounds(vcaX + 6, r1Y + 28, 48, 80);
    masterPanKnob.setBounds(vcaX + 54, r1Y + 28, 48, 80);
    stereoWidthKnob.setBounds(vcaX + 6, r1Y + 118, 48, 80);
    vcaWarmthKnob.setBounds(vcaX + 54, r1Y + 118, 48, 80);

    // ==========================================
    // ROW 2: VOICE ENGINE, LFOs & ENVELOPES
    // ==========================================
    int r2Y = 272;

    // 1. Voice Engine & Polyphony (w: 205)
    int vEngX = innerX;
    voiceModeBox.setBounds(vEngX + 10, r2Y + 28, 185, 24);
    glide1Knob.setBounds(vEngX + 4, r2Y + 68, 62, 146);
    glide2Knob.setBounds(vEngX + 68, r2Y + 68, 62, 146);
    voiceXModKnob.setBounds(vEngX + 132, r2Y + 68, 68, 146);

    // 2. LFO 1 & 2 (w: 155)
    int lfoX = innerX + 211;
    lfo1RateKnob.setBounds(lfoX + 6, r2Y + 26, 65, 80);
    lfo1ShapeBox.setBounds(lfoX + 74, r2Y + 36, 75, 22);
    lfo1SyncBtn.setBounds(lfoX + 74, r2Y + 66, 75, 20);

    lfo2RateKnob.setBounds(lfoX + 6, r2Y + 115, 65, 80);
    lfo2ShapeBox.setBounds(lfoX + 74, r2Y + 125, 75, 22);
    lfo2SyncBtn.setBounds(lfoX + 74, r2Y + 155, 75, 20);

    // 3. Envelopes (Amp, Filter, Mod) (w: 636)
    int envBaseX = innerX + 378;
    int envCardW = 202;
    int knobW = 46;
    int knobH = 145;
    int knobY = r2Y + 44;

    // Amp Envelope Knobs
    int aX = envBaseX;
    ampAttackKnob.setBounds(aX + 8 + 0 * 47, knobY, knobW, knobH);
    ampDecayKnob.setBounds(aX + 8 + 1 * 47, knobY, knobW, knobH);
    ampSustainKnob.setBounds(aX + 8 + 2 * 47, knobY, knobW, knobH);
    ampReleaseKnob.setBounds(aX + 8 + 3 * 47, knobY, knobW, knobH);

    // Filter Envelope Knobs
    int feX = envBaseX + envCardW + 6;
    filtAttackKnob.setBounds(feX + 8 + 0 * 47, knobY, knobW, knobH);
    filtDecayKnob.setBounds(feX + 8 + 1 * 47, knobY, knobW, knobH);
    filtSustainKnob.setBounds(feX + 8 + 2 * 47, knobY, knobW, knobH);
    filtReleaseKnob.setBounds(feX + 8 + 3 * 47, knobY, knobW, knobH);

    // Mod Envelope Knobs
    int meX = envBaseX + (envCardW + 6) * 2;
    modAttackKnob.setBounds(meX + 8 + 0 * 47, knobY, knobW, knobH);
    modDecayKnob.setBounds(meX + 8 + 1 * 47, knobY, knobW, knobH);
    modSustainKnob.setBounds(meX + 8 + 2 * 47, knobY, knobW, knobH);
    modReleaseKnob.setBounds(meX + 8 + 3 * 47, knobY, knobW, knobH);

    // ==========================================
    // ROW 3: MODULATION MATRIX (8 FULL-WIDTH SLOTS)
    // ==========================================
    int r3Y = 502;
    int r3H = 155;
    modMatrixComponent.setBounds(innerX, r3Y, 1008, r3H);

    // ==========================================
    // ROW 4: STUDIO MULTI-FX SECTION (MOD, DELAY, REVERB, ORDER)
    // ==========================================
    int r4Y = 663;
    int r4H = 152;
    fxSectionComponent.setBounds(innerX, r4Y, 1008, r4H);
}
