#include "VirtualKeyboardComponent.h"
#include <cmath>
#include <algorithm>

namespace MidiFlux
{

static juce::String noteNumberToName(int note)
{
    if (note < 0 || note > 127) return "";
    static const char* const noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int oct = note / 12 - 1;
    return juce::String(noteNames[note % 12]) + juce::String(oct);
}

// ==============================================================================
// KeysComponent Implementation
// ==============================================================================

VirtualKeyboardComponent::KeysComponent::KeysComponent(VirtualKeyboardComponent& o, int startNote, int numKeys)
    : owner(o), baseNote(startNote), totalKeys(numKeys)
{
}

bool VirtualKeyboardComponent::KeysComponent::isBlackKey(int noteNumber) const
{
    int semitone = (noteNumber % 12 + 12) % 12;
    return (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10);
}

int VirtualKeyboardComponent::KeysComponent::getNoteAtPosition(int x, int y) const
{
    int numWhiteKeys = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        if (!isBlackKey(baseNote + i))
            numWhiteKeys++;
    }
    if (numWhiteKeys <= 0) return -1;

    float blackKeyWidth = whiteKeyWidth * 0.62f;
    float blackKeyHeight = getHeight() * 0.62f;

    // Check black keys first (top layer)
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
                float kx = whiteIdx * whiteKeyWidth - (blackKeyWidth * 0.5f);
                if (x >= kx && x <= kx + blackKeyWidth)
                    return note;
            }
        }
    }

    // Check white keys
    int wIdx = static_cast<int>(x / whiteKeyWidth);
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

void VirtualKeyboardComponent::KeysComponent::paint(juce::Graphics& g)
{
    int numWhiteKeys = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        if (!isBlackKey(baseNote + i))
            numWhiteKeys++;
    }
    if (numWhiteKeys <= 0) return;

    float whiteKeyHeight = (float)getHeight();
    float blackKeyWidth = whiteKeyWidth * 0.62f;
    float blackKeyHeight = whiteKeyHeight * 0.62f;

    // 1. Draw White Keys
    int whiteIdx = 0;
    for (int i = 0; i < totalKeys; ++i)
    {
        int note = baseNote + i;
        if (!isBlackKey(note))
        {
            float x = whiteIdx * whiteKeyWidth;
            auto keyRect = juce::Rectangle<float>(x, 0.0f, whiteKeyWidth, whiteKeyHeight).reduced(0.5f, 0.0f);

            bool isActive = (owner.activeNotes.find(note) != owner.activeNotes.end());

            if (isActive)
            {
                g.setColour(juce::Colour(0xff00e5ff));
                g.fillRoundedRectangle(keyRect, 2.5f);
            }
            else
            {
                g.setColour(juce::Colour(0xffe6edf3));
                g.fillRoundedRectangle(keyRect, 2.5f);
            }

            g.setColour(juce::Colour(0xff21262d));
            g.drawRoundedRectangle(keyRect, 2.5f, 1.0f);

            // Prominent Octave Badge on C keys
            if ((note % 12) == 0)
            {
                int oct = note / 12 - 1;
                bool isMidC = (note == 60);

                auto badgeRect = keyRect.removeFromBottom(22.0f).reduced(2.0f, 2.0f);

                if (isActive)
                {
                    g.setColour(juce::Colour(0xff0d1117));
                    g.fillRoundedRectangle(badgeRect, 3.0f);
                    g.setColour(juce::Colour(0xff00e5ff));
                    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
                    g.drawText(isMidC ? "C4 MID" : ("C" + juce::String(oct)), badgeRect, juce::Justification::centred, false);
                }
                else if (isMidC)
                {
                    // Middle C accent badge
                    g.setColour(juce::Colour(0xff0d1117));
                    g.fillRoundedRectangle(badgeRect, 3.0f);
                    g.setColour(juce::Colour(0xff00e5ff));
                    g.drawRoundedRectangle(badgeRect, 3.0f, 1.5f);
                    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
                    g.drawText("C4 MID", badgeRect, juce::Justification::centred, false);
                }
                else
                {
                    // Standard C octave badge
                    g.setColour(juce::Colour(0xff161b22));
                    g.fillRoundedRectangle(badgeRect, 3.0f);
                    g.setColour(juce::Colour(0xff8b949e));
                    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
                    g.drawText("C" + juce::String(oct), badgeRect, juce::Justification::centred, false);
                }
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
            float x = whiteIdx * whiteKeyWidth - (blackKeyWidth * 0.5f);
            auto keyRect = juce::Rectangle<float>(x, 0.0f, blackKeyWidth, blackKeyHeight);

            bool isActive = (owner.activeNotes.find(note) != owner.activeNotes.end());

            if (isActive)
            {
                g.setColour(juce::Colour(0xffa855f7));
                g.fillRoundedRectangle(keyRect, 2.0f);
            }
            else
            {
                g.setColour(juce::Colour(0xff161b22));
                g.fillRoundedRectangle(keyRect, 2.0f);
            }

            g.setColour(juce::Colour(0xff30363d));
            g.drawRoundedRectangle(keyRect, 2.0f, 1.0f);
        }
    }
}

