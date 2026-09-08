#include "AudioRoutingDiagramComponent.h"
#include <cmath>

AudioRoutingDiagramComponent::AudioRoutingDiagramComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse)
    : apvts(state), engine(engineToUse)
{
    sourcePins[0]  = { "STEREO IN",   juce::Colour(0xff00d4ff), {}, {} };
    sourcePins[1]  = { "ATTACK",      juce::Colour(0xffff9900), {}, {} };
    sourcePins[2]  = { "BODY",        juce::Colour(0xffa855f7), {}, {} };
    sourcePins[3]  = { "FILTER 1",    juce::Colour(0xff38bdf8), {}, {} };
    sourcePins[4]  = { "FILTER 2",    juce::Colour(0xfff59e0b), {}, {} };
    sourcePins[5]  = { "FILTER 3",    juce::Colour(0xffec4899), {}, {} };
    sourcePins[6]  = { "LEFT (L)",    juce::Colour(0xff60a5fa), {}, {} };
    sourcePins[7]  = { "RIGHT (R)",   juce::Colour(0xff818cf8), {}, {} };
    sourcePins[8]  = { "MID (M)",     juce::Colour(0xff2dd4bf), {}, {} };
    sourcePins[9]  = { "SIDE (S)",    juce::Colour(0xffc084fc), {}, {} };
    sourcePins[10] = { "DYN HIGH",    juce::Colour(0xfffacc15), {}, {} };
    sourcePins[11] = { "DYN LOW",     juce::Colour(0xfffb923c), {}, {} };
    sourcePins[12] = { "HARMONIC",    juce::Colour(0xffe879f9), {}, {} };
    sourcePins[13] = { "NOISE",       juce::Colour(0xff94a3b8), {}, {} };
    sourcePins[14] = { "IN-PHASE",    juce::Colour(0xff4ade80), {}, {} };
    sourcePins[15] = { "SPATIAL",     juce::Colour(0xff34d399), {}, {} };

    destPins[0]   = { "BUS 1 (MAIN 1+2)", juce::Colour(0xff00d4ff), {}, {} };
    destPins[1]   = { "BUS 2 (AUX 3+4)",  juce::Colour(0xfffbbf24), {}, {} };
    destPins[2]   = { "BUS 3 (AUX 5+6)",  juce::Colour(0xff34d399), {}, {} };
    destPins[3]   = { "BUS 4 (AUX 7+8)",  juce::Colour(0xfff472b6), {}, {} };

    for (int s = 0; s < 8; ++s)
    {
        slotNodes[s].slotIndex = s;

        const bool isBank1 = (s < 4);
        const int localIdx = isBank1 ? s : (s - 4);
        const auto inGainId  = isBank1 ? IDs::getTransSlotInGain(localIdx).getParamID()  : IDs::getSustSlotInGain(localIdx).getParamID();
        const auto outGainId = isBank1 ? IDs::getTransSlotOutGain(localIdx).getParamID() : IDs::getSustSlotOutGain(localIdx).getParamID();
        const auto bypassId  = isBank1 ? IDs::getTransSlotBypass(localIdx).getParamID()  : IDs::getSustSlotBypass(localIdx).getParamID();

        // 1. Power Toggle Button (ON / OFF)
        powerButtons[s] = std::make_unique<juce::ToggleButton>("ON");
        auto& pwr = *powerButtons[s];
        pwr.getProperties().set("isBypassToggle", true);
        pwr.getProperties().set("accentColor", (s < 4 ? TransientLookAndFeel::accentTransient : TransientLookAndFeel::accentSustain).toString());
        addAndMakeVisible(pwr);
        if (apvts.getParameter(bypassId) != nullptr)
            powerAttachments[s] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, bypassId, pwr);

        // 2. Mini In Level Knob
        inGainSliders[s] = std::make_unique<juce::Slider>();
        auto& inS = *inGainSliders[s];
        inS.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        inS.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        inS.setTooltip("Slot " + juce::String(s + 1) + " Input Level");
        inS.getProperties().set("accentColor", juce::Colour(0xff4ade80).toString());
        addAndMakeVisible(inS);

        // 3. Mini Out Level Knob
        outGainSliders[s] = std::make_unique<juce::Slider>();
        auto& outS = *outGainSliders[s];
        outS.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        outS.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        outS.setTooltip("Slot " + juce::String(s + 1) + " Output Level");
        outS.getProperties().set("accentColor", juce::Colour(0xff60a5fa).toString());
        addAndMakeVisible(outS);

        if (apvts.getParameter(inGainId) != nullptr)
            inGainAttachments[s]  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, inGainId, inS);
        if (apvts.getParameter(outGainId) != nullptr)
            outGainAttachments[s] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, outGainId, outS);
    }

    startTimerHz(30);
}

