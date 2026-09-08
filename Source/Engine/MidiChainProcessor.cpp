#include "MidiChainProcessor.h"

namespace MidiFlux
{

MidiChainProcessor::MidiChainProcessor()
{
}

MidiChainProcessor::~MidiChainProcessor()
{
}

void MidiChainProcessor::prepare(double sampleRate, int maxSamplesPerBlock)
{
    std::lock_guard<std::recursive_mutex> lock(chainMutex);
    currentSampleRate = sampleRate;
    currentBlockSize = maxSamplesPerBlock;

    for (auto& block : blocks)
    {
        if (block)
            block->prepare(sampleRate, maxSamplesPerBlock);
    }
}

void MidiChainProcessor::reset()
{
    std::lock_guard<std::recursive_mutex> lock(chainMutex);
    for (auto& block : blocks)
    {
        if (block)
            block->reset();
    }
}

void MidiChainProcessor::panic(juce::MidiBuffer& outBuffer)
{
    std::lock_guard<std::recursive_mutex> lock(chainMutex);
    for (auto& block : blocks)
    {
        if (block)
            block->allNotesOff(outBuffer);
    }
    for (int ch = 1; ch <= 16; ++ch)
    {
        outBuffer.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
        outBuffer.addEvent(juce::MidiMessage::allSoundOff(ch), 0);
    }
}

void MidiChainProcessor::pushActivityEvent(int note, uint8_t vel, bool isNoteOn, bool isOutput)
{
    int w = activityWritePos.load(std::memory_order_relaxed);
    activityBuffer[w] = { note, vel, isNoteOn, isOutput };
    activityWritePos.store((w + 1) % activityBufferSize, std::memory_order_release);
}

int MidiChainProcessor::getPendingActivityEvents(std::vector<MidiActivityEvent>& dest)
{
    dest.clear();
    int w = activityWritePos.load(std::memory_order_acquire);
    while (activityReadPos != w)
    {
        dest.push_back(activityBuffer[activityReadPos]);
        activityReadPos = (activityReadPos + 1) % activityBufferSize;
    }
    return static_cast<int>(dest.size());
}

void MidiChainProcessor::processMidi(juce::MidiBuffer& midiBuffer, BlockContext ctx)
{
    // Log incoming MIDI for visualizer
    for (const auto meta : midiBuffer)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn())
            pushActivityEvent(msg.getNoteNumber(), msg.getVelocity(), true, false);
        else if (msg.isNoteOff())
            pushActivityEvent(msg.getNoteNumber(), 0, false, false);
        else if (msg.isController())
            midiLearnManager.processMidiController(msg, *this);
    }

    isDawPlayingFlag.store(ctx.isPlaying, std::memory_order_relaxed);

    if (masterBypassed.load(std::memory_order_relaxed))
        return;

    {
        std::unique_lock<std::recursive_mutex> lock(chainMutex, std::try_to_lock);
        if (lock.owns_lock() && scaleProgression.isEnabled())
        {
            auto pb = scaleProgression.getPlaybackState(ctx.ppqPosition, ctx.timeSigNumerator, ctx.timeSigDenominator);
            pb.isPlaying = ctx.isPlaying;
            lastPlaybackState = pb;

            ctx.rootKey = pb.activeRootKey;
            ctx.scaleType = pb.activeScaleType;
        }
        else
        {
            ctx.rootKey = globalRootKey.load(std::memory_order_relaxed);
            ctx.scaleType = globalScaleType.load(std::memory_order_relaxed);
        }
    }

    std::unique_lock<std::recursive_mutex> lock(chainMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        // If UI is currently modifying the chain, pass MIDI safely without blocking audio thread
        return;
    }

    // Check if any block is soloed
    bool hasSolo = false;
    for (const auto& b : blocks)
    {
        if (b && b->isSoloed())
        {
            hasSolo = true;
            break;
        }
    }

    juce::MidiBuffer mainInput = midiBuffer;
    juce::MidiBuffer currentBuffer = midiBuffer;
    juce::MidiBuffer nextBuffer;

    for (size_t i = 0; i < blocks.size(); ++i)
    {
        auto& block = blocks[i];
        if (!block)
            continue;

        if (hasSolo && !block->isSoloed())
        {
            // If another block is soloed, this block is bypassed
            continue;
        }

        if (block->isBypassed())
        {
            continue;
        }

        nextBuffer.clear();

        if (block->getRoutingMode() == RoutingMode::Parallel && i > 0)
        {
            // Parallel routing: processes raw DAW input directly, output merges into chain
            block->processBlock(mainInput, nextBuffer, ctx);
            currentBuffer.addEvents(nextBuffer, 0, -1, 0);
        }
        else
        {
            // Series routing: processes output of previous module
            block->processBlock(currentBuffer, nextBuffer, ctx);
            currentBuffer.swapWith(nextBuffer);
        }
    }

    midiBuffer.swapWith(currentBuffer);

    // Log outgoing MIDI for visualizer
    for (const auto meta : midiBuffer)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn())
            pushActivityEvent(msg.getNoteNumber(), msg.getVelocity(), true, true);
        else if (msg.isNoteOff())
            pushActivityEvent(msg.getNoteNumber(), 0, false, true);
    }
}

