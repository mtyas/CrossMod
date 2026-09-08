#include "ScaleSequencerComponent.h"

namespace MidiFlux
{

static const struct { float val; const char* name; } barOptions[] = {
    { 0.25f, "1/4 Bar" },
    { 0.5f,  "1/2 Bar" },
    { 1.0f,  "1 Bar" },
    { 2.0f,  "2 Bars" },
    { 3.0f,  "3 Bars" },
    { 4.0f,  "4 Bars" },
    { 6.0f,  "6 Bars" },
    { 8.0f,  "8 Bars" },
    { 12.0f, "12 Bars" },
    { 16.0f, "16 Bars" }
};
static constexpr int numBarOptions = 10;

// ==============================================================================
// ScaleBlockCard Implementation
// ==============================================================================

ScaleBlockCard::ScaleBlockCard(int index, ScaleProgression& prog, std::function<void()> onChg)
    : blockIndex(index), progression(prog), onChanged(onChg)
{
    setupControls();
    updateFromProgression();
}

void ScaleBlockCard::setIndex(int idx)
{
    blockIndex = idx;
    indexLabel.setText("#" + juce::String(blockIndex + 1), juce::dontSendNotification);
}

void ScaleBlockCard::setupControls()
{
    // Index Label
    indexLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    indexLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    indexLabel.setText("#" + juce::String(blockIndex + 1), juce::dontSendNotification);
    addAndMakeVisible(indexLabel);

    // Remove Button
    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    removeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff7b72));
    removeButton.setTooltip("Remove this block");
    removeButton.onClick = [this]() {
        if (onRemove)
            onRemove(blockIndex);
    };
    addAndMakeVisible(removeButton);

    // Root Key Box
    rootKeyBox.setJustificationType(juce::Justification::centred);
    for (int k = 0; k < 12; ++k)
        rootKeyBox.addItem(juce::String(ScaleTheory::getNoteName(k)), k + 1);
    rootKeyBox.onChange = [this]() {
        auto b = progression.getBlock(blockIndex);
        b.rootKey = rootKeyBox.getSelectedId() - 1;
        progression.setBlock(blockIndex, b);
        if (onChanged) onChanged();
    };
    addAndMakeVisible(rootKeyBox);

    // Scale Type Box
    scaleTypeBox.setJustificationType(juce::Justification::centredLeft);
    const auto& scales = ScaleTheory::getAllScales();
    for (size_t i = 0; i < scales.size(); ++i)
        scaleTypeBox.addItem(scales[i].name, static_cast<int>(i + 1));
    scaleTypeBox.onChange = [this]() {
        auto b = progression.getBlock(blockIndex);
        b.scaleType = scaleTypeBox.getSelectedId() - 1;
        progression.setBlock(blockIndex, b);
        if (onChanged) onChanged();
    };
    addAndMakeVisible(scaleTypeBox);

    // Bars Box
    barsBox.setJustificationType(juce::Justification::centred);
    for (int i = 0; i < numBarOptions; ++i)
        barsBox.addItem(barOptions[i].name, i + 1);
    barsBox.onChange = [this]() {
        int id = barsBox.getSelectedId() - 1;
        if (id >= 0 && id < numBarOptions)
        {
            auto b = progression.getBlock(blockIndex);
            b.bars = barOptions[id].val;
            progression.setBlock(blockIndex, b);
            if (onChanged) onChanged();
        }
    };
    addAndMakeVisible(barsBox);
}

void ScaleBlockCard::updateFromProgression()
{
    auto b = progression.getBlock(blockIndex);
    rootKeyBox.setSelectedId(b.rootKey + 1, juce::dontSendNotification);
    scaleTypeBox.setSelectedId(b.scaleType + 1, juce::dontSendNotification);

    int barId = 3; // Default 1 Bar (index 2)
    for (int i = 0; i < numBarOptions; ++i)
    {
        if (std::abs(barOptions[i].val - b.bars) < 0.01f)
        {
            barId = i + 1;
            break;
        }
    }
    barsBox.setSelectedId(barId, juce::dontSendNotification);
}

void ScaleBlockCard::setActive(bool active, float progress)
{
    if (isActive != active || std::abs(playbackProgress - progress) > 0.02f)
    {
        isActive = active;
        playbackProgress = progress;
        repaint();
    }
}

