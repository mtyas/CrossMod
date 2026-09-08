#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <set>
#include <memory>

namespace MidiFlux
{

class VirtualKeyboardComponent : public juce::Component, public juce::KeyListener
{
public:
    VirtualKeyboardComponent(int startNote = 21, int numKeys = 88); // Default 88-key piano (A0 to C8)
    ~VirtualKeyboardComponent() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override;

    void clearAllHeldNotes();
    void setHoldEnabled(bool enabled);
    bool isHoldEnabled() const { return holdEnabled; }

    std::function<void(int channel, int note, uint8_t velocity)> onNoteOn;
    std::function<void(int channel, int note)> onNoteOff;

private:
    class KeysComponent : public juce::Component
    {
    public:
        KeysComponent(VirtualKeyboardComponent& owner, int startNote, int numKeys);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

        int getStartNote() const { return baseNote; }
        int getNumKeys() const { return totalKeys; }
        float getWhiteKeyWidth() const { return whiteKeyWidth; }

        int getNoteAtPosition(int x, int y) const;
        bool isBlackKey(int noteNumber) const;

    private:
        VirtualKeyboardComponent& owner;
        int baseNote;
        int totalKeys;
        float whiteKeyWidth = 34.0f;

        bool isRightDragging = false;
        int dragStartX = 0;
        int dragStartViewX = 0;
    };

    friend class KeysComponent;

    int baseNote = 21;
    int totalKeys = 88;
    bool holdEnabled = false;

    std::set<int> activeNotes;
    int currentMouseNote = -1;

    // Floating overlay buttons
    juce::TextButton holdButton{ "HOLD CHORD" };
    juce::TextButton clearButton{ "CLEAR" };
    juce::TextButton octDownButton{ "< OCT" };
    juce::TextButton octUpButton{ "OCT >" };

    juce::Viewport viewport;
    std::unique_ptr<KeysComponent> keys;

    void triggerNoteDown(int note, uint8_t velocity = 100);
    void triggerNoteUp(int note);
    void scrollOctave(int direction);

    int keyCodeToNoteOffset(int keyCode) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualKeyboardComponent)
};

} // namespace MidiFlux
