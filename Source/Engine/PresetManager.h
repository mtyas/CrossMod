#pragma once

#include "MidiChainProcessor.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>
#include <vector>

namespace MidiFlux
{

struct Preset
{
    juce::String name;
    juce::ValueTree state;
    bool isFactory = false;
};

class PresetManager
{
public:
    static std::vector<Preset>& getPresets();
    static std::vector<Preset> getFactoryPresets();
    static void applyPreset(MidiChainProcessor& chain, int presetIndex);

    static void saveCurrentPreset(int index, const juce::ValueTree& state);
    static void addPreset(const juce::String& name, const juce::ValueTree& state);
    static void deletePreset(int index);
    static void resetToFactoryDefaults();

    static void saveToDisk();
    static void loadFromDisk();

private:
    static std::vector<Preset> activePresets;
    static bool initialized;

    static void ensureInitialized();
    static juce::File getPresetsFile();
};

} // namespace MidiFlux
