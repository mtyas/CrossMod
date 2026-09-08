#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ParameterIDs.h"

class SplitControlsComponent : public juce::Component
{
public:
    explicit SplitControlsComponent(juce::AudioProcessorValueTreeState& apvts);
    ~SplitControlsComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label sectionTitle;

    juce::Slider attackSlider;
    juce::Label attackLabel;

    juce::Slider releaseSlider;
    juce::Label releaseLabel;

    juce::Slider sensSlider;
    juce::Label sensLabel;

    juce::Slider threshSlider;
    juce::Label threshLabel;

    juce::ComboBox modeCombo;
    juce::Label modeLabel;

    juce::ComboBox soloCombo;
    juce::Label soloLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> threshAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> soloAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitControlsComponent)
};
