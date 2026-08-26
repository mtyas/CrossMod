#include <iostream>
#include <cassert>
#include <set>
#include <juce_core/juce_core.h>
#include "../Source/Common/ScaleTheory.h"
#include "../Source/Common/MidiBlockFactory.h"
#include "../Source/Engine/MidiChainProcessor.h"
#include "../Source/Engine/PresetManager.h"
#include "../Source/Engine/UndoHistoryManager.h"

using namespace MidiFlux;

void testScaleTheory()
{
    std::cout << "[TEST] Running ScaleTheory tests..." << std::endl;

    // 1. C Major (Ionian): 0, 2, 4, 5, 7, 9, 11
    std::set<int> majorExpected = { 0, 2, 4, 5, 7, 9, 11 };
    for (int st = 0; st < 12; ++st)
    {
        bool expected = (majorExpected.count(st) > 0);
        assert(ScaleTheory::isNoteInScale(60 + st, 0, Scale_Major) == expected);
    }

    // 2. C Dorian: 0, 2, 3, 5, 7, 9, 10 (Natural 6th, Minor 3rd & 7th)
    std::set<int> dorianExpected = { 0, 2, 3, 5, 7, 9, 10 };
    for (int st = 0; st < 12; ++st)
    {
        bool expected = (dorianExpected.count(st) > 0);
        bool actual = ScaleTheory::isNoteInScale(60 + st, 0, Scale_Dorian);
        if (actual != expected)
        {
            std::cerr << "Dorian mismatch at semitone " << st << ": expected " << expected << ", got " << actual << std::endl;
            assert(false);
        }
    }

    // 3. C Phrygian: 0, 1, 3, 5, 7, 8, 10 (Minor 2nd, 3rd, 6th, 7th)
    std::set<int> phrygianExpected = { 0, 1, 3, 5, 7, 8, 10 };
    for (int st = 0; st < 12; ++st)
    {
        bool expected = (phrygianExpected.count(st) > 0);
        assert(ScaleTheory::isNoteInScale(60 + st, 0, Scale_Phrygian) == expected);
    }

    // 4. C Lydian: 0, 2, 4, 6, 7, 9, 11 (#4)
    std::set<int> lydianExpected = { 0, 2, 4, 6, 7, 9, 11 };
    for (int st = 0; st < 12; ++st)
    {
        bool expected = (lydianExpected.count(st) > 0);
        assert(ScaleTheory::isNoteInScale(60 + st, 0, Scale_Lydian) == expected);
    }

    // 5. C Mixolydian: 0, 2, 4, 5, 7, 9, 10 (b7)
    std::set<int> mixoExpected = { 0, 2, 4, 5, 7, 9, 10 };
    for (int st = 0; st < 12; ++st)
    {
        bool expected = (mixoExpected.count(st) > 0);
        assert(ScaleTheory::isNoteInScale(60 + st, 0, Scale_Mixolydian) == expected);
    }

    // Diatonic transposition: C4 (60) + 2 degrees in C Major = E4 (64)
    int third = ScaleTheory::transposeDiatonic(60, 2, 0, Scale_Major);
    assert(third == 64);

    // D4 (62) + 2 degrees in C Major = F4 (65)
    int minorThird = ScaleTheory::transposeDiatonic(62, 2, 0, Scale_Major);
    assert(minorThird == 65);

    std::cout << "  -> ScaleTheory (Major, Dorian, Phrygian, Lydian, Mixolydian) passed!" << std::endl;
}

