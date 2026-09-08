#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum KnobColor
    {
        Amber,
        Cyan,
        Crimson,
        Emerald,
        Gold,
        Ivory
    };

    VintageLookAndFeel();
    ~VintageLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider&) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    static juce::Colour getKnobAccentColour(KnobColor color)
    {
        switch (color)
        {
            case Amber:   return juce::Colour(0xfff39c12);
            case Cyan:    return juce::Colour(0xff29b6f6);
            case Crimson: return juce::Colour(0xffe74c3c);
            case Emerald: return juce::Colour(0xff2ecc71);
            case Gold:    return juce::Colour(0xfff1c40f);
            case Ivory:
            default:      return juce::Colour(0xffe0dcd3);
        }
    }
};
