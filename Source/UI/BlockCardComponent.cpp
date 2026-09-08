#include "BlockCardComponent.h"
#include <cmath>

namespace MidiFlux
{

BlockCardComponent::BlockCardComponent(MidiChainProcessor& chain, MidiBlock* blk, int idx)
    : chainProcessor(chain), block(blk), blockIndex(idx)
{
    // Power (Bypass) toggle button
    powerButton.setClickingTogglesState(true);
    powerButton.setToggleState(block ? !block->isBypassed() : true, juce::dontSendNotification);
    powerButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    powerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff238636));
    powerButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    powerButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    powerButton.setTooltip("Enable or bypass this module");
    powerButton.onClick = [this]() {
        if (block)
        {
            block->setBypassed(!powerButton.getToggleState());
            repaint();
            if (onParameterChanged)
                onParameterChanged();
        }
    };
    powerButton.addMouseListener(this, false);
    addAndMakeVisible(powerButton);

    // Routing toggle button (Series / Parallel)
    routingButton.setClickingTogglesState(true);
    bool isPar = (block && block->getRoutingMode() == RoutingMode::Parallel);
    routingButton.setToggleState(isPar, juce::dontSendNotification);
    routingButton.setButtonText(isPar ? "PAR" : "SER");
    routingButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    routingButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff8957e5));
    routingButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    routingButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    routingButton.setTooltip("Routing Mode: SER (Series - receives MIDI from previous module) or PAR (Parallel - receives raw DAW input, merges output into chain)");
    routingButton.onClick = [this]() {
        if (block)
        {
            bool par = routingButton.getToggleState();
            routingButton.setButtonText(par ? "PAR" : "SER");
            block->setRoutingMode(par ? RoutingMode::Parallel : RoutingMode::Series);
            repaint();
            if (onParameterChanged)
                onParameterChanged();
        }
    };
    routingButton.addMouseListener(this, false);
    addAndMakeVisible(routingButton);

    // Dice button (randomize this block)
    diceButton.setButtonText("DICE");
    diceButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    diceButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    diceButton.setTooltip("Randomize module parameters");
    diceButton.onClick = [this]() {
        if (block)
        {
            block->randomize();
            updateControlsFromBlock();
            if (onParameterChanged)
                onParameterChanged();
        }
    };
    addAndMakeVisible(diceButton);

    // Reorder buttons
    leftButton.setButtonText("<");
    leftButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    leftButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    leftButton.setTooltip("Move module left in processing chain");
    leftButton.onClick = [this]() { if (onMoveLeft) onMoveLeft(blockIndex); };
    addAndMakeVisible(leftButton);

    rightButton.setButtonText(">");
    rightButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    rightButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    rightButton.setTooltip("Move module right in processing chain");
    rightButton.onClick = [this]() { if (onMoveRight) onMoveRight(blockIndex); };
    addAndMakeVisible(rightButton);

    // Remove button
    removeButton.setButtonText("X");
    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
    removeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff7b72));
    removeButton.setTooltip("Remove this module from chain");
    removeButton.onClick = [this]() { if (onRemove) onRemove(blockIndex); };
    addAndMakeVisible(removeButton);

    // Build parameter controls
    if (block)
    {
        int numParams = block->getNumParameters();
        for (int i = 0; i < numParams; ++i)
        {
            const auto& def = block->getParameterDef(i);
            ParamControl pc;
            pc.paramIndex = i;

            pc.label = std::make_unique<juce::Label>("", def.name);
            pc.label->setFont(juce::FontOptions(11.0f, juce::Font::bold));
            pc.label->setColour(juce::Label::textColourId, juce::Colour(0xffe6edf3));
            addAndMakeVisible(pc.label.get());

            if (!def.options.empty())
            {
                pc.comboBox = std::make_unique<juce::ComboBox>();
                for (size_t optIdx = 0; optIdx < def.options.size(); ++optIdx)
                {
                    pc.comboBox->addItem(def.options[optIdx], static_cast<int>(optIdx + 1));
                }

                int currentSel = static_cast<int>(std::round(block->getParameterValue(i))) + 1;
                pc.comboBox->setSelectedId(currentSel, juce::dontSendNotification);

                int paramIdx = i;
                pc.comboBox->onChange = [this, paramIdx, cb = pc.comboBox.get()]() {
                    if (block)
                    {
                        block->setParameterValue(paramIdx, static_cast<float>(cb->getSelectedId() - 1));
                        if (onParameterChanged)
                            onParameterChanged();
                    }
                };
                pc.comboBox->addMouseListener(this, false);
                addAndMakeVisible(pc.comboBox.get());
            }
            else
            {
                pc.slider = std::make_unique<juce::Slider>();
                pc.slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                pc.slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 15);
                pc.slider->setRange(def.minValue, def.maxValue, def.step);
                pc.slider->setValue(block->getParameterValue(i), juce::dontSendNotification);
                pc.slider->setColour(juce::Slider::rotarySliderFillColourId, block->getAccentColor());
                pc.slider->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffffffff));
                pc.slider->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0d1117));
                pc.slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff30363d));

                if (!def.suffix.isEmpty())
                    pc.slider->setTextValueSuffix(" " + def.suffix);

                int paramIdx = i;
                pc.slider->onValueChange = [this, paramIdx, sl = pc.slider.get()]() {
                    if (block)
                    {
                        block->setParameterValue(paramIdx, static_cast<float>(sl->getValue()));
                        if (onParameterChanged)
                            onParameterChanged();
                    }
                };
                pc.slider->addMouseListener(this, false);
                addAndMakeVisible(pc.slider.get());
            }

            controls.push_back(std::move(pc));
        }
    }

    startTimerHz(30);
}

