#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <vector>

struct PresetInfo
{
    juce::String name;
    juce::File file;
};

class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& apvtsToUse);
    ~PresetManager() = default;

    void initialize();
    void rescanPresets();

    int getNumPresets() const { return static_cast<int>(presets.size()); }
    juce::String getPresetName(int index) const;
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    juce::String getCurrentPresetName() const { return currentPresetName; }

    bool loadPreset(int index);
    bool loadPresetFromFile(const juce::File& file, bool copyToPresetsFolder = false);
    bool saveCurrentPreset(const juce::String& presetName);
    bool saveCurrentPresetToFile(const juce::File& file);

    juce::File getPresetsDirectory() const;
    void openPresetsFolderInExplorer();
    void restoreFactoryPresets();

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetInfo> presets;
    int currentPresetIndex = 0;
    juce::String currentPresetName = "Init Dual Sine";

    void createFactoryPresetsIfEmpty();
};
