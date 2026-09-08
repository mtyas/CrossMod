#pragma once

#include "../Common/MidiBlock.h"
#include "../Common/MidiBlockFactory.h"
#include "../Common/ScaleProgression.h"
#include "MidiLearnManager.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace MidiFlux
{

struct MidiActivityEvent
{
    int noteNumber;
    uint8_t velocity;
    bool isNoteOn;
    bool isOutput;
};

class MidiChainProcessor
{
public:
    MidiChainProcessor();
    ~MidiChainProcessor();

    void prepare(double sampleRate, int maxSamplesPerBlock);
    void reset();
    void panic(juce::MidiBuffer& outBuffer);

    // Audio thread processing
    void processMidi(juce::MidiBuffer& midiBuffer, BlockContext ctx);

    // Chain manipulation (thread-safe)
    int getNumBlocks();
    MidiBlock* getBlock(int index);
    void addBlock(const juce::String& typeId);
    void insertBlock(int index, const juce::String& typeId);
    void removeBlock(int index);
    void moveBlock(int fromIndex, int toIndex);
    void clearAllBlocks();
    void randomizeAll();
    void randomizeRack(bool randomizeModules = true);

    bool isDawPlaying() const { return isDawPlayingFlag.load(std::memory_order_relaxed); }
    void setIsDawPlaying(bool p) { isDawPlayingFlag.store(p, std::memory_order_relaxed); }

    // Global Key / Scale
    int getGlobalRootKey() const { return globalRootKey.load(std::memory_order_relaxed); }
    void setGlobalRootKey(int r) { globalRootKey.store(r, std::memory_order_relaxed); }

    int getGlobalScaleType() const { return globalScaleType.load(std::memory_order_relaxed); }
    void setGlobalScaleType(int s) { globalScaleType.store(s, std::memory_order_relaxed); }

    bool isMasterBypassed() const { return masterBypassed.load(std::memory_order_relaxed); }
    void setMasterBypassed(bool b) { masterBypassed.store(b, std::memory_order_relaxed); }

    // Scale Progression Sequencer
    ScaleProgression& getScaleProgression() { return scaleProgression; }
    const ScaleProgression& getScaleProgression() const { return scaleProgression; }
    ScaleProgression::PlaybackState getLastPlaybackState() const;

    // MIDI Learn Manager
    MidiLearnManager& getMidiLearnManager() { return midiLearnManager; }
    const MidiLearnManager& getMidiLearnManager() const { return midiLearnManager; }

    // Serialization to ValueTree
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& vt);

    // UI Activity polling
    int getPendingActivityEvents(std::vector<MidiActivityEvent>& dest);

    // Change listener callback for UI
    std::function<void()> onChainModified;

private:
    std::recursive_mutex chainMutex;
    std::vector<std::unique_ptr<MidiBlock>> blocks;

    MidiLearnManager midiLearnManager;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    std::atomic<int> globalRootKey{ 0 };      // C
    std::atomic<int> globalScaleType{ 0 };    // Major
    std::atomic<bool> masterBypassed{ false };
    std::atomic<bool> isDawPlayingFlag{ false };

    ScaleProgression scaleProgression;
    ScaleProgression::PlaybackState lastPlaybackState;

    // Real-time activity ring buffer for UI visualization
    static constexpr int activityBufferSize = 256;
    std::array<MidiActivityEvent, activityBufferSize> activityBuffer;
    std::atomic<int> activityWritePos{ 0 };
    int activityReadPos = 0;

    void pushActivityEvent(int note, uint8_t vel, bool isNoteOn, bool isOutput);
};

} // namespace MidiFlux