AudioRoutingDiagramComponent::~AudioRoutingDiagramComponent()
{
    stopTimer();
}

void AudioRoutingDiagramComponent::timerCallback()
{
    animationPhase += 0.04f;
    if (animationPhase > 1.0f)
        animationPhase -= 1.0f;

    repaint();
}

void AudioRoutingDiagramComponent::initNodePositions()
{
    auto area = getLocalBounds().toFloat().reduced(16.0f);
    area.removeFromTop(44.0f);

    const float totalH = area.getHeight();

    // 1. Left Source Column (Fixed, 16 Pre-Processor Streams)
    const float srcW = 145.0f;
    const float srcItemH = totalH / 16.0f;
    for (int i = 0; i < 16; ++i)
    {
        auto r = juce::Rectangle<float>(area.getX(), area.getY() + i * srcItemH, srcW, srcItemH).reduced(3.0f, 1.5f);
        sourcePins[i].bounds = r;
        sourcePins[i].pinPos = { r.getRight(), r.getCentreY() };
    }

    // 2. Right Output Column (4 Stereo Buses)
    const float destW = 155.0f;
    const float destItemH = totalH / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto r = juce::Rectangle<float>(area.getRight() - destW, area.getY() + i * destItemH, destW, destItemH).reduced(4.0f, 8.0f);
        destPins[i].bounds = r;
        destPins[i].pinPos = { r.getX(), r.getCentreY() };
    }

    // 3. Center Draggable Slot Nodes Grid (2 Columns of 4)
    const float centerLeft = area.getX() + srcW + 40.0f;
    const float centerRight = area.getRight() - destW - 40.0f;
    const float centerW = centerRight - centerLeft;
    const float colW = (centerW - 30.0f) / 2.0f;
    const float slotH = totalH / 4.0f;

    for (int s = 0; s < 8; ++s)
    {
        const int col = (s < 4) ? 0 : 1;
        const int row = (s < 4) ? s : (s - 4);

        const float x = centerLeft + col * (colW + 30.0f);
        const float y = area.getY() + row * slotH;

        auto r = juce::Rectangle<float>(x, y, colW, slotH).reduced(4.0f, 6.0f);
        slotNodes[s].bounds = r;
        slotNodes[s].algoClickBounds = juce::Rectangle<float>(r.getX() + 6.0f, r.getY() + 26.0f, r.getWidth() - 12.0f, 20.0f);
        slotNodes[s].inPinPos = { r.getX(), r.getCentreY() };
        slotNodes[s].outPinPos = { r.getRight(), r.getCentreY() };
    }

    positionsInitialized = true;
    updateMiniKnobPositions();
}

void AudioRoutingDiagramComponent::updateMiniKnobPositions()
{
    for (int s = 0; s < 8; ++s)
    {
        const auto& node = slotNodes[s];
        auto r = node.bounds.toNearestInt();

        if (powerButtons[s])
            powerButtons[s]->setBounds(r.getRight() - 44, r.getY() + 4, 38, 18);

        auto bottomArea = r.removeFromBottom(28).reduced(6, 2);

        if (inGainSliders[s])
            inGainSliders[s]->setBounds(bottomArea.getX() + 18, bottomArea.getY(), 24, 24);

        if (outGainSliders[s])
            outGainSliders[s]->setBounds(bottomArea.getRight() - 42, bottomArea.getY(), 24, 24);
    }
}

void AudioRoutingDiagramComponent::resized()
{
    if (!positionsInitialized || slotNodes[0].bounds.isEmpty())
    {
        initNodePositions();
    }
    else
    {
        auto area = getLocalBounds().toFloat().reduced(16.0f);
        area.removeFromTop(44.0f);
        const float totalH = area.getHeight();

        const float srcW = 145.0f;
        const float srcItemH = totalH / 16.0f;
        for (int i = 0; i < 16; ++i)
        {
            auto r = juce::Rectangle<float>(area.getX(), area.getY() + i * srcItemH, srcW, srcItemH).reduced(3.0f, 1.5f);
            sourcePins[i].bounds = r;
            sourcePins[i].pinPos = { r.getRight(), r.getCentreY() };
        }

        const float destW = 155.0f;
        const float destItemH = totalH / 4.0f;
        for (int i = 0; i < 4; ++i)
        {
            auto r = juce::Rectangle<float>(area.getRight() - destW, area.getY() + i * destItemH, destW, destItemH).reduced(4.0f, 8.0f);
            destPins[i].bounds = r;
            destPins[i].pinPos = { r.getX(), r.getCentreY() };
        }

        for (int s = 0; s < 8; ++s)
        {
            auto& r = slotNodes[s].bounds;
            slotNodes[s].algoClickBounds = juce::Rectangle<float>(r.getX() + 6.0f, r.getY() + 26.0f, r.getWidth() - 12.0f, 20.0f);
            slotNodes[s].inPinPos = { r.getX(), r.getCentreY() };
            slotNodes[s].outPinPos = { r.getRight(), r.getCentreY() };
        }

        updateMiniKnobPositions();
    }
}