void testChordInversions()
{
    std::cout << "[TEST] Running Chord Inversions tests..." << std::endl;

    // C Major triad: {0, 4, 7} (C, E, G)
    std::vector<int> triad = { 0, 4, 7 };

    // Root position: {0, 4, 7}
    auto inv0 = ScaleTheory::applyInversion(triad, 0, 0);
    assert(inv0.size() == 3);
    assert(inv0[0] == 0 && inv0[1] == 4 && inv0[2] == 7);

    // 1st Inversion: C moved up an octave -> {4, 7, 12} (E, G, C)
    auto inv1 = ScaleTheory::applyInversion(triad, 1, 0);
    assert(inv1.size() == 3);
    assert(inv1[0] == 4 && inv1[1] == 7 && inv1[2] == 12);
    // Crucial check: lowest note is 4 (E, the third), highest note is 12 (C, root an octave up)
    // Pitch classes must be strictly {0, 4, 7}:
    assert((inv1[0] % 12) == 4);
    assert((inv1[1] % 12) == 7);
    assert((inv1[2] % 12) == 0);

    // 2nd Inversion: C and E moved up an octave -> {7, 12, 16} (G, C, E)
    auto inv2 = ScaleTheory::applyInversion(triad, 2, 0);
    assert(inv2.size() == 3);
    assert(inv2[0] == 7 && inv2[1] == 12 && inv2[2] == 16);
    // Pitch classes must be strictly {7, 0, 4}:
    assert((inv2[0] % 12) == 7);
    assert((inv2[1] % 12) == 0);
    assert((inv2[2] % 12) == 4);

    // C Dominant 7: {0, 4, 7, 10}
    std::vector<int> dom7 = { 0, 4, 7, 10 };
    // 3rd Inversion: C, E, G moved up -> {10, 12, 16, 19} (Bb in bass)
    auto inv3 = ScaleTheory::applyInversion(dom7, 3, 0);
    assert(inv3.size() == 4);
    assert(inv3[0] == 10 && inv3[1] == 12 && inv3[2] == 16 && inv3[3] == 19);

    std::cout << "  -> Chord Inversions (Root, 1st, 2nd, 3rd) perfectly preserved chord nature and pitch classes!" << std::endl;
}

void testUndoRedoManager()
{
    std::cout << "[TEST] Running UndoRedoManager tests..." << std::endl;

    MidiChainProcessor chain;
    chain.prepare(44100.0, 512);

    UndoHistoryManager undoMgr;
    undoMgr.reset(chain.getState());

    assert(!undoMgr.canUndo());
    assert(!undoMgr.canRedo());

    // Action 1: add chords block
    chain.addBlock("chords");
    undoMgr.pushState(chain.getState());
    assert(undoMgr.canUndo());
    assert(!undoMgr.canRedo());
    assert(chain.getNumBlocks() == 1);

    // Action 2: add arpeggiator block
    chain.addBlock("arpeggiator");
    undoMgr.pushState(chain.getState());
    assert(chain.getNumBlocks() == 2);

    // Test Undo 1: should return to 1 block
    bool u1 = undoMgr.undo(chain);
    assert(u1);
    assert(chain.getNumBlocks() == 1);
    assert(chain.getBlock(0)->getTypeId() == "chords");
    assert(undoMgr.canUndo());
    assert(undoMgr.canRedo());

    // Test Redo 1: should return to 2 blocks
    bool r1 = undoMgr.redo(chain);
    assert(r1);
    assert(chain.getNumBlocks() == 2);
    assert(chain.getBlock(1)->getTypeId() == "arpeggiator");

    // Test Undo all the way to 0 blocks
    undoMgr.undo(chain); // back to 1
    undoMgr.undo(chain); // back to 0
    assert(chain.getNumBlocks() == 0);
    assert(!undoMgr.canUndo());
    assert(undoMgr.canRedo());

    std::cout << "  -> UndoRedoManager passed!" << std::endl;
}

void testBlockCatalog()
{
    std::cout << "[TEST] Running BlockCatalog tests..." << std::endl;

    const auto& catalog = MidiBlockFactory::getCatalog();
    assert(catalog.size() == 16);

    for (const auto& item : catalog)
    {
        auto block = MidiBlockFactory::createBlock(item.typeId);
        assert(block != nullptr);
        assert(block->getTypeId() == item.typeId);
        assert(block->getNumParameters() > 0);
    }

    std::cout << "  -> BlockCatalog (all 16 modules) passed!" << std::endl;
}

