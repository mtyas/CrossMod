#include "PluginEditor.h"

namespace MidiFlux
{

MidiFluxAudioProcessorEditor::MidiFluxAudioProcessorEditor(MidiFluxAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      header(p.getChain(), undoHistory),
      scaleSequencer(p.getChain()),
      rack(p.getChain()),
      midiMonitor(p.getChain()),
      virtualKeyboard(21, 88) // A0 to C8 (88 keys)
{
    setLookAndFeel(&customLookAndFeel);

    // Initialize Undo History with initial state
    undoHistory.reset(audioProcessor.getChain().getState());

    rack.onStateMutated = [this]() {
        undoHistory.pushState(audioProcessor.getChain().getState());
    };

    header.onRackNeedsRefresh = [this]() {
        rack.rebuildCards();
    };

    header.onStateChanged = [this]() {
        rack.rebuildCards();
        scaleSequencer.rebuildBlockCards();
        undoHistory.pushState(audioProcessor.getChain().getState());
    };

    scaleSequencer.onProgressionChanged = [this]() {
        undoHistory.pushState(audioProcessor.getChain().getState());
    };

    addAndMakeVisible(header);
    addChildComponent(scaleSequencer);
    scaleSequencer.setVisible(scaleSequencerVisible);
    addAndMakeVisible(rack);
    addAndMakeVisible(midiMonitor);
    addAndMakeVisible(virtualKeyboard);

    // Connect panic
    header.onPanicTriggered = [this]() {
        audioProcessor.triggerPanic();
    };

    // Connect keyboard toggle
    header.onToggleKeyboard = [this]() {
        keyboardVisible = !keyboardVisible;
        virtualKeyboard.setVisible(keyboardVisible);
        resized();
    };

    // Connect scale sequencer toggle
    header.onToggleScaleSequencer = [this]() {
        scaleSequencerVisible = !scaleSequencerVisible;
        scaleSequencer.setVisible(scaleSequencerVisible);
        resized();
    };

    // Connect virtual keyboard
    virtualKeyboard.onNoteOn = [this](int ch, int note, uint8_t vel) {
        audioProcessor.injectVirtualMidi(juce::MidiMessage::noteOn(ch, note, vel));
    };

    virtualKeyboard.onNoteOff = [this](int ch, int note) {
        audioProcessor.injectVirtualMidi(juce::MidiMessage::noteOff(ch, note));
    };

    setSize(1180, 680);
    setResizable(true, true);
    setResizeLimits(900, 540, 2560, 1440);
    setWantsKeyboardFocus(true);
}

MidiFluxAudioProcessorEditor::~MidiFluxAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

bool MidiFluxAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    // Ctrl+Z / Cmd+Z for Undo
    if (key.getModifiers().isCommandDown() && !key.getModifiers().isShiftDown()
        && (key.getKeyCode() == 'Z' || key.getKeyCode() == 'z'))
    {
        if (undoHistory.undo(audioProcessor.getChain()))
        {
            header.updateUndoRedoButtons();
            header.syncScaleBoxes();
            scaleSequencer.rebuildBlockCards();
            rack.rebuildCards();
            return true;
        }
    }

    // Ctrl+Y / Cmd+Y or Ctrl+Shift+Z for Redo
    if ((key.getModifiers().isCommandDown() && (key.getKeyCode() == 'Y' || key.getKeyCode() == 'y'))
        || (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && (key.getKeyCode() == 'Z' || key.getKeyCode() == 'z')))
    {
        if (undoHistory.redo(audioProcessor.getChain()))
        {
            header.updateUndoRedoButtons();
            header.syncScaleBoxes();
            scaleSequencer.rebuildBlockCards();
            rack.rebuildCards();
            return true;
        }
    }

    return false;
}

void MidiFluxAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d1117));
}

void MidiFluxAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    header.setBounds(bounds.removeFromTop(52));
    if (scaleSequencerVisible)
        scaleSequencer.setBounds(bounds.removeFromTop(86));
    if (keyboardVisible)
        virtualKeyboard.setBounds(bounds.removeFromBottom(82));
    midiMonitor.setBounds(bounds.removeFromBottom(28));
    rack.setBounds(bounds);
}

} // namespace MidiFlux
