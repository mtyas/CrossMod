#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/CrossModEngine.h"
#include "Parameters/ParameterIDs.h"
#include "Presets/FactoryPresets.h"
#include "Presets/PresetManager.h"
#include "MIDI/MidiLearnManager.h"

class CrossModProcessor : public juce::AudioProcessor
{
public:
    CrossModProcessor();
    ~CrossModProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CrossMod"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    CrossModEngine& getEngine() { return engine; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }
    juce::UndoManager& getUndoManager() { return undoManager; }
    PresetManager& getPresetManager() { return presetManager; }
    MidiLearnManager& getMidiLearnManager() { return midiLearnManager; }

    void loadPreset(int presetIndex);
    int getNumFactoryPresets() const;
    juce::String getFactoryPresetName(int index) const;

    void savePresetToFile(const juce::File& file);
    bool loadPresetFromFile(const juce::File& file);

private:
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;
    MidiLearnManager midiLearnManager;
    juce::MidiKeyboardState keyboardState;
    CrossModEngine engine;
    std::vector<PresetData> factoryPresets;
    int currentProgramIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossModProcessor)
};
