#include "TransientLookAndFeel.h"
#include <cmath>

const juce::Colour TransientLookAndFeel::bgDark           = juce::Colour(0xff0e1117);
const juce::Colour TransientLookAndFeel::bgCard           = juce::Colour(0xff161b24);
const juce::Colour TransientLookAndFeel::bgCardHeader     = juce::Colour(0xff1d2330);
const juce::Colour TransientLookAndFeel::accentTransient  = juce::Colour(0xff00d4ff); // Bright Cyan
const juce::Colour TransientLookAndFeel::accentSustain    = juce::Colour(0xffff9900); // Warm Orange
const juce::Colour TransientLookAndFeel::accentSplit      = juce::Colour(0xffa855f7); // Electric Purple
const juce::Colour TransientLookAndFeel::textBright       = juce::Colour(0xfff0f4f8);
const juce::Colour TransientLookAndFeel::textMuted        = juce::Colour(0xff8a94a6);
const juce::Colour TransientLookAndFeel::knobTrack        = juce::Colour(0xff252c3a);
const juce::Colour TransientLookAndFeel::borderCol        = juce::Colour(0xff2c3444);

TransientLookAndFeel::TransientLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, bgDark);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxTextColourId, textBright);
    setColour(juce::ComboBox::backgroundColourId, bgCardHeader);
    setColour(juce::ComboBox::outlineColourId, borderCol);
    setColour(juce::ComboBox::textColourId, textBright);
    setColour(juce::PopupMenu::backgroundColourId, bgCard);
    setColour(juce::PopupMenu::textColourId, textBright);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentTransient.withAlpha(0.3f));
    setColour(juce::PopupMenu::highlightedTextColourId, textBright);
}

void TransientLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPosProportional, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    const float radius = static_cast<float>(std::min(width, height)) * 0.5f - 4.0f;
    const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::Colour activeColor = accentTransient;
    if (slider.getProperties().contains("accentColor"))
    {
        activeColor = juce::Colour::fromString(slider.getProperties()["accentColor"].toString());
    }

    // 1. Background circle
    g.setColour(juce::Colour(0xff161922));
    g.fillEllipse(rx, ry, rw, rw);

    g.setColour(borderCol.withAlpha(0.7f));
    g.drawEllipse(rx, ry, rw, rw, 1.5f);

    // 2. Track background arc
    const float arcRadius = radius - 3.5f;
    juce::Path trackBg;
    trackBg.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(knobTrack);
    g.strokePath(trackBg, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Active Value arc with neon glow
    if (sliderPosProportional > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(activeColor.withAlpha(0.35f));
        g.strokePath(valueArc, juce::PathStrokeType(6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(activeColor);
        g.strokePath(valueArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. Inner metallic face
    const float innerRadius = radius - 8.0f;
    juce::ColourGradient grad(juce::Colour(0xff2a303d), centreX, centreY - innerRadius,
                              juce::Colour(0xff181c24), centreX, centreY + innerRadius, false);
    g.setGradientFill(grad);
    g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

    // 5. Dial pointer needle
    juce::Path needle;
    const float needleLen = innerRadius * 0.75f;
    needle.startNewSubPath(centreX + std::sin(angle) * (innerRadius * 0.2f),
                           centreY - std::cos(angle) * (innerRadius * 0.2f));
    needle.lineTo(centreX + std::sin(angle) * needleLen,
                  centreY - std::cos(angle) * needleLen);

    g.setColour(activeColor);
    g.strokePath(needle, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void TransientLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                        int buttonX, int buttonY, int buttonW, int buttonH,
                                        juce::ComboBox& /*box*/)
{
    const auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);

    g.setColour(bgCardHeader);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Arrow indicator
    juce::Path p;
    const float arrowX = static_cast<float>(buttonX + buttonW * 0.5f);
    const float arrowY = static_cast<float>(buttonY + buttonH * 0.5f);
    p.addTriangle(arrowX - 4.0f, arrowY - 2.0f,
                  arrowX + 4.0f, arrowY - 2.0f,
                  arrowX, arrowY + 3.0f);

    g.setColour(textMuted);
    g.fillPath(p);
}

void TransientLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                            bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const bool state = button.getToggleState();
    const bool isBypassToggle = button.getProperties().contains("isBypassToggle");
    const bool isSoloMute     = button.getProperties().contains("isSoloMute");
    const bool isSyncToggle   = button.getProperties().contains("isSyncToggle");

    juce::Colour activeCol = accentTransient;
    if (button.getProperties().contains("accentColor"))
        activeCol = juce::Colour::fromString(button.getProperties()["accentColor"].toString());

    if (isSoloMute)
    {
        g.setColour(state ? activeCol : juce::Colour(0xff161922));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(state ? activeCol.brighter(0.2f) : activeCol.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, 4.0f, state ? 1.5f : 1.0f);

        g.setColour(state ? (activeCol == juce::Colour(0xffffdd00) ? juce::Colours::black : textBright) : activeCol.withAlpha(0.7f));
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
    }
    else if (isSyncToggle)
    {
        g.setColour(state ? activeCol.withAlpha(0.35f) : juce::Colour(0xff161922));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(state ? activeCol : juce::Colour(0xff2a2f3e));
        g.drawRoundedRectangle(bounds, 4.0f, state ? 1.5f : 1.0f);

        g.setColour(state ? textBright : textMuted);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawFittedText(state ? "SYNC" : "FREE", button.getLocalBounds(), juce::Justification::centred, 1);
    }
    else if (isBypassToggle)
    {
        const bool isActive = !state;

        g.setColour(isActive ? activeCol.withAlpha(0.25f) : juce::Colour(0xff161922));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(isActive ? activeCol : juce::Colour(0xff2a2f3e));
        g.drawRoundedRectangle(bounds, 4.0f, isActive ? 1.5f : 1.0f);

        g.setColour(isActive ? textBright : textMuted);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText(isActive ? "ON" : "OFF", button.getLocalBounds(), juce::Justification::centred, 1);
    }
    else
    {
        const bool isActive = state;

        g.setColour(isActive ? activeCol.withAlpha(0.35f) : juce::Colour(0xff161922));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(isActive ? activeCol : juce::Colour(0xff2a2f3e));
        g.drawRoundedRectangle(bounds, 4.0f, isActive ? 1.5f : 1.0f);

        g.setColour(isActive ? textBright : textMuted);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));

        const juce::String text = button.getButtonText().isNotEmpty() ? button.getButtonText() : (isActive ? "ON" : "OFF");
        g.drawFittedText(text, button.getLocalBounds(), juce::Justification::centred, 1);
    }
}

void TransientLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const bool isNav = button.getProperties().contains("isNavTab");
    const bool isActive = button.getProperties().contains("isActiveTab") && (bool)button.getProperties()["isActiveTab"];

    if (isNav)
    {
        juce::Colour activeCol = accentTransient;
        if (button.getProperties().contains("accentColor"))
            activeCol = juce::Colour::fromString(button.getProperties()["accentColor"].toString());

        if (isActive)
        {
            g.setColour(activeCol);
            g.fillRoundedRectangle(bounds, 5.0f);
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(bounds, 5.0f, 1.5f);
        }
        else
        {
            g.setColour(juce::Colour(0xff141822));
            g.fillRoundedRectangle(bounds, 5.0f);
            g.setColour(juce::Colour(0xff262c3d));
            g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
        }
        return;
    }

    juce::Colour c = backgroundColour;
    if (shouldDrawButtonAsDown)
        c = c.brighter(0.2f);
    else if (shouldDrawButtonAsHighlighted)
        c = c.brighter(0.1f);

    g.setColour(c);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void TransientLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const bool isNav = button.getProperties().contains("isNavTab");
    const bool isActive = button.getProperties().contains("isActiveTab") && (bool)button.getProperties()["isActiveTab"];

    if (isNav)
    {
        g.setColour(isActive ? juce::Colour(0xff090b10) : textMuted);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
        return;
    }

    juce::LookAndFeel_V4::drawButtonText(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}
