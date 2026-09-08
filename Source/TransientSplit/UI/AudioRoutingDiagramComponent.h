#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/TransientSplitEngine.h"
#include "../Parameters/ParameterIDs.h"
#include "TransientLookAndFeel.h"
#include "EffectSlotComponent.h"
#include <array>
#include <vector>

enum class PinKind
{
    SourceOut,
    SlotIn,
    SlotOut,
    DestIn
};

struct PinRef
{
    PinKind kind;
    int index; // 0..5 for Source, 0..7 for Slot, 0..3 for Dest
};

class AudioRoutingDiagramComponent : public juce::Component,
                                     private juce::Timer
{
public:
    AudioRoutingDiagramComponent(juce::AudioProcessorValueTreeState& apvts, TransientSplitEngine& engineToUse);
    ~AudioRoutingDiagramComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void updateMiniKnobPositions();
    void showAlgorithmMenu(int slotIndex, juce::Point<int> screenPos);

private:
    void timerCallback() override;
    void initNodePositions();

    juce::AudioProcessorValueTreeState& apvts;
    TransientSplitEngine& engine;

    float animationPhase = 0.0f;
    bool positionsInitialized = false;

    struct SourcePin
    {
        juce::String name;
        juce::Colour colour;
        juce::Rectangle<float> bounds;
        juce::Point<float> pinPos;
    };

    struct SlotNode
    {
        int slotIndex; // 0..7
        juce::Rectangle<float> bounds;
        juce::Rectangle<float> algoClickBounds;
        juce::Point<float> inPinPos;
        juce::Point<float> outPinPos;
    };

    struct DestPin
    {
        juce::String name;
        juce::Colour colour;
        juce::Rectangle<float> bounds;
        juce::Point<float> pinPos;
    };

    std::array<SourcePin, 16> sourcePins;
    std::array<SlotNode, 8> slotNodes;
    std::array<DestPin, 4> destPins; // 4 Stereo Output Buses

    // Interactive In & Out Mini Rotary Knobs & Power Toggle per Slot
    std::array<std::unique_ptr<juce::Slider>, 8> inGainSliders;
    std::array<std::unique_ptr<juce::Slider>, 8> outGainSliders;
    std::array<std::unique_ptr<juce::ToggleButton>, 8> powerButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> inGainAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 8> outGainAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 8> powerAttachments;

    // Interactive Dragging State
    bool isDraggingNode = false;
    int draggedSlotIndex = -1;
    juce::Point<float> nodeDragOffset;

    bool isDraggingCable = false;
    PinRef dragStartPin { PinKind::SourceOut, -1 };
    juce::Point<float> dragStartPoint;
    juce::Point<float> dragCurrentPoint;
    juce::Colour dragCableColor { juce::Colours::cyan };

    PinRef findPinAt(juce::Point<float> p, float hitRadius = 16.0f) const;
    int findSlotNodeAt(juce::Point<float> p) const;
    int findAlgoClickAt(juce::Point<float> p) const;
    void connectPins(PinRef fromPin, PinRef toPin);
    void disconnectPin(PinRef pin);

    void drawCurvedCable(juce::Graphics& g, juce::Point<float> p1, juce::Point<float> p2, juce::Colour col, bool active);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRoutingDiagramComponent)
};
