#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MidiBlock.h"
#include "../Engine/MidiChainProcessor.h"

namespace MidiFlux
{

class BlockCardComponent : public juce::Component, public juce::Timer
{
public:
    BlockCardComponent(MidiChainProcessor& chain, MidiBlock* block, int blockIndex);
    ~BlockCardComponent() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void(int)> onMoveLeft;
    std::function<void(int)> onMoveRight;
    std::function<void(int)> onRemove;
    std::function<void()> onParameterChanged;

    int getBlockIndex() const { return blockIndex; }
    void setBlockIndex(int idx) { blockIndex = idx; }
    MidiBlock* getBlock() { return block; }

    void updateControlsFromBlock();

private:
    MidiChainProcessor& chainProcessor;
    MidiBlock* block = nullptr;
    int blockIndex = 0;
    bool ledActive = false;
    float activityGlow = 0.0f;
    bool isDawStopped = false;

    // Header buttons
    juce::TextButton powerButton{ "ON" };
    juce::TextButton routingButton{ "SER" };
    juce::TextButton diceButton{ "DICE" };
    juce::TextButton leftButton{ "<" };
    juce::TextButton rightButton{ ">" };
    juce::TextButton removeButton{ "X" };

    struct ParamControl
    {
        int paramIndex;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> comboBox;
    };
    std::vector<ParamControl> controls;

    void showMidiLearnMenu(const juce::String& paramID, juce::Component* targetComp);
};

} // namespace MidiFlux
