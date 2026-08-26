#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MidiBlock.h"

namespace MidiFlux
{

class BlockCardComponent : public juce::Component, public juce::Timer
{
public:
    BlockCardComponent(MidiBlock* block, int blockIndex);
    ~BlockCardComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    std::function<void(int)> onMoveLeft;
    std::function<void(int)> onMoveRight;
    std::function<void(int)> onRemove;
    std::function<void()> onParameterChanged;

    int getBlockIndex() const { return blockIndex; }
    void setBlockIndex(int idx) { blockIndex = idx; }
    MidiBlock* getBlock() { return block; }

private:
    MidiBlock* block = nullptr;
    int blockIndex = 0;
    bool ledActive = false;

    // Header buttons
    juce::TextButton powerButton{ "ON" };
    juce::TextButton diceButton{ "🎲" };
    juce::TextButton leftButton{ "◀" };
    juce::TextButton rightButton{ "▶" };
    juce::TextButton removeButton{ "✕" };

    struct ParamControl
    {
        int paramIndex;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> comboBox;
    };
    std::vector<ParamControl> controls;

    void updateControlsFromBlock();
};

} // namespace MidiFlux
