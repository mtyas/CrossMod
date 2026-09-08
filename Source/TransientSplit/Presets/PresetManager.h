#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"
#include <vector>

struct PresetItem
{
    juce::String name;
    juce::String category;
    bool isFactory = false;
    juce::File userFile;
    std::map<juce::String, float> paramValues; // For built-in presets
};

class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& state);

    void refreshPresetList();
    juce::File getPresetsDirectory() const;

    int getNumPresets() const noexcept { return static_cast<int>(presetList.size()); }
    juce::String getPresetName(int index) const;
    juce::String getPresetCategory(int index) const;
    bool isPresetFactory(int index) const;

    void loadPreset(int index);
    bool saveCurrentPresetAs(const juce::String& presetName, const juce::String& category = "User");
    bool overwritePreset(int index);
    bool deletePreset(int index);

    bool savePresetToFile(const juce::File& file);
    bool loadPresetFromFile(const juce::File& file);

    const std::vector<PresetItem>& getAllPresets() const noexcept { return presetList; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetItem> presetList;
};
