#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <initializer_list>

namespace MidiFlux
{

enum ScaleType
{
    Scale_Major = 0,
    Scale_NaturalMinor,
    Scale_HarmonicMinor,
    Scale_MelodicMinor,
    Scale_Dorian,
    Scale_Phrygian,
    Scale_Lydian,
    Scale_Mixolydian,
    Scale_Locrian,
    Scale_PentatonicMajor,
    Scale_PentatonicMinor,
    Scale_Blues,
    Scale_WholeTone,
    Scale_Diminished,
    Scale_ArabicDoubleHarmonic,
    Scale_Hirajoshi,
    Scale_InSen,
    Scale_HungarianMinor,
    Scale_Chromatic,
    NumScales
};

enum ChordType
{
    Chord_DiatonicAuto = 0,
    Chord_Major,
    Chord_Minor,
    Chord_Dominant7,
    Chord_Major7,
    Chord_Minor7,
    Chord_Diminished,
    Chord_Diminished7,
    Chord_HalfDiminished7,
    Chord_Sus2,
    Chord_Sus4,
    Chord_Augmented,
    Chord_Ninth,
    Chord_Minor9,
    Chord_Major9,
    Chord_PowerChord,
    NumChords
};

struct ScaleInfo
{
    const char* name;
    uint16_t mask; // 12-bit mask: bit 0 = root, bit 1 = m2, etc.
};

class ScaleTheory
{
public:
    static constexpr uint16_t makeMask(std::initializer_list<int> semitones)
    {
        uint16_t mask = 0;
        for (int s : semitones)
        {
            if (s >= 0 && s < 12)
                mask |= static_cast<uint16_t>(1 << s);
        }
        return mask;
    }

    static const std::array<ScaleInfo, NumScales>& getAllScales()
    {
        static const std::array<ScaleInfo, NumScales> scales = {{
            { "Major (Ionian)",          makeMask({0, 2, 4, 5, 7, 9, 11}) },
            { "Natural Minor (Aeolian)", makeMask({0, 2, 3, 5, 7, 8, 10}) },
            { "Harmonic Minor",          makeMask({0, 2, 3, 5, 7, 8, 11}) },
            { "Melodic Minor",           makeMask({0, 2, 3, 5, 7, 9, 11}) },
            { "Dorian",                  makeMask({0, 2, 3, 5, 7, 9, 10}) }, // 0, 2, b3, 4, 5, 6, b7
            { "Phrygian",                makeMask({0, 1, 3, 5, 7, 8, 10}) }, // 0, b2, b3, 4, 5, b6, b7
            { "Lydian",                  makeMask({0, 2, 4, 6, 7, 9, 11}) }, // 0, 2, 3, #4, 5, 6, 7
            { "Mixolydian",              makeMask({0, 2, 4, 5, 7, 9, 10}) }, // 0, 2, 3, 4, 5, 6, b7
            { "Locrian",                 makeMask({0, 1, 3, 5, 6, 8, 10}) }, // 0, b2, b3, 4, b5, b6, b7
            { "Pentatonic Major",        makeMask({0, 2, 4, 7, 9}) },
            { "Pentatonic Minor",        makeMask({0, 3, 5, 7, 10}) },
            { "Blues",                   makeMask({0, 3, 5, 6, 7, 10}) },
            { "Whole Tone",              makeMask({0, 2, 4, 6, 8, 10}) },
            { "Diminished (W-H)",        makeMask({0, 2, 3, 5, 6, 8, 9, 11}) },
            { "Arabic / Double Harm.",   makeMask({0, 1, 4, 5, 7, 8, 11}) }, // 0, b2, 3, 4, 5, b6, 7
            { "Hirajoshi (Japanese)",    makeMask({0, 2, 3, 7, 8}) },         // 0, 2, b3, 5, b6
            { "In Sen (Japanese)",       makeMask({0, 1, 5, 7, 10}) },        // 0, b2, 4, 5, b7
            { "Hungarian Minor",         makeMask({0, 2, 3, 6, 7, 8, 11}) }, // 0, 2, b3, #4, 5, b6, 7
            { "Chromatic",               makeMask({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}) }
        }};
        return scales;
    }

    static const char* getNoteName(int noteNumber)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return names[(noteNumber % 12 + 12) % 12];
    }

    static juce::String noteName(int noteNumber)
    {
        int oct = (noteNumber / 12) - 1;
        return juce::String(getNoteName(noteNumber)) + juce::String(oct);
    }

    static const ScaleInfo& getScaleDef(int scaleType)
    {
        int idx = juce::jlimit(0, static_cast<int>(NumScales) - 1, scaleType);
        return getAllScales()[idx];
    }

    static bool isNoteInScale(int noteNumber, int rootKey, int scaleType)
    {
        if (scaleType < 0 || scaleType >= NumScales || scaleType == Scale_Chromatic)
            return true;

        uint16_t mask = getAllScales()[scaleType].mask;
        int semitoneFromRoot = ((noteNumber - rootKey) % 12 + 12) % 12;
        return (mask & (1 << semitoneFromRoot)) != 0;
    }

