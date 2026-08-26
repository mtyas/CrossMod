#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Engine/MidiChainProcessor.h"
#include "BlockCardComponent.h"

namespace MidiFlux
{

class AddBlockCardComponent : public juce::Component
{
public:
    AddBlockCardComponent();
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void(const juce::String&)> onBlockSelected;

private:
    void showAddMenu();
};

class RackComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::DragAndDropTarget
{
public:
    RackComponent(MidiChainProcessor& chain);
    ~RackComponent() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void rebuildCards();

    // Drag and Drop Target implementation
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    std::function<void()> onStateMutated;

private:
    MidiChainProcessor& chainProcessor;
    juce::Viewport viewport;
    std::unique_ptr<juce::Component> contentComponent;
    std::vector<std::unique_ptr<BlockCardComponent>> cardComponents;
    std::unique_ptr<AddBlockCardComponent> addCard;

    int dragTargetInsertionIndex = -1;

    int calculateDropIndex(int mouseX) const;
    void handleMoveLeft(int index);
    void handleMoveRight(int index);
    void handleRemove(int index);
    void handleAdd(const juce::String& typeId);
};

} // namespace MidiFlux
