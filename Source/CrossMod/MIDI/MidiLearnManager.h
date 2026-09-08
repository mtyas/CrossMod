#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <atomic>

struct MidiMapping
{
    juce::String paramID;
    int channel = 0; // 0 = Any / Omni, 1-16 = specific channel
    int cc = 0;      // 0-127
};

class MidiLearnManager
{
public:
    MidiLearnManager();
    ~MidiLearnManager() = default;

    // --- Learning Management ---
    void startLearning(const juce::String& paramID);
    void cancelLearning();
    bool isLearning() const { return learningActive.load(std::memory_order_acquire); }
    bool isLearningParam(const juce::String& paramID) const;
    juce::String getLearningParamID() const;

    // --- Mappings CRUD ---
    void setMapping(const juce::String& paramID, int cc, int channel = 0);
    void removeMappingForParam(const juce::String& paramID);
    void removeMappingForCC(int cc, int channel = 0);
    void clearAllMappings();

    bool getMappingForParam(const juce::String& paramID, int& outCC, int& outChannel) const;
    std::vector<MidiMapping> getAllMappings() const;

    // --- Real-Time MIDI Processing (called from audio thread) ---
    void processMidiController(const juce::MidiMessage& msg, juce::AudioProcessorValueTreeState& apvts);

    // --- XML Serialization ---
    void saveToXml(juce::XmlElement& xml) const;
    void loadFromXml(const juce::XmlElement& xml);

    // Global persistence (default map on disk)
    static juce::File getDefaultMappingsFile();
    void saveDefaultMapToFile();
    void loadDefaultMapFromFile();

    // Callback when mappings or learning state change
    std::function<void()> onMappingsChanged;

private:
    mutable juce::SpinLock lock;
    std::vector<MidiMapping> mappings;

    std::atomic<bool> learningActive { false };
    juce::String learningTargetParam;

    void notifyChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnManager)
};
