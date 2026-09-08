#include "FilterBankPanelComponent.h"
#include "TransientLookAndFeel.h"

FilterBankPanelComponent::FilterBankPanelComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse)
    : apvts(state), engine(engineToUse)
{
    const juce::StringArray filterTypeChoices = {
        "Low-Pass 12dB",
        "Low-Pass 24dB",
        "High-Pass 12dB",
        "High-Pass 24dB",
        "Band-Pass",
        "Peak / Bell",
        "Notch",
        "Low Shelf",
        "High Shelf"
    };

    const juce::Colour bandColors[3] = {
        juce::Colour(0xff00d4ff), // Cyan for Filter 1 (Low)
        juce::Colour(0xffff9900), // Amber for Filter 2 (Mid)
        juce::Colour(0xffa855f7)  // Purple for Filter 3 (High)
    };

    const juce::String bandNames[3] = {
        "FILTER 1 [LOW / BASS]",
        "FILTER 2 [MID BAND]",
        "FILTER 3 [HIGH / AIR]"
    };

    for (int b = 0; b < 3; ++b)
    {
        auto& s = strips[b];
        const auto& col = bandColors[b];

        s.titleLabel.setText(bandNames[b], juce::dontSendNotification);
        s.titleLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        s.titleLabel.setColour(juce::Label::textColourId, col);
        s.titleLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(s.titleLabel);

        s.typeCombo.addItemList(filterTypeChoices, 1);
        addAndMakeVisible(s.typeCombo);

        s.bypassButton.getProperties().set("accentColor", col.toString());
        s.bypassButton.getProperties().set("isBypassToggle", true);
        s.bypassButton.setTooltip("Power ON/OFF for Filter " + juce::String(b + 1));
        addAndMakeVisible(s.bypassButton);

        s.soloButton.setTooltip("Solo Filter " + juce::String(b + 1));
        s.soloButton.getProperties().set("accentColor", juce::Colour(0xffffdd00).toString());
        s.soloButton.getProperties().set("isSoloMute", true);
        addAndMakeVisible(s.soloButton);

        s.muteButton.setTooltip("Mute Filter " + juce::String(b + 1));
        s.muteButton.getProperties().set("accentColor", juce::Colour(0xffff3b30).toString());
        s.muteButton.getProperties().set("isSoloMute", true);
        addAndMakeVisible(s.muteButton);

        auto setupDial = [this, &col](juce::Slider& sl, juce::Label& lb, const juce::String& name)
        {
            sl.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            sl.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
            sl.getProperties().set("accentColor", col.toString());
            addAndMakeVisible(sl);

            lb.setText(name, juce::dontSendNotification);
            lb.setFont(juce::FontOptions(9.0f, juce::Font::bold));
            lb.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
            lb.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(lb);
        };

        setupDial(s.freqSlider, s.freqLabel, "FREQ");
        s.freqSlider.setTextValueSuffix(" Hz");
        s.freqSlider.setNumDecimalPlacesToDisplay(0);
        s.freqSlider.textFromValueFunction = [](double v) {
            if (v >= 1000.0)
                return juce::String(v * 0.001, 2) + " kHz";
            return juce::String(std::round(v)) + " Hz";
        };

        setupDial(s.qSlider, s.qLabel, "Q / RESO");
        s.qSlider.setNumDecimalPlacesToDisplay(2);

        setupDial(s.gainSlider, s.gainLabel, "GAIN");
        s.gainSlider.setTextValueSuffix(" dB");
        s.gainSlider.setNumDecimalPlacesToDisplay(1);

        setupDial(s.driveSlider, s.driveLabel, "DRIVE");
        s.driveSlider.setTextValueSuffix("x");
        s.driveSlider.setNumDecimalPlacesToDisplay(2);

        // Attachments
        s.typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, IDs::getFilterType(b).getParamID(), s.typeCombo);
        s.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, IDs::getFilterBypass(b).getParamID(), s.bypassButton);
        s.soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, IDs::getFilterSolo(b).getParamID(), s.soloButton);
        s.muteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, IDs::getFilterMute(b).getParamID(), s.muteButton);

        s.freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, IDs::getFilterFreq(b).getParamID(), s.freqSlider);
        s.qAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, IDs::getFilterQ(b).getParamID(), s.qSlider);
        s.gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, IDs::getFilterGain(b).getParamID(), s.gainSlider);
        s.driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, IDs::getFilterDrive(b).getParamID(), s.driveSlider);
    }

    startTimerHz(30);
}

FilterBankPanelComponent::~FilterBankPanelComponent()
{
    stopTimer();
}

void FilterBankPanelComponent::timerCallback()
{
    repaint(curveBounds);
}

void FilterBankPanelComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0f14));

    // Section title & badges
    g.setColour(TransientLookAndFeel::textBright);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("3-BAND CONTINUOUS & OVERLAPPING FILTER BANK", 16, 10, 450, 18, juce::Justification::left);

    g.setColour(TransientLookAndFeel::textMuted);
    g.setFont(juce::FontOptions(10.5f));
    g.drawText("Freely overlapping independent filters available as universal inputs for all 8 slots", 16, 28, 550, 16, juce::Justification::left);

    if (!curveBounds.isEmpty())
    {
        paintFrequencyResponse(g, curveBounds.toFloat());
    }
}

