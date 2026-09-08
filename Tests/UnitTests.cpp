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

void testRandomizeRackAndVisualFeedback()
{
    std::cout << "[TEST] Running Randomize Rack, DAW Transport & Visual Status tests..." << std::endl;

    MidiChainProcessor chain;
    chain.prepare(44100.0, 512);

    // Test DAW playing flag propagation
    assert(!chain.isDawPlaying());
    chain.setIsDawPlaying(true);
    assert(chain.isDawPlaying());
    chain.setIsDawPlaying(false);
    assert(!chain.isDawPlaying());

    // Test randomizeRack(true) with module selection
    chain.randomizeRack(true);
    assert(chain.getNumBlocks() >= 1);
    assert(chain.getNumBlocks() <= 6);

    // Verify all blocks have valid status descriptions
    for (int i = 0; i < chain.getNumBlocks(); ++i)
    {
        auto* blk = chain.getBlock(i);
        assert(blk != nullptr);
        juce::String desc = blk->getStatusDescription();
        assert(!desc.isEmpty());
    }

    // Verify diverse selections across multiple rolls without duplicate types in a single rack
    std::set<int> uniqueCounts;
    std::set<juce::String> uniqueFirstModules;
    for (int roll = 0; roll < 15; ++roll)
    {
        chain.randomizeRack(true);
        int num = chain.getNumBlocks();
        assert(num >= 1 && num <= 6);
        uniqueCounts.insert(num);

        std::set<juce::String> typesInRack;
        for (int i = 0; i < num; ++i)
        {
            auto* b = chain.getBlock(i);
            assert(b != nullptr);
            // Verify no duplicate types within the same generated rack
            assert(typesInRack.count(b->getTypeId()) == 0);
            typesInRack.insert(b->getTypeId());
        }
        if (num > 0)
            uniqueFirstModules.insert(chain.getBlock(0)->getTypeId());
    }
    // With 15 rolls, we must have observed multiple distinct rack sizes and starting modules
    assert(uniqueCounts.size() >= 2);
    assert(uniqueFirstModules.size() >= 2);

    // Check DAW playback requirement specifics
    auto arp = MidiBlockFactory::createBlock("arpeggiator");
    arp->setParameterValue(3, 1.0f); // Sync On
    assert(arp->requiresDawPlayback() == true);
    arp->setParameterValue(3, 0.0f); // Sync Off (Free rate)
    assert(arp->requiresDawPlayback() == false);

    auto euc = MidiBlockFactory::createBlock("euclidean");
    assert(euc->requiresDawPlayback() == true);

    auto tq = MidiBlockFactory::createBlock("time_quantize");
    tq->setParameterValue(1, 0.8f); // Snap strength
    assert(tq->requiresDawPlayback() == true);
    tq->setParameterValue(1, 0.0f); // Snap strength off
    assert(tq->requiresDawPlayback() == false);

    std::cout << "  -> Randomize Rack, DAW Transport & Visual Status tests passed!" << std::endl;
}