void VirtualKeyboardComponent::KeysComponent::mouseDown(const juce::MouseEvent& e)
{
    owner.grabKeyboardFocus();

    if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
    {
        isRightDragging = true;
        dragStartX = e.getScreenX();
        dragStartViewX = owner.viewport.getViewPositionX();
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        owner.repaint();
        return;
    }

    int note = getNoteAtPosition(e.x, e.y);
    if (note != -1)
    {
        float normY = juce::jlimit(0.0f, 1.0f, (float)e.y / (float)getHeight());
        uint8_t vel = static_cast<uint8_t>(juce::jlimit(25, 127, (int)(30.0f + normY * 97.0f)));

        if (owner.isHoldEnabled())
        {
            // Toggle chord note latch
            if (owner.activeNotes.find(note) != owner.activeNotes.end())
                owner.triggerNoteUp(note);
            else
                owner.triggerNoteDown(note, vel);
        }
        else
        {
            owner.currentMouseNote = note;
            owner.triggerNoteDown(note, vel);
        }
    }
}

void VirtualKeyboardComponent::KeysComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isRightDragging)
    {
        int deltaX = e.getScreenX() - dragStartX;
        int newViewX = dragStartViewX - deltaX;
        int maxViewX = std::max(0, getWidth() - owner.viewport.getViewWidth());
        owner.viewport.setViewPosition(juce::jlimit(0, maxViewX, newViewX), 0);
        owner.repaint();
        return;
    }

    if (owner.isHoldEnabled())
        return; // Don't glissando during chord hold mode

    int note = getNoteAtPosition(e.x, e.y);
    if (note != owner.currentMouseNote)
    {
        if (owner.currentMouseNote != -1)
            owner.triggerNoteUp(owner.currentMouseNote);

        owner.currentMouseNote = note;
        if (note != -1)
        {
            float normY = juce::jlimit(0.0f, 1.0f, (float)e.y / (float)getHeight());
            uint8_t vel = static_cast<uint8_t>(juce::jlimit(25, 127, (int)(30.0f + normY * 97.0f)));
            owner.triggerNoteDown(note, vel);
        }
    }
}

void VirtualKeyboardComponent::KeysComponent::mouseUp(const juce::MouseEvent&)
{
    if (isRightDragging)
    {
        isRightDragging = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        owner.repaint();
        return;
    }

    if (!owner.isHoldEnabled() && owner.currentMouseNote != -1)
    {
        owner.triggerNoteUp(owner.currentMouseNote);
        owner.currentMouseNote = -1;
    }
}

void VirtualKeyboardComponent::KeysComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    float delta = (std::abs(wheel.deltaX) > std::abs(wheel.deltaY)) ? wheel.deltaX : wheel.deltaY;
    int scrollPx = static_cast<int>(delta * 140.0f);
    int newX = owner.viewport.getViewPositionX() - scrollPx;
    int maxViewX = std::max(0, getWidth() - owner.viewport.getViewWidth());
    owner.viewport.setViewPosition(juce::jlimit(0, maxViewX, newX), 0);
    owner.repaint();
}

// ==============================================================================
// VirtualKeyboardComponent Implementation
// ==============================================================================

VirtualKeyboardComponent::VirtualKeyboardComponent(int startNote, int numKeys)
    : baseNote(startNote), totalKeys(numKeys)
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Viewport & Keys (takes full width)
    keys = std::make_unique<KeysComponent>(*this, baseNote, totalKeys);
    viewport.setViewedComponent(keys.get(), false);
    viewport.setScrollBarsShown(false, false); // Clean borderless look; drag & buttons handle scrolling
    addAndMakeVisible(viewport);

    // Floating Hold Button (top-left glass pill)
    holdButton.setClickingTogglesState(true);
    holdButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xd021262d));
    holdButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xd0d29922)); // Amber
    holdButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    holdButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6edf3));
    holdButton.setTooltip("Latch notes to build chords note-by-note");
    holdButton.onClick = [this]() {
        setHoldEnabled(holdButton.getToggleState());
    };
    addAndMakeVisible(holdButton);

    // Floating Clear Button (top-left glass pill)
    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xd021262d));
    clearButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff7b72));
    clearButton.setTooltip("Release all active held notes");
    clearButton.onClick = [this]() {
        clearAllHeldNotes();
    };
    addAndMakeVisible(clearButton);

    // Floating Octave Shift Buttons (top-right glass pills)
    octDownButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xd021262d));
    octDownButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    octDownButton.setTooltip("Scroll keyboard left one octave");
    octDownButton.onClick = [this]() { scrollOctave(-1); };
    addAndMakeVisible(octDownButton);

    octUpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xd021262d));
    octUpButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8b949e));
    octUpButton.setTooltip("Scroll keyboard right one octave");
    octUpButton.onClick = [this]() { scrollOctave(1); };
    addAndMakeVisible(octUpButton);
}

VirtualKeyboardComponent::~VirtualKeyboardComponent()
{
    clearAllHeldNotes();
    removeKeyListener(this);
}

