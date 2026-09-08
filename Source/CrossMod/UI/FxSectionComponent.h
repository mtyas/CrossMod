#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "VintageLookAndFeel.h"
#include "CrossModKnob.h"
#include "../Parameters/ParameterIDs.h"
#include "../MIDI/MidiLearnManager.h"

class FxSectionComponent : public juce::Component
{
public:
    FxSectionComponent(juce::AudioProcessorValueTreeState& apvts, MidiLearnManager& mlm);
    ~FxSectionComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Routing Order
    juce::Label orderLabel { "lbl", "FX ORDER:" };
    juce::ComboBox orderBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> orderAttachment;

    // Modulation FX
    juce::ComboBox modTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modTypeAttachment;
    CrossModKnob modRateKnob;
    CrossModKnob modDepthKnob;
    CrossModKnob modFeedbackKnob;
    CrossModKnob modMixKnob;

    // Delay FX
    juce::ComboBox delayTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayTypeAttachment;
    juce::ToggleButton delaySyncBtn { "BPM Sync" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delaySyncAttachment;
    CrossModKnob delayTimeKnob;
    CrossModKnob delayFeedbackKnob;
    CrossModKnob delayToneKnob;
    CrossModKnob delayMixKnob;

    // Reverb FX
    juce::ComboBox reverbTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> reverbTypeAttachment;
    CrossModKnob reverbDecayKnob;
    CrossModKnob reverbDampingKnob;
    CrossModKnob reverbToneKnob;
    CrossModKnob reverbMixKnob;

    juce::AudioProcessorValueTreeState& apvtsRef;
    MidiLearnManager& midiLearnRef;
    bool lastDelaySynced = false;
    void updateDelayTimeAttachment(bool isSynced);

public:
    void updateSyncState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxSectionComponent)
};
