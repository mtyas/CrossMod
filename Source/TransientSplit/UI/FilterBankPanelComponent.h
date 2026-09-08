#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/TransientSplitEngine.h"
#include "../Parameters/ParameterIDs.h"
#include "TransientLookAndFeel.h"
#include <array>

class FilterBankPanelComponent : public juce::Component,
                                 private juce::Timer
{
public:
    FilterBankPanelComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse);
    ~FilterBankPanelComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void paintFrequencyResponse(juce::Graphics& g, juce::Rectangle<float> bounds);

    juce::AudioProcessorValueTreeState& apvts;
    TransientSplitEngine& engine;

    struct FilterStrip
    {
        juce::Label titleLabel;
        juce::ComboBox typeCombo;
        juce::ToggleButton bypassButton { "ON" };
        juce::ToggleButton soloButton { "SOLO" };
        juce::ToggleButton muteButton { "MUTE" };

        juce::Slider freqSlider;
        juce::Label freqLabel;

        juce::Slider qSlider;
        juce::Label qLabel;

        juce::Slider gainSlider;
        juce::Label gainLabel;

        juce::Slider driveSlider;
        juce::Label driveLabel;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    };

    std::array<FilterStrip, 3> strips;
    juce::Rectangle<int> curveBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterBankPanelComponent)
};