void ScaleBlockCard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Card background
    g.setColour(isActive ? juce::Colour(0xff1f2937) : juce::Colour(0xff161b22));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Border
    if (isActive)
    {
        g.setColour(juce::Colour(0xff00e5ff).withAlpha(0.85f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.8f);

        // Active sweep progress bar on bottom
        float barW = bounds.getWidth() * playbackProgress;
        auto progRect = juce::Rectangle<float>(0.0f, bounds.getBottom() - 3.5f, barW, 3.5f);
        g.setColour(juce::Colour(0xff00e5ff));
        g.fillRoundedRectangle(progRect, 1.5f);
    }
    else
    {
        g.setColour(juce::Colour(0xff30363d));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
    }
}

void ScaleBlockCard::resized()
{
    auto area = getLocalBounds().reduced(5);

    // Header row
    auto headerRow = area.removeFromTop(18);
    removeButton.setBounds(headerRow.removeFromRight(18).reduced(1, 1));
    indexLabel.setBounds(headerRow);

    area.removeFromTop(3);
    // Row 1: Root & Scale
    auto r1 = area.removeFromTop(22);
    rootKeyBox.setBounds(r1.removeFromLeft(54));
    r1.removeFromLeft(5);
    scaleTypeBox.setBounds(r1);

    area.removeFromTop(4);
    // Row 2: Duration (bars)
    barsBox.setBounds(area.removeFromTop(20));
}

// ==============================================================================
// ScaleSequencerComponent Implementation
// ==============================================================================

ScaleSequencerComponent::ScaleSequencerComponent(MidiChainProcessor& chain)
    : chainProcessor(chain)
{
    contentComponent = std::make_unique<juce::Component>();
    viewport.setViewedComponent(contentComponent.get(), false);
    viewport.setScrollBarsShown(false, true); // show horizontal scrollbar if needed
    addAndMakeVisible(viewport);

    // Title label
    titleLabel.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(titleLabel);

    // Status / Timeline Readout
    statusLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(statusLabel);

    // Enable Toggle
    enableToggle.setClickingTogglesState(true);
    enableToggle.setToggleState(chainProcessor.getScaleProgression().isEnabled(), juce::dontSendNotification);
    enableToggle.setButtonText(enableToggle.getToggleState() ? "SEQ ON" : "SEQ OFF");
    enableToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    enableToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff238636));
    enableToggle.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    enableToggle.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    enableToggle.setTooltip("Enable timeline synchronized scale progression sequencing");
    enableToggle.onClick = [this]() {
        bool on = enableToggle.getToggleState();
        enableToggle.setButtonText(on ? "SEQ ON" : "SEQ OFF");
        chainProcessor.getScaleProgression().setEnabled(on);
        if (onProgressionChanged) onProgressionChanged();
        repaint();
    };
    addAndMakeVisible(enableToggle);

    // Loop Toggle
    loopToggle.setClickingTogglesState(true);
    loopToggle.setToggleState(chainProcessor.getScaleProgression().isLoop(), juce::dontSendNotification);
    loopToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    loopToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff1f6feb));
    loopToggle.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    loopToggle.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    loopToggle.setTooltip("Loop the scale progression continuously");
    loopToggle.onClick = [this]() {
        chainProcessor.getScaleProgression().setLoop(loopToggle.getToggleState());
        if (onProgressionChanged) onProgressionChanged();
    };
    addAndMakeVisible(loopToggle);

    // Presets Button
    presetsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    presetsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    presetsButton.setTooltip("Load a musical chord/scale progression preset");
    presetsButton.onClick = [this]() { showPresetsMenu(); };
    addAndMakeVisible(presetsButton);

    // Add Block Button
    addBlockButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    addBlockButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e5ff));
    addBlockButton.setTooltip("Add a new scale block to the song progression");
    addBlockButton.onClick = [this]() { addNewBlock(); };
    contentComponent->addAndMakeVisible(addBlockButton);

    rebuildBlockCards();
    startTimerHz(30);
}

ScaleSequencerComponent::~ScaleSequencerComponent()
{
    stopTimer();
}

