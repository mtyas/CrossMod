#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Engine/MidiChainProcessor.h"
#include <array>

namespace MidiFlux
{

class MidiMonitorComponent : public juce::Component, public juce::Timer
{
public:
    MidiMonitorComponent(MidiChainProcessor& chain);
    ~MidiMonitorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    MidiChainProcessor& chainProcessor;

    // Track active pitch states (0.0 to 1.0 glow intensity)
    std::array<float, 128> inputGlow;
    std::array<float, 128> outputGlow;

    int inNoteCount = 0;
    int outNoteCount = 0;
};

} // namespace MidiFlux
