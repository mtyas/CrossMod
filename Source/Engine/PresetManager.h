#pragma once

#include "MidiChainProcessor.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

namespace MidiFlux
{

struct Preset
{
    juce::String name;
    juce::ValueTree state;
};

class PresetManager
{
public:
    static std::vector<Preset> getFactoryPresets();
    static void applyPreset(MidiChainProcessor& chain, int presetIndex);
};

} // namespace MidiFlux
