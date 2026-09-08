#include "EffectSlotComponent.h"
#include "TransientLookAndFeel.h"

juce::Colour EffectSlotComponent::getCategoryColour(EffectType type)
{
    const int t = static_cast<int>(type);
    if (type == EffectType::Bypass)
        return juce::Colour(0xff94a3b8); // Slate Gray

    // 1. Dynamics (1..4)
    if (t >= static_cast<int>(EffectType::CompressorVCA) && t <= static_cast<int>(EffectType::Gate))
        return juce::Colour(0xff10b981); // Emerald Jade

    // 2. EQ & Filter (5..9)
    if (t >= static_cast<int>(EffectType::EqualizerParametric) && t <= static_cast<int>(EffectType::FilterBandpass))
        return juce::Colour(0xff06b6d4); // Cyan

    // 3. Distortion & Saturation (10..16)
    if (t >= static_cast<int>(EffectType::DistortionHardClip) && t <= static_cast<int>(EffectType::TapeSat))
        return juce::Colour(0xfff97316); // Warm Amber / Orange

    // 4. Lo-Fi & Glitch (17..21)
    if (t >= static_cast<int>(EffectType::Bitcrush) && t <= static_cast<int>(EffectType::GlitchGranular))
        return juce::Colour(0xfff43f5e); // Pink / Rose

    // 5. Pitch & Modulation (22..27)
    if (t >= static_cast<int>(EffectType::PitchShifter) && t <= static_cast<int>(EffectType::ModulationTremolo))
        return juce::Colour(0xff8b5cf6); // Electric Violet

    // 6. Delay (28..30)
    if (t >= static_cast<int>(EffectType::DelayStereo) && t <= static_cast<int>(EffectType::DelayTapeEcho))
        return juce::Colour(0xffeab308); // Gold / Yellow

    // 7. Reverb (31..35)
    if (t >= static_cast<int>(EffectType::ReverbRoom) && t <= static_cast<int>(EffectType::ReverbShimmer))
        return juce::Colour(0xff3b82f6); // Ocean Blue

    return juce::Colour(0xff94a3b8);
}

