#include "ModulatorsPanelComponent.h"
#include "TransientLookAndFeel.h"

ModulatorsPanelComponent::ModulatorsPanelComponent(juce::AudioProcessorValueTreeState& state, TransientSplitEngine& engineToUse)
    : apvts(state), engine(engineToUse), xyPad1(state, 1), xyPad2(state, 2)
{
    const juce::Colour colLfo  = juce::Colour(0xff00d4ff); // Neon Cyan
    const juce::Colour colEnv  = juce::Colour(0xffff9900); // Warm Amber
    const juce::Colour colTur  = juce::Colour(0xffa855f7); // Electric Purple
    const juce::Colour colMac  = juce::Colour(0xff4ade80); // Emerald Green

    const juce::StringArray lfoWaveNames = { "Sine", "Triangle", "Sawtooth", "Square", "S & H", "Smooth Rnd" };
    const juce::StringArray subdivNames  = { "1/32", "1/16", "1/8", "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars", "8 Bars" };
    const juce::StringArray envSrcNames  = {
        "In Audio", "Attack", "Body",
        "Filter 1", "Filter 2", "Filter 3",
        "Left (L)", "Right (R)", "Mid (M)", "Side (S)",
        "Dyn High", "Dyn Low", "Harmonic", "Noise", "In-Phase", "Spatial"
    };

    auto setupDial = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col,
                           const juce::String& suffix = "", double dVal = 0.5)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 12);
        s.setDoubleClickReturnValue(true, dVal);
        s.getProperties().set("accentColor", col.toString());
        if (suffix.isNotEmpty()) s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    auto attachSlider = [this](const juce::String& paramId, juce::Slider& slider)
    {
        if (apvts.getParameter(paramId) != nullptr)
            sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider));
    };

    auto attachCombo = [this](const juce::String& paramId, juce::ComboBox& combo)
    {
        if (apvts.getParameter(paramId) != nullptr)
            comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramId, combo));
    };

    auto attachButton = [this](const juce::String& paramId, juce::Button& button)
    {
        if (apvts.getParameter(paramId) != nullptr)
            buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, button));
    };

    // 1. 3 LFOs Setup
    const juce::ParameterID lfoWaveIDs[3]   = { IDs::lfo1Wave, IDs::lfo2Wave, IDs::lfo3Wave };
    const juce::ParameterID lfoRateIDs[3]   = { IDs::lfo1Rate, IDs::lfo2Rate, IDs::lfo3Rate };
    const juce::ParameterID lfoSyncIDs[3]   = { IDs::lfo1Sync, IDs::lfo2Sync, IDs::lfo3Sync };
    const juce::ParameterID lfoSubdivIDs[3] = { IDs::lfo1Subdiv, IDs::lfo2Subdiv, IDs::lfo3Subdiv };
    const juce::ParameterID lfoSmoothIDs[3] = { IDs::lfo1Smooth, IDs::lfo2Smooth, IDs::lfo3Smooth };

    for (int i = 0; i < 3; ++i)
    {
        auto& lfo = lfos[i];
        lfo.title.setText("LFO " + juce::String(i + 1), juce::dontSendNotification);
        lfo.title.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        lfo.title.setColour(juce::Label::textColourId, colLfo);
        addAndMakeVisible(lfo.title);

        lfo.waveCombo.addItemList(lfoWaveNames, 1);
        addAndMakeVisible(lfo.waveCombo);

        setupDial(lfo.rateSlider, lfo.rateLabel, "RATE", colLfo, " Hz", 1.0);
        lfo.rateSlider.setNumDecimalPlacesToDisplay(2);

        lfo.syncToggle.getProperties().set("accentColor", colLfo.toString());
        lfo.syncToggle.getProperties().set("isSyncToggle", true);
        addAndMakeVisible(lfo.syncToggle);

        lfo.subdivCombo.addItemList(subdivNames, 1);
        addAndMakeVisible(lfo.subdivCombo);

        setupDial(lfo.smoothSlider, lfo.smoothLabel, "SMOOTH", colLfo, "", 0.0);

        attachCombo(lfoWaveIDs[i].getParamID(), lfo.waveCombo);
        attachSlider(lfoRateIDs[i].getParamID(), lfo.rateSlider);
        attachButton(lfoSyncIDs[i].getParamID(), lfo.syncToggle);
        attachCombo(lfoSubdivIDs[i].getParamID(), lfo.subdivCombo);
        attachSlider(lfoSmoothIDs[i].getParamID(), lfo.smoothSlider);
    }

    // 2. 3 Envelope Followers Setup
    const juce::ParameterID envSrcIDs[3]  = { IDs::env1Source, IDs::env2Source, IDs::env3Source };
    const juce::ParameterID envAttIDs[3]  = { IDs::env1Attack, IDs::env2Attack, IDs::env3Attack };
    const juce::ParameterID envRelIDs[3]  = { IDs::env1Release, IDs::env2Release, IDs::env3Release };
    const juce::ParameterID envGainIDs[3] = { IDs::env1Gain, IDs::env2Gain, IDs::env3Gain };

    for (int i = 0; i < 3; ++i)
    {
        auto& env = envs[i];
        env.title.setText("ENV FOLLOWER " + juce::String(i + 1), juce::dontSendNotification);
        env.title.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        env.title.setColour(juce::Label::textColourId, colEnv);
        addAndMakeVisible(env.title);

        env.srcCombo.addItemList(envSrcNames, 1);
        addAndMakeVisible(env.srcCombo);

        setupDial(env.attSlider, env.attLabel, "ATTACK", colEnv, " ms", 20.0);
        setupDial(env.relSlider, env.relLabel, "RELEASE", colEnv, " ms", 100.0);
        setupDial(env.gainSlider, env.gainLabel, "SENS", colEnv, "x", 1.0);

        attachCombo(envSrcIDs[i].getParamID(), env.srcCombo);
        attachSlider(envAttIDs[i].getParamID(), env.attSlider);
        attachSlider(envRelIDs[i].getParamID(), env.relSlider);
        attachSlider(envGainIDs[i].getParamID(), env.gainSlider);
    }

    // 3. 2 Turing Machines Setup
    const juce::ParameterID turProbIDs[2]   = { IDs::turing1Prob, IDs::turing2Prob };
    const juce::ParameterID turLenIDs[2]    = { IDs::turing1Length, IDs::turing2Length };
    const juce::ParameterID turSyncIDs[2]   = { IDs::turing1Sync, IDs::turing2Sync };
    const juce::ParameterID turSubdivIDs[2] = { IDs::turing1Subdiv, IDs::turing2Subdiv };
    const juce::ParameterID turGlideIDs[2]  = { IDs::turing1Glide, IDs::turing2Glide };

    for (int i = 0; i < 2; ++i)
    {
        auto& tur = turings[i];
        tur.title.setText("TURING MACHINE " + juce::String(i + 1), juce::dontSendNotification);
        tur.title.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        tur.title.setColour(juce::Label::textColourId, colTur);
        addAndMakeVisible(tur.title);

        setupDial(tur.probSlider, tur.probLabel, "PROB", colTur, "%", 0.5);
        tur.lenCombo.addItemList({ "4 Steps", "8 Steps", "16 Steps", "32 Steps" }, 1);
        addAndMakeVisible(tur.lenCombo);

        tur.syncToggle.getProperties().set("accentColor", colTur.toString());
        tur.syncToggle.getProperties().set("isSyncToggle", true);
        addAndMakeVisible(tur.syncToggle);

        tur.subdivCombo.addItemList(subdivNames, 1);
        addAndMakeVisible(tur.subdivCombo);

        setupDial(tur.glideSlider, tur.glideLabel, "GLIDE", colTur, "", 0.0);

        attachSlider(turProbIDs[i].getParamID(), tur.probSlider);
        attachCombo(turLenIDs[i].getParamID(), tur.lenCombo);
        attachButton(turSyncIDs[i].getParamID(), tur.syncToggle);
        attachCombo(turSubdivIDs[i].getParamID(), tur.subdivCombo);
        attachSlider(turGlideIDs[i].getParamID(), tur.glideSlider);
    }

    // 4. 4 Macros Setup
    macroTitle.setText("MACRO CONTROLS", juce::dontSendNotification);
    macroTitle.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    macroTitle.setColour(juce::Label::textColourId, colMac);
    addAndMakeVisible(macroTitle);

    const juce::ParameterID macIDs[4] = { IDs::macro1, IDs::macro2, IDs::macro3, IDs::macro4 };
    for (int i = 0; i < 4; ++i)
    {
        setupDial(macroSliders[i], macroLabels[i], "M" + juce::String(i + 1), colMac, "", 0.0);
        attachSlider(macIDs[i].getParamID(), macroSliders[i]);
    }

    // 5. 2 XY Pads
    addAndMakeVisible(xyPad1);
    addAndMakeVisible(xyPad2);

    startTimerHz(30);
}

