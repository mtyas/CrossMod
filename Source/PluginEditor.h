#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Engine/UndoHistoryManager.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/HeaderComponent.h"
#include "UI/RackComponent.h"
#include "UI/MidiMonitorComponent.h"
#include "UI/ScaleSequencerComponent.h"
#include "UI/VirtualKeyboardComponent.h"

namespace MidiFlux
{

class MidiFluxAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiFluxAudioProcessorEditor(MidiFluxAudioProcessor&);
    ~MidiFluxAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    MidiFluxAudioProcessor& audioProcessor;
    CustomLookAndFeel customLookAndFeel;
    UndoHistoryManager undoHistory;

    HeaderComponent header;
    ScaleSequencerComponent scaleSequencer;
    bool scaleSequencerVisible = false;

    RackComponent rack;
    MidiMonitorComponent midiMonitor;
    VirtualKeyboardComponent virtualKeyboard;
    bool keyboardVisible = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFluxAudioProcessorEditor)
};

} // namespace MidiFlux