    // Quantize / Snap pitch to nearest note in scale
    static int quantizeToScale(int noteNumber, int rootKey, int scaleType, bool snapUpward = false)
    {
        if (scaleType < 0 || scaleType >= NumScales || scaleType == Scale_Chromatic)
            return juce::jlimit(0, 127, noteNumber);

        if (isNoteInScale(noteNumber, rootKey, scaleType))
            return juce::jlimit(0, 127, noteNumber);

        int bestNote = noteNumber;
        int minDistance = 999;

        for (int offset = -6; offset <= 6; ++offset)
        {
            int candidate = noteNumber + offset;
            if (candidate >= 0 && candidate <= 127 && isNoteInScale(candidate, rootKey, scaleType))
            {
                int dist = std::abs(offset);
                if (dist < minDistance || (dist == minDistance && snapUpward && offset > 0))
                {
                    minDistance = dist;
                    bestNote = candidate;
                }
            }
        }
        return juce::jlimit(0, 127, bestNote);
    }

    // Transpose note by diatonic steps in the given key/scale
    static int transposeDiatonic(int noteNumber, int steps, int rootKey, int scaleType)
    {
        if (steps == 0)
            return noteNumber;

        int current = quantizeToScale(noteNumber, rootKey, scaleType);
        int direction = steps > 0 ? 1 : -1;
        int remaining = std::abs(steps);

        while (remaining > 0)
        {
            current += direction;
            if (current < 0 || current > 127)
                break;

            if (isNoteInScale(current, rootKey, scaleType))
                --remaining;
        }

        return juce::jlimit(0, 127, current);
    }

    static std::vector<int> getChordIntervals(int rootNote, ChordType chordType, int rootKey, int scaleType)
    {
        if (chordType == Chord_DiatonicAuto)
        {
            int third = transposeDiatonic(rootNote, 2, rootKey, scaleType);
            int fifth = transposeDiatonic(rootNote, 4, rootKey, scaleType);
            return { 0, third - rootNote, fifth - rootNote };
        }

        switch (chordType)
        {
            case Chord_Major:             return { 0, 4, 7 };
            case Chord_Minor:             return { 0, 3, 7 };
            case Chord_Dominant7:         return { 0, 4, 7, 10 };
            case Chord_Major7:            return { 0, 4, 7, 11 };
            case Chord_Minor7:            return { 0, 3, 7, 10 };
            case Chord_Diminished:        return { 0, 3, 6 };
            case Chord_Diminished7:       return { 0, 3, 6, 9 };
            case Chord_HalfDiminished7:   return { 0, 3, 6, 10 };
            case Chord_Sus2:              return { 0, 2, 7 };
            case Chord_Sus4:              return { 0, 5, 7 };
            case Chord_Augmented:         return { 0, 4, 8 };
            case Chord_Ninth:             return { 0, 4, 7, 10, 14 };
            case Chord_Minor9:            return { 0, 3, 7, 10, 14 };
            case Chord_Major9:            return { 0, 4, 7, 11, 14 };
            case Chord_PowerChord:        return { 0, 7, 12 };
            default:                      return { 0, 4, 7 };
        }
    }

    // Apply inversion and voicing to chord intervals.
    // Preserves chord nature and root note relationship!
    static std::vector<int> applyInversion(const std::vector<int>& intervals, int inversion, int voicing)
    {
        if (intervals.empty())
            return intervals;

        std::vector<int> result = intervals;
        int n = static_cast<int>(result.size());

        // Inversion: Shift the first (inversion % n) notes up by 12 semitones.
        // DO NOT re-zero against the lowest note; rootNote stays unchanged.
        if (inversion > 0 && n > 1)
        {
            int inv = inversion % n;
            for (int i = 0; i < inv; ++i)
            {
                result[i] += 12;
            }
            std::sort(result.begin(), result.end());
        }

        // Voicing:
        // 0 = Close Voicing (standard)
        // 1 = Drop-2 (take 2nd note from top and drop down by 1 octave -12)
        // 2 = Drop-3 (take 3rd note from top and drop down by 1 octave -12)
        // 3 = Spread Open (transpose alternate notes up by 12)
        if (voicing == 1 && result.size() >= 3) // Drop-2
        {
            int dropIdx = static_cast<int>(result.size()) - 2;
            result[dropIdx] -= 12;
            std::sort(result.begin(), result.end());
        }
        else if (voicing == 2 && result.size() >= 4) // Drop-3
        {
            int dropIdx = static_cast<int>(result.size()) - 3;
            result[dropIdx] -= 12;
            std::sort(result.begin(), result.end());
        }
        else if (voicing == 3 && result.size() >= 2) // Spread Open
        {
            for (size_t i = 1; i < result.size(); i += 2)
            {
                result[i] += 12;
            }
            std::sort(result.begin(), result.end());
        }

        return result;
    }
};

} // namespace MidiFlux