void FilterBankPanelComponent::paintFrequencyResponse(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Background box
    g.setColour(juce::Colour(0xff12151c));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour(0xff222836));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Grid lines for 100Hz, 1kHz, 10kHz
    const double freqs[4] = { 100.0, 1000.0, 5000.0, 10000.0 };
    const juce::String labels[4] = { "100Hz", "1kHz", "5kHz", "10kHz" };

    g.setFont(juce::FontOptions(9.0f));
    for (int i = 0; i < 4; ++i)
    {
        const float normX = static_cast<float>(std::log10(freqs[i] / 20.0) / std::log10(20000.0 / 20.0));
        const float x = bounds.getX() + normX * bounds.getWidth();

        g.setColour(juce::Colour(0xff1d2330));
        g.drawVerticalLine(static_cast<int>(x), bounds.getY() + 4.0f, bounds.getBottom() - 4.0f);

        g.setColour(TransientLookAndFeel::textMuted.withAlpha(0.6f));
        g.drawText(labels[i], x + 3.0f, bounds.getY() + 4.0f, 40.0f, 12.0f, juce::Justification::left);
    }

    // 0dB center line
    const float midY = bounds.getCentreY();
    g.setColour(juce::Colour(0xff262e40));
    g.drawHorizontalLine(static_cast<int>(midY), bounds.getX() + 4.0f, bounds.getRight() - 4.0f);

    const juce::Colour bandColors[3] = {
        juce::Colour(0xff00d4ff),
        juce::Colour(0xffff9900),
        juce::Colour(0xffa855f7)
    };

    const auto& filterBank = engine.getFilterBank();

    // Draw individual band curves
    for (int b = 0; b < 3; ++b)
    {
        juce::Path p;
        const int numPts = static_cast<int>(bounds.getWidth());
        bool started = false;

        for (int i = 0; i < numPts; i += 2)
        {
            const float normX = static_cast<float>(i) / static_cast<float>(numPts);
            const double freq = 20.0 * std::pow(1000.0, normX);
            const float mag = filterBank.getMagnitudeForFrequency(b, freq);
            const float gainDb = 20.0f * std::log10(std::max(1e-4f, mag));

            // Map -24dB to +24dB -> bounds.bottom to bounds.top
            const float normY = std::clamp((gainDb + 24.0f) / 48.0f, 0.0f, 1.0f);
            const float y = bounds.getBottom() - normY * bounds.getHeight();
            const float x = bounds.getX() + static_cast<float>(i);

            if (!started) { p.startNewSubPath(x, y); started = true; }
            else { p.lineTo(x, y); }
        }

        g.setColour(bandColors[b].withAlpha(0.85f));
        g.strokePath(p, juce::PathStrokeType(1.8f));
    }
}

void FilterBankPanelComponent::resized()
{
    auto area = getLocalBounds().reduced(12, 8);
    area.removeFromTop(34); // Header space

    // Left side (36% width): Visual Frequency Curve
    const int curveW = static_cast<int>(area.getWidth() * 0.36f);
    curveBounds = area.removeFromLeft(curveW).reduced(2);

    area.removeFromLeft(12); // Gap

    // Right side: 3 Filter Strips side by side
    const int stripW = (area.getWidth() - 16) / 3;

    for (int b = 0; b < 3; ++b)
    {
        auto sBounds = area.removeFromLeft(stripW);
        if (b < 2) area.removeFromLeft(8); // Gap between strips

        auto& s = strips[b];

        auto topRow = sBounds.removeFromTop(24);
        s.bypassButton.setBounds(topRow.removeFromLeft(40));
        topRow.removeFromLeft(4);
        s.soloButton.setBounds(topRow.removeFromRight(34));
        topRow.removeFromRight(4);
        s.muteButton.setBounds(topRow.removeFromRight(34));
        topRow.removeFromRight(4);
        s.titleLabel.setBounds(topRow);

        sBounds.removeFromTop(4);
        s.typeCombo.setBounds(sBounds.removeFromTop(24));
        sBounds.removeFromTop(8);

        // 4 Rotary Dials in 2x2 grid with tight attached labels
        const int rowH = (sBounds.getHeight() - 4) / 2;
        const int colW = sBounds.getWidth() / 2;

        auto row1 = sBounds.removeFromTop(rowH);
        sBounds.removeFromTop(4);
        auto row2 = sBounds;

        auto positionTight = [](juce::Rectangle<int> cell, juce::Slider& sl, juce::Label& lb)
        {
            lb.setBounds(cell.removeFromTop(13));
            sl.setBounds(cell);
        };

        positionTight(row1.removeFromLeft(colW), s.freqSlider, s.freqLabel);
        positionTight(row1, s.qSlider, s.qLabel);

        positionTight(row2.removeFromLeft(colW), s.gainSlider, s.gainLabel);
        positionTight(row2, s.driveSlider, s.driveLabel);
    }
}