void ScaleSequencerComponent::rebuildBlockCards()
{
    blockCards.clear();
    contentComponent->removeAllChildren();

    auto& prog = chainProcessor.getScaleProgression();
    int num = prog.getNumBlocks();

    int cardW = 168;
    int cardH = 76;
    int spacing = 8;

    for (int i = 0; i < num; ++i)
    {
        auto card = std::make_unique<ScaleBlockCard>(i, prog, [this]() {
            if (onProgressionChanged) onProgressionChanged();
        });
        card->onRemove = [this](int idx) { removeBlock(idx); };
        contentComponent->addAndMakeVisible(card.get());
        blockCards.push_back(std::move(card));
    }

    contentComponent->addAndMakeVisible(addBlockButton);

    int totalW = (num + 1) * (cardW + spacing) + spacing;
    contentComponent->setBounds(0, 0, std::max(totalW, viewport.getWidth()), cardH);

    int curX = spacing;
    for (size_t i = 0; i < blockCards.size(); ++i)
    {
        blockCards[i]->setBounds(curX, 0, cardW, cardH);
        curX += cardW + spacing;
    }
    addBlockButton.setBounds(curX, 0, 100, cardH);

    enableToggle.setToggleState(prog.isEnabled(), juce::dontSendNotification);
    enableToggle.setButtonText(prog.isEnabled() ? "SEQ ON" : "SEQ OFF");
    loopToggle.setToggleState(prog.isLoop(), juce::dontSendNotification);
}

void ScaleSequencerComponent::addNewBlock()
{
    auto& prog = chainProcessor.getScaleProgression();
    int lastRoot = 0;
    int lastScale = 0;
    float lastBars = 2.0f;

    if (prog.getNumBlocks() > 0)
    {
        auto last = prog.getBlock(prog.getNumBlocks() - 1);
        lastRoot = (last.rootKey + 5) % 12; // Musical circle-of-fifths step
        lastScale = last.scaleType;
        lastBars = last.bars;
    }

    prog.addBlock({ lastRoot, lastScale, lastBars, "" });
    rebuildBlockCards();
    if (onProgressionChanged) onProgressionChanged();
}

void ScaleSequencerComponent::removeBlock(int index)
{
    auto& prog = chainProcessor.getScaleProgression();
    if (prog.getNumBlocks() > 1)
    {
        prog.removeBlock(index);
        rebuildBlockCards();
        if (onProgressionChanged) onProgressionChanged();
    }
}

void ScaleSequencerComponent::showPresetsMenu()
{
    juce::PopupMenu m;

    juce::PopupMenu factoryMenu;
    factoryMenu.addItem(1, "Pop 4-Chords (C - G - Am - F) [8b]");
    factoryMenu.addItem(2, "Jazz Turnaround (ii - V - I - VI) [4b]");
    factoryMenu.addItem(3, "Cinematic Dark Hero (Am - F - C - G) [16b]");
    factoryMenu.addItem(4, "12-Bar Blues in E [12b]");
    factoryMenu.addItem(5, "Modal Odyssey (Dorian - Mixo - Lydian - Phryg) [8b]");
    factoryMenu.addItem(6, "Neo-Soul Journey (F - Em - Dm - C) [8b]");
    m.addSubMenu("Factory Progressions", factoryMenu);

    auto& userList = ScaleProgression::getUserProgressions();
    juce::PopupMenu userMenu;
    if (userList.empty())
    {
        userMenu.addItem(0, "(No user progressions saved)", false, false);
    }
    else
    {
        for (size_t i = 0; i < userList.size(); ++i)
        {
            userMenu.addItem(static_cast<int>(100 + i), userList[i].name);
        }
    }
    m.addSubMenu("User Progressions", userMenu);

    m.addSeparator();
    m.addItem(10, "Save Current Progression as User Preset...");
    m.addItem(11, "Export Progression to File (.midifluxprog)...");
    m.addItem(12, "Import Progression from File (.midifluxprog)...");

    if (!userList.empty())
    {
        juce::PopupMenu deleteMenu;
        for (size_t i = 0; i < userList.size(); ++i)
        {
            deleteMenu.addItem(static_cast<int>(200 + i), userList[i].name);
        }
        m.addSubMenu("Delete User Progression", deleteMenu);
    }

    m.addSeparator();
    m.addItem(13, "Reset to Default Progression");

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetsButton),
        [this](int result) {
            if (result >= 1 && result <= 6)
            {
                chainProcessor.getScaleProgression().loadPresetProgression(result - 1);
                rebuildBlockCards();
                if (onProgressionChanged) onProgressionChanged();
            }
            else if (result >= 100 && result < 200)
            {
                int userIdx = result - 100;
                auto& list = ScaleProgression::getUserProgressions();
                if (userIdx >= 0 && userIdx < static_cast<int>(list.size()))
                {
                    chainProcessor.getScaleProgression().setAllBlocks(list[userIdx].blocks);
                    rebuildBlockCards();
                    if (onProgressionChanged) onProgressionChanged();
                }
            }
            else if (result >= 200 && result < 300)
            {
                int userIdx = result - 200;
                ScaleProgression::deleteUserProgression(userIdx);
            }
            else if (result == 10)
            {
                auto* w = new juce::AlertWindow("Save Progression", "Enter a name for this progression preset:", juce::AlertWindow::QuestionIcon);
                w->addTextEditor("name", "Custom Progression " + juce::String(ScaleProgression::getUserProgressions().size() + 1));
                w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
                w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int modalResult) {
                    if (modalResult == 1)
                    {
                        auto name = w->getTextEditorContents("name").trim();
                        if (name.isNotEmpty())
                        {
                            ScaleProgression::addUserProgression(name, chainProcessor.getScaleProgression().getAllBlocks());
                        }
                    }
                }), true);
            }
            else if (result == 11)
            {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Export Scale Progression",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Progression.midifluxprog"),
                    "*.midifluxprog");
                auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;
                fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File{})
                        chainProcessor.getScaleProgression().saveToFile(file);
                });
            }
            else if (result == 12)
            {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Import Scale Progression",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.midifluxprog");
                auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
                fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File{} && file.existsAsFile())
                    {
                        if (chainProcessor.getScaleProgression().loadFromFile(file))
                        {
                            rebuildBlockCards();
                            if (onProgressionChanged) onProgressionChanged();
                        }
                    }
                });
            }
            else if (result == 13)
            {
                chainProcessor.getScaleProgression().loadDefaultProgression();
                rebuildBlockCards();
                if (onProgressionChanged) onProgressionChanged();
            }
        });
}

