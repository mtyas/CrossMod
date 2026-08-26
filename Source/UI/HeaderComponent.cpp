#include "HeaderComponent.h"
#include "../Common/ScaleTheory.h"

namespace MidiFlux
{

HeaderComponent::HeaderComponent(MidiChainProcessor& chain, UndoHistoryManager& history)
    : chainProcessor(chain), undoHistory(history)
{
    // Logo & title
    titleLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(titleLabel);

    subtitleLabel.setFont(juce::FontOptions(8.5f));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(subtitleLabel);

    // Undo / Redo
    undoBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    undoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    undoBtn.setTooltip("Undo last rack action (Ctrl+Z)");
    undoBtn.onClick = [this]() {
        if (undoHistory.undo(chainProcessor))
        {
            updateUndoRedoButtons();
            syncScaleBoxes();
        }
    };
    addAndMakeVisible(undoBtn);

    redoBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    redoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    redoBtn.setTooltip("Redo last rack action (Ctrl+Y)");
    redoBtn.onClick = [this]() {
        if (undoHistory.redo(chainProcessor))
        {
            updateUndoRedoButtons();
            syncScaleBoxes();
        }
    };
    addAndMakeVisible(redoBtn);

    undoHistory.onHistoryChanged = [this]() {
        updateUndoRedoButtons();
    };
    updateUndoRedoButtons();

    // Keyboard Toggle button
    kbdToggleBtn.setClickingTogglesState(true);
    kbdToggleBtn.setToggleState(true, juce::dontSendNotification);
    kbdToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    kbdToggleBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00e5ff).withAlpha(0.25f));
    kbdToggleBtn.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff00e5ff));
    kbdToggleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    kbdToggleBtn.setTooltip("Show / Hide On-Screen Keyboard");
    kbdToggleBtn.onClick = [this]() {
        if (onToggleKeyboard)
            onToggleKeyboard();
    };
    addAndMakeVisible(kbdToggleBtn);

    // Global scale controls
    scaleLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    scaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(scaleLabel);

    setupScales();
    setupPresets();

    // File Save & Load buttons
    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    savePresetBtn.setTooltip("Save current rack configuration to a preset file");
    savePresetBtn.onClick = [this]() { savePresetToFile(); };
    addAndMakeVisible(savePresetBtn);

    loadPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    loadPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    loadPresetBtn.setTooltip("Load a preset file from disk");
    loadPresetBtn.onClick = [this]() { loadPresetFromFile(); };
    addAndMakeVisible(loadPresetBtn);

    // Randomize all button
    randomAllBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    randomAllBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    randomAllBtn.setTooltip("Randomize all modules in rack");
    randomAllBtn.onClick = [this]() {
        chainProcessor.randomizeAll();
        if (onStateChanged)
            onStateChanged();
    };
    addAndMakeVisible(randomAllBtn);

    // Master Bypass button
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setToggleState(chainProcessor.isMasterBypassed(), juce::dontSendNotification);
    bypassBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    bypassBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xfff85149));
    bypassBtn.setTooltip("Bypass the entire MIDI rack");
    bypassBtn.onClick = [this]() {
        chainProcessor.setMasterBypassed(bypassBtn.getToggleState());
    };
    addAndMakeVisible(bypassBtn);

    // Panic button
    panicBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff842029));
    panicBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff7b72));
    panicBtn.setTooltip("Send All Notes Off to all 16 MIDI channels");
    panicBtn.onClick = [this]() {
        if (onPanicTriggered)
            onPanicTriggered();
    };
    addAndMakeVisible(panicBtn);
}

HeaderComponent::~HeaderComponent()
{
}

void HeaderComponent::updateUndoRedoButtons()
{
    undoBtn.setEnabled(undoHistory.canUndo());
    redoBtn.setEnabled(undoHistory.canRedo());
}

void HeaderComponent::syncScaleBoxes()
{
    rootKeyBox.setSelectedId(chainProcessor.getGlobalRootKey() + 1, juce::dontSendNotification);
    scaleTypeBox.setSelectedId(chainProcessor.getGlobalScaleType() + 1, juce::dontSendNotification);
}

void HeaderComponent::setupScales()
{
    rootKeyBox.setJustificationType(juce::Justification::centred);
    for (int k = 0; k < 12; ++k)
        rootKeyBox.addItem(juce::String(ScaleTheory::getNoteName(k)), k + 1);

    rootKeyBox.setSelectedId(chainProcessor.getGlobalRootKey() + 1, juce::dontSendNotification);
    rootKeyBox.onChange = [this]() {
        chainProcessor.setGlobalRootKey(rootKeyBox.getSelectedId() - 1);
        if (onStateChanged)
            onStateChanged();
    };
    addAndMakeVisible(rootKeyBox);

    const auto& scales = ScaleTheory::getAllScales();
    for (size_t i = 0; i < scales.size(); ++i)
        scaleTypeBox.addItem(scales[i].name, static_cast<int>(i + 1));

    scaleTypeBox.setSelectedId(chainProcessor.getGlobalScaleType() + 1, juce::dontSendNotification);
    scaleTypeBox.onChange = [this]() {
        chainProcessor.setGlobalScaleType(scaleTypeBox.getSelectedId() - 1);
        if (onStateChanged)
            onStateChanged();
    };
    addAndMakeVisible(scaleTypeBox);
}

