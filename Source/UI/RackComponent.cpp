#include "RackComponent.h"
#include "../Common/MidiBlockFactory.h"

namespace MidiFlux
{

AddBlockCardComponent::AddBlockCardComponent()
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void AddBlockCardComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(4.0f);

    // Dashed border
    g.setColour(juce::Colour(0xff30363d));
    g.drawRoundedRectangle(bounds, 8.0f, 1.5f);

    // Subtle background
    g.setColour(juce::Colour(0xff161b22).withAlpha(0.5f));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Plus icon
    auto center = bounds.getCentre();
    g.setColour(juce::Colour(0xff00e5ff));
    g.drawLine(center.x - 14.0f, center.y - 10.0f, center.x + 14.0f, center.y - 10.0f, 2.5f);
    g.drawLine(center.x, center.y - 24.0f, center.x, center.y + 4.0f, 2.5f);

    // Text
    g.setColour(juce::Colour(0xfff0f6fc));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("+ Add Effect", bounds.getX(), center.y + 14.0f, bounds.getWidth(), 24.0f, juce::Justification::centred);

    g.setColour(juce::Colour(0xff8b949e));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Click to choose module", bounds.getX(), center.y + 36.0f, bounds.getWidth(), 18.0f, juce::Justification::centred);
}

void AddBlockCardComponent::mouseUp(const juce::MouseEvent&)
{
    showAddMenu();
}

void AddBlockCardComponent::showAddMenu()
{
    juce::PopupMenu menu;
    juce::PopupMenu genMenu, harmMenu, timeMenu, filterMenu, utilMenu;

    const auto& catalog = MidiBlockFactory::getCatalog();
    for (size_t i = 0; i < catalog.size(); ++i)
    {
        const auto& item = catalog[i];
        int id = static_cast<int>(i + 1);

        if (item.category == "Generative")
            genMenu.addItem(id, item.displayName);
        else if (item.category == "Harmonic")
            harmMenu.addItem(id, item.displayName);
        else if (item.category == "Timing")
            timeMenu.addItem(id, item.displayName);
        else if (item.category == "Filter")
            filterMenu.addItem(id, item.displayName);
        else
            utilMenu.addItem(id, item.displayName);
    }

    menu.addSubMenu("Generative", genMenu);
    menu.addSubMenu("Harmonic", harmMenu);
    menu.addSubMenu("Timing", timeMenu);
    menu.addSubMenu("Filter", filterMenu);
    menu.addSubMenu("Utility", utilMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, catalog](int result)
        {
            if (result > 0 && result <= static_cast<int>(catalog.size()))
            {
                if (onBlockSelected)
                    onBlockSelected(catalog[result - 1].typeId);
            }
        });
}

// -------------------------------------------------------------
// RackComponent implementation
// -------------------------------------------------------------

RackComponent::RackComponent(MidiChainProcessor& chain)
    : chainProcessor(chain)
{
    contentComponent = std::make_unique<juce::Component>();
    viewport.setViewedComponent(contentComponent.get(), false);
    viewport.setScrollBarsShown(false, true, false, true); // show horizontal scrollbar
    viewport.setScrollBarThickness(8);
    addAndMakeVisible(viewport);

    addCard = std::make_unique<AddBlockCardComponent>();
    addCard->onBlockSelected = [this](const juce::String& typeId) {
        handleAdd(typeId);
    };

    chainProcessor.onChainModified = [this]() {
        juce::MessageManager::callAsync([this]() {
            rebuildCards();
        });
    };

    rebuildCards();
}

RackComponent::~RackComponent()
{
}

void RackComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d1117));
}

void RackComponent::paintOverChildren(juce::Graphics& g)
{
    // Draw drop insertion indicator line
    if (dragTargetInsertionIndex >= 0 && !cardComponents.empty())
    {
        int cardWidth = 260;
        int cardSpacing = 14;
        int targetX = cardSpacing + dragTargetInsertionIndex * (cardWidth + cardSpacing) - viewport.getViewPositionX() - 7;

        g.setColour(juce::Colour(0xff00e5ff).withAlpha(0.35f));
        g.fillRect(targetX - 4, 4, 8, getHeight() - 8);

        g.setColour(juce::Colour(0xff00e5ff));
        g.fillRect(targetX - 1, 4, 3, getHeight() - 8);
    }
}

