#include "TransientSplitEditor.h"
#include "../TransientSplitProcessor.h"

TransientSplitEditor::TransientSplitEditor(TransientSplitProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);
    setWantsKeyboardFocus(true);

    // Title & Branding
    pluginTitle.setText("OMNISPLIT", juce::dontSendNotification);
    pluginTitle.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    pluginTitle.setColour(juce::Label::textColourId, TransientLookAndFeel::accentTransient);
    addAndMakeVisible(pluginTitle);

    pluginSubtitle.setText("Created by mtyas", juce::dontSendNotification);
    pluginSubtitle.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    pluginSubtitle.setColour(juce::Label::textColourId, juce::Colour(0xffffa116));
    addAndMakeVisible(pluginSubtitle);

    // Undo / Redo Buttons
    undoBtn.onClick = [this]() { processor.getUndoManager().undo(); };
    redoBtn.onClick = [this]() { processor.getUndoManager().redo(); };
    addAndMakeVisible(undoBtn);
    addAndMakeVisible(redoBtn);

    // Preset Controls
    updatePresetList();
    presetCombo.onChange = [this]()
    {
        const int idx = presetCombo.getSelectedId() - 1;
        if (idx >= 0)
            processor.getPresetManager().loadPreset(idx);
    };
    addAndMakeVisible(presetCombo);

    prevPresetBtn.onClick = [this]()
    {
        int curr = presetCombo.getSelectedId() - 1;
        if (curr > 0)
            presetCombo.setSelectedId(curr, juce::sendNotificationAsync);
    };
    addAndMakeVisible(prevPresetBtn);

    nextPresetBtn.onClick = [this]()
    {
        int curr = presetCombo.getSelectedId() - 1;
        if (curr < presetCombo.getNumItems() - 1)
            presetCombo.setSelectedId(curr + 2, juce::sendNotificationAsync);
    };
    addAndMakeVisible(nextPresetBtn);

    savePresetBtn.onClick = [this]() { showSavePresetDialog(); };
    addAndMakeVisible(savePresetBtn);

    loadPresetBtn.onClick = [this]()
    {
        activeFileChooser = std::make_unique<juce::FileChooser>(
            "Import Preset XML", processor.getPresetManager().getPresetsDirectory(), "*.xml");
        activeFileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();
                if (file.existsAsFile())
                {
                    if (processor.getPresetManager().loadPresetFromFile(file))
                    {
                        processor.getPresetManager().refreshPresetList();
                        updatePresetList();
                    }
                }
            });
    };
    addAndMakeVisible(loadPresetBtn);

    deletePresetBtn.onClick = [this]()
    {
        const int idx = presetCombo.getSelectedId() - 1;
        if (idx >= 0 && !processor.getPresetManager().isPresetFactory(idx))
        {
            processor.getPresetManager().deletePreset(idx);
            updatePresetList();
            presetCombo.setSelectedId(1, juce::sendNotificationAsync);
        }
    };
    addAndMakeVisible(deletePresetBtn);

    // Big Master Dials
    auto setupMasterDial = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 14);
        s.getProperties().set("accentColor", col.toString());
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    setupMasterDial(balanceSlider, balanceLabel, "BALANCE", TransientLookAndFeel::accentSplit);
    setupMasterDial(dryWetSlider, dryWetLabel, "DRY / WET", TransientLookAndFeel::accentTransient);
    setupMasterDial(outGainSlider, outGainLabel, "OUTPUT", TransientLookAndFeel::accentSustain);

    limiterButton.getProperties().set("accentColor", juce::Colour(0xffff9500).toString());
    addAndMakeVisible(limiterButton);

    outputRoutingCombo.addItem("Stereo Mix (2 Ch)", 1);
    outputRoutingCombo.addItem("Split Direct (4 Ch)", 2);
    outputRoutingCombo.addItem("Auto (DAW Bus)", 3);
    addAndMakeVisible(outputRoutingCombo);

    // APVTS Attachments for Master
    auto& apvts = processor.getAPVTS();
    balanceAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::masterBalance.getParamID(), balanceSlider);
    dryWetAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::masterDryWet.getParamID(), dryWetSlider);
    outGainAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::masterGain.getParamID(), outGainSlider);
    limiterAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, IDs::masterLimiter.getParamID(), limiterButton);
    outputRoutingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, IDs::outputRoutingMode.getParamID(), outputRoutingCombo);

    // Top Navigation Tabs Setup
    auto setupNavBtn = [](juce::TextButton& btn, const juce::Colour& accent)
    {
        btn.getProperties().set("isNavTab", true);
        btn.getProperties().set("accentColor", accent.toString());
    };

    setupNavBtn(preProcTabBtn, juce::Colour(0xffff9900));
    setupNavBtn(fxRackTabBtn,  TransientLookAndFeel::accentTransient);
    setupNavBtn(routingTabBtn, juce::Colour(0xff38bdf8));
    setupNavBtn(modTabBtn,     juce::Colour(0xffa855f7));
    setupNavBtn(matrixTabBtn,  juce::Colour(0xffec4899));

    preProcTabBtn.onClick = [this]() { setViewTab(EditorViewTab::PreProcessors); };
    fxRackTabBtn.onClick  = [this]() { setViewTab(EditorViewTab::FxRack); };
    routingTabBtn.onClick = [this]() { setViewTab(EditorViewTab::RoutingDiagram); };
    modTabBtn.onClick     = [this]() { setViewTab(EditorViewTab::Modulators); };
    matrixTabBtn.onClick  = [this]() { setViewTab(EditorViewTab::ModulationMatrix); };

    addAndMakeVisible(preProcTabBtn);
    addAndMakeVisible(fxRackTabBtn);
    addAndMakeVisible(routingTabBtn);
    addAndMakeVisible(modTabBtn);
    addAndMakeVisible(matrixTabBtn);

    // 1. Pre-Processors Panel (Splitter & 3-Band Filters)
    preProcessorsPanel = std::make_unique<PreProcessorsCombinedComponent>(apvts, processor.getEngine());
    addChildComponent(*preProcessorsPanel);

    // 2. 8-Slot Modular FX Rack
    for (int s = 0; s < 8; ++s)
    {
        fxSlots[s] = std::make_unique<EffectSlotComponent>(apvts, processor.getEngine(), s);
        addChildComponent(*fxSlots[s]);
    }

    // 3. Audio Routing Diagram
    routingDiagram = std::make_unique<AudioRoutingDiagramComponent>(apvts, processor.getEngine());
    addChildComponent(*routingDiagram);

    // 4. Modulators Panel
    modulatorsPanel = std::make_unique<ModulatorsPanelComponent>(apvts, processor.getEngine());
    addChildComponent(*modulatorsPanel);

    // 5. Modulation Matrix Panel
    modMatrixPanel = std::make_unique<ModMatrixComponent>(apvts, processor.getEngine());
    addChildComponent(*modMatrixPanel);

    // Footer Status
    statusLabel.setText("OMNISPLIT Multi-Split Modular FX Processor  •  Created by mtyas", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    // Default View: Pre-Processors (Tab 1)
    setViewTab(EditorViewTab::PreProcessors);

    setSize(1320, 860);
    startTimerHz(30);
}

