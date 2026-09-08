#include "VintageLookAndFeel.h"

VintageLookAndFeel::VintageLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff181a1e));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff22252a));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe8e4d9));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a515a));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1e2126));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe8e4d9));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3b424c));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffffffff));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffdcd6c8));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
}

void VintageLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    float diameter = std::min(bounds.getWidth(), bounds.getHeight()) - 8.0f;
    if (diameter <= 4.0f) return;

    auto center = bounds.getCentre();
    auto knobBounds = juce::Rectangle<float>(center.x - diameter * 0.5f,
                                            center.y - diameter * 0.5f,
                                            diameter, diameter);

    // 1. Drop shadow
    g.setColour(juce::Colour(0x60000000));
    g.fillEllipse(knobBounds.translated(0.0f, 3.0f));

    // 2. Outer Bezel (Brushed metal rim)
    juce::ColourGradient rimGrad(juce::Colour(0xff525760), knobBounds.getTopLeft(),
                                juce::Colour(0xff282c34), knobBounds.getBottomRight(), false);
    g.setGradientFill(rimGrad);
    g.fillEllipse(knobBounds);

    g.setColour(juce::Colour(0xff14161a));
    g.drawEllipse(knobBounds, 1.2f);

    // 3. Inner Dial Face
    auto faceBounds = knobBounds.reduced(3.5f);
    juce::ColourGradient faceGrad(juce::Colour(0xff32363e), faceBounds.getTopLeft(),
                                 juce::Colour(0xff1c1f24), faceBounds.getBottomRight(), false);
    g.setGradientFill(faceGrad);
    g.fillEllipse(faceBounds);

    // 4. Tick marks track around dial
    float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    
    // Determine accent color from slider properties or fallback
    juce::Colour accentColour = juce::Colour(0xffe5a823); // Default warm amber
    if (slider.getProperties().contains("KnobAccent"))
    {
        int colIdx = slider.getProperties()["KnobAccent"];
        accentColour = getKnobAccentColour(static_cast<KnobColor>(colIdx));
    }

    // Draw active arc
    juce::Path arcPath;
    arcPath.addCentredArc(center.x, center.y, diameter * 0.5f - 1.5f, diameter * 0.5f - 1.5f,
                          0.0f, rotaryStartAngle, currentAngle, true);
    g.setColour(accentColour.withAlpha(0.85f));
    g.strokePath(arcPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 5. Center Cap (Colored anodized aluminum disc)
    auto capBounds = faceBounds.reduced(diameter * 0.22f);
    juce::ColourGradient capGrad(accentColour.withMultipliedBrightness(0.8f), capBounds.getTopLeft(),
                                accentColour.withMultipliedBrightness(0.35f), capBounds.getBottomRight(), false);
    g.setGradientFill(capGrad);
    g.fillEllipse(capBounds);

    g.setColour(juce::Colour(0x40ffffff));
    g.drawEllipse(capBounds, 1.0f);

    // 6. Indicator Line Notch
    float radius = diameter * 0.5f - 4.0f;
    float pointerAngle = currentAngle - juce::MathConstants<float>::halfPi;
    juce::Point<float> p1(center.x + std::cos(pointerAngle) * (radius * 0.35f),
                          center.y + std::sin(pointerAngle) * (radius * 0.35f));
    juce::Point<float> p2(center.x + std::cos(pointerAngle) * (radius * 0.95f),
                          center.y + std::sin(pointerAngle) * (radius * 0.95f));

    g.setColour(juce::Colours::white);
    g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.5f);
}

void VintageLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos, style, slider);
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

    // Track
    float trackHeight = 4.0f;
    auto trackRect = juce::Rectangle<float>(bounds.getX() + 4.0f,
                                            bounds.getCentreY() - trackHeight * 0.5f,
                                            bounds.getWidth() - 8.0f,
                                            trackHeight);
    g.setColour(juce::Colour(0xff121418));
    g.fillRoundedRectangle(trackRect, 2.0f);
    g.setColour(juce::Colour(0xff3a3f47));
    g.drawRoundedRectangle(trackRect, 2.0f, 1.0f);

    // Bipolar center line if needed
    float centerX = trackRect.getCentreX();
    g.setColour(juce::Colour(0x60ffffff));
    g.drawVerticalLine((int)centerX, trackRect.getY() - 3.0f, trackRect.getBottom() + 3.0f);

    // Active track
    g.setColour(juce::Colour(0xffe5a823));
    if (sliderPos > centerX)
    {
        g.fillRoundedRectangle(juce::Rectangle<float>(centerX, trackRect.getY(), sliderPos - centerX, trackHeight), 2.0f);
    }
    else
    {
        g.fillRoundedRectangle(juce::Rectangle<float>(sliderPos, trackRect.getY(), centerX - sliderPos, trackHeight), 2.0f);
    }

    // Thumb thumb handle
    float thumbW = 12.0f;
    float thumbH = bounds.getHeight() - 4.0f;
    auto thumbRect = juce::Rectangle<float>(sliderPos - thumbW * 0.5f,
                                            bounds.getCentreY() - thumbH * 0.5f,
                                            thumbW, thumbH);

    juce::ColourGradient thumbGrad(juce::Colour(0xff686e7a), thumbRect.getTopLeft(),
                                  juce::Colour(0xff2d3138), thumbRect.getBottomRight(), false);
    g.setGradientFill(thumbGrad);
    g.fillRoundedRectangle(thumbRect, 2.5f);

    g.setColour(juce::Colour(0xff181a1f));
    g.drawRoundedRectangle(thumbRect, 2.5f, 1.0f);

    // Center groove on thumb
    g.setColour(juce::Colour(0xffe5a823));
    g.drawVerticalLine((int)sliderPos, thumbRect.getY() + 3.0f, thumbRect.getBottom() - 3.0f);
}

void VintageLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH, box);
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

    juce::ColourGradient bgGrad(juce::Colour(0xff282c34), bounds.getTopLeft(),
                                juce::Colour(0xff1c1f24), bounds.getBottomRight(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(juce::Colour(0xff4a515c));
    g.drawRoundedRectangle(bounds, 3.0f, 1.2f);

    // Vintage Arrow
    auto arrowZone = juce::Rectangle<float>((float)(width - 18), 0.0f, 16.0f, (float)height);
    juce::Path arrow;
    float ax = arrowZone.getCentreX();
    float ay = arrowZone.getCentreY();
    arrow.addTriangle(ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour(juce::Colour(0xffdcd6c8));
    g.fillPath(arrow);
}

void VintageLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    bool isToggled = button.getToggleState();

    juce::Colour base = isToggled ? juce::Colour(0xffc0392b) : juce::Colour(0xff2b2f37);
    if (shouldDrawButtonAsHighlighted)
        base = base.brighter(0.15f);
    if (shouldDrawButtonAsDown)
        base = base.darker(0.2f);

    juce::ColourGradient grad(base.brighter(0.12f), bounds.getTopLeft(),
                              base.darker(0.15f), bounds.getBottomRight(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(isToggled ? juce::Colour(0xffe74c3c) : juce::Colour(0xff4c535e));
    g.drawRoundedRectangle(bounds, 3.0f, 1.2f);

    // If button has toggle state, draw vintage LED dot in top right
    if (isToggled)
    {
        float dotSize = 5.0f;
        auto dotRect = juce::Rectangle<float>(bounds.getRight() - dotSize - 4.0f,
                                              bounds.getY() + 4.0f,
                                              dotSize, dotSize);
        g.setColour(juce::Colour(0xffff5252));
        g.fillEllipse(dotRect);
        g.setColour(juce::Colour(0x80ffffff));
        g.drawEllipse(dotRect, 0.8f);
    }
}

void VintageLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(juce::Colour(0xff1a1d22));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colour(0xff4a515c));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}
