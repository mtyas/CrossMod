#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "EffectSlotComponent.h"
#include "../DSP/TransientSplitEngine.h"
#include <array>

class ChainRackComponent : public juce::Component,
                           private juce::Timer
{
public:
    ChainRackComponent(juce::AudioProcessorValueTreeState& apvts, TransientSplitEngine& engineToUse, bool isTransientChain);
    ~ChainRackComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    TransientSplitEngine& engine;
    bool isTransient;

    juce::Label chainBadge;
    juce::ToggleButton soloButton { "SOLO" };
    juce::ToggleButton muteButton { "MUTE" };

    juce::Slider gainSlider;
    juce::Label gainLabel;
    juce::Slider panSlider;
    juce::Label panLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment;

    std::array<std::unique_ptr<EffectSlotComponent>, 4> slots;

    float currentMeterLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainRackComponent)
};