void ScaleSequencerComponent::timerCallback()
{
    auto pb = chainProcessor.getLastPlaybackState();
    auto& prog = chainProcessor.getScaleProgression();

    if (prog.isEnabled() && pb.isPlaying)
    {
        // Update active card indicators
        for (size_t i = 0; i < blockCards.size(); ++i)
        {
            if (static_cast<int>(i) == pb.activeBlockIndex)
                blockCards[i]->setActive(true, pb.blockProgress);
            else
                blockCards[i]->setActive(false, 0.0f);
        }

        juce::String rootName = ScaleTheory::getNoteName(pb.activeRootKey);
        juce::String scaleName = ScaleTheory::getAllScales()[pb.activeScaleType].name;
        statusLabel.setText("BAR " + juce::String(pb.currentBar + 1.0, 1) + " | BLOCK #"
                            + juce::String(pb.activeBlockIndex + 1) + " (" + rootName + " " + scaleName + ")",
                            juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    }
    else
    {
        for (size_t i = 0; i < blockCards.size(); ++i)
            blockCards[i]->setActive(false, 0.0f);

        statusLabel.setText("TOTAL: " + juce::String(prog.getTotalBars(), 1) + " BARS | "
                            + juce::String(prog.getNumBlocks()) + " BLOCKS",
                            juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    }
}

void ScaleSequencerComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(juce::Colour(0xff0d1117));
    g.fillRect(bounds);

    // Left control area backdrop
    auto leftArea = bounds.removeFromLeft(190.0f);
    g.setColour(juce::Colour(0xff161b22));
    g.fillRect(leftArea);

    // Dividing lines
    g.setColour(juce::Colour(0xff30363d));
    g.drawLine(leftArea.getRight(), 0, leftArea.getRight(), bounds.getHeight(), 1.0f);
    g.drawLine(0, bounds.getHeight(), getWidth(), bounds.getHeight(), 1.0f);
}

void ScaleSequencerComponent::resized()
{
    int leftW = 190;
    int h = getHeight();

    titleLabel.setBounds(10, 8, 170, 16);
    statusLabel.setBounds(10, 26, 170, 14);

    enableToggle.setBounds(10, 48, 54, 26);
    loopToggle.setBounds(68, 48, 46, 26);
    presetsButton.setBounds(118, 48, 66, 26);

    viewport.setBounds(leftW + 4, 4, getWidth() - leftW - 8, h - 8);
    if (contentComponent)
    {
        int cardW = 168;
        int spacing = 8;
        int totalW = (chainProcessor.getScaleProgression().getNumBlocks() + 1) * (cardW + spacing) + spacing;
        contentComponent->setBounds(0, 0, std::max(totalW, viewport.getWidth()), h - 8);
    }
}

} // namespace MidiFlux
