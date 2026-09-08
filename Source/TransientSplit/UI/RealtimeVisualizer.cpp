#include "RealtimeVisualizer.h"
#include "TransientLookAndFeel.h"

RealtimeVisualizer::RealtimeVisualizer(TransientSplitEngine& engineToUse)
    : engine(engineToUse)
{
    snapshot.assign(TransientSplitEngine::VisualizerBufferSize, { 0.0f, 0.0f, 0.0f, 0.0f });
    startTimerHz(30);
}

RealtimeVisualizer::~RealtimeVisualizer()
{
    stopTimer();
}

void RealtimeVisualizer::timerCallback()
{
    engine.getVisualizerSnapshot(snapshot);

    const float decay = 0.85f;
    smoothTransMeter = std::max(engine.getTransMeterLevel(), smoothTransMeter * decay);
    smoothSustMeter = std::max(engine.getSustMeterLevel(), smoothSustMeter * decay);

    repaint();
}

void RealtimeVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(juce::Colour(0xff0d0f14));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(TransientLookAndFeel::borderCol);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Reserve right 50px for the Transient & Sustain meters
    auto meterArea = bounds.removeFromRight(55.0f).reduced(4.0f);
    auto scopeArea = bounds.reduced(6.0f);

    // Grid lines
    g.setColour(juce::Colour(0xff1a1e28));
    const float midY = scopeArea.getCentreY();
    g.drawHorizontalLine(static_cast<int>(midY), scopeArea.getX(), scopeArea.getRight());
    g.drawHorizontalLine(static_cast<int>(midY - scopeArea.getHeight() * 0.25f), scopeArea.getX(), scopeArea.getRight());
    g.drawHorizontalLine(static_cast<int>(midY + scopeArea.getHeight() * 0.25f), scopeArea.getX(), scopeArea.getRight());

    // Draw Waveforms
    if (!snapshot.empty())
    {
        const int numPoints = static_cast<int>(snapshot.size());
        const float xStep = scopeArea.getWidth() / static_cast<float>(numPoints);
        const float ampScale = scopeArea.getHeight() * 0.45f;

        juce::Path dryPath;
        juce::Path transPath;
        juce::Path sustPath;

        for (int i = 0; i < numPoints; ++i)
        {
            const float px = scopeArea.getX() + static_cast<float>(i) * xStep;
            const float pyDry = midY - snapshot[i].input * ampScale;
            const float pyTrans = midY - snapshot[i].transient * ampScale;
            const float pySust = midY - snapshot[i].sustain * ampScale;

            if (i == 0)
            {
                dryPath.startNewSubPath(px, pyDry);
                transPath.startNewSubPath(px, pyTrans);
                sustPath.startNewSubPath(px, pySust);
            }
            else
            {
                dryPath.lineTo(px, pyDry);
                transPath.lineTo(px, pyTrans);
                sustPath.lineTo(px, pySust);
            }
        }

        // Render Dry Input (Subtle slate)
        g.setColour(juce::Colour(0x35ffffff));
        g.strokePath(dryPath, juce::PathStrokeType(1.0f));

        // Render Sustain Waveform (Glowing Cyan)
        g.setColour(TransientLookAndFeel::accentSustain.withAlpha(0.2f));
        g.strokePath(sustPath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved));
        g.setColour(TransientLookAndFeel::accentSustain);
        g.strokePath(sustPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));

        // Render Transient Waveform (Glowing Amber/Orange)
        g.setColour(TransientLookAndFeel::accentTransient.withAlpha(0.25f));
        g.strokePath(transPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved));
        g.setColour(TransientLookAndFeel::accentTransient);
        g.strokePath(transPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
    }

    // Legend in Top-Left of oscilloscope
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));

    g.setColour(TransientLookAndFeel::accentTransient);
    g.fillEllipse(scopeArea.getX() + 8.0f, scopeArea.getY() + 8.0f, 6.0f, 6.0f);
    g.drawText("TRANSIENT", static_cast<int>(scopeArea.getX() + 18.0f), static_cast<int>(scopeArea.getY() + 4.0f), 70, 14, juce::Justification::left);

    g.setColour(TransientLookAndFeel::accentSustain);
    g.fillEllipse(scopeArea.getX() + 95.0f, scopeArea.getY() + 8.0f, 6.0f, 6.0f);
    g.drawText("SUSTAIN", static_cast<int>(scopeArea.getX() + 105.0f), static_cast<int>(scopeArea.getY() + 4.0f), 60, 14, juce::Justification::left);

    // Energy Meters (Right Side)
    auto transMeterArea = meterArea.removeFromLeft(meterArea.getWidth() * 0.48f);
    auto sustMeterArea  = meterArea;

    // Transient Meter Bar
    g.setColour(juce::Colour(0xff161922));
    g.fillRoundedRectangle(transMeterArea, 3.0f);
    const float tH = transMeterArea.getHeight() * std::clamp(smoothTransMeter, 0.0f, 1.0f);
    auto tFill = transMeterArea.removeFromBottom(tH);
    g.setColour(TransientLookAndFeel::accentTransient);
    g.fillRoundedRectangle(tFill, 3.0f);

    // Sustain Meter Bar
    g.setColour(juce::Colour(0xff161922));
    g.fillRoundedRectangle(sustMeterArea, 3.0f);
    const float sH = sustMeterArea.getHeight() * std::clamp(smoothSustMeter, 0.0f, 1.0f);
    auto sFill = sustMeterArea.removeFromBottom(sH);
    g.setColour(TransientLookAndFeel::accentSustain);
    g.fillRoundedRectangle(sFill, 3.0f);

    // Meter labels
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.setColour(TransientLookAndFeel::textMuted);
    g.drawText("T", transMeterArea.getX(), transMeterArea.getY() - 12, transMeterArea.getWidth(), 12, juce::Justification::centred);
    g.drawText("S", sustMeterArea.getX(), sustMeterArea.getY() - 12, sustMeterArea.getWidth(), 12, juce::Justification::centred);
}

void RealtimeVisualizer::resized()
{
}
