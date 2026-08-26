#pragma once

#include "MidiBlock.h"
#include <vector>
#include <functional>

namespace MidiFlux
{

struct BlockCatalogItem
{
    juce::String typeId;
    juce::String displayName;
    juce::String category;
    juce::Colour color;
    std::function<std::unique_ptr<MidiBlock>()> creator;
};

class MidiBlockFactory
{
public:
    static const std::vector<BlockCatalogItem>& getCatalog();
    static std::unique_ptr<MidiBlock> createBlock(const juce::String& typeId);
};

} // namespace MidiFlux
