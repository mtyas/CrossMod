#include "ModMatrixComponent.h"

ModMatrixComponent::ModMatrixComponent(juce::AudioProcessorValueTreeState& apvts)
{
    auto sources = ParameterFactory::getModSourceChoices();
    auto dests = ParameterFactory::getModDestinationChoices();

    for (int i = 0; i < CrossModIDs::NUM_MOD_SLOTS; ++i)
    {
        auto& slot = slots[i];

        // Slot title badge
        slot.titleLabel = std::make_unique<juce::Label>("lbl", "SLOT " + juce::String(i + 1));
        slot.titleLabel->setFont(juce::FontOptions(11.0f, juce::Font::bold));
        slot.titleLabel->setColour(juce::Label::textColourId, juce::Colour(0xffe5a823));
        slot.titleLabel->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(slot.titleLabel.get());

        // Zero / Clear button
        slot.zeroBtn = std::make_unique<juce::TextButton>("0");
        slot.zeroBtn->setTooltip("Reset amount to 0%");
        addAndMakeVisible(slot.zeroBtn.get());

        // Source Box
        slot.sourceBox = std::make_unique<juce::ComboBox>();
        for (int s = 0; s < sources.size(); ++s)
            slot.sourceBox->addItem(sources[s], s + 1);
        addAndMakeVisible(slot.sourceBox.get());
        slot.sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, CrossModIDs::modSlotSource(i).getParamID(), *slot.sourceBox);

        // Dest Box
        slot.destBox = std::make_unique<juce::ComboBox>();
        for (int d = 0; d < dests.size(); ++d)
            slot.destBox->addItem(dests[d], d + 1);
        addAndMakeVisible(slot.destBox.get());
        slot.destAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, CrossModIDs::modSlotDest(i).getParamID(), *slot.destBox);

        // Amount Slider with visible digital readout textbox!
        slot.amountSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        slot.amountSlider->setRange(-1.0, 1.0, 0.01);
        slot.amountSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 16);
        slot.amountSlider->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8e4d9));
        slot.amountSlider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff383e47));
        
        slot.amountSlider->textFromValueFunction = [](double val) {
            int pct = (int)std::round(val * 100.0);
            if (pct > 0) return "+" + juce::String(pct) + "%";
            return juce::String(pct) + "%";
        };
        slot.amountSlider->valueFromTextFunction = [](const juce::String& text) {
            return text.replace("%", "").replace("+", "").trim().getDoubleValue() / 100.0;
        };

        addAndMakeVisible(slot.amountSlider.get());
        slot.amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, CrossModIDs::modSlotAmount(i).getParamID(), *slot.amountSlider);

        // Hook up zero button
        slot.zeroBtn->onClick = [slider = slot.amountSlider.get()]() {
            slider->setValue(0.0, juce::sendNotificationAsync);
        };
    }
}

void ModMatrixComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Recessed chassis
    g.setColour(juce::Colour(0xff16181d));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(juce::Colour(0xff353b45));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    // 2. Section Header
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffe5a823)); // Gold
    g.drawText("MODULATION MATRIX", 14, 6, 170, 18, juce::Justification::centredLeft);

    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xff8c94a0));
    g.drawText("8 BIPOLAR ROUTING CHANNELS (-100% TO +100%)", 185, 6, 350, 18, juce::Justification::centredLeft);

    // 3. Card Backgrounds for the 8 slots
    auto contentArea = bounds.reduced(10.0f, 6.0f);
    contentArea.removeFromTop(22.0f);

    int numCols = 4;
    int numRows = 2;
    float gapX = 8.0f;
    float gapY = 6.0f;
    float cardW = (contentArea.getWidth() - (numCols - 1) * gapX) / (float)numCols;
    float cardH = (contentArea.getHeight() - (numRows - 1) * gapY) / (float)numRows;

    for (int i = 0; i < CrossModIDs::NUM_MOD_SLOTS; ++i)
    {
        int col = i % numCols;
        int row = i / numCols;

        float cx = contentArea.getX() + col * (cardW + gapX);
        float cy = contentArea.getY() + row * (cardH + gapY);
        auto cardRect = juce::Rectangle<float>(cx, cy, cardW, cardH);

        // Check if slot is actively modulating
        bool isActive = slots[i].sourceBox->getSelectedId() > 1 
                     && slots[i].destBox->getSelectedId() > 1 
                     && std::abs(slots[i].amountSlider->getValue()) > 0.001;

        g.setColour(juce::Colour(0xff1f2228));
        g.fillRoundedRectangle(cardRect, 4.0f);

        g.setColour(isActive ? juce::Colour(0xff556275) : juce::Colour(0xff2d323b));
        g.drawRoundedRectangle(cardRect, 4.0f, 1.0f);

        // Active LED dot near slot title
        float ledX = cx + cardW - 38.0f;
        float ledY = cy + 6.0f;
        g.setColour(isActive ? juce::Colour(0xfff39c12) : juce::Colour(0xff282e38));
        g.fillEllipse(ledX, ledY, 6.0f, 6.0f);
        if (isActive)
        {
            g.setColour(juce::Colour(0x80ffffff));
            g.drawEllipse(ledX, ledY, 6.0f, 6.0f, 0.8f);
        }

        // Sub-labels inside card: SRC, DEST, AMT
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff7d8795));
        g.drawText("SRC", (int)cx + 6, (int)cy + 22, 28, 14, juce::Justification::left);
        g.drawText("DST", (int)cx + (int)(cardW * 0.5f) + 2, (int)cy + 22, 28, 14, juce::Justification::left);
        g.drawText("AMT", (int)cx + 6, (int)cy + 45, 28, 14, juce::Justification::left);
    }
}

void ModMatrixComponent::resized()
{
    auto bounds = getLocalBounds().toFloat().reduced(10.0f, 6.0f);
    bounds.removeFromTop(22.0f); // Title bar

    int numCols = 4;
    int numRows = 2;
    float gapX = 8.0f;
    float gapY = 6.0f;
    float cardW = (bounds.getWidth() - (numCols - 1) * gapX) / (float)numCols;
    float cardH = (bounds.getHeight() - (numRows - 1) * gapY) / (float)numRows;

    for (int i = 0; i < CrossModIDs::NUM_MOD_SLOTS; ++i)
    {
        int col = i % numCols;
        int row = i / numCols;

        float cx = bounds.getX() + col * (cardW + gapX);
        float cy = bounds.getY() + row * (cardH + gapY);

        auto& slot = slots[i];

        // Top line: Slot Title & Zero button
        slot.titleLabel->setBounds((int)cx + 6, (int)cy + 3, 60, 16);
        slot.zeroBtn->setBounds((int)cx + (int)cardW - 24, (int)cy + 4, 18, 14);

        // Middle line: Source and Dest ComboBoxes
        float halfW = (cardW - 14.0f) * 0.5f;
        float comboH = 19.0f;
        slot.sourceBox->setBounds((int)cx + 32, (int)cy + 20, (int)halfW - 28, (int)comboH);
        slot.destBox->setBounds((int)cx + (int)halfW + 30, (int)cy + 20, (int)halfW - 24, (int)comboH);

        // Bottom line: Amount Slider with text box
        slot.amountSlider->setBounds((int)cx + 32, (int)cy + 42, (int)cardW - 38, 20);
    }
}
