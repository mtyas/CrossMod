#include "MidiMonitorComponent.h"

namespace MidiFlux
{

MidiMonitorComponent::MidiMonitorComponent(MidiChainProcessor& chain)
    : chainProcessor(chain)
{
    inputGlow.fill(0.0f);
    outputGlow.fill(0.0f);
    startTimerHz(30);
}

MidiMonitorComponent::~MidiMonitorComponent()
{
    stopTimer();
}

void MidiMonitorComponent::timerCallback()
{
    std::vector<MidiActivityEvent> events;
    int count = chainProcessor.getPendingActivityEvents(events);

    for (const auto& ev : events)
    {
        int n = juce::jlimit(0, 127, ev.noteNumber);
        if (ev.isOutput)
        {
            if (ev.isNoteOn) { outputGlow[n] = 1.0f; outNoteCount++; }
        }
        else
        {
            if (ev.isNoteOn) { inputGlow[n] = 1.0f; inNoteCount++; }
        }
    }

    // Decay glow
    bool needsRepaint = (count > 0);
    for (int i = 0; i < 128; ++i)
    {
        if (inputGlow[i] > 0.05f) { inputGlow[i] *= 0.85f; needsRepaint = true; }
        else inputGlow[i] = 0.0f;

        if (outputGlow[i] > 0.05f) { outputGlow[i] *= 0.85f; needsRepaint = true; }
        else outputGlow[i] = 0.0f;
    }

    if (needsRepaint)
        repaint();
}

void MidiMonitorComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark bar background
    g.setColour(juce::Colour(0xff161b22));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(0xff30363d));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Labels
    g.setFont(juce::FontOptions("Segoe UI", 10.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff00e5ff));
    g.drawText("IN", 8, 2, 24, (int)bounds.getHeight() / 2, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffa855f7));
    g.drawText("OUT", 8, (int)bounds.getHeight() / 2, 24, (int)bounds.getHeight() / 2, juce::Justification::centredLeft);

    float startX = 36.0f;
    float availW = bounds.getWidth() - startX - 10.0f;
    float slotW = availW / 128.0f;
    float halfH = bounds.getHeight() * 0.5f;

    // Draw input activity line
    for (int n = 0; n < 128; ++n)
    {
        float x = startX + n * slotW;
        if (inputGlow[n] > 0.01f)
        {
            g.setColour(juce::Colour(0xff00e5ff).withAlpha(inputGlow[n]));
            g.fillRect(x, 3.0f, std::max(1.5f, slotW), halfH - 4.0f);
        }

        if (outputGlow[n] > 0.01f)
        {
            g.setColour(juce::Colour(0xffec4899).withAlpha(outputGlow[n]));
            g.fillRect(x, halfH + 1.0f, std::max(1.5f, slotW), halfH - 4.0f);
        }
    }
}

void MidiMonitorComponent::resized()
{
}

} // namespace MidiFlux