PinRef AudioRoutingDiagramComponent::findPinAt(juce::Point<float> p, float hitRadius) const
{
    for (int i = 0; i < 16; ++i)
    {
        if (p.getDistanceFrom(sourcePins[i].pinPos) <= hitRadius)
            return { PinKind::SourceOut, i };
    }

    for (int i = 0; i < 4; ++i)
    {
        if (p.getDistanceFrom(destPins[i].pinPos) <= hitRadius)
            return { PinKind::DestIn, i };
    }

    for (int s = 0; s < 8; ++s)
    {
        if (p.getDistanceFrom(slotNodes[s].inPinPos) <= hitRadius)
            return { PinKind::SlotIn, s };

        if (p.getDistanceFrom(slotNodes[s].outPinPos) <= hitRadius)
            return { PinKind::SlotOut, s };
    }

    return { PinKind::SourceOut, -1 };
}

int AudioRoutingDiagramComponent::findSlotNodeAt(juce::Point<float> p) const
{
    for (int s = 0; s < 8; ++s)
    {
        if (slotNodes[s].bounds.contains(p))
            return s;
    }
    return -1;
}

int AudioRoutingDiagramComponent::findAlgoClickAt(juce::Point<float> p) const
{
    for (int s = 0; s < 8; ++s)
    {
        if (slotNodes[s].algoClickBounds.contains(p))
            return s;
    }
    return -1;
}