void VirtualKeyboardComponent::setHoldEnabled(bool enabled)
{
    holdEnabled = enabled;
    holdButton.setToggleState(holdEnabled, juce::dontSendNotification);
    holdButton.setButtonText(holdEnabled ? "HOLD ON" : "HOLD CHORD");

    if (!holdEnabled)
        clearAllHeldNotes();
}

void VirtualKeyboardComponent::clearAllHeldNotes()
{
    std::vector<int> notes(activeNotes.begin(), activeNotes.end());
    for (int n : notes)
        triggerNoteUp(n);
    activeNotes.clear();
    currentMouseNote = -1;
    if (keys)
        keys->repaint();
}

void VirtualKeyboardComponent::scrollOctave(int direction)
{
    if (!keys) return;
    int curX = viewport.getViewPositionX();
    int shift = static_cast<int>(7.0f * keys->getWhiteKeyWidth() * direction);
    int maxX = std::max(0, keys->getWidth() - viewport.getViewWidth());
    viewport.setViewPosition(juce::jlimit(0, maxX, curX + shift), 0);
    repaint();
}

void VirtualKeyboardComponent::triggerNoteDown(int note, uint8_t velocity)
{
    if (note >= 0 && note <= 127 && activeNotes.find(note) == activeNotes.end())
    {
        activeNotes.insert(note);
        if (onNoteOn)
            onNoteOn(1, note, velocity);
        if (keys)
            keys->repaint();
    }
}

void VirtualKeyboardComponent::triggerNoteUp(int note)
{
    if (note >= 0 && activeNotes.find(note) != activeNotes.end())
    {
        activeNotes.erase(note);
        if (onNoteOff)
            onNoteOff(1, note);
        if (keys)
            keys->repaint();
    }
}

void VirtualKeyboardComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d1117));
}

void VirtualKeyboardComponent::paintOverChildren(juce::Graphics& g)
{
    if (!keys || viewport.getViewWidth() <= 0) return;

    int viewX = viewport.getViewPositionX();
    int viewW = viewport.getViewWidth();
    int firstNote = keys->getNoteAtPosition(viewX + 16, getHeight() - 12);
    int lastNote = keys->getNoteAtPosition(viewX + viewW - 16, getHeight() - 12);

    if (firstNote >= 0 && lastNote >= 0)
    {
        juce::String hudText = "OCTAVES: " + noteNumberToName(firstNote) + " - " + noteNumberToName(lastNote)
                             + "  (Right-Click Drag or Wheel to Scroll)";

        auto hudRect = juce::Rectangle<float>((getWidth() - 340) * 0.5f, 4.0f, 340.0f, 20.0f);
        g.setColour(juce::Colour(0xd00d1117));
        g.fillRoundedRectangle(hudRect, 10.0f);
        g.setColour(juce::Colour(0xff30363d));
        g.drawRoundedRectangle(hudRect, 10.0f, 1.0f);

        g.setColour(juce::Colour(0xff00e5ff));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(hudText, hudRect, juce::Justification::centred, false);
    }
}

void VirtualKeyboardComponent::resized()
{
    int w = getWidth();
    int h = getHeight();

    // 1. Full-width viewport
    viewport.setBounds(0, 0, w, h);

    if (keys)
    {
        int numWhiteKeys = 0;
        for (int i = 0; i < totalKeys; ++i)
        {
            if (!keys->isBlackKey(baseNote + i))
                numWhiteKeys++;
        }
        int totalKeyW = static_cast<int>(numWhiteKeys * keys->getWhiteKeyWidth());
        keys->setBounds(0, 0, totalKeyW, h);

        // Center on Middle C (note 60) on initial layout
        static bool initialCentered = false;
        if (!initialCentered && totalKeyW > w)
        {
            initialCentered = true;
            int midCWhiteIdx = 0;
            for (int i = 0; i < (60 - baseNote); ++i)
            {
                if (!keys->isBlackKey(baseNote + i))
                    midCWhiteIdx++;
            }
            int midCX = static_cast<int>(midCWhiteIdx * keys->getWhiteKeyWidth());
            int targetViewX = midCX - (w / 2);
            int maxViewX = std::max(0, totalKeyW - w);
            viewport.setViewPosition(juce::jlimit(0, maxViewX, targetViewX), 0);
        }
    }

    // 2. Floating pill buttons
    holdButton.setBounds(8, 4, 82, 20);
    clearButton.setBounds(94, 4, 48, 20);

    octDownButton.setBounds(w - 98, 4, 44, 20);
    octUpButton.setBounds(w - 50, 4, 44, 20);
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
        int note = 48 + offset; // Start at C3
        triggerNoteDown(note);
        return true;
    }
    return false;
}

bool VirtualKeyboardComponent::keyStateChanged(bool isKeyDown, juce::Component*)
{
    if (!isKeyDown && !holdEnabled)
    {
        std::vector<int> notesToRelease;
        for (int note : activeNotes)
        {
            int offset = note - 48;
            if (offset >= 0 && offset <= 14)
            {
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