int MidiChainProcessor::getNumBlocks()
{
    std::lock_guard<std::recursive_mutex> lock(chainMutex);
    return static_cast<int>(blocks.size());
}

MidiBlock* MidiChainProcessor::getBlock(int index)
{
    std::lock_guard<std::recursive_mutex> lock(chainMutex);
    if (index >= 0 && index < static_cast<int>(blocks.size()))
        return blocks[index].get();
    return nullptr;
}

void MidiChainProcessor::addBlock(const juce::String& typeId)
{
    auto newBlock = MidiBlockFactory::createBlock(typeId);
    if (!newBlock)
        return;

    newBlock->prepare(currentSampleRate, currentBlockSize);

    {
        std::lock_guard<std::recursive_mutex> lock(chainMutex);
        blocks.push_back(std::move(newBlock));
    }

    if (onChainModified)
        onChainModified();
}

void MidiChainProcessor::insertBlock(int index, const juce::String& typeId)
{
    auto newBlock = MidiBlockFactory::createBlock(typeId);
    if (!newBlock)
        return;

    newBlock->prepare(currentSampleRate, currentBlockSize);

    {
        std::lock_guard<std::recursive_mutex> lock(chainMutex);
        index = juce::jlimit(0, static_cast<int>(blocks.size()), index);
        blocks.insert(blocks.begin() + index, std::move(newBlock));
    }

    if (onChainModified)
        onChainModified();
}

void MidiChainProcessor::removeBlock(int index)
{
    {
        std::lock_guard<std::recursive_mutex> lock(chainMutex);
        if (index >= 0 && index < static_cast<int>(blocks.size()))
        {
            blocks.erase(blocks.begin() + index);
            midiLearnManager.remapBlockRemoved(index);
        }
    }

    if (onChainModified)
        onChainModified();
}

void MidiChainProcessor::moveBlock(int fromIndex, int toIndex)
{
    {
        std::lock_guard<std::recursive_mutex> lock(chainMutex);
        int n = static_cast<int>(blocks.size());
        if (fromIndex >= 0 && fromIndex < n && toIndex >= 0 && toIndex < n && fromIndex != toIndex)
        {
            auto moved = std::move(blocks[fromIndex]);
            blocks.erase(blocks.begin() + fromIndex);
            blocks.insert(blocks.begin() + toIndex, std::move(moved));
            midiLearnManager.remapBlockMoved(fromIndex, toIndex);
        }
    }

    if (onChainModified)
        onChainModified();
}

void MidiChainProcessor::clearAllBlocks()
{
    {
        std::lock_guard<std::recursive_mutex> lock(chainMutex);
        blocks.clear();
    }

    if (onChainModified)
        onChainModified();
}

void MidiChainProcessor::randomizeAll()
{
    randomizeRack(true);
}