EffectSlotComponent::EffectSlotComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse, int slotIndex)
    : apvts(state), engine(engineToUse), globalSlotIdx(slotIndex)
{
    const bool isBank1 = (globalSlotIdx < 4);
    const int localSlotIdx = isBank1 ? globalSlotIdx : (globalSlotIdx - 4);
    const juce::Colour accent = getCategoryColour(EffectType::Bypass);

    srcParamID     = isBank1 ? IDs::getTransSlotSrc(localSlotIdx).getParamID()     : IDs::getSustSlotSrc(localSlotIdx).getParamID();
    typeParamID    = isBank1 ? IDs::getTransSlotType(localSlotIdx).getParamID()    : IDs::getSustSlotType(localSlotIdx).getParamID();
    bypassParamID  = isBank1 ? IDs::getTransSlotBypass(localSlotIdx).getParamID()  : IDs::getSustSlotBypass(localSlotIdx).getParamID();
    inGainParamID  = isBank1 ? IDs::getTransSlotInGain(localSlotIdx).getParamID()  : IDs::getSustSlotInGain(localSlotIdx).getParamID();
    outGainParamID = isBank1 ? IDs::getTransSlotOutGain(localSlotIdx).getParamID() : IDs::getSustSlotOutGain(localSlotIdx).getParamID();
    p1ParamID      = isBank1 ? IDs::getTransSlotP1(localSlotIdx).getParamID()      : IDs::getSustSlotP1(localSlotIdx).getParamID();
    p2ParamID      = isBank1 ? IDs::getTransSlotP2(localSlotIdx).getParamID()      : IDs::getSustSlotP2(localSlotIdx).getParamID();
    p3ParamID      = isBank1 ? IDs::getTransSlotP3(localSlotIdx).getParamID()      : IDs::getSustSlotP3(localSlotIdx).getParamID();
    p4ParamID      = isBank1 ? IDs::getTransSlotP4(localSlotIdx).getParamID()      : IDs::getSustSlotP4(localSlotIdx).getParamID();
    p5ParamID      = isBank1 ? IDs::getTransSlotP5(localSlotIdx).getParamID()      : IDs::getSustSlotP5(localSlotIdx).getParamID();
    mixParamID     = isBank1 ? IDs::getTransSlotMix(localSlotIdx).getParamID()     : IDs::getSustSlotMix(localSlotIdx).getParamID();
    destParamID    = isBank1 ? IDs::getTransSlotDest(localSlotIdx).getParamID()    : IDs::getSustSlotDest(localSlotIdx).getParamID();

    for (int t = 0; t < 36; ++t)
    {
        const auto effType = static_cast<EffectType>(t);
        for (int p = 0; p < 5; ++p)
            effectParamCache[t][p] = EffectSlot::getParamInfoForType(effType, p).defaultVal;
        effectParamCache[t][5] = 1.0f;
    }

    slotTitle.setText("SLOT " + juce::String(globalSlotIdx + 1), juce::dontSendNotification);
    slotTitle.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    slotTitle.setColour(juce::Label::textColourId, accent);
    slotTitle.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(slotTitle);

    bypassButton.setClickingTogglesState(true);
    bypassButton.setButtonText("ON");
    bypassButton.getProperties().set("accentColor", accent.toString());
    bypassButton.getProperties().set("isBypassToggle", true);
    bypassButton.onClick = [this]() {
        updateParamLabels();
        repaint();
    };
    addAndMakeVisible(bypassButton);

    inputSourceCombo.addItem("IN: Disconnected", 1);
    inputSourceCombo.addItem("IN: Stereo In", 2);
    inputSourceCombo.addItem("IN: Attack", 3);
    inputSourceCombo.addItem("IN: Body", 4);
    inputSourceCombo.addItem("IN: Filter 1", 5);
    inputSourceCombo.addItem("IN: Filter 2", 6);
    inputSourceCombo.addItem("IN: Filter 3", 7);
    inputSourceCombo.addItem("IN: Left (L)", 8);
    inputSourceCombo.addItem("IN: Right (R)", 9);
    inputSourceCombo.addItem("IN: Mid (M)", 10);
    inputSourceCombo.addItem("IN: Side (S)", 11);
    inputSourceCombo.addItem("IN: Dynamic High", 12);
    inputSourceCombo.addItem("IN: Dynamic Low", 13);
    inputSourceCombo.addItem("IN: Harmonic", 14);
    inputSourceCombo.addItem("IN: Noise", 15);
    inputSourceCombo.addItem("IN: In-Phase", 16);
    inputSourceCombo.addItem("IN: Spatial", 17);
    for (int s = 1; s <= 8; ++s)
        inputSourceCombo.addItem("IN: Slot " + juce::String(s), 17 + s);
    inputSourceCombo.setTooltip("Select audio input source feeding Slot " + juce::String(globalSlotIdx + 1));
    addAndMakeVisible(inputSourceCombo);

    // Algorithm Selection ComboBox with Submenus
    juce::PopupMenu menu;
    menu.addItem(static_cast<int>(EffectType::Bypass) + 1, "Bypass (Direct Dry Through)");

    // 1. Dynamics
    juce::PopupMenu dynamicsMenu;
    dynamicsMenu.addItem(static_cast<int>(EffectType::CompressorVCA) + 1, "VCA Compressor");
    dynamicsMenu.addItem(static_cast<int>(EffectType::CompressorOpto) + 1, "Opto Compressor (LA-2A)");
    dynamicsMenu.addItem(static_cast<int>(EffectType::CompressorFET) + 1, "FET Compressor (1176)");
    dynamicsMenu.addItem(static_cast<int>(EffectType::Gate) + 1, "Noise Gate / Expander");
    menu.addSubMenu("Dynamics", dynamicsMenu);

    // 2. EQ & Filter
    juce::PopupMenu eqMenu;
    eqMenu.addItem(static_cast<int>(EffectType::EqualizerParametric) + 1, "Parametric 3-Band EQ");
    eqMenu.addItem(static_cast<int>(EffectType::EqualizerGraphic) + 1, "Graphic 5-Band EQ");
    eqMenu.addItem(static_cast<int>(EffectType::EqualizerTilt) + 1, "Tilt EQ");
    eqMenu.addItem(static_cast<int>(EffectType::FilterDualHPLP) + 1, "Dual HP / LP Filter");
    eqMenu.addItem(static_cast<int>(EffectType::FilterBandpass) + 1, "Bandpass & Notch Filter");
    menu.addSubMenu("EQ & Filter", eqMenu);

    // 3. Distortion & Saturation
    juce::PopupMenu distMenu;
    distMenu.addItem(static_cast<int>(EffectType::DistortionHardClip) + 1, "Hard Clip Distortion");
    distMenu.addItem(static_cast<int>(EffectType::DistortionTube) + 1, "Tube Saturation");
    distMenu.addItem(static_cast<int>(EffectType::DistortionWavefolder) + 1, "Wavefolder Distortion");
    distMenu.addItem(static_cast<int>(EffectType::FuzzGermanium) + 1, "Germanium Fuzz");
    distMenu.addItem(static_cast<int>(EffectType::FuzzOctave) + 1, "Octave Fuzz");
    distMenu.addItem(static_cast<int>(EffectType::Overdrive) + 1, "Overdrive");
    distMenu.addItem(static_cast<int>(EffectType::TapeSat) + 1, "Tape Saturation");
    menu.addSubMenu("Distortion & Sat", distMenu);

    // 4. Lo-Fi & Glitch
    juce::PopupMenu lofiMenu;
    lofiMenu.addItem(static_cast<int>(EffectType::Bitcrush) + 1, "Bitcrusher & Decimator");
    lofiMenu.addItem(static_cast<int>(EffectType::GlitchStutter) + 1, "Stutter Repeater");
    lofiMenu.addItem(static_cast<int>(EffectType::GlitchTapeStop) + 1, "Tape Stop Glitch");
    lofiMenu.addItem(static_cast<int>(EffectType::GlitchReverse) + 1, "Reverse Slice");
    lofiMenu.addItem(static_cast<int>(EffectType::GlitchGranular) + 1, "Granular Jitter");
    menu.addSubMenu("Lo-Fi & Glitch", lofiMenu);

    // 5. Pitch & Modulation
    juce::PopupMenu pitchModMenu;
    pitchModMenu.addItem(static_cast<int>(EffectType::PitchShifter) + 1, "Pitch Shifter (-24..+24 st)");
    pitchModMenu.addItem(static_cast<int>(EffectType::FrequencyShifter) + 1, "Bode Frequency Shifter");
    pitchModMenu.addItem(static_cast<int>(EffectType::ModulationChorus) + 1, "Stereo Chorus");
    pitchModMenu.addItem(static_cast<int>(EffectType::ModulationFlanger) + 1, "Flanger");
    pitchModMenu.addItem(static_cast<int>(EffectType::ModulationPhaser) + 1, "4-Stage Phaser");
    pitchModMenu.addItem(static_cast<int>(EffectType::ModulationTremolo) + 1, "Tremolo / Auto-Pan");
    menu.addSubMenu("Pitch & Modulation", pitchModMenu);

    // 6. Delay
    juce::PopupMenu delayMenu;
    delayMenu.addItem(static_cast<int>(EffectType::DelayStereo) + 1, "Stereo Delay");
    delayMenu.addItem(static_cast<int>(EffectType::DelayPingPong) + 1, "Ping-Pong Delay");
    delayMenu.addItem(static_cast<int>(EffectType::DelayTapeEcho) + 1, "Tape Echo");
    menu.addSubMenu("Delay & Echo", delayMenu);

    // 7. Reverb
    juce::PopupMenu reverbMenu;
    reverbMenu.addItem(static_cast<int>(EffectType::ReverbRoom) + 1, "Room Reverb");
    reverbMenu.addItem(static_cast<int>(EffectType::ReverbHall) + 1, "Hall Reverb");
    reverbMenu.addItem(static_cast<int>(EffectType::ReverbPlate) + 1, "Plate Reverb");
    reverbMenu.addItem(static_cast<int>(EffectType::ReverbSpring) + 1, "Spring Reverb");
    reverbMenu.addItem(static_cast<int>(EffectType::ReverbShimmer) + 1, "Shimmer Reverb");
    menu.addSubMenu("Reverb & Space", reverbMenu);

    *typeCombo.getRootMenu() = menu;
    typeCombo.setTextWhenNothingSelected("Select Algorithm...");

    typeCombo.onChange = [this]()
    {
        const int newId = typeCombo.getSelectedId();
        const EffectType newType = static_cast<EffectType>(newId - 1);

        if (newType != lastType)
        {
            const int lastIdx = static_cast<int>(lastType);
            if (lastIdx >= 0 && lastIdx < 36)
            {
                effectParamCache[lastIdx][0] = static_cast<float>(p1Slider.getValue());
                effectParamCache[lastIdx][1] = static_cast<float>(p2Slider.getValue());
                effectParamCache[lastIdx][2] = static_cast<float>(p3Slider.getValue());
                effectParamCache[lastIdx][3] = static_cast<float>(p4Slider.getValue());
                effectParamCache[lastIdx][4] = static_cast<float>(p5Slider.getValue());
                effectParamCache[lastIdx][5] = static_cast<float>(mixSlider.getValue());
            }

            const int newIdx = static_cast<int>(newType);
            if (newIdx >= 0 && newIdx < 36)
            {
                if (auto* p = apvts.getParameter(p1ParamID)) p->setValueNotifyingHost(p->convertTo0to1(effectParamCache[newIdx][0]));
                if (auto* p = apvts.getParameter(p2ParamID)) p->setValueNotifyingHost(p->convertTo0to1(effectParamCache[newIdx][1]));
                if (auto* p = apvts.getParameter(p3ParamID)) p->setValueNotifyingHost(p->convertTo0to1(effectParamCache[newIdx][2]));
                if (auto* p = apvts.getParameter(p4ParamID)) p->setValueNotifyingHost(p->convertTo0to1(effectParamCache[newIdx][3]));
                if (auto* p = apvts.getParameter(p5ParamID)) p->setValueNotifyingHost(p->convertTo0to1(effectParamCache[newIdx][4]));
                if (auto* p = apvts.getParameter(mixParamID)) p->setValueNotifyingHost(p->convertTo0to1(effectParamCache[newIdx][5]));
            }

            lastType = newType;
        }

        updateParamLabels();
        resized();
        repaint();
    };
    addAndMakeVisible(typeCombo);

    // 4 Stereo Output Bus routing destination ComboBox
    routeCombo.addItem("OUT: Disconnected", 1);
    routeCombo.addItem("OUT: Bus 1 (Main)", 2);
    routeCombo.addItem("OUT: Bus 2 (Aux 1)", 3);
    routeCombo.addItem("OUT: Bus 3 (Aux 2)", 4);
    routeCombo.addItem("OUT: Bus 4 (Aux 3)", 5);
    routeCombo.addItem("OUT: All Buses", 6);
    routeCombo.addItem("OUT: Next Slot", 7);
    for (int s = 1; s <= 8; ++s)
        routeCombo.addItem("OUT: -> Slot " + juce::String(s), 7 + s);
    routeCombo.setTooltip("Select audio destination for Slot " + juce::String(globalSlotIdx + 1));
    addAndMakeVisible(routeCombo);

    passThroughInfoLabel.setText("DIRECT DRY PASS-THROUGH", juce::dontSendNotification);
    passThroughInfoLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    passThroughInfoLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    passThroughInfoLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(passThroughInfoLabel);

    auto setupRotary = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col, bool showDb = false)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 13);
        s.getProperties().set("accentColor", col.toString());
        if (showDb)
        {
            s.setTextValueSuffix(" dB");
            s.setNumDecimalPlacesToDisplay(1);
        }
        else
        {
            s.setNumDecimalPlacesToDisplay(2);
        }
        addChildComponent(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addChildComponent(l);
    };

    setupRotary(inGainSlider, inGainLabel, "IN LEVEL", juce::Colour(0xff4ade80), true);
    setupRotary(p1Slider, p1Label, "P1", accent);
    setupRotary(p2Slider, p2Label, "P2", accent);
    setupRotary(p3Slider, p3Label, "P3", accent);
    setupRotary(p4Slider, p4Label, "P4", accent);
    setupRotary(p5Slider, p5Label, "P5", accent);
    setupRotary(mixSlider, mixLabel, "MIX", accent);
    mixSlider.setTextValueSuffix(" %");
    mixSlider.setNumDecimalPlacesToDisplay(0);
    mixSlider.textFromValueFunction = [](double v) {
        return juce::String(static_cast<int>(std::round(v * 100.0))) + " %";
    };

    setupRotary(outGainSlider, outGainLabel, "OUT LEVEL", juce::Colour(0xff60a5fa), true);

    inputSourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, srcParamID, inputSourceCombo);
    typeAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, typeParamID, typeCombo);
    routeAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, destParamID, routeCombo);
    bypassAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, bypassParamID, bypassButton);
    inGainAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, inGainParamID, inGainSlider);
    outGainAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, outGainParamID, outGainSlider);
    p1Attachment          = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, p1ParamID, p1Slider);
    p2Attachment          = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, p2ParamID, p2Slider);
    p3Attachment          = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, p3ParamID, p3Slider);
    p4Attachment          = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, p4ParamID, p4Slider);
    p5Attachment          = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, p5ParamID, p5Slider);
    mixAttachment         = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, mixParamID, mixSlider);

    apvts.addParameterListener(srcParamID, this);
    apvts.addParameterListener(typeParamID, this);
    apvts.addParameterListener(bypassParamID, this);
    apvts.addParameterListener(destParamID, this);

    updateParamLabels();
    updateRouteChoices();
    updateInputSourceChoices();
}

