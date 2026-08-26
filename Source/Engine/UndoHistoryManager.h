#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "MidiChainProcessor.h"
#include <vector>
#include <functional>

namespace MidiFlux
{

class UndoHistoryManager
{
public:
    UndoHistoryManager(int maxHistory = 50);

    void reset(const juce::ValueTree& initialState);
    void pushState(const juce::ValueTree& state);

    bool canUndo() const;
    bool canRedo() const;

    bool undo(MidiChainProcessor& chain);
    bool redo(MidiChainProcessor& chain);

    std::function<void()> onHistoryChanged;

private:
    int maxHistorySteps;
    std::vector<juce::ValueTree> history;
    int currentIndex = -1;
    bool isPerformingUndoRedo = false;
};

} // namespace MidiFlux