void MidiChainProcessor::randomizeRack(bool randomizeModules)
{
    {
        std::lock_guard<std::recursive_mutex> lock(chainMutex);
        FastRandom rng;

        if (randomizeModules)
        {
            blocks.clear();

            // Choose between 1 and 6 modules
            int count = rng.nextInt(1, 6);

            static const std::vector<const char*> allTypes = {
                "arpeggiator", "chords", "euclidean", "ratchet", "probability",
                "scale_quantize", "harmonizer", "transpose", "transform",
                "time_quantize", "humanizer", "delay",
                "lfo", "mutator", "mapper", "filter"
            };

            std::vector<const char*> pool = allTypes;
            // Fisher-Yates shuffle
            for (int i = static_cast<int>(pool.size()) - 1; i > 0; --i)
            {
                int j = rng.nextInt(0, i);
                std::swap(pool[i], pool[j]);
            }

            for (int i = 0; i < count; ++i)
            {
                auto blk = MidiBlockFactory::createBlock(pool[i]);
                if (blk)
                {
                    blk->prepare(currentSampleRate, currentBlockSize);
                    blk->randomize();
                    blocks.push_back(std::move(blk));
                }
            }
        }
        else
        {
            for (auto& block : blocks)
            {
                if (block)
                    block->randomize();
            }
        }
    }

    if (onChainModified)
        onChainModified();
}

ScaleProgression::PlaybackState MidiChainProcessor::getLastPlaybackState() const
{
    std::lock_guard<std::recursive_mutex> lock(const_cast<std::recursive_mutex&>(chainMutex));
    return lastPlaybackState;
}

juce::ValueTree MidiChainProcessor::getState() const
{
    juce::ValueTree root("MidiFluxState");
    root.setProperty("rootKey", globalRootKey.load(), nullptr);
    root.setProperty("scaleType", globalScaleType.load(), nullptr);
    root.setProperty("masterBypassed", masterBypassed.load(), nullptr);

    root.addChild(scaleProgression.getState(), -1, nullptr);
    root.addChild(midiLearnManager.getState(), -1, nullptr);

    juce::ValueTree blocksTree("Blocks");
    for (const auto& block : blocks)
    {
        if (block)
            blocksTree.addChild(block->getState(), -1, nullptr);
    }
    root.addChild(blocksTree, -1, nullptr);
    return root;
}

void MidiChainProcessor::setState(const juce::ValueTree& vt)
{
    if (vt.hasType("MidiFluxState"))
    {
        if (vt.hasProperty("rootKey"))
            setGlobalRootKey(static_cast<int>(vt.getProperty("rootKey")));
        if (vt.hasProperty("scaleType"))
            setGlobalScaleType(static_cast<int>(vt.getProperty("scaleType")));
        if (vt.hasProperty("masterBypassed"))
            setMasterBypassed(static_cast<bool>(vt.getProperty("masterBypassed")));

        auto seqTree = vt.getChildWithName("ScaleProgression");
        if (seqTree.isValid())
        {
            std::lock_guard<std::recursive_mutex> lock(chainMutex);
            scaleProgression.setState(seqTree);
        }

        auto learnTree = vt.getChildWithName("MidiMappings");
        if (learnTree.isValid())
        {
            midiLearnManager.setState(learnTree);
        }

        auto blocksTree = vt.getChildWithName("Blocks");
        if (blocksTree.isValid())
        {
            std::vector<std::unique_ptr<MidiBlock>> newBlocks;
            for (int i = 0; i < blocksTree.getNumChildren(); ++i)
            {
                auto child = blocksTree.getChild(i);
                juce::String typeId = child.getProperty("type").toString();
                auto block = MidiBlockFactory::createBlock(typeId);
                if (block)
                {
                    block->prepare(currentSampleRate, currentBlockSize);
                    block->setState(child);
                    newBlocks.push_back(std::move(block));
                }
            }

            {
                std::lock_guard<std::recursive_mutex> lock(chainMutex);
                blocks = std::move(newBlocks);
            }
        }
    }

    if (onChainModified)
        onChainModified();
}

} // namespace MidiFlux