void AudioRoutingDiagramComponent::showAlgorithmMenu(int slotIndex, juce::Point<int> screenPos)
{
    if (slotIndex < 0 || slotIndex >= 8) return;

    juce::PopupMenu menu;
    const bool isBank1 = (slotIndex < 4);
    const int localIdx = isBank1 ? slotIndex : (slotIndex - 4);
    const auto typeId = isBank1 ? IDs::getTransSlotType(localIdx).getParamID() : IDs::getSustSlotType(localIdx).getParamID();

    int currentType = 0;
    if (auto* p = apvts.getRawParameterValue(typeId))
        currentType = static_cast<int>(p->load());

    // Grouping by categories
    menu.addItem(1, "Bypass (Clean Pass-Through)", true, currentType == 0);
    menu.addSeparator();

    // Dynamics (IDs 2..5)
    juce::PopupMenu dynMenu;
    dynMenu.addItem(2, "VCA Compressor", true, currentType == 1);
    dynMenu.addItem(3, "Opto Compressor", true, currentType == 2);
    dynMenu.addItem(4, "FET Fast Compressor", true, currentType == 3);
    dynMenu.addItem(5, "Noise Gate / Expander", true, currentType == 4);
    menu.addSubMenu("Dynamics & Gain", dynMenu);

    // EQ & Filters (IDs 6..10)
    juce::PopupMenu eqMenu;
    eqMenu.addItem(6, "4-Band Parametric EQ", true, currentType == 5);
    eqMenu.addItem(7, "8-Band Graphic EQ", true, currentType == 6);
    eqMenu.addItem(8, "Tilt Spectral Shifter", true, currentType == 7);
    eqMenu.addItem(9, "Dual HP/LP State Variable", true, currentType == 8);
    eqMenu.addItem(10, "Resonant Bandpass", true, currentType == 9);
    menu.addSubMenu("EQ & Filters", eqMenu);

    // Distortion & Saturation (IDs 11..18)
    juce::PopupMenu distMenu;
    distMenu.addItem(11, "Hard Diode Clipper", true, currentType == 10);
    distMenu.addItem(12, "Warm Tube Saturation", true, currentType == 11);
    distMenu.addItem(13, "Wavefolder Multiplier", true, currentType == 12);
    distMenu.addItem(14, "Germanium Fuzz", true, currentType == 13);
    distMenu.addItem(15, "Octave Fuzz", true, currentType == 14);
    distMenu.addItem(16, "Analog Overdrive", true, currentType == 15);
    distMenu.addItem(17, "Tape Saturation & Age", true, currentType == 16);
    distMenu.addItem(18, "Bitcrusher & Reducer", true, currentType == 17);
    menu.addSubMenu("Distortion & Fuzz", distMenu);

    // Glitch & Pitch (IDs 19..24)
    juce::PopupMenu glitchMenu;
    glitchMenu.addItem(19, "Rhythmic Stutter", true, currentType == 18);
    glitchMenu.addItem(20, "Vinyl Tape Stop", true, currentType == 19);
    glitchMenu.addItem(21, "Buffer Reverse Echo", true, currentType == 20);
    glitchMenu.addItem(22, "Granular Cloud Scatter", true, currentType == 21);
    glitchMenu.addItem(23, "Pitch Shifter (-12..+12)", true, currentType == 22);
    glitchMenu.addItem(24, "Bode Frequency Shifter", true, currentType == 23);
    menu.addSubMenu("Glitch & Pitch", glitchMenu);

    // Modulation (IDs 25..28)
    juce::PopupMenu modMenu;
    modMenu.addItem(25, "Stereo Dimension Chorus", true, currentType == 24);
    modMenu.addItem(26, "Through-Zero Flanger", true, currentType == 25);
    modMenu.addItem(27, "12-Stage Optical Phaser", true, currentType == 26);
    modMenu.addItem(28, "Sine / Harmonic Tremolo", true, currentType == 27);
    menu.addSubMenu("Modulation", modMenu);

    // Delays (IDs 29..31)
    juce::PopupMenu delayMenu;
    delayMenu.addItem(29, "Stereo Dual Digital Delay", true, currentType == 28);
    delayMenu.addItem(30, "Cross-Feedback Ping-Pong", true, currentType == 29);
    delayMenu.addItem(31, "Analog Tape Echo + Flutter", true, currentType == 30);
    menu.addSubMenu("Delays", delayMenu);

    // Reverbs (IDs 32..36)
    juce::PopupMenu revMenu;
    revMenu.addItem(32, "Studio Acoustic Room", true, currentType == 31);
    revMenu.addItem(33, "Cathedral Concert Hall", true, currentType == 32);
    revMenu.addItem(34, "Vintage EMT 140 Plate", true, currentType == 33);
    revMenu.addItem(35, "Dual Spring Reverb Tank", true, currentType == 34);
    revMenu.addItem(36, "Pitch-Shifted Shimmer Reverb", true, currentType == 35);
    menu.addSubMenu("Reverbs", revMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)),
        [this, typeId](int result)
        {
            if (result > 0)
            {
                const int chosenTypeIdx = result - 1;
                if (auto* param = apvts.getParameter(typeId))
                {
                    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param))
                        *choice = chosenTypeIdx;
                    else
                        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(chosenTypeIdx)));
                }
                repaint();
            }
        });
}

void AudioRoutingDiagramComponent::connectPins(PinRef fromPin, PinRef toPin)
{
    PinRef outPin = (fromPin.kind == PinKind::SourceOut || fromPin.kind == PinKind::SlotOut) ? fromPin : toPin;
    PinRef inPin  = (fromPin.kind == PinKind::SlotIn || fromPin.kind == PinKind::DestIn) ? fromPin : toPin;

    if (outPin.index < 0 || inPin.index < 0)
        return;

    // Case 1: Connect Source Pin (0..5) -> Slot In Pin (0..7)
    if (outPin.kind == PinKind::SourceOut && inPin.kind == PinKind::SlotIn)
    {
        const int targetSlot = inPin.index;
        const int srcChoice = 1 + outPin.index; // Choice 1..6 (0 is Unassigned)
        const bool isBank1 = (targetSlot < 4);
        const int localIdx = isBank1 ? targetSlot : (targetSlot - 4);
        const auto srcId = isBank1 ? IDs::getTransSlotSrc(localIdx).getParamID() : IDs::getSustSlotSrc(localIdx).getParamID();

        if (auto* param = apvts.getParameter(srcId))
        {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param))
                *choice = srcChoice;
            else
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(srcChoice)));
        }
    }
    // Case 2: Connect Slot Out Pin (upSlot) -> Slot In Pin (targetSlot) [BIDIRECTIONAL PATCHING!]
    else if (outPin.kind == PinKind::SlotOut && inPin.kind == PinKind::SlotIn)
    {
        const int upSlot = outPin.index;
        const int targetSlot = inPin.index;

        if (upSlot != targetSlot && !engine.wouldCreateCycle(upSlot, targetSlot))
        {
            // 1. Set targetSlot's input source to upSlot (Choices 17..24)
            const bool isBank1Target = (targetSlot < 4);
            const int localIdxTarget = isBank1Target ? targetSlot : (targetSlot - 4);
            const auto srcId = isBank1Target ? IDs::getTransSlotSrc(localIdxTarget).getParamID() : IDs::getSustSlotSrc(localIdxTarget).getParamID();

            if (auto* param = apvts.getParameter(srcId))
            {
                if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param))
                    *choice = 17 + upSlot;
                else
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(17 + upSlot)));
            }

            // 2. Set upSlot's destination to targetSlot
            const bool isBank1Up = (upSlot < 4);
            const int localIdxUp = isBank1Up ? upSlot : (upSlot - 4);
            const auto destId = isBank1Up ? IDs::getTransSlotDest(localIdxUp).getParamID() : IDs::getSustSlotDest(localIdxUp).getParamID();

            if (auto* param = apvts.getParameter(destId))
            {
                if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param))
                    *choice = 7 + targetSlot;
                else
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(7 + targetSlot)));
            }
        }
    }
    // Case 3: Connect Slot Out Pin (s) -> 4 Output Buses (0..3)
    else if (outPin.kind == PinKind::SlotOut && inPin.kind == PinKind::DestIn)
    {
        const int s = outPin.index;
        const int destChoice = 1 + inPin.index; // 1: Bus 1, 2: Bus 2, 3: Bus 3, 4: Bus 4
        const bool isBank1 = (s < 4);
        const int localIdx = isBank1 ? s : (s - 4);
        const auto destId = isBank1 ? IDs::getTransSlotDest(localIdx).getParamID() : IDs::getSustSlotDest(localIdx).getParamID();

        if (auto* param = apvts.getParameter(destId))
        {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param))
                *choice = destChoice;
            else
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(destChoice)));
        }
    }
}

