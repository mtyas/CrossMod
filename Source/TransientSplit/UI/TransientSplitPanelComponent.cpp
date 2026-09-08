#include "TransientSplitPanelComponent.h"

TransientSplitPanelComponent::TransientSplitPanelComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse)
    : apvts(state), engine(engineToUse), visualizer(engineToUse)
{
    addAndMakeVisible(visualizer);

    auto setupDial = [this](juce::Slider& s, juce::Label& l, const juce::String& name, const juce::Colour& col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
        s.getProperties().set("accentColor", col.toString());
        addAndMakeVisible(s);

        l.setText(name, juce::dontSendNotification);
        l.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    const auto cyan = TransientLookAndFeel::accentTransient;
    const auto amber = TransientLookAndFeel::accentSustain;

    setupDial(attackSlider, attackLabel, "ATTACK", cyan);
    attackSlider.setTextValueSuffix(" ms");
    attackSlider.setNumDecimalPlacesToDisplay(1);

    setupDial(releaseSlider, releaseLabel, "RELEASE", amber);
    releaseSlider.setTextValueSuffix(" ms");
    releaseSlider.setNumDecimalPlacesToDisplay(1);

    setupDial(sensSlider, sensLabel, "SENSITIVITY", cyan);
    sensSlider.setNumDecimalPlacesToDisplay(2);

    setupDial(threshSlider, threshLabel, "THRESHOLD", amber);
    threshSlider.setTextValueSuffix(" dB");
    threshSlider.setNumDecimalPlacesToDisplay(1);

    setupDial(lookaheadSlider, lookaheadLabel, "LOOKAHEAD", cyan);
    lookaheadSlider.setTextValueSuffix(" spls");
    lookaheadSlider.setNumDecimalPlacesToDisplay(0);

    modeCombo.addItem("Fast Transient", 1);
    modeCombo.addItem("Extended Pluck", 2);
    addAndMakeVisible(modeCombo);

    modeLabel.setText("DETECTION", juce::dontSendNotification);
    modeLabel.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    modeLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);

    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, IDs::splitAttack.getParamID(), attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, IDs::splitRelease.getParamID(), releaseSlider);
    sensAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, IDs::splitSensitivity.getParamID(), sensSlider);
    threshAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, IDs::splitThreshold.getParamID(), threshSlider);
    lookaheadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, IDs::splitLookahead.getParamID(), lookaheadSlider);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, IDs::splitMode.getParamID(), modeCombo);
}

void TransientSplitPanelComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0f14));

    g.setColour(TransientLookAndFeel::textBright);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("TRANSIENT & SUSTAIN SPECTRAL SPLITTER ENGINE", 16, 10, 450, 18, juce::Justification::left);

    g.setColour(TransientLookAndFeel::textMuted);
    g.setFont(juce::FontOptions(10.5f));
    g.drawText("High-speed envelope & ratio extraction feeds pure Attack & Sustain streams to any slot", 16, 28, 550, 16, juce::Justification::left);
}

void TransientSplitPanelComponent::resized()
{
    auto area = getLocalBounds().reduced(14, 10);
    area.removeFromTop(38); // Header space

    // Visualizer takes the top 44%
    visualizer.setBounds(area.removeFromTop(area.getHeight() * 0.44f).reduced(2));

    area.removeFromTop(8); // Gap

    // Bottom row: 5 dials + 1 combo
    const int itemW = area.getWidth() / 6;

    auto d1 = area.removeFromLeft(itemW);
    attackLabel.setBounds(d1.removeFromBottom(14));
    attackSlider.setBounds(d1);

    auto d2 = area.removeFromLeft(itemW);
    releaseLabel.setBounds(d2.removeFromBottom(14));
    releaseSlider.setBounds(d2);

    auto d3 = area.removeFromLeft(itemW);
    sensLabel.setBounds(d3.removeFromBottom(14));
    sensSlider.setBounds(d3);

    auto d4 = area.removeFromLeft(itemW);
    threshLabel.setBounds(d4.removeFromBottom(14));
    threshSlider.setBounds(d4);

    auto d5 = area.removeFromLeft(itemW);
    lookaheadLabel.setBounds(d5.removeFromBottom(14));
    lookaheadSlider.setBounds(d5);

    auto d6 = area;
    modeLabel.setBounds(d6.removeFromBottom(14));
    d6.reduce(4, 12);
    modeCombo.setBounds(d6);
}
