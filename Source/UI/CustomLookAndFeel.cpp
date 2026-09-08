#include "CustomLookAndFeel.h"
#include <cmath>

namespace MidiFlux
{

CustomLookAndFeel::CustomLookAndFeel()
{
    setDefaultSansSerifTypefaceName("Segoe UI");

    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0d1117));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff161b22));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xfff0f6fc));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff30363d));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff00e5ff));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff161b22));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6edf3));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff21262d));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff00e5ff));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe6edf3));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, juce::Colour(0xffe6edf3));
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.5f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto center = bounds.getCentre();

    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    float trackWidth = 4.5f;

    auto accentColor = slider.findColour(juce::Slider::rotarySliderFillColourId);
    if (accentColor.isTransparent())
        accentColor = juce::Colour(0xff00e5ff);

    // 1. Tick marks/dots around the perimeter at 0%, 25%, 50%, 75%, 100%
    float tickTrackRadius = radius - trackWidth * 0.5f;
    for (int i = 0; i <= 4; ++i)
    {
        float frac = static_cast<float>(i) / 4.0f;
        float angle = rotaryStartAngle + frac * (rotaryEndAngle - rotaryStartAngle);
        auto pt = center.getPointOnCircumference(tickTrackRadius + trackWidth * 0.5f + 3.0f, angle);
        bool isHit = (toAngle >= angle - 0.03f);
        g.setColour(isHit ? accentColor.brighter(0.4f) : juce::Colour(0xff8b949e));
        g.fillEllipse(pt.x - 1.25f, pt.y - 1.25f, 2.5f, 2.5f);
    }

    // 2. Background track groove (crisp, high-contrast dark slate rim)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(center.x, center.y, tickTrackRadius, tickTrackRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
    // Outer groove boundary
    g.setColour(juce::Colour(0xff3d444d));
    g.strokePath(backgroundArc, juce::PathStrokeType(trackWidth + 1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    // Inner track bed
    g.setColour(juce::Colour(0xff161b22));
    g.strokePath(backgroundArc, juce::PathStrokeType(trackWidth - 1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Foreground active value arc with luminous glow
    if (toAngle > rotaryStartAngle + 0.005f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(center.x, center.y, tickTrackRadius, tickTrackRadius,
                               0.0f, rotaryStartAngle, toAngle, true);

        // Ambient neon aura
        g.setColour(accentColor.withAlpha(0.30f));
        g.strokePath(valueArc, juce::PathStrokeType(trackWidth + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Core luminous line
        g.setColour(accentColor.brighter(0.2f));
        g.strokePath(valueArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. Elevated tactile metallic knob cap
    float knobRadius = radius - trackWidth - 3.5f;
    if (knobRadius > 4.0f)
    {
        // Soft outer drop shadow
        g.setColour(juce::Colour(0x66000000));
        g.fillEllipse(center.x - knobRadius, center.y - knobRadius + 1.5f, knobRadius * 2.0f, knobRadius * 2.0f);

        // Brushed metallic gradient from top-left to bottom-right
        juce::ColourGradient knobGrad(juce::Colour(0xff384252), center.x - knobRadius * 0.7f, center.y - knobRadius * 0.7f,
                                      juce::Colour(0xff212733), center.x + knobRadius * 0.7f, center.y + knobRadius * 0.7f, false);
        g.setGradientFill(knobGrad);
        g.fillEllipse(center.x - knobRadius, center.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

        // Crisp highlight bevel rim
        g.setColour(juce::Colour(0xff5b687a));
        g.drawEllipse(center.x - knobRadius, center.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.4f);

        // Subtle concentric groove inside
        float innerGrooveR = knobRadius * 0.65f;
        g.setColour(juce::Colour(0xff181e28).withAlpha(0.6f));
        g.drawEllipse(center.x - innerGrooveR, center.y - innerGrooveR, innerGrooveR * 2.0f, innerGrooveR * 2.0f, 1.0f);

        // 5. High-contrast indicator needle with white core + illuminated pip
        float thumbLength = knobRadius * 0.78f;
        auto thumbPoint = center.getPointOnCircumference(thumbLength, toAngle);
        auto innerPoint = center.getPointOnCircumference(knobRadius * 0.18f, toAngle);

        // Needle accent shadow / glow
        g.setColour(accentColor.withAlpha(0.6f));
        g.drawLine(innerPoint.x, innerPoint.y, thumbPoint.x, thumbPoint.y, 4.0f);

        // Needle sharp white core
        g.setColour(juce::Colour(0xffffffff));
        g.drawLine(innerPoint.x, innerPoint.y, thumbPoint.x, thumbPoint.y, 2.0f);

        // Illuminated pointer pip at knob edge
        auto pipPoint = center.getPointOnCircumference(knobRadius * 0.72f, toAngle);
        g.setColour(accentColor);
        g.fillEllipse(pipPoint.x - 3.2f, pipPoint.y - 3.2f, 6.4f, 6.4f);
        g.setColour(juce::Colour(0xffffffff));
        g.fillEllipse(pipPoint.x - 1.8f, pipPoint.y - 1.8f, 3.6f, 3.6f);
    }
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float, float,
                                         const juce::Slider::SliderStyle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    float trackHeight = 4.0f;
    float trackY = bounds.getCentreY() - trackHeight * 0.5f;

    g.setColour(juce::Colour(0xff21262d));
    g.fillRoundedRectangle(bounds.getX(), trackY, bounds.getWidth(), trackHeight, 2.0f);

    auto accentColor = slider.findColour(juce::Slider::thumbColourId);
    if (accentColor.isTransparent())
        accentColor = juce::Colour(0xff00e5ff);

    g.setColour(accentColor);
    g.fillRoundedRectangle(bounds.getX(), trackY, sliderPos - bounds.getX(), trackHeight, 2.0f);

    float thumbW = 12.0f;
    float thumbH = 14.0f;
    auto thumbRect = juce::Rectangle<float>(sliderPos - thumbW * 0.5f, bounds.getCentreY() - thumbH * 0.5f, thumbW, thumbH);

    g.setColour(accentColor.withAlpha(0.35f));
    g.fillRoundedRectangle(thumbRect.expanded(2.0f), 3.0f);

    g.setColour(juce::Colour(0xfff0f6fc));
    g.fillRoundedRectangle(thumbRect, 2.5f);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto bg = backgroundColour;

    if (shouldDrawButtonAsDown)
        bg = bg.brighter(0.25f);
    else if (shouldDrawButtonAsHighlighted)
        bg = bg.brighter(0.12f);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 5.0f);

    auto borderCol = shouldDrawButtonAsHighlighted ? juce::Colour(0xff58a6ff) : juce::Colour(0xff30363d);
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);
}

void CustomLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto text = button.getButtonText();
    auto color = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId);

    if (shouldDrawButtonAsHighlighted)
        color = color.brighter(0.2f);

    auto center = bounds.getCentre();

    // 1. Vector Undo Icon
    if (text == "UNDO" || text == "undo")
    {
        g.setColour(color);
        juce::Path p;
        p.addCentredArc(center.x + 1.0f, center.y + 1.0f, 5.0f, 5.0f, 0.0f, -0.4f, 2.8f, true);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // arrow tip
        juce::Path tip;
        tip.addTriangle(center.x - 4.5f, center.y - 1.0f, center.x - 1.0f, center.y - 4.5f, center.x - 1.0f, center.y + 2.5f);
        g.fillPath(tip);
        return;
    }

    // 2. Vector Redo Icon
    if (text == "REDO" || text == "redo")
    {
        g.setColour(color);
        juce::Path p;
        p.addCentredArc(center.x - 1.0f, center.y + 1.0f, 5.0f, 5.0f, 0.0f, -2.8f, 0.4f, true);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path tip;
        tip.addTriangle(center.x + 4.5f, center.y - 1.0f, center.x + 1.0f, center.y - 4.5f, center.x + 1.0f, center.y + 2.5f);
        g.fillPath(tip);
        return;
    }

    // 3. Vector Dice Icon
    if (text == "DICE" || text == "dice")
    {
        g.setColour(color);
        auto diceRect = juce::Rectangle<float>(center.x - 6.0f, center.y - 6.0f, 12.0f, 12.0f);
        g.drawRoundedRectangle(diceRect, 2.5f, 1.4f);
        // pips
        g.fillEllipse(center.x - 3.5f, center.y - 3.5f, 2.0f, 2.0f);
        g.fillEllipse(center.x + 1.5f, center.y + 1.5f, 2.0f, 2.0f);
        g.fillEllipse(center.x - 1.0f, center.y - 1.0f, 2.0f, 2.0f);
        return;
    }

    // 4. Vector Left Arrow
    if (text == "<" || text == "left")
    {
        g.setColour(color);
        juce::Path tip;
        tip.addTriangle(center.x - 4.0f, center.y, center.x + 3.0f, center.y - 4.5f, center.x + 3.0f, center.y + 4.5f);
        g.fillPath(tip);
        return;
    }

    // 5. Vector Right Arrow
    if (text == ">" || text == "right")
    {
        g.setColour(color);
        juce::Path tip;
        tip.addTriangle(center.x + 4.0f, center.y, center.x - 3.0f, center.y - 4.5f, center.x - 3.0f, center.y + 4.5f);
        g.fillPath(tip);
        return;
    }

    // 6. Vector Close X
    if (text == "X" || text == "close")
    {
        g.setColour(color);
        float sz = 4.0f;
        g.drawLine(center.x - sz, center.y - sz, center.x + sz, center.y + sz, 1.8f);
        g.drawLine(center.x - sz, center.y + sz, center.x + sz, center.y - sz, 1.8f);
        return;
    }

    // Default clean typography
    g.setColour(color);
    g.setFont(juce::FontOptions("Segoe UI", 11.5f, juce::Font::bold));
    g.drawText(text, bounds, juce::Justification::centred, true);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    g.setColour(juce::Colour(0xff161b22));
    g.fillRoundedRectangle(bounds, 4.5f);

    g.setColour(box.hasKeyboardFocus(true) ? juce::Colour(0xff00e5ff) : juce::Colour(0xff30363d));
    g.drawRoundedRectangle(bounds, 4.5f, 1.0f);

    // Clean vector dropdown arrow
    float arrowX = width - 15.0f;
    float arrowY = height * 0.5f - 2.0f;
    juce::Path p;
    p.addTriangle(arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f, arrowY + 5.0f);
    g.setColour(juce::Colour(0xff8b949e));
    g.fillPath(p);
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    float fontHeight = juce::jlimit(11.0f, 13.5f, box.getHeight() * 0.58f);
    return juce::FontOptions("Segoe UI", fontHeight, juce::Font::bold);
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(3, 1, box.getWidth() - 18, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.setColour(juce::Colour(0xff161b22));
    g.fillRoundedRectangle(0, 0, (float)width, (float)height, 6.0f);

    g.setColour(juce::Colour(0xff30363d));
    g.drawRoundedRectangle(0, 0, (float)width, (float)height, 6.0f, 1.0f);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool isSeparator, bool isActive, bool isHighlighted,
                                        bool isTicked, bool,
                                        const juce::String& text, const juce::String&,
                                        const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour(juce::Colour(0xff30363d));
        g.fillRect(area.reduced(6, 0).withHeight(1));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(juce::Colour(0xff21262d));
        g.fillRoundedRectangle(area.toFloat().reduced(2.0f), 4.0f);
    }

    g.setColour(isHighlighted ? juce::Colour(0xff00e5ff) : (isActive ? juce::Colour(0xffe6edf3) : juce::Colour(0xff8b949e)));
    g.setFont(juce::FontOptions("Segoe UI", 12.5f, juce::Font::plain));
    g.drawText(text, area.reduced(10, 0), juce::Justification::centredLeft, true);

    if (isTicked)
    {
        g.setColour(juce::Colour(0xff00e5ff));
        // draw vector checkmark
        float cx = area.getRight() - 14.0f;
        float cy = area.getCentreY();
        juce::Path p;
        p.startNewSubPath(cx - 4.0f, cy);
        p.lineTo(cx - 1.0f, cy + 3.5f);
        p.lineTo(cx + 4.0f, cy - 3.5f);
        g.strokePath(p, juce::PathStrokeType(1.8f));
    }
}

void CustomLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&,
                                      int x, int y, int width, int height,
                                      bool, int thumbStartPosition,
                                      int thumbSize, bool, bool)
{
    g.setColour(juce::Colour(0xff161b22));
    g.fillRect(x, y, width, height);

    g.setColour(juce::Colour(0xff30363d));
    if (width > height)
    {
        g.fillRoundedRectangle((float)thumbStartPosition, (float)y + 2.0f, (float)thumbSize, (float)height - 4.0f, 3.0f);
    }
    else
    {
        g.fillRoundedRectangle((float)x + 2.0f, (float)thumbStartPosition, (float)width - 4.0f, (float)thumbSize, 3.0f);
    }
}

} // namespace MidiFlux