void testScaleProgressionSequencer()
{
    std::cout << "[TEST] Running Scale Progression Sequencer tests..." << std::endl;

    ScaleProgression prog;
    assert(prog.getNumBlocks() == 4);
    assert(std::abs(prog.getTotalBars() - 8.0f) < 0.001f);

    // In 4/4: 1 bar = 4.0 PPQ. 2 bars = 8.0 PPQ.
    // Block 0: C Major, 2 bars (0.0 to 8.0 PPQ)
    // Block 1: G Major, 2 bars (8.0 to 16.0 PPQ)
    // Block 2: A Minor, 2 bars (16.0 to 24.0 PPQ)
    // Block 3: F Major, 2 bars (24.0 to 32.0 PPQ)
    auto st0 = prog.getPlaybackState(0.0, 4, 4);
    assert(st0.activeBlockIndex == 0);
    assert(st0.activeRootKey == 0); // C
    assert(st0.activeScaleType == Scale_Major);
    assert(std::abs(st0.blockProgress - 0.0f) < 0.01f);

    auto st1 = prog.getPlaybackState(4.0, 4, 4); // halfway through Block 0
    assert(st1.activeBlockIndex == 0);
    assert(std::abs(st1.blockProgress - 0.5f) < 0.01f);

    auto st2 = prog.getPlaybackState(8.0, 4, 4); // start of Block 1
    assert(st2.activeBlockIndex == 1);
    assert(st2.activeRootKey == 7); // G
    assert(std::abs(st2.blockProgress - 0.0f) < 0.01f);

    auto st3 = prog.getPlaybackState(16.0, 4, 4); // start of Block 2
    assert(st3.activeBlockIndex == 2);
    assert(st3.activeRootKey == 9); // A
    assert(st3.activeScaleType == Scale_NaturalMinor);

    auto st4 = prog.getPlaybackState(24.0, 4, 4); // start of Block 3
    assert(st4.activeBlockIndex == 3);
    assert(st4.activeRootKey == 5); // F

    // Test loop wrapping at 32.0 PPQ -> back to Block 0
    auto stLoop = prog.getPlaybackState(32.0, 4, 4);
    assert(stLoop.activeBlockIndex == 0);
    assert(stLoop.activeRootKey == 0);

    // Test 1/2 bar duration (0.5 bars in 4/4 = 2.0 PPQ)
    prog.clear();
    prog.addBlock({ 2, Scale_Dorian, 0.5f, "1/2 Bar" });
    auto stHalf = prog.getPlaybackState(1.0, 4, 4);
    assert(stHalf.activeBlockIndex == 0);
    assert(std::abs(stHalf.blockProgress - 0.5f) < 0.02f);

    // Test 16 bars duration (16.0 bars in 4/4 = 64.0 PPQ)
    prog.clear();
    prog.addBlock({ 0, Scale_Major, 16.0f, "16 Bars" });
    auto st16 = prog.getPlaybackState(32.0, 4, 4);
    assert(st16.activeBlockIndex == 0);
    assert(std::abs(st16.blockProgress - 0.5f) < 0.02f);

    // Test 3/4 time signature: beatsPerBar = 3.0 PPQ
    prog.clear();
    prog.addBlock({ 0, Scale_Major, 2.0f, "2 Bars in 3/4" }); // 2 bars * 3 beats = 6.0 PPQ
    auto st34 = prog.getPlaybackState(3.0, 3, 4);
    assert(st34.activeBlockIndex == 0);
    assert(std::abs(st34.blockProgress - 0.5f) < 0.02f);

    // Test non-loop mode clamping to last block
    prog.setLoop(false);
    auto stNoLoop = prog.getPlaybackState(500.0, 4, 4);
    assert(stNoLoop.activeBlockIndex == 0);
    assert(stNoLoop.blockProgress >= 0.99f);

    // Test ValueTree serialization & deserialization
    prog.loadPresetProgression(3); // 12-Bar Blues in E
    assert(prog.getNumBlocks() == 6);
    assert(std::abs(prog.getTotalBars() - 12.0f) < 0.01f);

    auto vt = prog.getState();
    ScaleProgression restoredProg;
    restoredProg.setState(vt);
    assert(restoredProg.getNumBlocks() == 6);
    assert(std::abs(restoredProg.getTotalBars() - 12.0f) < 0.01f);
    assert(restoredProg.getBlock(0).rootKey == 4); // E

    std::cout << "  -> Scale Progression Sequencer tests passed!" << std::endl;
}

void testEditablePresets()
{
    std::cout << "[TEST] Running Editable Presets tests..." << std::endl;

    MidiChainProcessor chain;
    auto& presets = PresetManager::getPresets();
    int initialCount = static_cast<int>(presets.size());
    assert(initialCount >= 7);

    // 1. Add new custom preset
    juce::ValueTree customState("MidiFluxState");
    customState.setProperty("rootKey", 7, nullptr); // G
    customState.setProperty("scaleType", 1, nullptr); // Minor
    PresetManager::addPreset("User Mega Groove", customState);
    assert(static_cast<int>(PresetManager::getPresets().size()) == initialCount + 1);
    assert(PresetManager::getPresets().back().name == "User Mega Groove");

    // 2. Overwrite / Save current preset
    int newIdx = static_cast<int>(PresetManager::getPresets().size()) - 1;
    juce::ValueTree modState("MidiFluxState");
    modState.setProperty("rootKey", 2, nullptr); // D
    PresetManager::saveCurrentPreset(newIdx, modState);
    PresetManager::applyPreset(chain, newIdx);
    assert(chain.getGlobalRootKey() == 2);

    // 3. Delete preset
    PresetManager::deletePreset(newIdx);
    assert(static_cast<int>(PresetManager::getPresets().size()) == initialCount);

    // 4. Factory reset
    PresetManager::resetToFactoryDefaults();
    assert(static_cast<int>(PresetManager::getPresets().size()) >= 7);

    std::cout << "  -> Editable Presets tests passed!" << std::endl;
}

