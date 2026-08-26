#include "MidiBlockFactory.h"
#include "../Blocks/ProbabilityBlock.h"
#include "../Blocks/MutatorBlock.h"
#include "../Blocks/ArpeggiatorBlock.h"
#include "../Blocks/ChordBlock.h"
#include "../Blocks/HumanizerBlock.h"
#include "../Blocks/ScaleQuantizeBlock.h"
#include "../Blocks/TimeQuantizeBlock.h"
#include "../Blocks/TransposeBlock.h"
#include "../Blocks/FilterBlock.h"
#include "../Blocks/MapperBlock.h"
#include "../Blocks/TransformBlock.h"
#include "../Blocks/LfoBlock.h"
#include "../Blocks/RatchetBlock.h"
#include "../Blocks/DelayBlock.h"
#include "../Blocks/EuclideanBlock.h"
#include "../Blocks/HarmonizerBlock.h"

namespace MidiFlux
{

const std::vector<BlockCatalogItem>& MidiBlockFactory::getCatalog()
{
    static const std::vector<BlockCatalogItem> catalog = {
        // Generative
        { "probability",    "Probability",           "Generative", juce::Colour(0xff00e5ff), []() { return std::make_unique<ProbabilityBlock>(); } },
        { "mutator",        "Mutator",               "Generative", juce::Colour(0xff10b981), []() { return std::make_unique<MutatorBlock>(); } },
        { "arpeggiator",    "Arpeggiator",           "Generative", juce::Colour(0xff38bdf8), []() { return std::make_unique<ArpeggiatorBlock>(); } },
        { "ratchet",        "Ratchet & Roll",        "Generative", juce::Colour(0xfff43f5e), []() { return std::make_unique<RatchetBlock>(); } },
        { "euclidean",      "Euclidean Rhythms",     "Generative", juce::Colour(0xff14b8a6), []() { return std::make_unique<EuclideanBlock>(); } },

        // Harmonic
        { "chords",         "Chord Generator",       "Harmonic",   juce::Colour(0xffa855f7), []() { return std::make_unique<ChordBlock>(); } },
        { "scale_quantize", "Scale Quantizer",       "Harmonic",   juce::Colour(0xff6366f1), []() { return std::make_unique<ScaleQuantizeBlock>(); } },
        { "harmonizer",     "Harmonizer",            "Harmonic",   juce::Colour(0xff9333ea), []() { return std::make_unique<HarmonizerBlock>(); } },
        { "transpose",      "Transpose",             "Harmonic",   juce::Colour(0xff8b5cf6), []() { return std::make_unique<TransposeBlock>(); } },

        // Timing
        { "time_quantize",  "Time Quantizer",        "Timing",     juce::Colour(0xff06b6d4), []() { return std::make_unique<TimeQuantizeBlock>(); } },
        { "humanizer",      "Humanizer",             "Timing",     juce::Colour(0xfff59e0b), []() { return std::make_unique<HumanizerBlock>(); } },
        { "delay",          "MIDI Delay & Cascade",  "Timing",     juce::Colour(0xffd97706), []() { return std::make_unique<DelayBlock>(); } },

        // Filter
        { "filter",         "Filter & Split",        "Filter",     juce::Colour(0xff64748b), []() { return std::make_unique<FilterBlock>(); } },

        // Utility
        { "mapper",         "Mapper",                "Utility",    juce::Colour(0xff0ea5e9), []() { return std::make_unique<MapperBlock>(); } },
        { "transform",      "Invert & Mirror",       "Utility",    juce::Colour(0xffec4899), []() { return std::make_unique<TransformBlock>(); } },
        { "lfo",            "MIDI LFO",              "Utility",    juce::Colour(0xffd946ef), []() { return std::make_unique<LfoBlock>(); } }
    };
    return catalog;
}

std::unique_ptr<MidiBlock> MidiBlockFactory::createBlock(const juce::String& typeId)
{
    // Check aliases for backwards compatibility
    if (typeId == "quantizer")
        return std::make_unique<ScaleQuantizeBlock>();

    for (const auto& item : getCatalog())
    {
        if (item.typeId == typeId)
            return item.creator();
    }
    return nullptr;
}

} // namespace MidiFlux
