#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Engine/MidiChainProcessor.h"
#include "../Engine/PresetManager.h"
#include "../Engine/UndoHistoryManager.h"

namespace MidiFlux
{

class HeaderComponent : public juce::Component
{
public:
    HeaderComponent(MidiChainProcessor& chain, UndoHistoryManager& history);
    ~HeaderComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateUndoRedoButtons();
    void syncScaleBoxes();

    std::function<void()> onPanicTriggered;
    std::function<void()> onStateChanged;
    std::function<void()> onToggleKeyboard;

private:
    MidiChainProcessor& chainProcessor;
    UndoHistoryManager& undoHistory;

    juce::Label titleLabel{ "", "MIDIFLUX" };
    juce::Label subtitleLabel{ "", "MODULAR MIDI RACK" };

    // Undo / Redo
    juce::TextButton undoBtn{ "UNDO" };
    juce::TextButton redoBtn{ "REDO" };

    // Keyboard Toggle
    juce::TextButton kbdToggleBtn{ "KBD" };

    // Global Scale
    juce::Label scaleLabel{ "", "SCALE:" };
    juce::ComboBox rootKeyBox;
    juce::ComboBox scaleTypeBox;

    // Presets
    juce::Label presetLabel{ "", "PRESET:" };
    juce::ComboBox presetBox;
    juce::TextButton prevPresetBtn{ "<" };
    juce::TextButton nextPresetBtn{ ">" };

    // File Save / Load
    juce::TextButton savePresetBtn{ "SAVE" };
    juce::TextButton loadPresetBtn{ "LOAD" };

    // Global Action Buttons
    juce::TextButton randomAllBtn{ "DICE" };
    juce::TextButton bypassBtn{ "BYPASS" };
    juce::TextButton panicBtn{ "PANIC" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    void setupPresets();
    void setupScales();
    void savePresetToFile();
    void loadPresetFromFile();
};

} // namespace MidiFlux
