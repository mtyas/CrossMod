#include "SplitControlsComponent.h"
#include "TransientLookAndFeel.h"

SplitControlsComponent::SplitControlsComponent(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    sectionTitle.setText("SPLIT ENGINE / DECOMPOSITION", juce::dontSendNotification);
    sectionTitle.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    sectionTitle.setColour(juce::Label::textColourId, TransientLookAndFeel::textBright);
    addAndMakeVisible(sectionTitle);

    auto setupDial = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.getProperties().set("accentColor", col.toString());
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    setupDial(attackSlider, attackLabel, "Attack Win", TransientLookAndFeel::accentTransient);
    setupDial(releaseSlider, releaseLabel, "Sustain Rel", TransientLookAndFeel::accentSustain);
    setupDial(sensSlider, sensLabel, "Sensitivity", juce::Colour(0xffe5e9f0));
    setupDial(threshSlider, threshLabel, "Threshold", juce::Colour(0xffe5e9f0));

    // Mode Combo
    modeCombo.addItem("Fast Transient", 1);
    modeCombo.addItem("Extended Attack", 2);
    addAndMakeVisible(modeCombo);

    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    modeLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);

    // Solo / Mute Combo
    soloCombo.addItem("Normal (Both)", 1);
    soloCombo.addItem("Solo Transient", 2);
    soloCombo.addItem("Solo Sustain", 3);
    soloCombo.addItem("Mute Transient", 4);
    soloCombo.addItem("Mute Sustain", 5);
    addAndMakeVisible(soloCombo);

    soloLabel.setText("Routing / Solo", juce::dontSendNotification);
    soloLabel.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    soloLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    soloLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(soloLabel);

    // Attachments
    attackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::splitAttack.getParamID(), attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::splitRelease.getParamID(), releaseSlider);
    sensAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::splitSensitivity.getParamID(), sensSlider);
    threshAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::splitThreshold.getParamID(), threshSlider);
    modeAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, IDs::splitMode.getParamID(), modeCombo);
    soloAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, IDs::soloMode.getParamID(), soloCombo);
}

void SplitControlsComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(TransientLookAndFeel::bgCard);
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(TransientLookAndFeel::borderCol);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void SplitControlsComponent::resized()
{
    auto bounds = getLocalBounds().reduced(6);

    auto titleArea = bounds.removeFromTop(18);
    sectionTitle.setBounds(titleArea);

    bounds.removeFromTop(4);

    const int totalWidth = bounds.getWidth();
    const int dialW = 60;

    // Dials on left
    auto setupDialBounds = [&bounds, dialW](juce::Slider& s, juce::Label& l)
    {
        auto area = bounds.removeFromLeft(dialW);
        l.setBounds(area.removeFromBottom(14));
        s.setBounds(area.reduced(2));
    };

    setupDialBounds(attackSlider, attackLabel);
    setupDialBounds(releaseSlider, releaseLabel);
    setupDialBounds(sensSlider, sensLabel);
    setupDialBounds(threshSlider, threshLabel);

    bounds.removeFromLeft(12);

    // Dropdowns on right
    auto comboCol1 = bounds.removeFromLeft(110);
    modeLabel.setBounds(comboCol1.removeFromTop(14));
    modeCombo.setBounds(comboCol1.removeFromTop(24).reduced(0, 2));

    bounds.removeFromLeft(8);

    auto comboCol2 = bounds.removeFromLeft(120);
    soloLabel.setBounds(comboCol2.removeFromTop(14));
    soloCombo.setBounds(comboCol2.removeFromTop(24).reduced(0, 2));
}