void HeaderComponent::setupPresets()
{
    presetLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    presetLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(presetLabel);

    auto presets = PresetManager::getFactoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        presetBox.addItem(presets[i].name, static_cast<int>(i + 1));

    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this]() {
        int idx = presetBox.getSelectedId() - 1;
        PresetManager::applyPreset(chainProcessor, idx);
        syncScaleBoxes();
        if (onStateChanged)
            onStateChanged();
    };
    addAndMakeVisible(presetBox);

    prevPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    prevPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    prevPresetBtn.setTooltip("Previous factory preset");
    prevPresetBtn.onClick = [this]() {
        int cur = presetBox.getSelectedId();
        if (cur > 1) presetBox.setSelectedId(cur - 1);
        else presetBox.setSelectedId(presetBox.getNumItems());
    };
    addAndMakeVisible(prevPresetBtn);

    nextPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    nextPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    nextPresetBtn.setTooltip("Next factory preset");
    nextPresetBtn.onClick = [this]() {
        int cur = presetBox.getSelectedId();
        if (cur < presetBox.getNumItems()) presetBox.setSelectedId(cur + 1);
        else presetBox.setSelectedId(1);
    };
    addAndMakeVisible(nextPresetBtn);
}

void HeaderComponent::savePresetToFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save MidiFlux Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("MidiFluxPreset.midiflux"),
        "*.midiflux");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File())
        {
            auto vt = chainProcessor.getState();
            std::unique_ptr<juce::XmlElement> xml(vt.createXml());
            if (xml != nullptr)
            {
                xml->writeTo(file);
            }
        }
    });
}

void HeaderComponent::loadPresetFromFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load MidiFlux Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.midiflux");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File() && file.existsAsFile())
        {
            auto xml = juce::parseXML(file);
            if (xml != nullptr)
            {
                auto vt = juce::ValueTree::fromXml(*xml);
                if (vt.isValid())
                {
                    chainProcessor.setState(vt);
                    syncScaleBoxes();
                    if (onStateChanged)
                        onStateChanged();
                }
            }
        }
    });
}

void HeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient bg(juce::Colour(0xff161b22), 0, 0, juce::Colour(0xff0d1117), 0, bounds.getHeight(), false);
    g.setGradientFill(bg);
    g.fillRect(bounds);

    // Vector lightning bolt logo icon
    juce::Path bolt;
    bolt.startNewSubPath(16.0f, 13.0f);
    bolt.lineTo(10.0f, 24.0f);
    bolt.lineTo(15.0f, 24.0f);
    bolt.lineTo(12.0f, 35.0f);
    bolt.lineTo(21.0f, 21.0f);
    bolt.lineTo(16.0f, 21.0f);
    bolt.closeSubPath();
    g.setColour(juce::Colour(0xff00e5ff));
    g.fillPath(bolt);

    // Subtle bottom border
    g.setColour(juce::Colour(0xff30363d));
    g.drawLine(0, bounds.getHeight(), bounds.getWidth(), bounds.getHeight(), 1.0f);

    // Subtle vertical dividers between functional sections
    g.setColour(juce::Colour(0xff21262d));
    g.fillRect(195, 8, 1, (int)bounds.getHeight() - 16);
    g.fillRect(460, 8, 1, (int)bounds.getHeight() - 16);
    g.fillRect(720, 8, 1, (int)bounds.getHeight() - 16);
    g.fillRect(845, 8, 1, (int)bounds.getHeight() - 16);
}

void HeaderComponent::resized()
{
    int y = 11;
    int h = 28;

    // Logo on the left (offset slightly to leave room for vector lightning bolt)
    titleLabel.setBounds(24, 6, 96, 20);
    subtitleLabel.setBounds(24, 26, 100, 14);

    // Undo / Redo buttons right next to logo
    int x = 130;
    undoBtn.setBounds(x, y, 26, h);
    x += 28;
    redoBtn.setBounds(x, y, 26, h);

    // Scale Controls
    x = 205;
    scaleLabel.setBounds(x, y + 4, 44, 18);
    x += 46;
    rootKeyBox.setBounds(x, y, 62, h);
    x += 66;
    scaleTypeBox.setBounds(x, y, 136, h);

    // Preset Controls
    x = 470;
    presetLabel.setBounds(x, y + 4, 50, 18);
    x += 52;
    prevPresetBtn.setBounds(x, y, 22, h);
    x += 24;
    presetBox.setBounds(x, y, 144, h);
    x += 146;
    nextPresetBtn.setBounds(x, y, 22, h);

    // Save & Load Preset Buttons
    x = 730;
    savePresetBtn.setBounds(x, y, 52, h);
    x += 56;
    loadPresetBtn.setBounds(x, y, 52, h);

    // Right Action Buttons
    int right = getWidth() - 12;

    right -= 66;
    panicBtn.setBounds(right, y, 66, h);

    right -= 62;
    bypassBtn.setBounds(right, y, 58, h);

    right -= 44;
    randomAllBtn.setBounds(right, y, 40, h);

    right -= 46;
    kbdToggleBtn.setBounds(right, y, 42, h);
}

} // namespace MidiFlux
