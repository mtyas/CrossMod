#include "VirtualKeyboardComponent.h"

namespace MidiFlux
{

VirtualKeyboardComponent::VirtualKeyboardComponent(int startNote, int numKeys)
    : baseNote(startNote), totalKeys(numKeys)
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);
}

VirtualKeyboardComponent::~VirtualKeyboardComponent()
{
    removeKeyListener(this);
}

bool VirtualKeyboardComponent::isBlackKey(int noteNumber) const
{
    int semitone = (noteNumber % 12 + 12) % 12;
    return (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10);
}

void VirtualKeyboardComponent::resized()
{
}

void VirtualKeyboardComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff0d1117));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Count white keys
    int numWhiteKeys = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        if (!isBlackKey(baseNote + i))
            numWhiteKeys++;
    }

    if (numWhiteKeys <= 0) return;

    float whiteKeyWidth = bounds.getWidth() / (float)numWhiteKeys;
    float whiteKeyHeight = bounds.getHeight();
    float blackKeyWidth = whiteKeyWidth * 0.65f;
    float blackKeyHeight = whiteKeyHeight * 0.62f;

    // 1. Draw White Keys
    int whiteIdx = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int note = baseNote + i;
        if (!isBlackKey(note))
        {
            float x = bounds.getX() + whiteIdx * whiteKeyWidth;
            auto keyRect = juce::Rectangle<float>(x, bounds.getY(), whiteKeyWidth, whiteKeyHeight).reduced(0.5f, 0.0f);

            bool isActive = (activeNotes.find(note) != activeNotes.end());

            if (isActive)
            {
                g.setColour(juce::Colour(0xff00e5ff));
                g.fillRoundedRectangle(keyRect, 3.0f);
            }
            else
            {
                g.setColour(juce::Colour(0xffe6edf3));
                g.fillRoundedRectangle(keyRect, 3.0f);
            }

            g.setColour(juce::Colour(0xff21262d));
            g.drawRoundedRectangle(keyRect, 3.0f, 1.0f);

            // Note label on C keys
            if ((note % 12) == 0)
            {
                g.setColour(isActive ? juce::Colours::black : juce::Colour(0xff8b949e));
                g.setFont(juce::FontOptions("Segoe UI", 10.5f, juce::Font::bold));
                g.drawText("C" + juce::String(note / 12 - 1), keyRect.removeFromBottom(16.0f), juce::Justification::centred, false);
            }

            whiteIdx++;
        }
    }

    // 2. Draw Black Keys
    whiteIdx = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int note = baseNote + i;
        if (!isBlackKey(note))
        {
            whiteIdx++;
        }
        else
        {
            float x = bounds.getX() + whiteIdx * whiteKeyWidth - (blackKeyWidth * 0.5f);
            auto keyRect = juce::Rectangle<float>(x, bounds.getY(), blackKeyWidth, blackKeyHeight);

            bool isActive = (activeNotes.find(note) != activeNotes.end());

            if (isActive)
            {
                g.setColour(juce::Colour(0xffa855f7));
                g.fillRoundedRectangle(keyRect, 2.5f);
            }
            else
            {
                g.setColour(juce::Colour(0xff161b22));
                g.fillRoundedRectangle(keyRect, 2.5f);
            }

            g.setColour(juce::Colour(0xff30363d));
            g.drawRoundedRectangle(keyRect, 2.5f, 1.0f);
        }
    }
}

int VirtualKeyboardComponent::getNoteAtPosition(int x, int y) const
{
    auto bounds = getLocalBounds().toFloat();
    int numWhiteKeys = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        if (!isBlackKey(baseNote + i))
            numWhiteKeys++;
    }
    if (numWhiteKeys <= 0) return -1;

    float whiteKeyWidth = bounds.getWidth() / (float)numWhiteKeys;
    float blackKeyWidth = whiteKeyWidth * 0.65f;
    float blackKeyHeight = bounds.getHeight() * 0.62f;

    // Check black keys first (they are on top)
    if ((float)y <= blackKeyHeight)
    {
        int whiteIdx = 0;
        for (int i = 0; i < totalKeys; ++i)
        {
            int note = baseNote + i;
            if (!isBlackKey(note))
            {
                whiteIdx++;
            }
            else
            {
                float kx = bounds.getX() + whiteIdx * whiteKeyWidth - (blackKeyWidth * 0.5f);
                if (x >= kx && x <= kx + blackKeyWidth)
                    return note;
            }
        }
    }

    // Check white keys
    int wIdx = static_cast<int>((x - bounds.getX()) / whiteKeyWidth);
    int currentWhite = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int note = baseNote + i;
        if (!isBlackKey(note))
        {
            if (currentWhite == wIdx)
                return note;
            currentWhite++;
        }
    }

    return -1;
}

