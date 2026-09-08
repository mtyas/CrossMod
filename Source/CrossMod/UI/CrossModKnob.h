#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "VintageLookAndFeel.h"
#include "../MIDI/MidiLearnManager.h"

class CrossModSlider : public juce::Slider
{
public:
    CrossModSlider();
    ~CrossModSlider() override = default;

    void setParamInfo(const juce::String& paramID, juce::RangedAudioParameter* param, MidiLearnManager* mlm);
    const juce::String& getParamID() const { return targetParamID; }

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void paint(juce::Graphics& g) override;

private:
    juce::String targetParamID;
    juce::RangedAudioParameter* rangedParam = nullptr;
    MidiLearnManager* midiLearnManager = nullptr;

    void showContextMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossModSlider)
};

struct CrossModKnob
{
    std::unique_ptr<CrossModSlider> slider;
    std::unique_ptr<juce::Label> label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void init(juce::Component& parent,
              juce::AudioProcessorValueTreeState& apvts,
              MidiLearnManager& mlm,
              const juce::ParameterID& pid,
              const juce::String& text,
              VintageLookAndFeel::KnobColor color = VintageLookAndFeel::Ivory,
              const juce::String& suffix = "");

    void setBounds(int x, int y, int w, int h);
};