void AudioRoutingDiagramComponent::disconnectPin(PinRef pin)
{
    if (pin.index < 0) return;

    if (pin.kind == PinKind::SlotIn)
    {
        const int targetSlot = pin.index;
        const bool isBank1 = (targetSlot < 4);
        const int localIdx = isBank1 ? targetSlot : (targetSlot - 4);
        const auto srcId = isBank1 ? IDs::getTransSlotSrc(localIdx).getParamID() : IDs::getSustSlotSrc(localIdx).getParamID();

        if (auto* param = apvts.getParameter(srcId))
        {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param))
                *choice = 0; // Reset to Unassigned
            else
                param->setValueNotifyingHost(0.0f);
        }
    }
    else if (pin.kind == PinKind::SlotOut)
    {
        const int s = pin.index;
        const bool isBank1 = (s < 4);
        const int localIdx = isBank1 ? s : (s - 4);
        const auto destId = isBank1 ? IDs::getTransSlotDest(localIdx).getParamID() : IDs::getSustSlotDest(localIdx).getParamID();

        if (auto* param = apvts.getParameter(destId))
        {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param))
                *choice = 0; // Reset to Unassigned
            else
                param->setValueNotifyingHost(0.0f);
        }
    }
}

void AudioRoutingDiagramComponent::mouseDown(const juce::MouseEvent& e)
{
    const auto mousePos = e.position;

    // Right-click to disconnect
    if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
    {
        auto clickedPin = findPinAt(mousePos, 18.0f);
        if (clickedPin.index >= 0)
        {
            disconnectPin(clickedPin);
            repaint();
            return;
        }
    }

    // Left-click on algorithm name banner: open popup menu
    int clickedAlgoSlot = findAlgoClickAt(mousePos);
    if (clickedAlgoSlot >= 0 && !e.mods.isRightButtonDown())
    {
        showAlgorithmMenu(clickedAlgoSlot, e.getScreenPosition());
        return;
    }

    // Left-click on pin: start dragging cable
    auto hitPin = findPinAt(mousePos, 16.0f);
    if (hitPin.index >= 0)
    {
        isDraggingCable = true;
        dragStartPin = hitPin;
        dragCurrentPoint = mousePos;

        if (hitPin.kind == PinKind::SourceOut)
        {
            dragStartPoint = sourcePins[hitPin.index].pinPos;
            dragCableColor = sourcePins[hitPin.index].colour;
        }
        else if (hitPin.kind == PinKind::SlotOut)
        {
            dragStartPoint = slotNodes[hitPin.index].outPinPos;
            dragCableColor = (hitPin.index < 4) ? TransientLookAndFeel::accentTransient : TransientLookAndFeel::accentSustain;
        }
        else if (hitPin.kind == PinKind::SlotIn)
        {
            dragStartPoint = slotNodes[hitPin.index].inPinPos;
            dragCableColor = juce::Colour(0xff4ade80);
        }
        else if (hitPin.kind == PinKind::DestIn)
        {
            dragStartPoint = destPins[hitPin.index].pinPos;
            dragCableColor = destPins[hitPin.index].colour;
        }

        repaint();
        return;
    }

    // Left-click on slot node body: start moving node
    int hitSlot = findSlotNodeAt(mousePos);
    if (hitSlot >= 0)
    {
        isDraggingNode = true;
        draggedSlotIndex = hitSlot;
        nodeDragOffset = mousePos - slotNodes[hitSlot].bounds.getTopLeft();
    }
}

void AudioRoutingDiagramComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingCable)
    {
        dragCurrentPoint = e.position;
        repaint();
    }
    else if (isDraggingNode && draggedSlotIndex >= 0)
    {
        auto area = getLocalBounds().toFloat().reduced(16.0f);
        area.removeFromTop(44.0f);

        auto& node = slotNodes[draggedSlotIndex];
        const float w = node.bounds.getWidth();
        const float h = node.bounds.getHeight();

        float newX = std::clamp(e.position.x - nodeDragOffset.x, area.getX() + 180.0f, area.getRight() - 190.0f - w);
        float newY = std::clamp(e.position.y - nodeDragOffset.y, area.getY(), area.getBottom() - h);

        node.bounds.setPosition(newX, newY);
        node.algoClickBounds = juce::Rectangle<float>(newX + 6.0f, newY + 26.0f, w - 12.0f, 20.0f);
        node.inPinPos  = { newX, newY + h * 0.5f };
        node.outPinPos = { newX + w, newY + h * 0.5f };

        updateMiniKnobPositions();
        repaint();
    }
}

void AudioRoutingDiagramComponent::mouseUp(const juce::MouseEvent& e)
{
    if (isDraggingCable)
    {
        isDraggingCable = false;
        auto endPin = findPinAt(e.position, 18.0f);
        if (endPin.index >= 0 && endPin.kind != dragStartPin.kind)
        {
            connectPins(dragStartPin, endPin);
        }
        repaint();
    }

    isDraggingNode = false;
    draggedSlotIndex = -1;
}