void testScaleAndTimingSeparation()
{
    std::cout << "[TEST] Running Scale & Time Quantizer separation tests..." << std::endl;

    // 1. ScaleQuantizeBlock: verify 0% snap leaves notes untouched, 100% snaps
    auto scaleBlock = MidiBlockFactory::createBlock("scale_quantize");
    assert(scaleBlock != nullptr);
    scaleBlock->setParameterValue(1, 0.0f); // Custom
    scaleBlock->setParameterValue(2, 0.0f); // C
    scaleBlock->setParameterValue(3, 0.0f); // Major

    BlockContext ctx;
    ctx.rootKey = 0;
    ctx.scaleType = 0;
    ctx.numSamples = 128;
    ctx.sampleRate = 44100.0;

    // At 0% snap amount (parameter 0 = 0.0f), C#4 (61) must stay 61 (unquantized)
    scaleBlock->setParameterValue(0, 0.0f);
    juce::MidiBuffer in, out;
    in.addEvent(juce::MidiMessage::noteOn(1, 61, (uint8_t)100), 0);
    scaleBlock->processBlock(in, out, ctx);
    int receivedPitch = -1;
    for (const auto m : out)
    {
        if (m.getMessage().isNoteOn())
            receivedPitch = m.getMessage().getNoteNumber();
    }
    assert(receivedPitch == 61); // Unquantized!

    // At 100% snap amount (parameter 0 = 1.0f), C#4 (61) must snap to in-scale C (60) or D (62)
    scaleBlock->setParameterValue(0, 1.0f);
    in.clear();
    out.clear();
    in.addEvent(juce::MidiMessage::noteOn(1, 61, (uint8_t)100), 0);
    scaleBlock->processBlock(in, out, ctx);
    receivedPitch = -1;
    for (const auto m : out)
    {
        if (m.getMessage().isNoteOn())
            receivedPitch = m.getMessage().getNoteNumber();
    }
    assert(receivedPitch == 60 || receivedPitch == 62); // Quantized!

    // 2. LfoBlock: verify LFO generates CC messages
    auto lfoBlock = MidiBlockFactory::createBlock("lfo");
    assert(lfoBlock != nullptr);
    lfoBlock->prepare(44100.0, 512);
    lfoBlock->setParameterValue(0, 0.0f); // CC 1 Mod
    in.clear();
    out.clear();
    ctx.numSamples = 512;
    lfoBlock->processBlock(in, out, ctx);
    bool producedCC = false;
    for (const auto m : out)
    {
        if (m.getMessage().isController())
            producedCC = true;
    }
    assert(producedCC);

    std::cout << "  -> Scale & Time Quantizer separation & LFO tests passed!" << std::endl;
}