BlockCardComponent::~BlockCardComponent()
{
    stopTimer();
}

void BlockCardComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() || e.mods.isRightButtonDown())
    {
        if (e.eventComponent == &powerButton)
        {
            showMidiLearnMenu("block:" + juce::String(blockIndex) + ":power", &powerButton);
            return;
        }
        if (e.eventComponent == &routingButton)
        {
            showMidiLearnMenu("block:" + juce::String(blockIndex) + ":routing", &routingButton);
            return;
        }

        for (const auto& pc : controls)
        {
            if (pc.slider && (e.eventComponent == pc.slider.get() || pc.slider->isParentOf(e.eventComponent)))
            {
                showMidiLearnMenu("block:" + juce::String(blockIndex) + ":param:" + juce::String(pc.paramIndex), pc.slider.get());
                return;
            }
            if (pc.comboBox && (e.eventComponent == pc.comboBox.get() || pc.comboBox->isParentOf(e.eventComponent)))
            {
                showMidiLearnMenu("block:" + juce::String(blockIndex) + ":param:" + juce::String(pc.paramIndex), pc.comboBox.get());
                return;
            }
        }
        return;
    }

    if (e.eventComponent == this && e.y <= 38)
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
}

void BlockCardComponent::mouseDrag(const juce::MouseEvent& e)
{
    // Crucial fix: Only initiate card reordering if the drag started on this card's header,
    // never when tweaking rotary knobs, sliders, or dropdowns!
    if (e.eventComponent != this)
        return;

    if (e.getMouseDownY() <= 38 && e.getDistanceFromDragStart() > 5)
    {
        if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            if (!dragContainer->isDragAndDropActive())
            {
                auto dragImage = createComponentSnapshot(getLocalBounds().withHeight(38));
                dragContainer->startDragging("midiflux_card:" + juce::String(blockIndex), this, dragImage);
            }
        }
    }
}

