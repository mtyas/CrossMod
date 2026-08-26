#include "UndoHistoryManager.h"

namespace MidiFlux
{

UndoHistoryManager::UndoHistoryManager(int maxHistory)
    : maxHistorySteps(maxHistory)
{
}

void UndoHistoryManager::reset(const juce::ValueTree& initialState)
{
    history.clear();
    if (initialState.isValid())
    {
        history.push_back(initialState.createCopy());
        currentIndex = 0;
    }
    else
    {
        currentIndex = -1;
    }
    if (onHistoryChanged)
        onHistoryChanged();
}

void UndoHistoryManager::pushState(const juce::ValueTree& state)
{
    if (isPerformingUndoRedo || !state.isValid())
        return;

    // Check if new state is identical to current state to avoid duplicate steps
    if (currentIndex >= 0 && currentIndex < static_cast<int>(history.size()))
    {
        if (history[currentIndex].isEquivalentTo(state))
            return;
    }

    // Truncate any redo steps
    if (currentIndex + 1 < static_cast<int>(history.size()))
    {
        history.erase(history.begin() + (currentIndex + 1), history.end());
    }

    // Add new state
    history.push_back(state.createCopy());

    // Limit size
    if (static_cast<int>(history.size()) > maxHistorySteps)
    {
        history.erase(history.begin());
    }

    currentIndex = static_cast<int>(history.size()) - 1;

    if (onHistoryChanged)
        onHistoryChanged();
}

bool UndoHistoryManager::canUndo() const
{
    return currentIndex > 0;
}

bool UndoHistoryManager::canRedo() const
{
    return currentIndex >= 0 && currentIndex + 1 < static_cast<int>(history.size());
}

bool UndoHistoryManager::undo(MidiChainProcessor& chain)
{
    if (!canUndo())
        return false;

    isPerformingUndoRedo = true;
    currentIndex--;
    chain.setState(history[currentIndex]);
    isPerformingUndoRedo = false;

    if (onHistoryChanged)
        onHistoryChanged();

    return true;
}

bool UndoHistoryManager::redo(MidiChainProcessor& chain)
{
    if (!canRedo())
        return false;

    isPerformingUndoRedo = true;
    currentIndex++;
    chain.setState(history[currentIndex]);
    isPerformingUndoRedo = false;

    if (onHistoryChanged)
        onHistoryChanged();

    return true;
}

} // namespace MidiFlux
