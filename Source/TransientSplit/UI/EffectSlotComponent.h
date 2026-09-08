#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/EffectChain.h"
#include "../DSP/TransientSplitEngine.h"
#include "../Parameters/ParameterIDs.h"
#include <array>

class EffectSlotComponent : public juce::Component,
                            public juce::AudioProcessorValueTreeState::Listener
{
public:
    EffectSlotComponent(juce::AudioProcessorValueTreeState& apvts, TransientSplitEngine& engineToUse, int slotIndex);
    ~EffectSlotComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateParamLabels();
    void updateRouteChoices();
    void updateInputSourceChoices();

    static juce::Colour getCategoryColour(EffectType type);

    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    TransientSplitEngine& engine;
    int globalSlotIdx; // 0..7

    juce::String srcParamID;
    juce::String typeParamID;
    juce::String bypassParamID;
    juce::String destParamID;
    juce::String inGainParamID;
    juce::String outGainParamID;
    juce::String p1ParamID, p2ParamID, p3ParamID, p4ParamID, p5ParamID, mixParamID;

    juce::Label slotTitle;
    juce::ToggleButton bypassButton;
    juce::ComboBox inputSourceCombo;
    juce::ComboBox typeCombo;
    juce::ComboBox routeCombo;

    juce::Label passThroughInfoLabel;

    // Rotary Dials with text labels and value boxes
    juce::Slider inGainSlider, p1Slider, p2Slider, p3Slider;
    juce::Label inGainLabel, p1Label, p2Label, p3Label;

    juce::Slider p4Slider, p5Slider, mixSlider, outGainSlider;
    juce::Label p4Label, p5Label, mixLabel, outGainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inputSourceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> routeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> p1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> p2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> p3Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> p4Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> p5Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    std::array<std::array<float, 6>, 36> effectParamCache;
    EffectType lastType = EffectType::Bypass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectSlotComponent)
};