void RackComponent::resized()
{
    viewport.setBounds(getLocalBounds());
    rebuildCards();
}

void RackComponent::rebuildCards()
{
    cardComponents.clear();
    contentComponent->removeAllChildren();

    int numBlocks = chainProcessor.getNumBlocks();
    int cardWidth = 260;
    int cardSpacing = 14;
    int cardHeight = std::max(300, getHeight() - 20);

    for (int i = 0; i < numBlocks; ++i)
    {
        auto* block = chainProcessor.getBlock(i);
        if (block)
        {
            auto card = std::make_unique<BlockCardComponent>(block, i);
            card->onMoveLeft = [this](int idx) { handleMoveLeft(idx); };
            card->onMoveRight = [this](int idx) { handleMoveRight(idx); };
            card->onRemove = [this](int idx) { handleRemove(idx); };
            card->onParameterChanged = [this]() {
                if (onStateMutated)
                    onStateMutated();
            };

            contentComponent->addAndMakeVisible(card.get());
            cardComponents.push_back(std::move(card));
        }
    }

    contentComponent->addAndMakeVisible(addCard.get());

    int totalCards = numBlocks + 1; // blocks + AddCard
    int totalWidth = totalCards * cardWidth + (totalCards + 1) * cardSpacing;
    contentComponent->setBounds(0, 0, std::max(getWidth(), totalWidth), getHeight());

    int x = cardSpacing;
    for (size_t i = 0; i < cardComponents.size(); ++i)
    {
        cardComponents[i]->setBounds(x, 6, cardWidth, cardHeight);
        x += cardWidth + cardSpacing;
    }

    addCard->setBounds(x, 6, 180, cardHeight);
}

int RackComponent::calculateDropIndex(int mouseX) const
{
    int contentX = mouseX + viewport.getViewPositionX();
    int cardWidth = 260;
    int cardSpacing = 14;
    int numBlocks = chainProcessor.getNumBlocks();

    if (numBlocks <= 0)
        return 0;

    int idx = (contentX - cardSpacing / 2) / (cardWidth + cardSpacing);
    return juce::jlimit(0, numBlocks, idx);
}

bool RackComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description.toString().startsWith("midiflux_card:");
}

void RackComponent::itemDragMove(const SourceDetails& dragSourceDetails)
{
    int newIdx = calculateDropIndex(dragSourceDetails.localPosition.x);
    if (newIdx != dragTargetInsertionIndex)
    {
        dragTargetInsertionIndex = newIdx;
        repaint();
    }
}

void RackComponent::itemDragExit(const SourceDetails&)
{
    dragTargetInsertionIndex = -1;
    repaint();
}

void RackComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    int targetIdx = dragTargetInsertionIndex;
    dragTargetInsertionIndex = -1;
    repaint();

    auto desc = dragSourceDetails.description.toString();
    if (desc.startsWith("midiflux_card:"))
    {
        int sourceIdx = desc.fromFirstOccurrenceOf("midiflux_card:", false, false).getIntValue();
        int numBlocks = chainProcessor.getNumBlocks();

        if (sourceIdx >= 0 && sourceIdx < numBlocks)
        {
            if (targetIdx > sourceIdx)
                targetIdx--; // account for removal shift

            targetIdx = juce::jlimit(0, numBlocks - 1, targetIdx);
            if (sourceIdx != targetIdx)
            {
                chainProcessor.moveBlock(sourceIdx, targetIdx);
                if (onStateMutated)
                    onStateMutated();
            }
        }
    }
}

void RackComponent::handleMoveLeft(int index)
{
    if (index > 0)
    {
        chainProcessor.moveBlock(index, index - 1);
        if (onStateMutated)
            onStateMutated();
    }
}

void RackComponent::handleMoveRight(int index)
{
    if (index < chainProcessor.getNumBlocks() - 1)
    {
        chainProcessor.moveBlock(index, index + 1);
        if (onStateMutated)
            onStateMutated();
    }
}

void RackComponent::handleRemove(int index)
{
    chainProcessor.removeBlock(index);
    if (onStateMutated)
        onStateMutated();
}

void RackComponent::handleAdd(const juce::String& typeId)
{
    chainProcessor.addBlock(typeId);
    if (onStateMutated)
        onStateMutated();
}

} // namespace MidiFlux