void VirtualKeyboardComponent::triggerNoteDown(int note, uint8_t velocity)
{
    if (note >= 0 && note <= 127 && activeNotes.find(note) == activeNotes.end())
    {
        activeNotes.insert(note);
        if (onNoteOn)
            onNoteOn(1, note, velocity);
        repaint();
    }
}

void VirtualKeyboardComponent::triggerNoteUp(int note)
{
    if (note >= 0 && activeNotes.find(note) != activeNotes.end())
    {
        activeNotes.erase(note);
        if (onNoteOff)
            onNoteOff(1, note);
        repaint();
    }
}

void VirtualKeyboardComponent::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    int note = getNoteAtPosition(e.x, e.y);
    if (note != -1)
    {
        currentMouseNote = note;
        float normY = juce::jlimit(0.0f, 1.0f, (float)e.y / (float)getHeight());
        uint8_t vel = static_cast<uint8_t>(juce::jlimit(25, 127, (int)(35.0f + normY * 92.0f)));
        triggerNoteDown(note, vel);
    }
}

void VirtualKeyboardComponent::mouseDrag(const juce::MouseEvent& e)
{
    int note = getNoteAtPosition(e.x, e.y);
    if (note != currentMouseNote)
    {
        if (currentMouseNote != -1)
            triggerNoteUp(currentMouseNote);

        currentMouseNote = note;
        if (note != -1)
        {
            float normY = juce::jlimit(0.0f, 1.0f, (float)e.y / (float)getHeight());
            uint8_t vel = static_cast<uint8_t>(juce::jlimit(25, 127, (int)(35.0f + normY * 92.0f)));
            triggerNoteDown(note, vel);
        }
    }
}

void VirtualKeyboardComponent::mouseUp(const juce::MouseEvent&)
{
    if (currentMouseNote != -1)
    {
        triggerNoteUp(currentMouseNote);
        currentMouseNote = -1;
    }
}

int VirtualKeyboardComponent::keyCodeToNoteOffset(int keyCode) const
{
    // QWERTY mapping: A=0, W=1, S=2, E=3, D=4, F=5, T=6, G=7, Y=8, H=9, U=10, J=11, K=12, O=13, L=14
    switch (keyCode)
    {
        case 'A': case 'a': return 0;
        case 'W': case 'w': return 1;
        case 'S': case 's': return 2;
        case 'E': case 'e': return 3;
        case 'D': case 'd': return 4;
        case 'F': case 'f': return 5;
        case 'T': case 't': return 6;
        case 'G': case 'g': return 7;
        case 'Y': case 'y': return 8;
        case 'H': case 'h': return 9;
        case 'U': case 'u': return 10;
        case 'J': case 'j': return 11;
        case 'K': case 'k': return 12;
        case 'O': case 'o': return 13;
        case 'L': case 'l': return 14;
        default: return -1;
    }
}

bool VirtualKeyboardComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    int offset = keyCodeToNoteOffset(key.getKeyCode());
    if (offset >= 0)
    {
        int note = baseNote + 12 + offset; // Start at middle octave
        triggerNoteDown(note);
        return true;
    }
    return false;
}

bool VirtualKeyboardComponent::keyStateChanged(bool isKeyDown, juce::Component*)
{
    if (!isKeyDown)
    {
        // Release any key no longer held down
        std::vector<int> notesToRelease;
        for (int note : activeNotes)
        {
            // If it was triggered via keyboard, check if key is still down
            int offset = note - (baseNote + 12);
            if (offset >= 0 && offset <= 14)
            {
                // check corresponding key
                static const int keyCodes[] = { 'A', 'W', 'S', 'E', 'D', 'F', 'T', 'G', 'Y', 'H', 'U', 'J', 'K', 'O', 'L' };
                if (offset < 15 && !juce::KeyPress::isKeyCurrentlyDown(keyCodes[offset]))
                    notesToRelease.push_back(note);
            }
        }
        for (int n : notesToRelease)
            triggerNoteUp(n);
    }
    return true;
}

} // namespace MidiFlux