ModulatorsPanelComponent::~ModulatorsPanelComponent()
{
    stopTimer();
}

void ModulatorsPanelComponent::timerCallback()
{
    repaint();
}

void ModulatorsPanelComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0c0e14));

    auto drawCard = [&g](juce::Rectangle<int> bounds, const juce::Colour& accent)
    {
        if (bounds.isEmpty()) return;
        auto b = bounds.toFloat();
        g.setColour(juce::Colour(0xff141822));
        g.fillRoundedRectangle(b, 6.0f);
        g.setColour(accent.withAlpha(0.65f));
        g.drawRoundedRectangle(b, 6.0f, 1.2f);
    };

    // Paint Cards
    for (int i = 0; i < 3; ++i)
    {
        drawCard(lfoCards[i], juce::Colour(0xff00d4ff));
        drawCard(envCards[i], juce::Colour(0xffff9900));
    }
    for (int i = 0; i < 2; ++i)
        drawCard(turingCards[i], juce::Colour(0xffa855f7));
    drawCard(macroCard, juce::Colour(0xff4ade80));

    // Paint Live Meters
    auto& mod = engine.getModEngine();

    // LFO Scopes
    const float lfoVals[3] = { mod.getLFO1Val(), mod.getLFO2Val(), mod.getLFO3Val() };
    for (int i = 0; i < 3; ++i)
    {
        if (!lfoScopes[i].isEmpty())
        {
            auto b = lfoScopes[i].toFloat();
            g.setColour(juce::Colour(0xff0a0c10));
            g.fillRoundedRectangle(b, 4.0f);

            const float v = (lfoVals[i] + 1.0f) * 0.5f; // 0..1
            g.setColour(juce::Colour(0xff00d4ff));
            g.fillEllipse(b.getX() + v * (b.getWidth() - 8.0f), b.getCentreY() - 4.0f, 8.0f, 8.0f);
        }
    }

    // Env Meters
    const float envVals[3] = { mod.getEnv1Val(), mod.getEnv2Val(), mod.getEnv3Val() };
    for (int i = 0; i < 3; ++i)
    {
        if (!envMeters[i].isEmpty())
        {
            auto b = envMeters[i].toFloat();
            g.setColour(juce::Colour(0xff0a0c10));
            g.fillRoundedRectangle(b, 4.0f);

            const float w = std::clamp(envVals[i], 0.0f, 1.0f) * b.getWidth();
            g.setColour(juce::Colour(0xffff9900));
            g.fillRoundedRectangle(b.getX(), b.getY(), w, b.getHeight(), 3.0f);
        }
    }
}

