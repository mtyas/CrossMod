#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/TransientSplitEngine.h"

class RealtimeVisualizer : public juce::Component,
                           private juce::Timer
{
public:
    explicit RealtimeVisualizer(TransientSplitEngine& engineToUse);
    ~RealtimeVisualizer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    TransientSplitEngine& engine;
    std::vector<VisualizerPoint> snapshot;

    float smoothTransMeter = 0.0f;
    float smoothSustMeter = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealtimeVisualizer)
};
