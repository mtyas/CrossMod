#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <set>

namespace MidiFlux
{

class VirtualKeyboardComponent : public juce::Component, public juce::KeyListener
{
public:
    VirtualKeyboardComponent(int startNote = 48, int numKeys = 37); // C3 to C6 default (3 octaves + 1)
    ~VirtualKeyboardComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override;

    std::function<void(int channel, int note, uint8_t velocity)> onNoteOn;
    std::function<void(int channel, int note)> onNoteOff;

private:
    int baseNote = 48; // C3
    int totalKeys = 37;

    std::set<int> activeNotes;
    int currentMouseNote = -1;

    bool isBlackKey(int noteNumber) const;
    int getNoteAtPosition(int x, int y) const;
    void triggerNoteDown(int note, uint8_t velocity = 100);
    void triggerNoteUp(int note);

    int keyCodeToNoteOffset(int keyCode) const;
};

} // namespace MidiFlux
