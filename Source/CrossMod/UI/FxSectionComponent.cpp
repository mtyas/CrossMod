#include "FxSectionComponent.h"

FxSectionComponent::FxSectionComponent(juce::AudioProcessorValueTreeState& apvts, MidiLearnManager& mlm)
    : apvtsRef(apvts),
      midiLearnRef(mlm)
{
    // Routing Order
    orderLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    orderLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe5a823));
    addAndMakeVisible(orderLabel);

    auto orderChoices = ParameterFactory::getFxOrderChoices();
    for (int i = 0; i < orderChoices.size(); ++i)
        orderBox.addItem(orderChoices[i], i + 1);
    addAndMakeVisible(orderBox);
    orderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::fxRoutingOrder.getParamID(), orderBox);

    // 1. Modulation FX Controls
    auto modTypes = ParameterFactory::getModFxTypeChoices();
    for (int i = 0; i < modTypes.size(); ++i)
        modTypeBox.addItem(modTypes[i], i + 1);
    addAndMakeVisible(modTypeBox);
    modTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::modFxType.getParamID(), modTypeBox);

    modRateKnob.init(*this, apvts, mlm, CrossModIDs::modFxRate, "RATE", VintageLookAndFeel::Cyan, " Hz");
    modDepthKnob.init(*this, apvts, mlm, CrossModIDs::modFxDepth, "DEPTH", VintageLookAndFeel::Cyan, "");
    modFeedbackKnob.init(*this, apvts, mlm, CrossModIDs::modFxFeedback, "FEEDBACK", VintageLookAndFeel::Cyan, "");
    modMixKnob.init(*this, apvts, mlm, CrossModIDs::modFxMix, "MOD AMOUNT", VintageLookAndFeel::Cyan, "");

    // 2. Delay FX Controls
    auto delayTypes = ParameterFactory::getDelayFxTypeChoices();
    for (int i = 0; i < delayTypes.size(); ++i)
        delayTypeBox.addItem(delayTypes[i], i + 1);
    addAndMakeVisible(delayTypeBox);
    delayTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::delayFxType.getParamID(), delayTypeBox);

    addAndMakeVisible(delaySyncBtn);
    delaySyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, CrossModIDs::delayFxSync.getParamID(), delaySyncBtn);
    delaySyncBtn.onClick = [this]() {
        updateDelayTimeAttachment(delaySyncBtn.getToggleState());
    };

    delayTimeKnob.init(*this, apvts, mlm, CrossModIDs::delayFxTime, "TIME", VintageLookAndFeel::Amber, " s");
    delayFeedbackKnob.init(*this, apvts, mlm, CrossModIDs::delayFxFeedback, "FEEDBACK", VintageLookAndFeel::Amber, "");
    delayToneKnob.init(*this, apvts, mlm, CrossModIDs::delayFxTone, "TONE", VintageLookAndFeel::Amber, "");
    delayMixKnob.init(*this, apvts, mlm, CrossModIDs::delayFxMix, "DELAY AMOUNT", VintageLookAndFeel::Amber, "");

    bool initialSync = apvts.getRawParameterValue(CrossModIDs::delayFxSync.getParamID())->load() > 0.5f;
    lastDelaySynced = initialSync;
    updateDelayTimeAttachment(initialSync);

    // 3. Reverb FX Controls
    auto reverbTypes = ParameterFactory::getReverbFxTypeChoices();
    for (int i = 0; i < reverbTypes.size(); ++i)
        reverbTypeBox.addItem(reverbTypes[i], i + 1);
    addAndMakeVisible(reverbTypeBox);
    reverbTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, CrossModIDs::reverbFxType.getParamID(), reverbTypeBox);

    reverbDecayKnob.init(*this, apvts, mlm, CrossModIDs::reverbFxDecay, "DECAY", VintageLookAndFeel::Emerald, " s");
    reverbDampingKnob.init(*this, apvts, mlm, CrossModIDs::reverbFxDamping, "DAMPING", VintageLookAndFeel::Emerald, "");
    reverbToneKnob.init(*this, apvts, mlm, CrossModIDs::reverbFxTone, "TONE", VintageLookAndFeel::Emerald, "");
    reverbMixKnob.init(*this, apvts, mlm, CrossModIDs::reverbFxMix, "REVERB AMOUNT", VintageLookAndFeel::Emerald, "");
}

void FxSectionComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Recessed background
    g.setColour(juce::Colour(0xff16181d));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(juce::Colour(0xff353b45));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    // Title bar
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffe5a823));
    g.drawText("STUDIO MULTI-EFFECTS PROCESSOR", 14, 6, 240, 18, juce::Justification::centredLeft);

    // 3 Module cards
    auto contentArea = bounds.reduced(10.0f, 6.0f);
    contentArea.removeFromTop(24.0f);

    float cardW = (contentArea.getWidth() - 16.0f) / 3.0f;
    float cardH = contentArea.getHeight();

    auto drawModuleCard = [&g](float x, float y, float w, float h, const juce::String& title, juce::Colour accent) {
        auto r = juce::Rectangle<float>(x, y, w, h);
        g.setColour(juce::Colour(0xff1f2228));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(juce::Colour(0xff303640));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);

        // Header tab
        g.setColour(accent.withAlpha(0.25f));
        g.fillRoundedRectangle(r.getX(), r.getY(), r.getWidth(), 18.0f, 4.0f);
        g.fillRect(r.getX(), r.getY() + 12.0f, r.getWidth(), 6.0f);

        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(accent);
        g.drawText(title, (int)r.getX() + 8, (int)r.getY() + 2, (int)r.getWidth() - 16, 14, juce::Justification::centredLeft);
    };

    drawModuleCard(contentArea.getX(), contentArea.getY(), cardW, cardH, "MODULATION (CHORUS / FLANGER / PHASER / ENSEMBLE)", juce::Colour(0xff29b6f6));
    drawModuleCard(contentArea.getX() + cardW + 8.0f, contentArea.getY(), cardW, cardH, "DELAY (TAPE / BBD / DIGITAL / PING-PONG)", juce::Colour(0xfff39c12));
    drawModuleCard(contentArea.getX() + (cardW + 8.0f) * 2.0f, contentArea.getY(), cardW, cardH, "REVERB (PLATE / ROOM / HALL)", juce::Colour(0xff2ecc71));
}

void FxSectionComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10, 6);

    // Top Header: FX Order selector
    orderLabel.setBounds(bounds.getRight() - 250, 4, 75, 20);
    orderBox.setBounds(bounds.getRight() - 170, 4, 160, 20);

    bounds.removeFromTop(24); // Title bar

    float cardW = (bounds.getWidth() - 16.0f) / 3.0f;
    int cardH = bounds.getHeight();

    int startY = bounds.getY();

    // 1. Modulation FX
    int mX = bounds.getX();
    modTypeBox.setBounds(mX + 10, startY + 22, (int)cardW - 20, 20);
    int knobW = 55;
    int knobH = 68;
    int kY = startY + 48;
    modRateKnob.setBounds(mX + 10, kY, knobW, knobH);
    modDepthKnob.setBounds(mX + 70, kY, knobW, knobH);
    modFeedbackKnob.setBounds(mX + 130, kY, knobW, knobH);
    // Big prominent Amount knob
    modMixKnob.setBounds(mX + (int)cardW - 85, kY - 4, 75, 76);

    // 2. Delay FX
    int dX = bounds.getX() + (int)cardW + 8;
    int syncBtnW = 85;
    delayTypeBox.setBounds(dX + 10, startY + 22, (int)cardW - syncBtnW - 25, 20);
    delaySyncBtn.setBounds(dX + (int)cardW - syncBtnW - 10, startY + 22, syncBtnW, 20);
    delayTimeKnob.setBounds(dX + 10, kY, knobW, knobH);
    delayFeedbackKnob.setBounds(dX + 70, kY, knobW, knobH);
    delayToneKnob.setBounds(dX + 130, kY, knobW, knobH);
    delayMixKnob.setBounds(dX + (int)cardW - 85, kY - 4, 75, 76);

    // 3. Reverb FX
    int rX = bounds.getX() + ((int)cardW + 8) * 2;
    reverbTypeBox.setBounds(rX + 10, startY + 22, (int)cardW - 20, 20);
    reverbDecayKnob.setBounds(rX + 10, kY, knobW, knobH);
    reverbDampingKnob.setBounds(rX + 70, kY, knobW, knobH);
    reverbToneKnob.setBounds(rX + 130, kY, knobW, knobH);
    reverbMixKnob.setBounds(rX + (int)cardW - 85, kY - 4, 75, 76);
}

void FxSectionComponent::updateSyncState()
{
    bool sync = apvtsRef.getRawParameterValue(CrossModIDs::delayFxSync.getParamID())->load() > 0.5f;
    if (sync != lastDelaySynced)
    {
        lastDelaySynced = sync;
        updateDelayTimeAttachment(sync);
    }
}

void FxSectionComponent::updateDelayTimeAttachment(bool isSynced)
{
    delayTimeKnob.attachment.reset();
    if (isSynced)
    {
        auto syncChoices = ParameterFactory::getSyncRateChoices();
        delayTimeKnob.slider->setRange(0, syncChoices.size() - 1, 1);
        delayTimeKnob.slider->textFromValueFunction = [](double v) {
            auto choices = ParameterFactory::getSyncRateChoices();
            int idx = std::clamp((int)std::round(v), 0, choices.size() - 1);
            return choices[idx];
        };
        delayTimeKnob.slider->valueFromTextFunction = [](const juce::String& text) {
            auto choices = ParameterFactory::getSyncRateChoices();
            for (int i = 0; i < choices.size(); ++i)
                if (choices[i].equalsIgnoreCase(text.trim()))
                    return (double)i;
            return 8.0;
        };
        delayTimeKnob.slider->setTextValueSuffix("");
        delayTimeKnob.slider->setParamInfo(CrossModIDs::delayFxSyncRate.getParamID(),
                                          apvtsRef.getParameter(CrossModIDs::delayFxSyncRate.getParamID()),
                                          &midiLearnRef);
        delayTimeKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvtsRef, CrossModIDs::delayFxSyncRate.getParamID(), *delayTimeKnob.slider);
        delayTimeKnob.label->setText("SYNC TIME", juce::dontSendNotification);
    }
    else
    {
        auto* param = apvtsRef.getParameter(CrossModIDs::delayFxTime.getParamID());
        auto range = param->getNormalisableRange();
        delayTimeKnob.slider->setRange(range.start, range.end, range.interval);
        delayTimeKnob.slider->textFromValueFunction = nullptr;
        delayTimeKnob.slider->valueFromTextFunction = nullptr;
        delayTimeKnob.slider->setTextValueSuffix(" s");
        delayTimeKnob.slider->setParamInfo(CrossModIDs::delayFxTime.getParamID(), param, &midiLearnRef);
        delayTimeKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvtsRef, CrossModIDs::delayFxTime.getParamID(), *delayTimeKnob.slider);
        delayTimeKnob.label->setText("TIME", juce::dontSendNotification);
    }
}
