#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "VintageLookAndFeel.h"
#include "CrossModVisualizer.h"
#include "ModMatrixComponent.h"
#include "FxSectionComponent.h"
#include "CrossModKnob.h"
#include "../CrossModProcessor.h"

class CrossModEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit CrossModEditor(CrossModProcessor& p);
    ~CrossModEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    CrossModProcessor& processor;
    VintageLookAndFeel vintageLaf;

    // Header Controls
    juce::ComboBox presetBox;
    juce::TextButton prevPresetBtn { "<" };
    juce::TextButton nextPresetBtn { ">" };
    juce::TextButton savePresetBtn { "SAVE" };
    juce::TextButton loadPresetBtn { "LOAD" };
    juce::TextButton openFolderBtn { "DIR" };
    juce::TextButton undoBtn { "UNDO" };
    juce::TextButton redoBtn { "REDO" };
    juce::TextButton midiBtn { "MIDI" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Voice & Polyphony
    juce::ComboBox voiceModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voiceModeAttachment;
    CrossModKnob glide1Knob;
    CrossModKnob glide2Knob;
    CrossModKnob voiceXModKnob;

    // Osc 1
    juce::ComboBox osc1WaveformBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveformAttachment;
    CrossModKnob osc1CoarseKnob;
    CrossModKnob osc1FineKnob;
    CrossModKnob osc1LevelKnob;

    // Sub Osc
    juce::ComboBox subOscWaveformBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subOscWaveformAttachment;
    juce::ComboBox subOscOctaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subOscOctaveAttachment;
    CrossModKnob subOscLevelKnob;

    // Cross-Mod Center
    juce::ComboBox crossModModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> crossModModeAttachment;
    CrossModKnob crossMod1to2Knob;
    CrossModKnob crossMod2to1Knob;
    CrossModVisualizer visualizer;

    // Osc 2
    juce::ComboBox osc2WaveformBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2WaveformAttachment;
    CrossModKnob osc2CoarseKnob;
    CrossModKnob osc2FineKnob;
    CrossModKnob osc2LevelKnob;

    // Filter (VCF)
    CrossModKnob filterCutoffKnob;
    CrossModKnob filterResoKnob;
    CrossModKnob filterDriveKnob;
    CrossModKnob filterKeyTrackKnob;
    CrossModKnob filterEnvAmtKnob;
    juce::ComboBox filterTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment;

    // VCA & Master
    CrossModKnob masterVolumeKnob;
    CrossModKnob masterPanKnob;
    CrossModKnob stereoWidthKnob;
    CrossModKnob vcaWarmthKnob;

    // LFOs
    CrossModKnob lfo1RateKnob;
    juce::ComboBox lfo1ShapeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo1ShapeAttachment;
    juce::ToggleButton lfo1SyncBtn { "BPM Sync" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfo1SyncAttachment;

    CrossModKnob lfo2RateKnob;
    juce::ComboBox lfo2ShapeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo2ShapeAttachment;
    juce::ToggleButton lfo2SyncBtn { "BPM Sync" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfo2SyncAttachment;

    // Envelopes (ADSR) - Amp, Filter, Mod
    CrossModKnob ampAttackKnob, ampDecayKnob, ampSustainKnob, ampReleaseKnob;
    CrossModKnob filtAttackKnob, filtDecayKnob, filtSustainKnob, filtReleaseKnob;
    CrossModKnob modAttackKnob, modDecayKnob, modSustainKnob, modReleaseKnob;

    // Modulation Matrix Table (8 Slots)
    ModMatrixComponent modMatrixComponent;

    // Studio Multi-FX Section (Modulation, Delay, Reverb, Order)
    FxSectionComponent fxSectionComponent;

    // Metering & Voice display
    uint32_t activeVoiceMask = 0;
    float peakMeterL = 0.0f;
    float peakMeterR = 0.0f;

    void updatePresetList();
    void showMidiMenu();
    bool lastLfo1Synced = false;
    bool lastLfo2Synced = false;
    void updateLfo1RateAttachment(bool isSynced);
    void updateLfo2RateAttachment(bool isSynced);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossModEditor)
};
