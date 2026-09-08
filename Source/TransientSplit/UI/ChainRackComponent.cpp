#include "ChainRackComponent.h"
#include "TransientLookAndFeel.h"

ChainRackComponent::ChainRackComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse, bool isTrans)
    : apvts(state), engine(engineToUse), isTransient(isTrans)
{
    const juce::Colour accent = isTransient ? TransientLookAndFeel::accentTransient : TransientLookAndFeel::accentSustain;

    // Chain Badge / Title
    chainBadge.setText(isTransient ? "TRANSIENT / ATTACK" : "SUSTAIN BODY", juce::dontSendNotification);
    chainBadge.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    chainBadge.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(chainBadge);

    // Solo & Mute Buttons
    soloButton.setClickingTogglesState(true);
    soloButton.setButtonText("SOLO");
    soloButton.setTooltip(isTransient ? "Solo Transient Audio (Mutes Sustain)" : "Solo Sustain Audio (Mutes Transient)");
    soloButton.getProperties().set("accentColor", juce::Colour(0xffffdd00).toString());
    soloButton.getProperties().set("isSoloMute", true);
    addAndMakeVisible(soloButton);

    muteButton.setClickingTogglesState(true);
    muteButton.setButtonText("MUTE");
    muteButton.setTooltip(isTransient ? "Mute Transient Channel" : "Mute Sustain Channel");
    muteButton.getProperties().set("accentColor", juce::Colour(0xffff3b30).toString());
    muteButton.getProperties().set("isSoloMute", true);
    addAndMakeVisible(muteButton);

    // Prominent Large Output Volume / Level Slider with visible dB text box
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.setNumDecimalPlacesToDisplay(1);
    gainSlider.setDoubleClickReturnValue(true, 0.0);
    gainSlider.getProperties().set("accentColor", accent.toString());
    gainSlider.setTooltip(isTransient ? "Direct Dry Transient Level to Output" : "Direct Dry Sustain Level to Output");
    addAndMakeVisible(gainSlider);

    gainSlider.textFromValueFunction = [](double v) {
        return (v <= -59.5) ? "-INF dB" : (juce::String(v, 1) + " dB");
    };

    gainLabel.setText("DRY LEVEL", juce::dontSendNotification);
    gainLabel.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    gainLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);

    // Pan Slider
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setDoubleClickReturnValue(true, 0.0);
    panSlider.getProperties().set("accentColor", accent.toString());
    addAndMakeVisible(panSlider);

    panLabel.setText("PAN", juce::dontSendNotification);
    panLabel.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    panLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    panLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(panLabel);

    // Attachments
    const auto gainId = isTransient ? IDs::transientGain.getParamID() : IDs::sustainGain.getParamID();
    const auto panId  = isTransient ? IDs::transientPan.getParamID()  : IDs::sustainPan.getParamID();
    const auto soloId = isTransient ? IDs::transientSolo.getParamID() : IDs::sustainSolo.getParamID();
    const auto muteId = isTransient ? IDs::transientMute.getParamID() : IDs::sustainMute.getParamID();

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, gainId, gainSlider);
    panAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, panId, panSlider);
    soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, soloId, soloButton);
    muteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, muteId, muteButton);

    // Create 4 Effect Slots
    for (int i = 0; i < 4; ++i)
    {
        slots[i] = std::make_unique<EffectSlotComponent>(apvts, engine, isTransient ? i : (4 + i));
        addAndMakeVisible(*slots[i]);
    }

    startTimerHz(30);
}

ChainRackComponent::~ChainRackComponent()
{
    stopTimer();
}

void ChainRackComponent::timerCallback()
{
    const int engineMode = engine.getEngineMode();
    const bool isSplitMode = (engineMode == 0);

    soloButton.setVisible(isSplitMode);
    muteButton.setVisible(isSplitMode);
    panSlider.setVisible(isSplitMode);
    panLabel.setVisible(isSplitMode);

    gainLabel.setText("DRY LEVEL", juce::dontSendNotification);

    if (!isSplitMode) // Modular FX mode
    {
        chainBadge.setText(isTransient ? "MODULAR FX: SLOTS 1 - 4" : "MODULAR FX: SLOTS 5 - 8", juce::dontSendNotification);
        for (int i = 0; i < 4; ++i)
            slots[i]->updateRouteChoices();
    }
    else // Split mode
    {
        chainBadge.setText(isTransient ? "TRANSIENT / ATTACK" : "SUSTAIN BODY", juce::dontSendNotification);
    }

    const float level = isTransient ? engine.getTransMeterLevel() : engine.getSustMeterLevel();
    currentMeterLevel = std::max(level, currentMeterLevel * 0.85f);
    repaint();
}

void ChainRackComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Rack frame background
    g.setColour(juce::Colour(0xff151820));
    g.fillRoundedRectangle(bounds, 8.0f);

    const juce::Colour accent = isTransient ? TransientLookAndFeel::accentTransient : TransientLookAndFeel::accentSustain;
    g.setColour(accent.withAlpha(0.25f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.5f);

    // Header strip background
    auto headerBounds = bounds.removeFromTop(56.0f).reduced(2.0f);
    g.setColour(juce::Colour(0xff1b1f2b));
    g.fillRoundedRectangle(headerBounds, 6.0f);

    // Level meter bar beside volume knob
    auto meterArea = headerBounds.removeFromRight(10.0f).reduced(1.0f, 6.0f);
    g.setColour(juce::Colour(0xff101218));
    g.fillRoundedRectangle(meterArea, 2.0f);

    const float meterH = meterArea.getHeight() * std::clamp(currentMeterLevel, 0.0f, 1.0f);
    auto fillArea = meterArea.removeFromBottom(meterH);
    g.setColour(accent);
    g.fillRoundedRectangle(fillArea, 2.0f);
}

void ChainRackComponent::resized()
{
    auto bounds = getLocalBounds().reduced(6);

    // Header bar (Height 54)
    auto header = bounds.removeFromTop(54).reduced(4, 2);

    // Left: Badge & Solo/Mute
    auto leftControls = header.removeFromLeft(160);
    chainBadge.setBounds(leftControls.removeFromTop(24));

    auto buttonRow = leftControls.removeFromTop(22);
    soloButton.setBounds(buttonRow.removeFromLeft(48).reduced(1));
    buttonRow.removeFromLeft(4);
    muteButton.setBounds(buttonRow.removeFromLeft(48).reduced(1));

    // Right: Pan & Large Output Volume Controls
    header.removeFromRight(12); // Meter gap

    auto gainArea = header.removeFromRight(68);
    gainLabel.setBounds(gainArea.removeFromTop(10));
    gainSlider.setBounds(gainArea);

    header.removeFromRight(6);

    auto panArea = header.removeFromRight(48);
    panLabel.setBounds(panArea.removeFromTop(10));
    panSlider.setBounds(panArea);

    bounds.removeFromTop(6);

    // Layout 4 slots vertically
    const int slotH = (bounds.getHeight() - 12) / 4;
    for (int i = 0; i < 4; ++i)
    {
        slots[i]->setBounds(bounds.removeFromTop(slotH));
        bounds.removeFromTop(4);
    }
}