TransientSplitEditor::~TransientSplitEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void TransientSplitEditor::setViewTab(EditorViewTab tab)
{
    currentTab = tab;

    preProcTabBtn.getProperties().set("isActiveTab", (tab == EditorViewTab::PreProcessors));
    fxRackTabBtn.getProperties().set("isActiveTab",  (tab == EditorViewTab::FxRack));
    routingTabBtn.getProperties().set("isActiveTab", (tab == EditorViewTab::RoutingDiagram));
    modTabBtn.getProperties().set("isActiveTab",     (tab == EditorViewTab::Modulators));
    matrixTabBtn.getProperties().set("isActiveTab",  (tab == EditorViewTab::ModulationMatrix));

    preProcTabBtn.repaint();
    fxRackTabBtn.repaint();
    routingTabBtn.repaint();
    modTabBtn.repaint();
    matrixTabBtn.repaint();

    // Show / hide components
    if (preProcessorsPanel)
        preProcessorsPanel->setVisible(tab == EditorViewTab::PreProcessors);

    const bool isRack = (tab == EditorViewTab::FxRack);
    for (int s = 0; s < 8; ++s)
    {
        if (fxSlots[s])
            fxSlots[s]->setVisible(isRack);
    }

    if (routingDiagram)
        routingDiagram->setVisible(tab == EditorViewTab::RoutingDiagram);

    if (modulatorsPanel)
        modulatorsPanel->setVisible(tab == EditorViewTab::Modulators);

    if (modMatrixPanel)
        modMatrixPanel->setVisible(tab == EditorViewTab::ModulationMatrix);

    resized();
}

