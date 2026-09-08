#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../Parameters/ParameterIDs.h"

class ModMatrixComponent : public juce::Component
{
public:
    ModMatrixComponent(juce::AudioProcessorValueTreeState& apvts);
    ~ModMatrixComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct SlotUI
    {
        std::unique_ptr<juce::Label> titleLabel;
        std::unique_ptr<juce::ComboBox> sourceBox;
        std::unique_ptr<juce::ComboBox> destBox;
        std::unique_ptr<juce::Slider> amountSlider;
        std::unique_ptr<juce::TextButton> zeroBtn;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    };

    std::array<SlotUI, CrossModIDs::NUM_MOD_SLOTS> slots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixComponent)
};