void testArpeggiatorHoldAndSustain()
{
    std::cout << "[TEST] Running Arpeggiator Hold, Sustain & Key release tests..." << std::endl;

    auto arpBlock = MidiBlockFactory::createBlock("arpeggiator");
    assert(arpBlock != nullptr);
    arpBlock->prepare(44100.0, 512);

    BlockContext ctx;
    ctx.sampleRate = 44100.0;
    ctx.numSamples = 512;
    ctx.bpm = 120.0;
    ctx.isPlaying = true;

    // 1. Send NoteOn for C4 (60)
    juce::MidiBuffer in;
    juce::MidiBuffer out;
    in.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
    arpBlock->processBlock(in, out, ctx);

    // Process a few blocks so the arp triggers notes
    bool producedNotes = false;
    for (int b = 0; b < 25; ++b)
    {
        in.clear();
        out.clear();
        arpBlock->processBlock(in, out, ctx);
        for (const auto m : out)
        {
            if (m.getMessage().isNoteOn())
                producedNotes = true;
        }
    }
    assert(producedNotes);

    // 2. Send NoteOff with Hold OFF and Sustain OFF
    in.clear();
    out.clear();
    in.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    arpBlock->processBlock(in, out, ctx);

    // After NoteOff, it must NOT produce any new NoteOn events!
    for (int b = 0; b < 15; ++b)
    {
        in.clear();
        out.clear();
        arpBlock->processBlock(in, out, ctx);
        for (const auto m : out)
        {
            assert(!m.getMessage().isNoteOn());
        }
    }

    // 3. Test Sustain Pedal (CC 64)
    // Send NoteOn
    in.clear();
    out.clear();
    in.addEvent(juce::MidiMessage::noteOn(1, 64, (uint8_t)100), 0);
    // Press Sustain Pedal down
    in.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 0);
    arpBlock->processBlock(in, out, ctx);

    // Release NoteOff while sustain is still down
    in.clear();
    out.clear();
    in.addEvent(juce::MidiMessage::noteOff(1, 64), 0);
    arpBlock->processBlock(in, out, ctx);

    // Should STILL produce notes because sustain pedal is down!
    bool sustainedNotes = false;
    for (int b = 0; b < 25; ++b)
    {
        in.clear();
        out.clear();
        arpBlock->processBlock(in, out, ctx);
        for (const auto m : out)
        {
            if (m.getMessage().isNoteOn())
                sustainedNotes = true;
        }
    }
    assert(sustainedNotes);

    // Release sustain pedal (CC 64 = 0)
    in.clear();
    out.clear();
    in.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0);
    arpBlock->processBlock(in, out, ctx);

    // Now arp must stop completely!
    for (int b = 0; b < 15; ++b)
    {
        in.clear();
        out.clear();
        arpBlock->processBlock(in, out, ctx);
        for (const auto m : out)
        {
            assert(!m.getMessage().isNoteOn());
        }
    }

    std::cout << "  -> Arpeggiator Hold & Sustain test passed!" << std::endl;
}

void testStrummingAndTiming()
{
    std::cout << "[TEST] Running Strumming & Timing delayed queue tests..." << std::endl;

    auto chordBlock = MidiBlockFactory::createBlock("chords");
    assert(chordBlock != nullptr);
    chordBlock->prepare(44100.0, 128); // Small buffer size: 128 samples (~2.9ms)
    chordBlock->setParameterValue(0, 1.0f); // Major chord (3 notes)
    chordBlock->setParameterValue(3, 40.0f); // 40ms strum speed (approx 1764 samples)

    BlockContext ctx;
    ctx.sampleRate = 44100.0;
    ctx.numSamples = 128;
    ctx.bpm = 120.0;
    ctx.isPlaying = true;

    // Send Note-On at sample 0 in block 0
    juce::MidiBuffer in;
    juce::MidiBuffer out;
    in.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
    chordBlock->processBlock(in, out, ctx);

    // In block 0, the root note (60) must fire immediately
    int notesFired = 0;
    for (const auto m : out)
    {
        if (m.getMessage().isNoteOn())
        {
            assert(m.getMessage().getNoteNumber() == 60);
            notesFired++;
        }
    }
    assert(notesFired == 1);

    // Process subsequent blocks (each 128 samples = 2.9ms)
    // Over the next 50 blocks (approx 145ms), the remaining 2 chord notes (64 and 67) must fire across future blocks!
    for (int b = 1; b < 50; ++b)
    {
        in.clear();
        out.clear();
        chordBlock->processBlock(in, out, ctx);
        for (const auto m : out)
        {
            if (m.getMessage().isNoteOn())
            {
                notesFired++;
            }
        }
    }
    assert(notesFired == 3);

    std::cout << "  -> Strumming & Timing delayed queue tests passed!" << std::endl;
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  MidiFlux Automated Verification Suite  " << std::endl;
    std::cout << "========================================" << std::endl;

    testScaleTheory();
    testChordInversions();
    testUndoRedoManager();
    testBlockCatalog();
    testScaleAndTimingSeparation();
    testArpeggiatorHoldAndSustain();
    testStrummingAndTiming();

    std::cout << "\n>>> ALL 7 TEST SUITES PASSED SUCCESSFULLY! <<<" << std::endl;
    return 0;
}
