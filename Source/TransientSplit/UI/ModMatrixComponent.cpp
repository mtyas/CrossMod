#include "ModMatrixComponent.h"
#include "TransientLookAndFeel.h"

ModMatrixComponent::ModMatrixComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse)
    : apvts(state), engine(engineToUse)
{
    tableHeader.setText("MODULATION MATRIX (8 ROUTING SLOTS WITH LIVE BIPOLAR OUTPUT METERS)", juce::dontSendNotification);
    tableHeader.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    tableHeader.setColour(juce::Label::textColourId, TransientLookAndFeel::textBright);
    addAndMakeVisible(tableHeader);

    const juce::StringArray modSourceChoices = {
        "None",
        "LFO 1", "LFO 2", "LFO 3",
        "Env Follow 1", "Env Follow 2", "Env Follow 3",
        "Turing 1", "Turing 2",
        "Macro 1", "Macro 2", "Macro 3", "Macro 4",
        "XY Pad 1 X", "XY Pad 1 Y", "XY Pad 2 X", "XY Pad 2 Y",
        "Audio: Transient Env", "Audio: Sustain Env",
        "Audio: Filter 1 Env", "Audio: Filter 2 Env", "Audio: Filter 3 Env"
    };

    juce::StringArray initialDestChoices;
    for (int d = 0; d < ModMatrix::NumDestinations; ++d)
    {
        initialDestChoices.add(ModMatrix::getDestName(static_cast<ModDest>(d)));
    }

    for (int i = 0; i < ModMatrix::NumRoutingSlots; ++i)
    {
        auto& row = rows[i];

        row.slotLabel.setText("SLOT " + juce::String(i + 1), juce::dontSendNotification);
        row.slotLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        row.slotLabel.setColour(juce::Label::textColourId, TransientLookAndFeel::accentTransient);
        addAndMakeVisible(row.slotLabel);

        row.srcCombo.addItemList(modSourceChoices, 1);
        addAndMakeVisible(row.srcCombo);

        row.dstCombo.addItemList(initialDestChoices, 1);
        addAndMakeVisible(row.dstCombo);

        row.depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        row.depthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 62, 14);
        row.depthSlider.setNumDecimalPlacesToDisplay(4);
        row.depthSlider.setDoubleClickReturnValue(true, 0.0);
        row.depthSlider.getProperties().set("accentColor", juce::Colour(0xff00d4ff).toString());
        row.depthSlider.textFromValueFunction = [](double v) {
            if (std::abs(v) < 1e-5) return juce::String("0.0000");
            return (v > 0.0 ? "+" : "") + juce::String(v, 4);
        };
        row.depthSlider.valueFromTextFunction = [](const juce::String& t) {
            return t.getDoubleValue();
        };
        addAndMakeVisible(row.depthSlider);

        const auto srcId = IDs::getModSlotSource(i).getParamID();
        const auto dstId = IDs::getModSlotDest(i).getParamID();
        const auto depId = IDs::getModSlotDepth(i).getParamID();

        if (apvts.getParameter(srcId) != nullptr)
            row.srcAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, srcId, row.srcCombo);
        if (apvts.getParameter(dstId) != nullptr)
            row.dstAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, dstId, row.dstCombo);
        if (apvts.getParameter(depId) != nullptr)
            row.depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, depId, row.depthSlider);
    }

    for (int s = 0; s < 8; ++s)
    {
        const auto& slot = (s < 4) ? engine.getTransientChain().getSlot(s) : engine.getSustainChain().getSlot(s - 4);
        cachedSlotTypes[s] = slot.getType();
    }

    updateDynamicDestNames();
    startTimerHz(30);
}

ModMatrixComponent::~ModMatrixComponent()
{
    stopTimer();
}

void ModMatrixComponent::visibilityChanged()
{
    if (isVisible())
        updateDynamicDestNames();
}

void ModMatrixComponent::updateDynamicDestNames()
{
    for (int s = 0; s < 8; ++s)
    {
        const bool isBank1 = (s < 4);
        const int localIdx = isBank1 ? s : (s - 4);
        const auto typeId = isBank1 ? IDs::getTransSlotType(localIdx).getParamID() : IDs::getSustSlotType(localIdx).getParamID();

        int typeIdx = 0;
        if (auto* p = apvts.getRawParameterValue(typeId))
            typeIdx = static_cast<int>(p->load());

        const auto effType = static_cast<EffectType>(typeIdx);
        const juce::String effName = EffectSlot::getEffectName(effType);
        const juce::String prefix = "Slot " + juce::String(s + 1) + " (" + effName + "): ";

        const int baseItemId = 1 + (17 + s * 6);

        for (int rowIdx = 0; rowIdx < ModMatrix::NumRoutingSlots; ++rowIdx)
        {
            auto& combo = rows[rowIdx].dstCombo;
            for (int p = 0; p < 5; ++p)
            {
                const auto pInfo = EffectSlot::getParamInfoForType(effType, p);
                combo.changeItemText(baseItemId + p, prefix + pInfo.name);
            }
            combo.changeItemText(baseItemId + 5, prefix + "Mix");
        }
    }
}