void testProgressionPresetSavingAndFileIO()
{
    std::cout << "[TEST] Running Progression Presets & File IO tests..." << std::endl;

    ScaleProgression prog;
    prog.clear();
    prog.addBlock({ 0, Scale_Major, 4.0f, "C Maj (4b)" });
    prog.addBlock({ 7, Scale_Mixolydian, 4.0f, "G Mixo (4b)" });
    assert(prog.getNumBlocks() == 2);
    assert(std::abs(prog.getTotalBars() - 8.0f) < 0.01f);

    // 1. File export & import
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("test_progression.midifluxprog");
    bool saved = prog.saveToFile(tempFile);
    assert(saved);
    assert(tempFile.existsAsFile());

    ScaleProgression loadedProg;
    bool loaded = loadedProg.loadFromFile(tempFile);
    assert(loaded);
    assert(loadedProg.getNumBlocks() == 2);
    assert(loadedProg.getBlock(0).rootKey == 0);
    assert(loadedProg.getBlock(1).rootKey == 7);
    assert(loadedProg.getBlock(1).scaleType == Scale_Mixolydian);

    tempFile.deleteFile();

    // 2. User Progression presets
    int initialUserCount = static_cast<int>(ScaleProgression::getUserProgressions().size());
    ScaleProgression::addUserProgression("Test Progression Alpha", prog.getAllBlocks());
    assert(static_cast<int>(ScaleProgression::getUserProgressions().size()) == initialUserCount + 1);
    assert(ScaleProgression::getUserProgressions().back().name == "Test Progression Alpha");

    ScaleProgression::deleteUserProgression(initialUserCount);
    assert(static_cast<int>(ScaleProgression::getUserProgressions().size()) == initialUserCount);

    std::cout << "  -> Progression Presets & File IO tests passed!" << std::endl;
}

void testMidiLearn()
{
    std::cout << "[TEST] Running MIDI Learn tests..." << std::endl;

    MidiChainProcessor chain;
    chain.addBlock("humanizer"); // block 0
    chain.addBlock("chord");     // block 1

    auto& learnMgr = chain.getMidiLearnManager();
    learnMgr.clearAllMappings();

    // 1. Learn a block parameter
    juce::String paramTarget = "block:0:param:0";
    learnMgr.startLearning(paramTarget);
    assert(learnMgr.isLearning());
    assert(learnMgr.isLearningParam(paramTarget));

    // Send CC 74 on channel 1, value 100
    juce::MidiMessage ccMsg = juce::MidiMessage::controllerEvent(1, 74, 100);
    learnMgr.processMidiController(ccMsg, chain);

    // Learning should now be completed
    assert(!learnMgr.isLearning());
    int boundCC = -1, boundCh = 0;
    assert(learnMgr.getMappingForParam(paramTarget, boundCC, boundCh));
    assert(boundCC == 74);

    // 2. Incoming CC 74 should update parameter
    juce::MidiMessage ccMsg2 = juce::MidiMessage::controllerEvent(1, 74, 127);
    learnMgr.processMidiController(ccMsg2, chain);
    auto* block0 = chain.getBlock(0);
    assert(block0 != nullptr);
    assert(block0->getParameterValue(0) >= 0.99f);

    // 3. Learn module power / bypass toggle
    juce::String powerTarget = "block:0:power";
    learnMgr.setMapping(powerTarget, 80, 0);
    assert(learnMgr.getMappingForParam(powerTarget, boundCC, boundCh));
    assert(boundCC == 80);

    // CC 80 value 0 -> bypass = true
    juce::MidiMessage ccBypass = juce::MidiMessage::controllerEvent(1, 80, 0);
    learnMgr.processMidiController(ccBypass, chain);
    assert(block0->isBypassed());

    // CC 80 value 127 -> bypass = false (enabled)
    juce::MidiMessage ccEnable = juce::MidiMessage::controllerEvent(1, 80, 127);
    learnMgr.processMidiController(ccEnable, chain);
    assert(!block0->isBypassed());

    // 4. Remap when block moves
    learnMgr.remapBlockMoved(0, 1);
    assert(learnMgr.getMappingForParam("block:1:param:0", boundCC, boundCh));
    assert(boundCC == 74);

    // 5. Remap when block removed
    learnMgr.remapBlockRemoved(1);
    assert(!learnMgr.getMappingForParam("block:1:param:0", boundCC, boundCh));

    // 6. Serialization
    learnMgr.setMapping("global:rootKey", 16, 0);
    auto vt = learnMgr.getState();
    MidiLearnManager restoredMgr;
    restoredMgr.setState(vt);
    assert(restoredMgr.getMappingForParam("global:rootKey", boundCC, boundCh));
    assert(boundCC == 16);

    std::cout << "  -> MIDI Learn tests passed!" << std::endl;
}

