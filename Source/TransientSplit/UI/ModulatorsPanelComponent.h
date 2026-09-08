#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/TransientSplitEngine.h"
#include "../Parameters/ParameterIDs.h"
#include "XYPadComponent.h"
#include <vector>

class ModulatorsPanelComponent : public juce::Component,
                                 private juce::Timer
{
public:
    ModulatorsPanelComponent(juce::AudioProcessorValueTreeState& apvts, TransientSplitEngine& engineToUse);
    ~ModulatorsPanelComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    TransientSplitEngine& engine;

    // Card Bounding Rectangles
    std::array<juce::Rectangle<int>, 3> lfoCards;
    std::array<juce::Rectangle<int>, 3> envCards;
    std::array<juce::Rectangle<int>, 2> turingCards;
    juce::Rectangle<int> macroCard;
    std::array<juce::Rectangle<int>, 2> xyCards;

    // Visual Meter Bounds
    std::array<juce::Rectangle<int>, 3> lfoScopes;
    std::array<juce::Rectangle<int>, 3> lfoMeters;
    std::array<juce::Rectangle<int>, 3> envMeters;
    std::array<juce::Rectangle<int>, 2> turingGrids;
    std::array<juce::Rectangle<int>, 2> turingMeters;
    std::array<juce::Rectangle<int>, 4> macroMeters;

    // 3 LFOs Controls
    struct LfoUI
    {
        juce::Label title;
        juce::ComboBox waveCombo;
        juce::Slider rateSlider;
        juce::Label rateLabel;
        juce::ToggleButton syncToggle { "SYNC" };
        juce::ComboBox subdivCombo;
        juce::Slider smoothSlider;
        juce::Label smoothLabel;
    };
    std::array<LfoUI, 3> lfos;

    // 3 Envelope Followers Controls
    struct EnvUI
    {
        juce::Label title;
        juce::ComboBox srcCombo;
        juce::Slider attSlider, relSlider, gainSlider;
        juce::Label attLabel, relLabel, gainLabel;
    };
    std::array<EnvUI, 3> envs;

    // 2 Turing Machines Controls
    struct TuringUI
    {
        juce::Label title;
        juce::Slider probSlider;
        juce::Label probLabel;
        juce::ComboBox lenCombo;
        juce::ToggleButton syncToggle { "SYNC" };
        juce::ComboBox subdivCombo;
        juce::Slider glideSlider;
        juce::Label glideLabel;
    };
    std::array<TuringUI, 2> turings;

    // 4 Macros
    juce::Label macroTitle;
    std::array<juce::Slider, 4> macroSliders;
    std::array<juce::Label, 4> macroLabels;

    // 2 XY Pads
    XYPadComponent xyPad1;
    XYPadComponent xyPad2;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorsPanelComponent)
};