bool TransientSplitEditor::keyPressed(const juce::KeyPress& key)
{
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
    {
        if (key.getKeyCode() == 'Z')
        {
            if (key.getModifiers().isShiftDown())
                processor.getUndoManager().redo();
            else
                processor.getUndoManager().undo();
            return true;
        }
        if (key.getKeyCode() == 'Y')
        {
            processor.getUndoManager().redo();
            return true;
        }
    }
    return false;
}

void TransientSplitEditor::updatePresetList()
{
    presetCombo.clear(juce::dontSendNotification);
    auto& pm = processor.getPresetManager();
    pm.refreshPresetList();

    const int numPresets = pm.getNumPresets();
    for (int i = 0; i < numPresets; ++i)
    {
        const juce::String prefix = pm.isPresetFactory(i) ? "[Factory] " : "[User] ";
        presetCombo.addItem(prefix + pm.getPresetName(i), i + 1);
    }
    presetCombo.setSelectedId(1, juce::dontSendNotification);
}

void TransientSplitEditor::showSavePresetDialog()
{
    auto* window = new juce::AlertWindow("Save User Preset", "Enter a name for the new preset:", juce::AlertWindow::QuestionIcon);
    window->addTextEditor("presetName", "My Omni Preset", "Preset Name:");
    window->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, window](int result)
        {
            if (result == 1)
            {
                auto name = window->getTextEditorContents("presetName");
                if (name.isNotEmpty())
                {
                    processor.getPresetManager().saveCurrentPresetAs(name, "User");
                    updatePresetList();
                    presetCombo.setText(name, juce::dontSendNotification);
                }
            }
            delete window;
        }), true);
}

void TransientSplitEditor::timerCallback()
{
    const float inPk = processor.getEngine().getInputMeterLevel();
    const float outPk = processor.getEngine().getOutputMeterLevel();
    const float inDb = (inPk > 1e-5f) ? 20.0f * std::log10(inPk) : -60.0f;
    const float outDb = (outPk > 1e-5f) ? 20.0f * std::log10(outPk) : -60.0f;

    statusLabel.setText("OMNISPLIT • Created by mtyas  |  In: " + juce::String(inDb, 1) + " dB  |  Out: " + juce::String(outDb, 1) + " dB", juce::dontSendNotification);
}

void TransientSplitEditor::paint(juce::Graphics& g)
{
    g.fillAll(TransientLookAndFeel::bgDark);

    // Top Header Background
    g.setColour(TransientLookAndFeel::bgCardHeader);
    g.fillRect(0, 0, getWidth(), 64);

    // Header divider line
    g.setColour(TransientLookAndFeel::borderCol);
    g.drawHorizontalLine(64, 0.0f, static_cast<float>(getWidth()));

    // Footer divider line
    g.drawHorizontalLine(getHeight() - 26, 0.0f, static_cast<float>(getWidth()));
}