EffectSlotComponent::~EffectSlotComponent()
{
    apvts.removeParameterListener(srcParamID, this);
    apvts.removeParameterListener(typeParamID, this);
    apvts.removeParameterListener(bypassParamID, this);
    apvts.removeParameterListener(destParamID, this);
}

void EffectSlotComponent::parameterChanged(const juce::String& /*parameterID*/, float /*newValue*/)
{
    juce::MessageManager::callAsync([this]() {
        updateParamLabels();
        updateRouteChoices();
        updateInputSourceChoices();
        resized();
        repaint();
    });
}

void EffectSlotComponent::updateRouteChoices()
{
    for (int s = 0; s < 8; ++s)
    {
        const int itemId = 8 + s;
        if (s == globalSlotIdx || engine.wouldCreateCycle(globalSlotIdx, s))
        {
            routeCombo.changeItemText(itemId, "OUT: -> Slot " + juce::String(s + 1) + " (Loop)");
        }
        else
        {
            routeCombo.changeItemText(itemId, "OUT: -> Slot " + juce::String(s + 1));
        }
    }
}

void EffectSlotComponent::updateInputSourceChoices()
{
    for (int s = 0; s < 8; ++s)
    {
        const int itemId = 18 + s;
        if (s == globalSlotIdx || engine.wouldCreateCycle(s, globalSlotIdx))
        {
            inputSourceCombo.changeItemText(itemId, "IN: Slot " + juce::String(s + 1) + " Out (Loop)");
        }
        else
        {
            inputSourceCombo.changeItemText(itemId, "IN: Slot " + juce::String(s + 1) + " Out");
        }
    }
}

