#include "XYPadComponent.h"
#include "TransientLookAndFeel.h"
#include <cmath>

XYPadComponent::XYPadComponent(juce::AudioProcessorValueTreeState& state, int padIndex)
    : apvts(state), padIdx(padIndex)
{
    const juce::Colour accent = (padIdx == 1) ? juce::Colour(0xffb800ff) : juce::Colour(0xff06b6d4);

    // Header Title
    titleLabel.setText("XY PAD " + juce::String(padIdx) + " & LOOPER", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, accent);
    addAndMakeVisible(titleLabel);

    // Coordinates Readout Label
    coordsLabel.setText("X: 0.00 | Y: 0.00", juce::dontSendNotification);
    coordsLabel.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    coordsLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textBright);
    coordsLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(coordsLabel);

    // REC Button
    recButton.setClickingTogglesState(true);
    recButton.getProperties().set("accentColor", juce::Colour(0xffff3b30).toString());
    recButton.onClick = [this]()
    {
        isRecording = recButton.getToggleState();
        if (isRecording)
        {
            recordedMotion.clear();
            loopPlaybackIndex = 0.0f;
            if (loopButton.getToggleState())
                loopButton.setToggleState(false, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(recButton);

    // LOOP Button
    loopButton.setClickingTogglesState(true);
    loopButton.getProperties().set("accentColor", juce::Colour(0xff00d4ff).toString());
    loopButton.onClick = [this]()
    {
        isLooping = loopButton.getToggleState();
        if (isLooping && isRecording)
        {
            isRecording = false;
            recButton.setToggleState(false, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(loopButton);

    // CLEAR Button
    clearButton.getProperties().set("accentColor", juce::Colour(0xff8a94a6).toString());
    clearButton.onClick = [this]()
    {
        recordedMotion.clear();
        loopPlaybackIndex = 0.0f;
        recButton.setToggleState(false, juce::dontSendNotification);
        loopButton.setToggleState(false, juce::dontSendNotification);
        isRecording = false;
        isLooping = false;
        setXYValues(0.0f, 0.0f);
        repaint();
    };
    addAndMakeVisible(clearButton);

    // Speed Slider
    speedSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    speedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 12);
    speedSlider.setNumDecimalPlacesToDisplay(2);
    speedSlider.setTextValueSuffix("x");
    speedSlider.getProperties().set("accentColor", accent.toString());
    speedSlider.setDoubleClickReturnValue(true, 1.0);
    addAndMakeVisible(speedSlider);

    speedLabel.setText("Speed", juce::dontSendNotification);
    speedLabel.setFont(juce::FontOptions(8.5f, juce::Font::plain));
    speedLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
    speedLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(speedLabel);

    const auto spdId = (padIdx == 1) ? IDs::xy1_speed.getParamID() : IDs::xy2_speed.getParamID();
    if (apvts.getParameter(spdId) != nullptr)
        speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, spdId, speedSlider);

    startTimerHz(30);
}

XYPadComponent::~XYPadComponent()
{
    stopTimer();
}

void XYPadComponent::setXYValues(float x, float y)
{
    currentX = std::clamp(x, -1.0f, 1.0f);
    currentY = std::clamp(y, -1.0f, 1.0f);

    const auto xId = (padIdx == 1) ? IDs::xy1_x.getParamID() : IDs::xy2_x.getParamID();
    const auto yId = (padIdx == 1) ? IDs::xy1_y.getParamID() : IDs::xy2_y.getParamID();

    if (auto* rawX = apvts.getRawParameterValue(xId))
        rawX->store(currentX);

    if (auto* rawY = apvts.getRawParameterValue(yId))
        rawY->store(currentY);

    if (isDragging)
    {
        if (auto* pX = apvts.getParameter(xId))
            pX->setValueNotifyingHost(pX->convertTo0to1(currentX));

        if (auto* pY = apvts.getParameter(yId))
            pY->setValueNotifyingHost(pY->convertTo0to1(currentY));
    }

    coordsLabel.setText("X: " + juce::String(currentX, 2) + " | Y: " + juce::String(currentY, 2),
                        juce::dontSendNotification);
}

void XYPadComponent::updatePointFromMouse(const juce::MouseEvent& e)
{
    if (padBounds.isEmpty()) return;

    const float px = static_cast<float>(e.x - padBounds.getX()) / static_cast<float>(padBounds.getWidth());
    const float py = static_cast<float>(e.y - padBounds.getY()) / static_cast<float>(padBounds.getHeight());

    const float mappedX = std::clamp(px * 2.0f - 1.0f, -1.0f, 1.0f);
    const float mappedY = std::clamp((1.0f - py) * 2.0f - 1.0f, -1.0f, 1.0f);

    setXYValues(mappedX, mappedY);

    if (isRecording && recordedMotion.size() < MaxRecordedPoints)
    {
        recordedMotion.push_back({ currentX, currentY });
    }
}

void XYPadComponent::mouseDown(const juce::MouseEvent& e)
{
    if (padBounds.contains(e.getPosition()))
    {
        isDragging = true;
        updatePointFromMouse(e);
        repaint();
    }
}

void XYPadComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        updatePointFromMouse(e);
        repaint();
    }
}

void XYPadComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    isDragging = false;
    if (isRecording && recordedMotion.size() > 10)
    {
        isRecording = false;
        recButton.setToggleState(false, juce::dontSendNotification);
        isLooping = true;
        loopButton.setToggleState(true, juce::dontSendNotification);
    }
}

