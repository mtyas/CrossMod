#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "TransientLookAndFeel.h"
#include "EffectSlotComponent.h"
#include "AudioRoutingDiagramComponent.h"
#include "PreProcessorsCombinedComponent.h"
#include "ModulatorsPanelComponent.h"
#include "ModMatrixComponent.h"
#include <array>

class TransientSplitProcessor;

enum class EditorViewTab
{
    PreProcessors = 0,
    FxRack = 1,
    RoutingDiagram = 2,
    Modulators = 3,
    ModulationMatrix = 4
};

class TransientSplitEditor : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit TransientSplitEditor(TransientSplitProcessor& p);
    ~TransientSplitEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setViewTab(EditorViewTab tab);
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    void updatePresetList();
    void showSavePresetDialog();

    TransientSplitProcessor& processor;
    TransientLookAndFeel lnf;

    // Header Components
    juce::Label pluginTitle;
    juce::Label pluginSubtitle;

    // Undo / Redo
    juce::TextButton undoBtn { "UNDO" };
    juce::TextButton redoBtn { "REDO" };

    // Preset Controls
    juce::ComboBox presetCombo;
    juce::TextButton prevPresetBtn { "<" };
    juce::TextButton nextPresetBtn { ">" };
    juce::TextButton savePresetBtn { "SAVE" };
    juce::TextButton loadPresetBtn { "IMPORT" };
    juce::TextButton deletePresetBtn { "DEL" };

    // Big Master Controls
    juce::Slider balanceSlider;
    juce::Label balanceLabel;

    juce::Slider dryWetSlider;
    juce::Label dryWetLabel;

    juce::Slider outGainSlider;
    juce::Label outGainLabel;

    juce::ToggleButton limiterButton { "LIMITER" };
    juce::ComboBox outputRoutingCombo;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> balanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> outputRoutingAttachment;

    // Top Navigation Tabs (in user's exact requested order)
    juce::TextButton preProcTabBtn { "1. PRE-PROCESSORS" };
    juce::TextButton fxRackTabBtn { "2. EFFECTS RACK" };
    juce::TextButton routingTabBtn { "3. AUDIO ROUTING" };
    juce::TextButton modTabBtn { "4. MODULATORS" };
    juce::TextButton matrixTabBtn { "5. MOD MATRIX" };
    EditorViewTab currentTab = EditorViewTab::PreProcessors;

    // 1. Pre-Processors Suite (Transient Splitter & 3-Band Overlapping Filter Bank)
    std::unique_ptr<PreProcessorsCombinedComponent> preProcessorsPanel;

    // 2. Unified 8-Slot Modular FX Rack (2x4 Grid)
    std::array<std::unique_ptr<EffectSlotComponent>, 8> fxSlots;

    // 3. Visual Audio Routing Matrix Diagram Canvas
    std::unique_ptr<AudioRoutingDiagramComponent> routingDiagram;

    // 4. Modulators Panel (Blocks on Top)
    std::unique_ptr<ModulatorsPanelComponent> modulatorsPanel;

    // 5. Modulation Matrix Panel
    std::unique_ptr<ModMatrixComponent> modMatrixPanel;

    // Footer
    juce::Label statusLabel;

    std::unique_ptr<juce::FileChooser> activeFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientSplitEditor)
};