void AudioRoutingDiagramComponent::drawCurvedCable(juce::Graphics& g, juce::Point<float> p1, juce::Point<float> p2, juce::Colour col, bool active)
{
    juce::Path cable;
    cable.startNewSubPath(p1);

    const float dx = std::abs(p2.x - p1.x);
    const float ctrlOffset = std::max(40.0f, dx * 0.5f);

    const juce::Point<float> c1(p1.x + ctrlOffset, p1.y);
    const juce::Point<float> c2(p2.x - ctrlOffset, p2.y);

    cable.cubicTo(c1, c2, p2);

    if (active)
    {
        g.setColour(col.withAlpha(0.25f));
        g.strokePath(cable, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(col.withAlpha(0.9f));
        g.strokePath(cable, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pulsing audio flow dashes
        const float dashLengths[2] = { 6.0f, 10.0f };
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawDashedLine(juce::Line<float>(p1, p2), dashLengths, 2, 1.5f);
    }
    else
    {
        g.setColour(juce::Colour(0xff2a3040));
        g.strokePath(cable, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void AudioRoutingDiagramComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090b10));

    if (!positionsInitialized)
        initNodePositions();

    // Top Header Banner
    auto topBanner = juce::Rectangle<float>(16.0f, 12.0f, static_cast<float>(getWidth() - 32), 34.0f);
    g.setColour(juce::Colour(0xff12151e));
    g.fillRoundedRectangle(topBanner, 6.0f);
    g.setColour(TransientLookAndFeel::borderCol);
    g.drawRoundedRectangle(topBanner, 6.0f, 1.0f);

    g.setColour(TransientLookAndFeel::textBright);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText("VISUAL AUDIO ROUTING MATRIX (DRAGGABLE NODES & MULTI-BUS PATCHING)", 28, 12, 600, 34, juce::Justification::centredLeft);

    g.setColour(TransientLookAndFeel::textMuted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("• Drag pins to patch  • Click Effect Name to switch algorithm  • Right-click pin to unpatch", getWidth() - 580, 12, 550, 34, juce::Justification::centredRight);

    // 1. Draw All Connected Cables
    for (int s = 0; s < 8; ++s)
    {
        const bool isBank1 = (s < 4);
        const int localIdx = isBank1 ? s : (s - 4);
        const auto srcId  = isBank1 ? IDs::getTransSlotSrc(localIdx).getParamID()  : IDs::getSustSlotSrc(localIdx).getParamID();
        const auto destId = isBank1 ? IDs::getTransSlotDest(localIdx).getParamID() : IDs::getSustSlotDest(localIdx).getParamID();
        const auto bypId  = isBank1 ? IDs::getTransSlotBypass(localIdx).getParamID() : IDs::getSustSlotBypass(localIdx).getParamID();

        int srcVal = 0;
        int destVal = 0;
        bool isBypassed = false;

        if (auto* p = apvts.getRawParameterValue(srcId))  srcVal = static_cast<int>(p->load());
        if (auto* p = apvts.getRawParameterValue(destId)) destVal = static_cast<int>(p->load());
        if (auto* p = apvts.getRawParameterValue(bypId))  isBypassed = p->load() > 0.5f;

        const bool isSlotActive = !isBypassed;

        // Input Wire
        if (srcVal >= 1 && srcVal <= 16)
        {
            auto p1 = sourcePins[srcVal - 1].pinPos;
            auto p2 = slotNodes[s].inPinPos;
            drawCurvedCable(g, p1, p2, sourcePins[srcVal - 1].colour, isSlotActive);
        }
        else if (srcVal >= 17 && srcVal <= 24)
        {
            int upSlot = srcVal - 17;
            if (upSlot >= 0 && upSlot < 8 && upSlot != s)
            {
                auto p1 = slotNodes[upSlot].outPinPos;
                auto p2 = slotNodes[s].inPinPos;
                drawCurvedCable(g, p1, p2, juce::Colour(0xffeab308), isSlotActive);
            }
        }

        // Output Wire
        if (isSlotActive && destVal > 0)
        {
            auto p1 = slotNodes[s].outPinPos;
            switch (destVal)
            {
                case 1: // Bus 1
                    drawCurvedCable(g, p1, destPins[0].pinPos, destPins[0].colour, true);
                    break;
                case 2: // Bus 2
                    drawCurvedCable(g, p1, destPins[1].pinPos, destPins[1].colour, true);
                    break;
                case 3: // Bus 3
                    drawCurvedCable(g, p1, destPins[2].pinPos, destPins[2].colour, true);
                    break;
                case 4: // Bus 4
                    drawCurvedCable(g, p1, destPins[3].pinPos, destPins[3].colour, true);
                    break;
                case 5: // All Buses
                    drawCurvedCable(g, p1, destPins[0].pinPos, destPins[0].colour, true);
                    drawCurvedCable(g, p1, destPins[1].pinPos, destPins[1].colour, true);
                    drawCurvedCable(g, p1, destPins[2].pinPos, destPins[2].colour, true);
                    drawCurvedCable(g, p1, destPins[3].pinPos, destPins[3].colour, true);
                    break;
                case 6: // Next Slot
                    if (s < 7)
                        drawCurvedCable(g, p1, slotNodes[s + 1].inPinPos, juce::Colour(0xff38bdf8), true);
                    else
                        drawCurvedCable(g, p1, destPins[0].pinPos, destPins[0].colour, true);
                    break;
                default:
                    if (destVal >= 7 && destVal <= 14)
                    {
                        int target = destVal - 7;
                        if (target >= 0 && target < 8 && target != s)
                            drawCurvedCable(g, p1, slotNodes[target].inPinPos, juce::Colour(0xffa855f7), true);
                    }
                    break;
            }
        }
    }

    // 2. Draw Live Dragged Cable
    if (isDraggingCable)
    {
        drawCurvedCable(g, dragStartPoint, dragCurrentPoint, dragCableColor, true);
    }

    // 3. Draw Source Node Boxes (Left Column, 16 Sources)
    for (int i = 0; i < 16; ++i)
    {
        const auto& sp = sourcePins[i];
        g.setColour(juce::Colour(0xff141822));
        g.fillRoundedRectangle(sp.bounds, 4.0f);
        g.setColour(sp.colour.withAlpha(0.8f));
        g.drawRoundedRectangle(sp.bounds, 4.0f, 1.2f);

        g.setColour(juce::Colour(0xff090b10));
        g.fillRoundedRectangle(sp.bounds.getX() + 3.0f, sp.bounds.getY() + 3.0f, sp.bounds.getWidth() - 6.0f, sp.bounds.getHeight() - 6.0f, 3.0f);

        g.setColour(sp.colour);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(sp.name, sp.bounds.reduced(8.0f, 0.0f), juce::Justification::centredLeft);

        // Jack Pin Circle
        g.setColour(sp.colour);
        g.fillEllipse(sp.pinPos.x - 5.0f, sp.pinPos.y - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(sp.pinPos.x - 2.0f, sp.pinPos.y - 2.0f, 4.0f, 4.0f);
    }

    // 4. Draw Center Slot Nodes
    for (int s = 0; s < 8; ++s)
    {
        const auto& node = slotNodes[s];
        const bool isBank1 = (s < 4);
        const int localIdx = isBank1 ? s : (s - 4);
        const auto bypId  = isBank1 ? IDs::getTransSlotBypass(localIdx).getParamID() : IDs::getSustSlotBypass(localIdx).getParamID();
        const auto typeId = isBank1 ? IDs::getTransSlotType(localIdx).getParamID()   : IDs::getSustSlotType(localIdx).getParamID();

        bool isBypassed = false;
        int effTypeIdx = 0;
        if (auto* p = apvts.getRawParameterValue(bypId))  isBypassed = p->load() > 0.5f;
        if (auto* p = apvts.getRawParameterValue(typeId)) effTypeIdx = static_cast<int>(p->load());

        const auto effectType = static_cast<EffectType>(effTypeIdx);
        const juce::String algoName = (s < 4) ? engine.getTransientChain().getSlot(s).getName() : engine.getSustainChain().getSlot(s - 4).getName();

        const juce::Colour accent = isBank1 ? TransientLookAndFeel::accentTransient : TransientLookAndFeel::accentSustain;

        // Card Body
        g.setColour(isBypassed ? juce::Colour(0xff12141c) : juce::Colour(0xff161b26));
        g.fillRoundedRectangle(node.bounds, 6.0f);

        g.setColour(isBypassed ? juce::Colour(0xff262b3a) : accent.withAlpha(0.85f));
        g.drawRoundedRectangle(node.bounds, 6.0f, isBypassed ? 1.0f : 1.5f);

        // Header Title Bar
        auto bCopy = node.bounds;
        auto header = bCopy.removeFromTop(24.0f).reduced(8.0f, 2.0f);
        g.setColour(isBypassed ? TransientLookAndFeel::textMuted : accent);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("SLOT " + juce::String(s + 1), header.removeFromLeft(70.0f).toNearestInt(), juce::Justification::centredLeft);

        // Algorithm Name Button (Clickable Banner)
        g.setColour(juce::Colour(0xff0e1118));
        g.fillRoundedRectangle(node.algoClickBounds, 4.0f);
        g.setColour(accent.withAlpha(0.5f));
        g.drawRoundedRectangle(node.algoClickBounds, 4.0f, 1.0f);

        g.setColour(isBypassed ? TransientLookAndFeel::textMuted : TransientLookAndFeel::textBright);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText(algoName + " ▾", node.algoClickBounds, juce::Justification::centred);

        // In & Out Mini Labels above/below dials
        const auto b = node.bounds;
        g.setColour(TransientLookAndFeel::textMuted);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawText("IN", b.getX() + 6.0f, b.getBottom() - 24.0f, 16.0f, 18.0f, juce::Justification::centred);
        g.drawText("OUT", b.getRight() - 62.0f, b.getBottom() - 24.0f, 22.0f, 18.0f, juce::Justification::centred);

        // Input Jack Pin (Left)
        g.setColour(accent);
        g.fillEllipse(node.inPinPos.x - 6.0f, node.inPinPos.y - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(node.inPinPos.x - 2.5f, node.inPinPos.y - 2.5f, 5.0f, 5.0f);

        // Output Jack Pin (Right)
        g.setColour(accent);
        g.fillEllipse(node.outPinPos.x - 6.0f, node.outPinPos.y - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(node.outPinPos.x - 2.5f, node.outPinPos.y - 2.5f, 5.0f, 5.0f);
    }

    // 5. Draw 4 Output Bus Boxes (Right Column)
    for (int i = 0; i < 4; ++i)
    {
        const auto& dp = destPins[i];
        g.setColour(juce::Colour(0xff141822));
        g.fillRoundedRectangle(dp.bounds, 6.0f);
        g.setColour(dp.colour.withAlpha(0.85f));
        g.drawRoundedRectangle(dp.bounds, 6.0f, 1.4f);

        g.setColour(juce::Colour(0xff090b10));
        g.fillRoundedRectangle(dp.bounds.getX() + 6.0f, dp.bounds.getY() + 6.0f, dp.bounds.getWidth() - 12.0f, dp.bounds.getHeight() - 12.0f, 4.0f);

        g.setColour(dp.colour);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(dp.name, dp.bounds.reduced(10.0f, 0.0f), juce::Justification::centredRight);

        // Jack Pin Circle
        g.setColour(dp.colour);
        g.fillEllipse(dp.pinPos.x - 6.0f, dp.pinPos.y - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(dp.pinPos.x - 2.5f, dp.pinPos.y - 2.5f, 5.0f, 5.0f);
    }
}