void XYPadComponent::timerCallback()
{
    if (isLooping && !recordedMotion.empty() && !isDragging)
    {
        const float spd = static_cast<float>(speedSlider.getValue());
        loopPlaybackIndex += spd;

        if (loopPlaybackIndex >= static_cast<float>(recordedMotion.size()))
            loopPlaybackIndex = 0.0f;

        const int idx0 = static_cast<int>(loopPlaybackIndex);
        const int idx1 = (idx0 + 1) % recordedMotion.size();
        const float frac = loopPlaybackIndex - static_cast<float>(idx0);

        const auto& p0 = recordedMotion[idx0];
        const auto& p1 = recordedMotion[idx1];

        const float interpX = p0.x + (p1.x - p0.x) * frac;
        const float interpY = p0.y + (p1.y - p0.y) * frac;

        setXYValues(interpX, interpY);
        repaint();
    }
    else if (!isDragging)
    {
        const auto xId = (padIdx == 1) ? IDs::xy1_x.getParamID() : IDs::xy2_x.getParamID();
        const auto yId = (padIdx == 1) ? IDs::xy1_y.getParamID() : IDs::xy2_y.getParamID();

        auto* rawX = apvts.getRawParameterValue(xId);
        auto* rawY = apvts.getRawParameterValue(yId);

        if (rawX != nullptr && rawY != nullptr)
        {
            const float apvtsX = rawX->load();
            const float apvtsY = rawY->load();

            if (std::abs(apvtsX - currentX) > 0.01f || std::abs(apvtsY - currentY) > 0.01f)
            {
                currentX = apvtsX;
                currentY = apvtsY;
                coordsLabel.setText("X: " + juce::String(currentX, 2) + " | Y: " + juce::String(currentY, 2),
                                    juce::dontSendNotification);
                repaint();
            }
        }
    }
}

void XYPadComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const juce::Colour accent = (padIdx == 1) ? juce::Colour(0xffb800ff) : juce::Colour(0xff06b6d4);

    g.setColour(juce::Colour(0xff12141a));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(juce::Colour(0xff222736));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    if (padBounds.isEmpty()) return;

    auto pBounds = padBounds.toFloat();

    g.setColour(juce::Colour(0xff0a0c10));
    g.fillRoundedRectangle(pBounds, 4.0f);

    g.setColour(juce::Colour(0xff1a1e28));
    g.drawRoundedRectangle(pBounds, 4.0f, 1.0f);

    // Crosshairs
    const float midX = pBounds.getCentreX();
    const float midY = pBounds.getCentreY();
    const float dashes[2] = { 4.0f, 4.0f };
    g.setColour(juce::Colour(0xff2a3040));
    g.drawDashedLine(juce::Line<float>(pBounds.getX(), midY, pBounds.getRight(), midY), dashes, 2, 1.0f);
    g.drawDashedLine(juce::Line<float>(midX, pBounds.getY(), midX, pBounds.getBottom()), dashes, 2, 1.0f);

    // Draw Motion Trail
    if (recordedMotion.size() > 1)
    {
        juce::Path trail;
        auto getCoord = [this, pBounds](const XYMotionPoint& pt) -> juce::Point<float>
        {
            const float normX = (pt.x + 1.0f) * 0.5f;
            const float normY = (1.0f - pt.y) * 0.5f;
            return { pBounds.getX() + normX * pBounds.getWidth(),
                     pBounds.getY() + normY * pBounds.getHeight() };
        };

        trail.startNewSubPath(getCoord(recordedMotion[0]));
        for (size_t i = 1; i < recordedMotion.size(); ++i)
            trail.lineTo(getCoord(recordedMotion[i]));

        if (isLooping)
            trail.closeSubPath();

        g.setColour(accent.withAlpha(0.35f));
        g.strokePath(trail, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Draw Puck
    const float normX = (currentX + 1.0f) * 0.5f;
    const float normY = (1.0f - currentY) * 0.5f;
    const float puckX = pBounds.getX() + normX * pBounds.getWidth();
    const float puckY = pBounds.getY() + normY * pBounds.getHeight();

    // Puck Outer Glow
    g.setColour(accent.withAlpha(0.4f));
    g.fillEllipse(puckX - 12.0f, puckY - 12.0f, 24.0f, 24.0f);

    // Puck Body
    g.setColour(accent);
    g.fillEllipse(puckX - 6.0f, puckY - 6.0f, 12.0f, 12.0f);

    g.setColour(juce::Colours::white);
    g.fillEllipse(puckX - 2.5f, puckY - 2.5f, 5.0f, 5.0f);
}

void XYPadComponent::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Header
    auto header = area.removeFromTop(20);
    titleLabel.setBounds(header.removeFromLeft(header.getWidth() / 2));
    coordsLabel.setBounds(header);

    area.removeFromTop(4);

    // Bottom Controls Bar (REC, LOOP, CLR, Speed)
    auto controls = area.removeFromBottom(42);
    const int btnW = 38;

    recButton.setBounds(controls.removeFromLeft(btnW).reduced(1, 8));
    controls.removeFromLeft(2);
    loopButton.setBounds(controls.removeFromLeft(btnW + 4).reduced(1, 8));
    controls.removeFromLeft(2);
    clearButton.setBounds(controls.removeFromLeft(btnW).reduced(1, 8));

    controls.removeFromLeft(6);
    auto speedArea = controls.removeFromRight(54);
    speedLabel.setBounds(speedArea.removeFromBottom(12));
    speedSlider.setBounds(speedArea);

    area.removeFromBottom(4);
    padBounds = area;
}