void ModulatorsPanelComponent::resized()
{
    auto area = getLocalBounds().reduced(8);

    // 3 Rows:
    // Row 1 (28% H): 3 LFOs
    // Row 2 (28% H): 3 Envelope Followers
    // Row 3 (44% H): 2 Turing + 4 Macros + 2 XY Pads
    const int totalH = area.getHeight();
    const int row1H = static_cast<int>(totalH * 0.28f);
    const int row2H = static_cast<int>(totalH * 0.28f);

    auto r1 = area.removeFromTop(row1H);
    area.removeFromTop(6);
    auto r2 = area.removeFromTop(row2H);
    area.removeFromTop(6);
    auto r3 = area;

    // Row 1: 3 LFOs
    const int lfoW = (r1.getWidth() - 12) / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto card = r1.removeFromLeft(lfoW);
        if (i < 2) r1.removeFromLeft(6);
        lfoCards[i] = card;

        auto cArea = card.reduced(8, 4);
        auto head = cArea.removeFromTop(20);
        lfos[i].title.setBounds(head.removeFromLeft(80));
        lfoScopes[i] = head.reduced(4, 2);

        cArea.removeFromTop(4);
        auto topCtrls = cArea.removeFromTop(22);
        lfos[i].waveCombo.setBounds(topCtrls.removeFromLeft(topCtrls.getWidth() / 2 - 2));
        topCtrls.removeFromLeft(4);
        lfos[i].syncToggle.setBounds(topCtrls.removeFromLeft(44));
        lfos[i].subdivCombo.setBounds(topCtrls);

        cArea.removeFromTop(4);
        const int dialW = cArea.getWidth() / 2;
        auto d1 = cArea.removeFromLeft(dialW);
        lfos[i].rateLabel.setBounds(d1.removeFromTop(12));
        lfos[i].rateSlider.setBounds(d1);

        auto d2 = cArea;
        lfos[i].smoothLabel.setBounds(d2.removeFromTop(12));
        lfos[i].smoothSlider.setBounds(d2);
    }

    // Row 2: 3 Envelope Followers
    const int envW = (r2.getWidth() - 12) / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto card = r2.removeFromLeft(envW);
        if (i < 2) r2.removeFromLeft(6);
        envCards[i] = card;

        auto cArea = card.reduced(8, 4);
        auto head = cArea.removeFromTop(20);
        envs[i].title.setBounds(head.removeFromLeft(120));
        envMeters[i] = head.reduced(4, 4);

        cArea.removeFromTop(4);
        envs[i].srcCombo.setBounds(cArea.removeFromTop(22));

        cArea.removeFromTop(4);
        const int dialW = cArea.getWidth() / 3;

        auto d1 = cArea.removeFromLeft(dialW);
        envs[i].attLabel.setBounds(d1.removeFromTop(12));
        envs[i].attSlider.setBounds(d1);

        auto d2 = cArea.removeFromLeft(dialW);
        envs[i].relLabel.setBounds(d2.removeFromTop(12));
        envs[i].relSlider.setBounds(d2);

        auto d3 = cArea;
        envs[i].gainLabel.setBounds(d3.removeFromTop(12));
        envs[i].gainSlider.setBounds(d3);
    }

    // Row 3: 2 Turing + 4 Macros + 2 XY Pads
    const int turW = static_cast<int>(r3.getWidth() * 0.22f);
    const int macW = static_cast<int>(r3.getWidth() * 0.22f);
    const int xyW  = (r3.getWidth() - turW * 2 - macW - 24) / 2;

    for (int i = 0; i < 2; ++i)
    {
        auto card = r3.removeFromLeft(turW);
        r3.removeFromLeft(6);
        turingCards[i] = card;

        auto cArea = card.reduced(8, 4);
        turings[i].title.setBounds(cArea.removeFromTop(20));
        cArea.removeFromTop(2);

        auto row1 = cArea.removeFromTop(22);
        turings[i].lenCombo.setBounds(row1.removeFromLeft(row1.getWidth() / 2 - 2));
        row1.removeFromLeft(4);
        turings[i].syncToggle.setBounds(row1.removeFromLeft(40));
        turings[i].subdivCombo.setBounds(row1);

        cArea.removeFromTop(4);
        const int dialW = cArea.getWidth() / 2;
        auto d1 = cArea.removeFromLeft(dialW);
        turings[i].probLabel.setBounds(d1.removeFromTop(12));
        turings[i].probSlider.setBounds(d1);

        auto d2 = cArea;
        turings[i].glideLabel.setBounds(d2.removeFromTop(12));
        turings[i].glideSlider.setBounds(d2);
    }

    // Macros
    macroCard = r3.removeFromLeft(macW);
    r3.removeFromLeft(6);
    auto mArea = macroCard.reduced(8, 4);
    macroTitle.setBounds(mArea.removeFromTop(20));
    mArea.removeFromTop(4);

    const int mDialW = mArea.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto d = (i < 3) ? mArea.removeFromLeft(mDialW) : mArea;
        macroLabels[i].setBounds(d.removeFromTop(12));
        macroSliders[i].setBounds(d);
    }

    // 2 XY Pads
    xyCards[0] = r3.removeFromLeft(xyW);
    r3.removeFromLeft(6);
    xyCards[1] = r3;

    xyPad1.setBounds(xyCards[0]);
    xyPad2.setBounds(xyCards[1]);
}
