#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class TransientLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TransientLookAndFeel();

    // Color Palette
    static const juce::Colour bgDark;
    static const juce::Colour bgCard;
    static const juce::Colour bgCardHeader;
    static const juce::Colour accentTransient;
    static const juce::Colour accentSustain;
    static const juce::Colour accentSplit;
    static const juce::Colour textBright;
    static const juce::Colour textMuted;
    static const juce::Colour knobTrack;
    static const juce::Colour borderCol;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;
};