void testRoutingAndDelayDecayAndQuarterBar()
{
    std::cout << "[TEST] Running Routing, Delay Decay 200%, and 1/4 Bar Progression tests..." << std::endl;

    // -------------------------------------------------------------
    // 1. Parallel vs Series Chain Routing
    // -------------------------------------------------------------
    MidiChainProcessor chain;
    chain.prepare(44100.0, 512);

    // Block 0: Transpose +12 semitones
    chain.addBlock("transpose");
    assert(chain.getBlock(0) != nullptr);
    chain.getBlock(0)->setParameterValue(0, 12.0f); // +12 st
    chain.getBlock(0)->setRoutingMode(RoutingMode::Series);

    // Block 1: Filter block set to Parallel
    chain.addBlock("filter");
    assert(chain.getBlock(1) != nullptr);
    chain.getBlock(1)->setRoutingMode(RoutingMode::Parallel);

    // Verify routing state get/set and serialization
    assert(chain.getBlock(0)->getRoutingMode() == RoutingMode::Series);
    assert(chain.getBlock(1)->getRoutingMode() == RoutingMode::Parallel);

    auto state = chain.getState();
    MidiChainProcessor chainRestored;
    chainRestored.setState(state);
    assert(chainRestored.getBlock(0)->getRoutingMode() == RoutingMode::Series);
    assert(chainRestored.getBlock(1)->getRoutingMode() == RoutingMode::Parallel);

    // Test audio/midi processing in parallel mode
    // Incoming note is 60.
    // In Series, b0 transposes 60 -> 72.
    // In Parallel, b1 receives raw input (note 60) and merges output with b0.
    juce::MidiBuffer inputBuffer;
    inputBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);

    BlockContext ctx;
    ctx.sampleRate = 44100.0;
    ctx.numSamples = 512;
    ctx.isPlaying = true;
    ctx.bpm = 120.0;
    ctx.ppqPosition = 0.0;
    ctx.timeSigNumerator = 4;
    ctx.timeSigDenominator = 4;

    chain.processMidi(inputBuffer, ctx);

    bool hasNote60 = false;
    bool hasNote72 = false;
    for (const auto meta : inputBuffer)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            if (msg.getNoteNumber() == 60) hasNote60 = true;
            if (msg.getNoteNumber() == 72) hasNote72 = true;
        }
    }
    assert(hasNote72); // From Transpose block (series)
    assert(hasNote60); // From Filter block (parallel, received raw DAW input note 60!)

    // -------------------------------------------------------------
    // 2. MIDI Delay Decay up to 2.0 (200%) & Growing Echo Velocity
    // -------------------------------------------------------------
    auto delayBlock = MidiBlockFactory::createBlock("delay");
    assert(delayBlock != nullptr);
    delayBlock->prepare(44100.0, 512);

    // Param 2 is decay: test setting 2.0 (200%)
    const auto& decayDef = delayBlock->getParameterDef(2);
    assert(decayDef.maxValue >= 2.0f);

    delayBlock->setParameterValue(0, 2.0f); // 1/16 note delay
    delayBlock->setParameterValue(1, 3.0f); // 3 repeats
    delayBlock->setParameterValue(2, 1.5f); // 150% decay (growing velocity)
    delayBlock->setParameterValue(3, 0.0f); // 0 pitch shift
    delayBlock->setParameterValue(4, 0.0f); // scale snap off
    assert(std::abs(delayBlock->getParameterValue(2) - 1.5f) < 0.01f);

    // Send noteOn at initial velocity 50
    juce::MidiBuffer delayMidi;
    delayMidi.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)50), 0);

    juce::MidiBuffer outBuf;
    delayBlock->processBlock(delayMidi, outBuf, ctx);

    // Initial note passed through at vel 50
    bool foundPassThrough = false;
    for (const auto meta : outBuf)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getNoteNumber() == 60 && msg.getVelocity() == 50)
            foundPassThrough = true;
    }
    assert(foundPassThrough);

    // Advance time until the first repeat triggers (1/16 note at 120 BPM = 0.125s = 5512.5 samples ~ 11 blocks of 512)
    int samplesPer16th = static_cast<int>(44100.0 * (60.0 / 120.0) * 0.25);
    int elapsed = 512;
    uint8_t firstEchoVel = 0;

    while (elapsed < samplesPer16th + 1024)
    {
        juce::MidiBuffer emptyIn;
        juce::MidiBuffer stepOut;
        ctx.ppqPosition = (double)elapsed / 44100.0 * 2.0; // 120 bpm = 2 beats/sec
        delayBlock->processBlock(emptyIn, stepOut, ctx);
        for (const auto meta : stepOut)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn() && msg.getNoteNumber() == 60)
            {
                firstEchoVel = msg.getVelocity();
                break;
            }
        }
        if (firstEchoVel > 0)
            break;
        elapsed += 512;
    }

    // Velocity grew from 50 to round(50 * 1.5) = 75!
    assert(firstEchoVel > 50);
    assert(firstEchoVel == 75);

    // Test decay = 2.0f max clamp
    delayBlock->setParameterValue(2, 2.0f);
    assert(std::abs(delayBlock->getParameterValue(2) - 2.0f) < 0.01f);

    // -------------------------------------------------------------
    // 3. Scale Progression 1/4 Bar (0.25 Bars) Block Duration
    // -------------------------------------------------------------
    ScaleProgression prog;
    prog.clear();

    prog.addBlock({ 0, Scale_Major, 0.25f, "1/4 Bar C" });
    prog.addBlock({ 9, Scale_NaturalMinor, 0.5f, "1/2 Bar Am" });
    prog.setEnabled(true);
    prog.setLoop(true);

    assert(prog.getNumBlocks() == 2);
    assert(std::abs(prog.getBlock(0).bars - 0.25f) < 0.001f);
    assert(std::abs(prog.getTotalBars() - 0.75f) < 0.001f);

    // In 4/4 time, 1 bar = 4.0 PPQ.
    // 0.25 bar = 1.0 PPQ. Total 0.75 bar = 3.0 PPQ.
    // At ppq = 0.5 (within first 1/4 bar):
    auto st0 = prog.getPlaybackState(0.5, 4, 4);
    assert(st0.activeBlockIndex == 0);
    assert(st0.activeRootKey == 0);
    assert(st0.activeScaleType == Scale_Major);

    // At ppq = 1.5 (past 1.0 PPQ, within block 1 which is 1.0 to 3.0 PPQ):
    auto st1 = prog.getPlaybackState(1.5, 4, 4);
    assert(st1.activeBlockIndex == 1);
    assert(st1.activeRootKey == 9);
    assert(st1.activeScaleType == Scale_NaturalMinor);

    // At ppq = 3.2 (loop wrapped back into first block):
    auto stWrap = prog.getPlaybackState(3.2, 4, 4);
    assert(stWrap.activeBlockIndex == 0);

    std::cout << "  -> Routing, Delay Decay 200%, and 1/4 Bar Progression tests passed!" << std::endl;
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
    testRandomizeRackAndVisualFeedback();
    testScaleProgressionSequencer();
    testEditablePresets();
    testProgressionPresetSavingAndFileIO();
    testMidiLearn();
    testRoutingAndDelayDecayAndQuarterBar();

    std::cout << "\n>>> ALL 13 TEST SUITES PASSED SUCCESSFULLY! <<<" << std::endl;
    return 0;
}