void ModMatrixComponent::timerCallback()
{
    bool typesChanged = false;
    for (int s = 0; s < 8; ++s)
    {
        const bool isBank1 = (s < 4);
        const int localIdx = isBank1 ? s : (s - 4);
        const auto typeId = isBank1 ? IDs::getTransSlotType(localIdx).getParamID() : IDs::getSustSlotType(localIdx).getParamID();

        int typeIdx = 0;
        if (auto* p = apvts.getRawParameterValue(typeId))
            typeIdx = static_cast<int>(p->load());

        const auto currentType = static_cast<EffectType>(typeIdx);
        if (currentType != cachedSlotTypes[s])
        {
            cachedSlotTypes[s] = currentType;
            typesChanged = true;
        }
    }

    if (typesChanged)
        updateDynamicDestNames();

    repaint();
}

void ModMatrixComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0c0e14));

    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(32); // Header space

    // Table Header Bar
    auto colHeader = area.removeFromTop(24);
    g.setColour(juce::Colour(0xff181d28));
    g.fillRoundedRectangle(colHeader.toFloat(), 4.0f);

    g.setColour(TransientLookAndFeel::textMuted);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));

    const int w = colHeader.getWidth();
    g.drawText("SLOT", 16, colHeader.getY(), 60, colHeader.getHeight(), juce::Justification::centred);
    g.drawText("SOURCE", 90, colHeader.getY(), static_cast<int>(w * 0.25f), colHeader.getHeight(), juce::Justification::centred);
    g.drawText("DESTINATION", 90 + static_cast<int>(w * 0.25f) + 12, colHeader.getY(), static_cast<int>(w * 0.28f), colHeader.getHeight(), juce::Justification::centred);
    g.drawText("AMOUNT", 90 + static_cast<int>(w * 0.53f) + 24, colHeader.getY(), static_cast<int>(w * 0.26f), colHeader.getHeight(), juce::Justification::centred);
    g.drawText("LIVE OUT", colHeader.getRight() - 95, colHeader.getY(), 80, colHeader.getHeight(), juce::Justification::centred);

    // Draw Rows Background & Live Bipolar Meters
    auto& mod = engine.getModEngine();

    for (int i = 0; i < ModMatrix::NumRoutingSlots; ++i)
    {
        if (rowBounds[i].isEmpty()) continue;

        auto r = rowBounds[i].toFloat();

        // Alternating row background
        g.setColour(i % 2 == 0 ? juce::Colour(0xff131620) : juce::Colour(0xff10121a));
        g.fillRoundedRectangle(r, 4.0f);

        g.setColour(juce::Colour(0xff222736));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);

        // Draw Bipolar Live Output Meter
        if (!meterBounds[i].isEmpty())
        {
            auto m = meterBounds[i].toFloat();

            g.setColour(juce::Colour(0xff0a0c10));
            g.fillRoundedRectangle(m, 3.0f);

            // Center zero line
            const float midX = m.getCentreX();
            g.setColour(juce::Colour(0xff333a4d));
            g.drawVerticalLine(static_cast<int>(midX), m.getY(), m.getBottom());

            const float liveVal = std::clamp(mod.getSlotLiveOutput(i), -1.0f, 1.0f);
            if (std::abs(liveVal) > 0.001f)
            {
                const float halfW = m.getWidth() * 0.5f;
                float barX = midX;
                float barW = liveVal * halfW;

                if (barW < 0.0f)
                {
                    barX += barW;
                    barW = -barW;
                }

                juce::Colour meterCol = (liveVal >= 0.0f) ? juce::Colour(0xff00d4ff) : juce::Colour(0xffff9900);
                g.setColour(meterCol);
                g.fillRoundedRectangle(barX, m.getY() + 1.0f, barW, m.getHeight() - 2.0f, 2.0f);
            }
        }
    }
}

void ModMatrixComponent::resized()
{
    auto area = getLocalBounds().reduced(16);

    tableHeader.setBounds(area.removeFromTop(28));
    area.removeFromTop(4);

    // Skip Column Header (24px)
    area.removeFromTop(24);
    area.removeFromTop(6); // Gap

    const int totalH = area.getHeight();
    const int rowH = (totalH - 7 * 6) / ModMatrix::NumRoutingSlots;

    for (int i = 0; i < ModMatrix::NumRoutingSlots; ++i)
    {
        auto r = area.removeFromTop(rowH);
        area.removeFromTop(6); // Gap between rows
        rowBounds[i] = r;

        auto cArea = r.reduced(8, 4);

        // 1. Slot Label
        rows[i].slotLabel.setBounds(cArea.removeFromLeft(60));
        cArea.removeFromLeft(8);

        // 2. Source Combo
        const int srcW = static_cast<int>(cArea.getWidth() * 0.25f);
        rows[i].srcCombo.setBounds(cArea.removeFromLeft(srcW));
        cArea.removeFromLeft(12);

        // 3. Destination Combo
        const int dstW = static_cast<int>(cArea.getWidth() * 0.38f);
        rows[i].dstCombo.setBounds(cArea.removeFromLeft(dstW));
        cArea.removeFromLeft(12);

        // 4. Live Output Meter
        meterBounds[i] = cArea.removeFromRight(85).reduced(0, 6);
        cArea.removeFromRight(12);

        // 5. Depth Slider
        rows[i].depthSlider.setBounds(cArea);
    }
}