void EffectSlotComponent::updateParamLabels()
{
    const int selectedId = typeCombo.getSelectedId();
    const EffectType effType = static_cast<EffectType>(selectedId - 1);
    const juce::Colour catCol = getCategoryColour(effType);

    const bool isBypassed = bypassButton.getToggleState();
    bypassButton.setButtonText(isBypassed ? "OFF" : "ON");
    bypassButton.getProperties().set("accentColor", isBypassed ? juce::Colour(0xff3a4152).toString() : catCol.toString());

    slotTitle.setColour(juce::Label::textColourId, isBypassed ? TransientLookAndFeel::textMuted : catCol);

    auto updateKnobCol = [&catCol](juce::Slider& s) {
        s.getProperties().set("accentColor", catCol.toString());
        s.repaint();
    };

    updateKnobCol(p1Slider);
    updateKnobCol(p2Slider);
    updateKnobCol(p3Slider);
    updateKnobCol(p4Slider);
    updateKnobCol(p5Slider);
    updateKnobCol(mixSlider);

    inGainSlider.setVisible(true);
    inGainLabel.setVisible(true);
    outGainSlider.setVisible(true);
    outGainLabel.setVisible(true);

    if (effType == EffectType::Bypass)
    {
        passThroughInfoLabel.setVisible(true);
        mixSlider.setVisible(false);
        mixLabel.setVisible(false);

        p1Slider.setVisible(false); p1Label.setVisible(false);
        p2Slider.setVisible(false); p2Label.setVisible(false);
        p3Slider.setVisible(false); p3Label.setVisible(false);
        p4Slider.setVisible(false); p4Label.setVisible(false);
        p5Slider.setVisible(false); p5Label.setVisible(false);
    }
    else
    {
        passThroughInfoLabel.setVisible(false);
        mixSlider.setVisible(true);
        mixLabel.setVisible(true);

        auto getSyncDivisionName = [](float v) -> juce::String
        {
            if (v < 0.03f) return "Free";
            const int divIdx = std::clamp(static_cast<int>(std::round(v * 14.0f)), 0, 14);
            switch (divIdx)
            {
                case 0:  return "Free";
                case 1:  return "1/32";
                case 2:  return "1/16";
                case 3:  return "1/8T";
                case 4:  return "1/16D";
                case 5:  return "1/8";
                case 6:  return "1/4T";
                case 7:  return "1/8D";
                case 8:  return "1/4";
                case 9:  return "1/2T";
                case 10: return "1/4D";
                case 11: return "1/2";
                case 12: return "1 Bar";
                case 13: return "2 Bars";
                case 14: return "4 Bars";
                default: return "Free";
            }
        };

        auto checkParam = [this, effType, getSyncDivisionName](int idx, juce::Slider& s, juce::Label& l)
        {
            auto info = EffectSlot::getParamInfoForType(effType, idx);
            if (info.name.isNotEmpty() && info.name != "-")
            {
                s.setVisible(true);
                l.setVisible(true);

                if (info.name == "Tempo Sync")
                {
                    l.setText("SYNC / DIV", juce::dontSendNotification);
                    s.setTextValueSuffix("");
                    s.setNumDecimalPlacesToDisplay(0);
                    s.textFromValueFunction = [getSyncDivisionName](double v) {
                        return getSyncDivisionName(static_cast<float>(v));
                    };
                    s.valueFromTextFunction = [](const juce::String& text) {
                        if (text.containsIgnoreCase("Free")) return 0.0;
                        if (text.containsIgnoreCase("1/32")) return 1.0 / 14.0;
                        if (text.containsIgnoreCase("1/16")) return 2.0 / 14.0;
                        if (text.containsIgnoreCase("1/8T")) return 3.0 / 14.0;
                        if (text.containsIgnoreCase("1/16D")) return 4.0 / 14.0;
                        if (text.containsIgnoreCase("1/8")) return 5.0 / 14.0;
                        if (text.containsIgnoreCase("1/4T")) return 6.0 / 14.0;
                        if (text.containsIgnoreCase("1/8D")) return 7.0 / 14.0;
                        if (text.containsIgnoreCase("1/4")) return 8.0 / 14.0;
                        if (text.containsIgnoreCase("1/2T")) return 9.0 / 14.0;
                        if (text.containsIgnoreCase("1/4D")) return 10.0 / 14.0;
                        if (text.containsIgnoreCase("1/2")) return 11.0 / 14.0;
                        if (text.containsIgnoreCase("1 Bar")) return 12.0 / 14.0;
                        if (text.containsIgnoreCase("2 Bar")) return 13.0 / 14.0;
                        if (text.containsIgnoreCase("4 Bar")) return 14.0 / 14.0;
                        return text.getDoubleValue();
                    };
                }
                else
                {
                    l.setText(info.name.toUpperCase(), juce::dontSendNotification);
                    if (info.label.isNotEmpty()) s.setTextValueSuffix(" " + info.label);
                    s.textFromValueFunction = nullptr;
                    s.valueFromTextFunction = nullptr;
                }
            }
            else
            {
                s.setVisible(false);
                l.setVisible(false);
            }
        };

        checkParam(0, p1Slider, p1Label);
        checkParam(1, p2Slider, p2Label);
        checkParam(2, p3Slider, p3Label);
        checkParam(3, p4Slider, p4Label);
        checkParam(4, p5Slider, p5Label);
    }
}

void EffectSlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const bool isBypassed = bypassButton.getToggleState();
    const bool isPowerOn = !isBypassed;

    const int selectedId = typeCombo.getSelectedId();
    const EffectType effType = static_cast<EffectType>(selectedId - 1);
    const juce::Colour catCol = getCategoryColour(effType);

    // Card background
    g.setColour(isPowerOn ? juce::Colour(0xff141824) : juce::Colour(0xff0e1017));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Card header bar
    auto headerBounds = bounds.removeFromTop(56.0f);
    g.setColour(isPowerOn ? catCol.withAlpha(0.16f) : juce::Colour(0xff161822));
    g.fillRoundedRectangle(headerBounds, 8.0f);

    // Glowing border when active
    g.setColour(isPowerOn ? catCol.withAlpha(0.85f) : juce::Colour(0xff222736));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, isPowerOn ? 1.5f : 1.0f);
}

void EffectSlotComponent::resized()
{
    auto bounds = getLocalBounds().reduced(6);

    // 1. Header Row 1 (24px): [Title] [Power] [IN Dropdown] [OUT Dropdown]
    auto row1 = bounds.removeFromTop(24);
    slotTitle.setBounds(row1.removeFromLeft(48));
    bypassButton.setBounds(row1.removeFromLeft(36).reduced(1));
    row1.removeFromLeft(4);
    inputSourceCombo.setBounds(row1.removeFromLeft(96).reduced(1, 0));
    row1.removeFromLeft(4);
    routeCombo.setBounds(row1.reduced(1, 0));

    bounds.removeFromTop(3);

    // 2. Header Row 2 (24px): [Full-Width Type Dropdown]
    typeCombo.setBounds(bounds.removeFromTop(24).reduced(1, 0));

    bounds.removeFromTop(4); // Gap

    const int selectedId = typeCombo.getSelectedId();
    const EffectType effType = static_cast<EffectType>(selectedId - 1);

    if (effType == EffectType::Bypass)
    {
        auto midArea = bounds.reduced(8, 6);
        const int dialH = 68;

        auto leftCell = midArea.removeFromLeft(76);
        auto lBox = leftCell.withHeight(dialH).withY(midArea.getCentreY() - dialH / 2);
        inGainLabel.setBounds(lBox.removeFromTop(12));
        inGainSlider.setBounds(lBox);

        auto rightCell = midArea.removeFromRight(76);
        auto rBox = rightCell.withHeight(dialH).withY(midArea.getCentreY() - dialH / 2);
        outGainLabel.setBounds(rBox.removeFromTop(12));
        outGainSlider.setBounds(rBox);

        passThroughInfoLabel.setBounds(midArea);
    }
    else
    {
        // 3x3 Spacious Grid of Larger Dials
        // Row 1: In Level, P1, P2
        // Row 2: P3, P4, P5
        // Row 3: Mix, Out Level
        const int totalH = bounds.getHeight();
        const int rowH = (totalH - 8) / 3;
        const int colW = bounds.getWidth() / 3;

        auto r1 = bounds.removeFromTop(rowH);
        bounds.removeFromTop(4);
        auto r2 = bounds.removeFromTop(rowH);
        bounds.removeFromTop(4);
        auto r3 = bounds;

        auto positionTightDial = [](juce::Rectangle<int> cell, juce::Slider& s, juce::Label& l)
        {
            l.setBounds(cell.removeFromTop(12));
            s.setBounds(cell);
        };

        // Row 1
        positionTightDial(r1.removeFromLeft(colW), inGainSlider, inGainLabel);
        positionTightDial(r1.removeFromLeft(colW), p1Slider, p1Label);
        positionTightDial(r1, p2Slider, p2Label);

        // Row 2
        positionTightDial(r2.removeFromLeft(colW), p3Slider, p3Label);
        positionTightDial(r2.removeFromLeft(colW), p4Slider, p4Label);
        positionTightDial(r2, p5Slider, p5Label);

        // Row 3 (Mix & Out Level centered/spaced)
        positionTightDial(r3.removeFromLeft(colW), mixSlider, mixLabel);
        positionTightDial(r3.removeFromLeft(colW), outGainSlider, outGainLabel);
    }
}
