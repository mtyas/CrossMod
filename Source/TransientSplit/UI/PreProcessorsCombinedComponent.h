#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "TransientSplitPanelComponent.h"
#include "FilterBankPanelComponent.h"
#include "AdvancedPreProcessorsComponent.h"

class PreProcessorsCombinedComponent : public juce::Component
{
public:
    enum class SubTab
    {
        TransientSplitter = 0,
        FilterBank3,
        MidSide,
        DynamicSplitter,
        SpectralHarmonic,
        PhaseSpatial
    };

    PreProcessorsCombinedComponent(juce::AudioProcessorValueTreeState& apvts, TransientSplitEngine& engineToUse)
    {
        // 1. Sub-Tab Selector Buttons
        auto setupTab = [this](juce::TextButton& btn, const juce::String& text, SubTab tab, const juce::Colour& col)
        {
            btn.setButtonText(text);
            btn.setClickingTogglesState(false);
            btn.getProperties().set("accentColor", col.toString());
            btn.onClick = [this, tab]() { setActiveSubTab(tab); };
            addAndMakeVisible(btn);
        };

        setupTab(tabTransient, "1. TRANSIENT SPLIT", SubTab::TransientSplitter, TransientLookAndFeel::accentTransient);
        setupTab(tabFilter,    "2. 3-BAND FILTER",   SubTab::FilterBank3,      juce::Colour(0xfff59e0b));
        setupTab(tabMidSide,   "3. MID / SIDES",     SubTab::MidSide,          juce::Colour(0xff38bdf8));
        setupTab(tabDynamic,   "4. DYNAMIC SPLIT",   SubTab::DynamicSplitter,  juce::Colour(0xfffbbf24));
        setupTab(tabSpectral,  "5. SPECTRAL HARMONIC", SubTab::SpectralHarmonic, juce::Colour(0xffa855f7));
        setupTab(tabSpatial,   "6. PHASE & SPATIAL", SubTab::PhaseSpatial,     juce::Colour(0xff10b981));

        // 2. Sub-Panels
        transPanel = std::make_unique<TransientSplitPanelComponent>(apvts, engineToUse);
        addChildComponent(*transPanel);

        filterPanel = std::make_unique<FilterBankPanelComponent>(apvts, engineToUse);
        addChildComponent(*filterPanel);

        midSidePanel = std::make_unique<MidSidePanelComponent>(apvts);
        addChildComponent(*midSidePanel);

        dynamicPanel = std::make_unique<DynamicSplitterPanelComponent>(apvts);
        addChildComponent(*dynamicPanel);

        spectralPanel = std::make_unique<SpectralHarmonicPanelComponent>(apvts);
        addChildComponent(*spectralPanel);

        spatialPanel = std::make_unique<PhaseSpatialPanelComponent>(apvts);
        addChildComponent(*spatialPanel);

        setActiveSubTab(SubTab::TransientSplitter);
    }

    ~PreProcessorsCombinedComponent() override = default;

    void setActiveSubTab(SubTab tab)
    {
        currentTab = tab;

        tabTransient.getProperties().set("isActiveTab", tab == SubTab::TransientSplitter);
        tabFilter.getProperties().set("isActiveTab", tab == SubTab::FilterBank3);
        tabMidSide.getProperties().set("isActiveTab", tab == SubTab::MidSide);
        tabDynamic.getProperties().set("isActiveTab", tab == SubTab::DynamicSplitter);
        tabSpectral.getProperties().set("isActiveTab", tab == SubTab::SpectralHarmonic);
        tabSpatial.getProperties().set("isActiveTab", tab == SubTab::PhaseSpatial);

        transPanel->setVisible(tab == SubTab::TransientSplitter);
        filterPanel->setVisible(tab == SubTab::FilterBank3);
        midSidePanel->setVisible(tab == SubTab::MidSide);
        dynamicPanel->setVisible(tab == SubTab::DynamicSplitter);
        spectralPanel->setVisible(tab == SubTab::SpectralHarmonic);
        spatialPanel->setVisible(tab == SubTab::PhaseSpatial);

        repaint();
        resized();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(6);

        // Top Sub-Navigation Bar
        auto navBar = bounds.removeFromTop(32);
        const int tabW = navBar.getWidth() / 6;

        tabTransient.setBounds(navBar.removeFromLeft(tabW).reduced(2, 2));
        tabFilter.setBounds(navBar.removeFromLeft(tabW).reduced(2, 2));
        tabMidSide.setBounds(navBar.removeFromLeft(tabW).reduced(2, 2));
        tabDynamic.setBounds(navBar.removeFromLeft(tabW).reduced(2, 2));
        tabSpectral.setBounds(navBar.removeFromLeft(tabW).reduced(2, 2));
        tabSpatial.setBounds(navBar.reduced(2, 2));

        bounds.removeFromTop(6); // Spacing

        if (transPanel->isVisible())   transPanel->setBounds(bounds);
        if (filterPanel->isVisible())  filterPanel->setBounds(bounds);
        if (midSidePanel->isVisible()) midSidePanel->setBounds(bounds);
        if (dynamicPanel->isVisible()) dynamicPanel->setBounds(bounds);
        if (spectralPanel->isVisible()) spectralPanel->setBounds(bounds);
        if (spatialPanel->isVisible()) spatialPanel->setBounds(bounds);
    }

private:
    SubTab currentTab = SubTab::TransientSplitter;

    juce::TextButton tabTransient, tabFilter, tabMidSide, tabDynamic, tabSpectral, tabSpatial;

    std::unique_ptr<TransientSplitPanelComponent> transPanel;
    std::unique_ptr<FilterBankPanelComponent> filterPanel;
    std::unique_ptr<MidSidePanelComponent> midSidePanel;
    std::unique_ptr<DynamicSplitterPanelComponent> dynamicPanel;
    std::unique_ptr<SpectralHarmonicPanelComponent> spectralPanel;
    std::unique_ptr<PhaseSpatialPanelComponent> spatialPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreProcessorsCombinedComponent)
};