void TransientSplitEditor::resized()
{
    auto area = getLocalBounds();

    // 1. Header Bar (64px)
    auto header = area.removeFromTop(64).reduced(12, 6);

    auto titleArea = header.removeFromLeft(180);
    pluginTitle.setBounds(titleArea.removeFromTop(26));
    pluginSubtitle.setBounds(titleArea.removeFromTop(18));

    header.removeFromLeft(8);

    // Preset & Undo Area
    auto presetArea = header.removeFromLeft(360);
    auto pRow1 = presetArea.removeFromTop(24);
    undoBtn.setBounds(pRow1.removeFromLeft(48));
    pRow1.removeFromLeft(4);
    redoBtn.setBounds(pRow1.removeFromLeft(48));
    pRow1.removeFromLeft(8);
    prevPresetBtn.setBounds(pRow1.removeFromLeft(24));
    pRow1.removeFromLeft(2);
    nextPresetBtn.setBounds(pRow1.removeFromLeft(24));
    pRow1.removeFromLeft(4);
    savePresetBtn.setBounds(pRow1.removeFromLeft(46));
    pRow1.removeFromLeft(4);
    loadPresetBtn.setBounds(pRow1.removeFromLeft(54));
    pRow1.removeFromLeft(4);
    deletePresetBtn.setBounds(pRow1.removeFromLeft(40));

    presetArea.removeFromTop(4);
    presetCombo.setBounds(presetArea.removeFromTop(22));

    // Master Controls on the Right
    auto masterArea = header.removeFromRight(360);
    const int masterDialW = 68;

    auto m1 = masterArea.removeFromLeft(masterDialW);
    balanceLabel.setBounds(m1.removeFromBottom(14));
    balanceSlider.setBounds(m1);

    auto m2 = masterArea.removeFromLeft(masterDialW);
    dryWetLabel.setBounds(m2.removeFromBottom(14));
    dryWetSlider.setBounds(m2);

    auto m3 = masterArea.removeFromLeft(masterDialW);
    outGainLabel.setBounds(m3.removeFromBottom(14));
    outGainSlider.setBounds(m3);

    masterArea.removeFromLeft(8);
    auto mRight = masterArea;
    limiterButton.setBounds(mRight.removeFromTop(24));
    mRight.removeFromTop(4);
    outputRoutingCombo.setBounds(mRight.removeFromTop(22));

    // 2. Footer (26px)
    auto footer = area.removeFromBottom(26).reduced(12, 2);
    statusLabel.setBounds(footer);

    // 3. 5-Tab Navigation Bar (32px)
    auto tabBar = area.removeFromTop(32).reduced(12, 2);
    const int tabW = tabBar.getWidth() / 5;
    preProcTabBtn.setBounds(tabBar.removeFromLeft(tabW).reduced(2, 0));
    fxRackTabBtn.setBounds(tabBar.removeFromLeft(tabW).reduced(2, 0));
    routingTabBtn.setBounds(tabBar.removeFromLeft(tabW).reduced(2, 0));
    modTabBtn.setBounds(tabBar.removeFromLeft(tabW).reduced(2, 0));
    matrixTabBtn.setBounds(tabBar.reduced(2, 0));

    area.removeFromTop(6); // Gap

    // 4. Viewport Area
    auto viewArea = area.reduced(12, 4);

    if (currentTab == EditorViewTab::PreProcessors && preProcessorsPanel)
    {
        preProcessorsPanel->setBounds(viewArea);
    }
    else if (currentTab == EditorViewTab::FxRack)
    {
        // 2 Rows of 4 Slots
        const int rowH = (viewArea.getHeight() - 8) / 2;
        const int colW = (viewArea.getWidth() - 18) / 4;

        auto row1 = viewArea.removeFromTop(rowH);
        viewArea.removeFromTop(8);
        auto row2 = viewArea;

        for (int i = 0; i < 4; ++i)
        {
            if (fxSlots[i])
            {
                fxSlots[i]->setBounds(row1.removeFromLeft(colW));
                if (i < 3) row1.removeFromLeft(6);
            }
        }

        for (int i = 4; i < 8; ++i)
        {
            if (fxSlots[i])
            {
                fxSlots[i]->setBounds(row2.removeFromLeft(colW));
                if (i < 7) row2.removeFromLeft(6);
            }
        }
    }
    else if (currentTab == EditorViewTab::RoutingDiagram && routingDiagram)
    {
        routingDiagram->setBounds(viewArea);
    }
    else if (currentTab == EditorViewTab::Modulators && modulatorsPanel)
    {
        modulatorsPanel->setBounds(viewArea);
    }
    else if (currentTab == EditorViewTab::ModulationMatrix && modMatrixPanel)
    {
        modMatrixPanel->setBounds(viewArea);
    }
}
