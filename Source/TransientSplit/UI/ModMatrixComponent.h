#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/TransientSplitEngine.h"
#include "../Parameters/ParameterIDs.h"
#include <array>

class ModMatrixComponent : public juce::Component,
                           private juce::Timer
{
public:
    ModMatrixComponent(juce::AudioProcessorValueTreeState& apvts, TransientSplitEngine& engineToUse);
    ~ModMatrixComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void updateDynamicDestNames();

private:
    void timerCallback() override;

    struct ModRow
    {
        juce::Label slotLabel;
        juce::ComboBox srcCombo;
        juce::ComboBox dstCombo;
        juce::Slider depthSlider;
        juce::Label depthLabel;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> dstAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    };

    juce::AudioProcessorValueTreeState& apvts;
    TransientSplitEngine& engine;

    juce::Label tableHeader;
    std::array<ModRow, ModMatrix::NumRoutingSlots> rows;
    std::array<EffectType, 8> cachedSlotTypes;
    std::array<juce::Rectangle<int>, ModMatrix::NumRoutingSlots> rowBounds;
    std::array<juce::Rectangle<int>, ModMatrix::NumRoutingSlots> meterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixComponent)
};