void BlockCardComponent::mouseUp(const juce::MouseEvent& e)
{
    if (e.eventComponent == this)
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void BlockCardComponent::updateControlsFromBlock()
{
    if (!block) return;

    bool isPar = (block->getRoutingMode() == RoutingMode::Parallel);
    routingButton.setToggleState(isPar, juce::dontSendNotification);
    routingButton.setButtonText(isPar ? "PAR" : "SER");

    for (auto& pc : controls)
    {
        if (pc.comboBox)
        {
            int sel = static_cast<int>(std::round(block->getParameterValue(pc.paramIndex))) + 1;
            pc.comboBox->setSelectedId(sel, juce::dontSendNotification);
        }
        else if (pc.slider)
        {
            pc.slider->setValue(block->getParameterValue(pc.paramIndex), juce::dontSendNotification);
        }
    }
    repaint();
}

void BlockCardComponent::timerCallback()
{
    if (!block) return;

    bool dawPlaying = chainProcessor.isDawPlaying();
    bool newDawStopped = block->requiresDawPlayback() && !dawPlaying;

    bool needsRepaint = false;
    if (newDawStopped != isDawStopped)
    {
        isDawStopped = newDawStopped;
        needsRepaint = true;
    }

    bool bypassed = block->isBypassed();
    if (powerButton.getToggleState() == bypassed)
    {
        powerButton.setToggleState(!bypassed, juce::dontSendNotification);
        powerButton.setButtonText(bypassed ? "OFF" : "ON");
        needsRepaint = true;
    }

    bool isPar = (block->getRoutingMode() == RoutingMode::Parallel);
    if (routingButton.getToggleState() != isPar)
    {
        routingButton.setToggleState(isPar, juce::dontSendNotification);
        routingButton.setButtonText(isPar ? "PAR" : "SER");
        needsRepaint = true;
    }

    for (auto& pc : controls)
    {
        float val = block->getParameterValue(pc.paramIndex);
        if (pc.slider && std::abs(pc.slider->getValue() - val) > 0.001f)
        {
            pc.slider->setValue(val, juce::dontSendNotification);
        }
        else if (pc.comboBox)
        {
            int sel = static_cast<int>(std::round(val)) + 1;
            if (pc.comboBox->getSelectedId() != sel)
                pc.comboBox->setSelectedId(sel, juce::dontSendNotification);
        }
    }

    if (chainProcessor.getMidiLearnManager().isLearning())
        needsRepaint = true;

    if (block->hasActivityAndDecay())
    {
        activityGlow = 1.0f;
        ledActive = true;
        needsRepaint = true;
    }
    else
    {
        if (activityGlow > 0.01f)
        {
            activityGlow = std::max(0.0f, activityGlow - 0.08f);
            needsRepaint = true;
        }
        if (ledActive && activityGlow <= 0.05f)
        {
            ledActive = false;
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

void BlockCardComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bool bypassed = block ? block->isBypassed() : false;
    auto accent = block ? block->getAccentColor() : juce::Colour(0xff00e5ff);

    // Card background
    g.setColour(juce::Colour(0xff161b22).withMultipliedAlpha(bypassed ? 0.55f : 1.0f));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Outer border with dynamic neon glow pulse on activity
    if (!bypassed)
    {
        if (activityGlow > 0.05f)
        {
            g.setColour(accent.withAlpha(0.20f * activityGlow));
            g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 3.0f);
        }

        g.setColour(accent.withAlpha(0.35f + 0.55f * activityGlow));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.5f);
    }
    else
    {
        g.setColour(juce::Colour(0xff30363d));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
    }

    // Header bar background
    auto headerRect = bounds.removeFromTop(38.0f);
    g.setColour(juce::Colour(0xff21262d));
    g.fillRoundedRectangle(headerRect, 8.0f);
    g.fillRect(headerRect.removeFromBottom(8.0f));

    // Category accent strip on the left edge
    g.setColour(accent);
    g.fillRoundedRectangle(bounds.getX(), 4.0f, 4.0f, 30.0f, 2.0f);

    // Drag handle grip dots
    g.setColour(juce::Colour(0xff8b949e));
    float gripX = 8.0f;
    float gripY = 12.0f;
    for (int col = 0; col < 2; ++col)
    {
        for (int row = 0; row < 3; ++row)
        {
            g.fillEllipse(gripX + col * 4.0f, gripY + row * 5.0f, 2.5f, 2.5f);
        }
    }

    // LED Activity Indicator
    float ledX = 18.0f;
    float ledY = 15.0f;
    float ledSize = 7.0f;

    if (ledActive && !bypassed)
    {
        g.setColour(accent.withAlpha(0.5f));
        g.fillEllipse(ledX - 2.0f, ledY - 2.0f, ledSize + 4.0f, ledSize + 4.0f);
        g.setColour(accent);
        g.fillEllipse(ledX, ledY, ledSize, ledSize);
    }
    else
    {
        g.setColour(juce::Colour(0xff30363d));
        g.fillEllipse(ledX, ledY, ledSize, ledSize);
    }

    // Block Title
    g.setColour(bypassed ? juce::Colour(0xff8b949e) : juce::Colour(0xfff0f6fc));
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText(block ? block->getDisplayName() : "Block", 28, 0, 82, 36, juce::Justification::centredLeft, true);

    // -------------------------------------------------------------
    // Live Status / Visualizer Bar directly below header
    // -------------------------------------------------------------
    auto statusBounds = juce::Rectangle<float>(8.0f, 42.0f, (float)getWidth() - 16.0f, 24.0f);

    if (isDawStopped)
    {
        // Amber warning: DAW PLAY REQUIRED
        g.setColour(juce::Colour(0xfff59e0b).withAlpha(0.18f));
        g.fillRoundedRectangle(statusBounds, 4.0f);
        g.setColour(juce::Colour(0xfff59e0b).withAlpha(0.85f));
        g.drawRoundedRectangle(statusBounds, 4.0f, 1.2f);

        g.setColour(juce::Colour(0xfff59e0b));
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText("PAUSED: DAW PLAY REQUIRED", statusBounds, juce::Justification::centred, true);
    }
    else
    {
        // Normal real-time visual feedback
        g.setColour(juce::Colour(0xff0d1117).withAlpha(0.85f));
        g.fillRoundedRectangle(statusBounds, 4.0f);
        g.setColour(juce::Colour(0xff21262d));
        g.drawRoundedRectangle(statusBounds, 4.0f, 1.0f);

        juce::String typeId = block ? block->getTypeId() : "";

        if (typeId == "arpeggiator")
        {
            // Arp status text
            g.setColour(accent);
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText(block->getStatusDescription(), statusBounds.reduced(8.0f, 0.0f).removeFromLeft(140.0f), juce::Justification::centredLeft, true);

            // 8 mini sequencer step dots on right
            int activeStep = block->getVisualizerStep() % 8;
            float dotAreaRight = statusBounds.getRight() - 6.0f;
            for (int s = 7; s >= 0; --s)
            {
                float dx = dotAreaRight - (7 - s) * 9.0f;
                float dy = statusBounds.getCentreY() - 2.5f;
                if (s == activeStep && !bypassed)
                {
                    g.setColour(accent);
                    g.fillEllipse(dx - 1.0f, dy - 1.0f, 7.0f, 7.0f);
                }
                else
                {
                    g.setColour(juce::Colour(0xff30363d));
                    g.fillEllipse(dx, dy, 5.0f, 5.0f);
                }
            }
        }
        else if (typeId == "euclidean")
        {
            g.setColour(accent);
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText(block->getStatusDescription(), statusBounds.reduced(8.0f, 0.0f).removeFromLeft(130.0f), juce::Justification::centredLeft, true);

            int activeStep = block->getVisualizerStep() % 8;
            float dotAreaRight = statusBounds.getRight() - 6.0f;
            for (int s = 7; s >= 0; --s)
            {
                float dx = dotAreaRight - (7 - s) * 9.0f;
                float dy = statusBounds.getCentreY() - 2.5f;
                if (s == activeStep && !bypassed)
                {
                    g.setColour(accent);
                    g.fillEllipse(dx - 1.0f, dy - 1.0f, 7.0f, 7.0f);
                }
                else
                {
                    g.setColour(juce::Colour(0xff21262d));
                    g.drawEllipse(dx, dy, 5.0f, 5.0f, 1.2f);
                }
            }
        }
        else if (typeId == "lfo")
        {
            g.setColour(accent);
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText(block->getStatusDescription(), statusBounds.reduced(8.0f, 0.0f).removeFromLeft(130.0f), juce::Justification::centredLeft, true);

            // Real-time oscilloscope wave line on right
            auto oscRect = statusBounds.removeFromRight(70.0f).reduced(4.0f, 3.0f);
            juce::Path wavePath;
            float cy = oscRect.getCentreY();
            float h = oscRect.getHeight() * 0.45f;
            float val = block->getVisualizerValue();

            for (int px = 0; px < (int)oscRect.getWidth(); px += 2)
            {
                float progress = (float)px / oscRect.getWidth();
                float yVal = cy + std::sin(progress * 6.283f + val * 6.283f) * h;
                if (px == 0) wavePath.startNewSubPath(oscRect.getX() + px, yVal);
                else wavePath.lineTo(oscRect.getX() + px, yVal);
            }
            g.setColour(accent.withAlpha(0.6f));
            g.strokePath(wavePath, juce::PathStrokeType(1.2f));

            // Cursor dot
            float curY = cy + (val * 2.0f - 1.0f) * h;
            g.setColour(accent);
            g.fillEllipse(oscRect.getRight() - 6.0f, curY - 2.5f, 5.0f, 5.0f);
        }
        else if (typeId == "chords")
        {
            g.setColour(accent);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText("CHORD: " + block->getStatusDescription(), statusBounds, juce::Justification::centred, true);
        }
        else
        {
            // Default elegant status display
            g.setColour(accent.withAlpha(0.9f));
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText(block ? block->getStatusDescription() : "", statusBounds.reduced(8.0f, 0.0f), juce::Justification::centred, true);
        }
    }
}

void BlockCardComponent::resized()
{
    // Header controls layout on the right
    int btnH = 22;
    int right = getWidth() - 6;

    right -= 18;
    removeButton.setBounds(right, 7, 18, btnH);

    right -= 18;
    rightButton.setBounds(right, 7, 18, btnH);

    right -= 18;
    leftButton.setBounds(right, 7, 18, btnH);

    right -= 26;
    diceButton.setBounds(right, 7, 24, btnH);

    right -= 34;
    routingButton.setBounds(right, 7, 32, btnH);

    right -= 30;
    powerButton.setBounds(right, 7, 28, btnH);

    // Parameters body layout (starts after Status & Visualizer Bar)
    int y = 74;
    int availWidth = getWidth() - 16;

    std::vector<ParamControl*> combos;
    std::vector<ParamControl*> rotaries;

    for (auto& pc : controls)
    {
        if (pc.comboBox)
            combos.push_back(&pc);
        else
            rotaries.push_back(&pc);
    }

    // Place combo boxes on full rows
    for (auto* pc : combos)
    {
        pc->label->setBounds(8, y, availWidth, 14);
        y += 15;
        pc->comboBox->setBounds(8, y, availWidth, 22);
        y += 26;
    }

    // Place rotary sliders in a 2-column or 3-column grid
    int numRot = static_cast<int>(rotaries.size());
    int cols = (numRot >= 5) ? 3 : 2;
    int colW = availWidth / cols;
    int dialSize = (cols == 3) ? 58 : 68;

    for (int idx = 0; idx < numRot; ++idx)
    {
        int row = idx / cols;
        int col = idx % cols;

        int rx = 8 + col * colW + (colW - dialSize) / 2;
        int ry = y + row * (dialSize + 18);

        rotaries[idx]->label->setBounds(8 + col * colW, ry, colW, 13);
        rotaries[idx]->label->setJustificationType(juce::Justification::centred);
        rotaries[idx]->slider->setBounds(rx, ry + 13, dialSize, dialSize);
    }
}

void BlockCardComponent::paintOverChildren(juce::Graphics& g)
{
    auto& learnMgr = chainProcessor.getMidiLearnManager();
    if (!learnMgr.isLearning())
        return;

    auto target = learnMgr.getLearningParamID();
    if (!target.startsWith("block:" + juce::String(blockIndex) + ":"))
        return;

    float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.008));
    auto highlightCol = juce::Colour(0xffff9900).withAlpha(0.6f + 0.4f * pulse);

    if (target.endsWith(":power"))
    {
        g.setColour(highlightCol);
        g.drawRoundedRectangle(powerButton.getBounds().toFloat().expanded(2.0f), 4.0f, 2.0f);
        return;
    }

    if (target.endsWith(":routing"))
    {
        g.setColour(highlightCol);
        g.drawRoundedRectangle(routingButton.getBounds().toFloat().expanded(2.0f), 4.0f, 2.0f);
        return;
    }

    for (const auto& pc : controls)
    {
        juce::String id = "block:" + juce::String(blockIndex) + ":param:" + juce::String(pc.paramIndex);
        if (id == target)
        {
            juce::Rectangle<float> r;
            if (pc.slider)
                r = pc.slider->getBounds().toFloat().expanded(2.0f);
            else if (pc.comboBox)
                r = pc.comboBox->getBounds().toFloat().expanded(2.0f);

            g.setColour(highlightCol);
            g.drawRoundedRectangle(r, 4.0f, 2.0f);

            g.setColour(juce::Colour(0xffff9900));
            g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
            g.drawText("LEARN", r.withHeight(12.0f).translated(0.0f, -12.0f), juce::Justification::centred, false);
            break;
        }
    }
}

