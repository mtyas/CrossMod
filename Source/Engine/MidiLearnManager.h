#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <atomic>

namespace MidiFlux
{

class MidiChainProcessor;

struct MidiMapping
{
    juce::String paramID; // e.g. "block:0:param:1" or "block:0:power" or "global:rootKey"
    int channel = 0;      // 0 = Any / Omni, 1-16 = specific channel
    int cc = 0;           // 0-127
};

class MidiLearnManager
{
public:
    MidiLearnManager();
    ~MidiLearnManager() = default;

    // --- Learning State ---
    void startLearning(const juce::String& paramID);
    void cancelLearning();
    bool isLearning() const { return learningActive.load(std::memory_order_acquire); }
    bool isLearningParam(const juce::String& paramID) const;
    juce::String getLearningParamID() const;

    // --- Mappings Management ---
    void setMapping(const juce::String& paramID, int cc, int channel = 0);
    void removeMappingForParam(const juce::String& paramID);
    void removeMappingForCC(int cc, int channel = 0);
    void clearAllMappings();

    bool getMappingForParam(const juce::String& paramID, int& outCC, int& outChannel) const;
    std::vector<MidiMapping> getAllMappings() const;

    // Chain manipulation tracking
    void remapBlockMoved(int fromIndex, int toIndex);
    void remapBlockRemoved(int index);

    // --- Audio Thread MIDI Controller Processing ---
    void processMidiController(const juce::MidiMessage& msg, MidiChainProcessor& chain);

    // --- Serialization ---
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& vt);

    // Global persistence
    static juce::File getDefaultMappingsFile();
    void saveDefaultMapToFile();
    void loadDefaultMapFromFile();

    std::function<void()> onMappingsChanged;

private:
    mutable juce::SpinLock lock;
    std::vector<MidiMapping> mappings;

    std::atomic<bool> learningActive{ false };
    juce::String learningTargetParam;

    void notifyChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnManager)
};

} // namespace MidiFlux
