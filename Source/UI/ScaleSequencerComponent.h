#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Engine/MidiChainProcessor.h"
#include "../Common/ScaleProgression.h"
#include <vector>

namespace MidiFlux
{

class ScaleBlockCard : public juce::Component
{
public:
    ScaleBlockCard(int index, ScaleProgression& progression, std::function<void()> onChanged);
    ~ScaleBlockCard() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateFromProgression();

    void setActive(bool active, float progress);

    int getIndex() const { return blockIndex; }
    void setIndex(int idx);

    std::function<void(int)> onRemove;

private:
    int blockIndex = 0;
    ScaleProgression& progression;
    std::function<void()> onChanged;

    bool isActive = false;
    float playbackProgress = 0.0f;

    juce::Label indexLabel;
    juce::TextButton removeButton{ "x" };
    juce::ComboBox rootKeyBox;
    juce::ComboBox scaleTypeBox;
    juce::ComboBox barsBox;

    void setupControls();
};

class ScaleSequencerComponent : public juce::Component, public juce::Timer
{
public:
    ScaleSequencerComponent(MidiChainProcessor& chain);
    ~ScaleSequencerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void rebuildBlockCards();

    std::function<void()> onProgressionChanged;

private:
    MidiChainProcessor& chainProcessor;

    // Controls
    juce::Label titleLabel{ "", "SCALE SEQUENCER" };
    juce::Label statusLabel{ "", "DAW TIMELINE SYNC" };
    juce::TextButton enableToggle{ "OFF" };
    juce::TextButton loopToggle{ "LOOP" };
    juce::TextButton presetsButton{ "PRESETS v" };

    juce::Viewport viewport;
    std::unique_ptr<juce::Component> contentComponent;
    std::vector<std::unique_ptr<ScaleBlockCard>> blockCards;
    juce::TextButton addBlockButton{ "+ ADD BLOCK" };

    int lastActiveIndex = -1;
    float lastProgress = 0.0f;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void showPresetsMenu();
    void addNewBlock();
    void removeBlock(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleSequencerComponent)
};

} // namespace MidiFlux