void BlockCardComponent::showMidiLearnMenu(const juce::String& paramID, juce::Component* targetComp)
{
    auto& learnMgr = chainProcessor.getMidiLearnManager();
    int boundCC = -1;
    int boundCh = 0;
    bool isBound = learnMgr.getMappingForParam(paramID, boundCC, boundCh);
    bool isLearningThis = learnMgr.isLearningParam(paramID);

    juce::PopupMenu menu;
    if (isLearningThis)
    {
        menu.addItem(1, "Cancel MIDI Learn (Waiting for CC...)");
    }
    else if (isBound)
    {
        menu.addItem(2, "Re-learn CC (Currently CC " + juce::String(boundCC) + ")");
        menu.addItem(3, "Unbind (CC " + juce::String(boundCC) + ")");
    }
    else
    {
        menu.addItem(4, "MIDI Learn");
    }

    menu.addSeparator();
    menu.addItem(5, "Clear All MIDI Mappings");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(targetComp),
        [this, paramID, &learnMgr](int result) {
            if (result == 1)
            {
                learnMgr.cancelLearning();
                repaint();
            }
            else if (result == 2 || result == 4)
            {
                learnMgr.startLearning(paramID);
                repaint();
            }
            else if (result == 3)
            {
                learnMgr.removeMappingForParam(paramID);
                repaint();
            }
            else if (result == 5)
            {
                learnMgr.clearAllMappings();
                repaint();
            }
        });
}

} // namespace MidiFlux
