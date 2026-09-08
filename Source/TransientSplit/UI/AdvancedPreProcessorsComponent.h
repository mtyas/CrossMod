#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ParameterIDs.h"
#include "../DSP/TransientSplitEngine.h"
#include "TransientLookAndFeel.h"

// 1. Mid / Side & Stereo Panel
class MidSidePanelComponent : public juce::Component
{
public:
    MidSidePanelComponent(juce::AudioProcessorValueTreeState& apvts)
    {
        setupDial(widthSlider, widthLabel, "STEREO WIDTH", juce::Colour(0xff38bdf8));
        setupDial(balSlider, balLabel, "L / R BALANCE", juce::Colour(0xff818cf8));

        widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::midSideWidth.getParamID(), widthSlider);
        balAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::midSideBalance.getParamID(), balSlider);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20, 16);
        const int colW = area.getWidth() / 2;

        auto leftBox = area.removeFromLeft(colW).reduced(15, 0);
        widthLabel.setBounds(leftBox.removeFromTop(16));
        widthSlider.setBounds(leftBox);

        auto rightBox = area.reduced(15, 0);
        balLabel.setBounds(rightBox.removeFromTop(16));
        balSlider.setBounds(rightBox);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff12151d));
        g.setColour(juce::Colour(0xff222736));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f), 6.0f, 1.0f);

        g.setColour(juce::Colour(0xff38bdf8));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("MID / SIDES & STEREO SPLITTING", getLocalBounds().removeFromTop(24), juce::Justification::centred);
    }

private:
    void setupDial(juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
        s.getProperties().set("accentColor", col.toString());
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    }

    juce::Slider widthSlider, balSlider;
    juce::Label widthLabel, balLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment, balAttachment;
};

// 2. Dynamic Splitter Panel (Loud vs Quiet)
class DynamicSplitterPanelComponent : public juce::Component
{
public:
    DynamicSplitterPanelComponent(juce::AudioProcessorValueTreeState& apvts)
    {
        setupDial(threshSlider, threshLabel, "THRESHOLD", juce::Colour(0xfff59e0b));
        setupDial(attSlider, attLabel, "ATTACK", juce::Colour(0xfffbbf24));
        setupDial(relSlider, relLabel, "RELEASE", juce::Colour(0xfff59e0b));
        setupDial(kneeSlider, kneeLabel, "KNEE", juce::Colour(0xffd97706));

        threshAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::dynSplitThreshold.getParamID(), threshSlider);
        attAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::dynSplitAttack.getParamID(), attSlider);
        relAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::dynSplitRelease.getParamID(), relSlider);
        kneeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::dynSplitKnee.getParamID(), kneeSlider);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20, 16);
        const int colW = area.getWidth() / 4;

        auto box1 = area.removeFromLeft(colW).reduced(10, 0);
        threshLabel.setBounds(box1.removeFromTop(16)); threshSlider.setBounds(box1);

        auto box2 = area.removeFromLeft(colW).reduced(10, 0);
        attLabel.setBounds(box2.removeFromTop(16)); attSlider.setBounds(box2);

        auto box3 = area.removeFromLeft(colW).reduced(10, 0);
        relLabel.setBounds(box3.removeFromTop(16)); relSlider.setBounds(box3);

        auto box4 = area.reduced(10, 0);
        kneeLabel.setBounds(box4.removeFromTop(16)); kneeSlider.setBounds(box4);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff12151d));
        g.setColour(juce::Colour(0xff222736));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f), 6.0f, 1.0f);

        g.setColour(juce::Colour(0xfff59e0b));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("DYNAMIC AMPLITUDE SPLITTING (LOUD / PEAKS VS QUIET / FLOOR)", getLocalBounds().removeFromTop(24), juce::Justification::centred);
    }

private:
    void setupDial(juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
        s.getProperties().set("accentColor", col.toString());
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    }

    juce::Slider threshSlider, attSlider, relSlider, kneeSlider;
    juce::Label threshLabel, attLabel, relLabel, kneeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> threshAttachment, attAttachment, relAttachment, kneeAttachment;
};

// 3. Spectral & Harmonic Decomposition Panel
class SpectralHarmonicPanelComponent : public juce::Component
{
public:
    SpectralHarmonicPanelComponent(juce::AudioProcessorValueTreeState& apvts)
    {
        setupDial(sensSlider, sensLabel, "SENSITIVITY", juce::Colour(0xffa855f7));
        setupDial(focusSlider, focusLabel, "SPECTRAL FOCUS", juce::Colour(0xffc084fc));
        setupDial(smoothSlider, smoothLabel, "SMOOTHING", juce::Colour(0xffa855f7));

        sensAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::spectralHarmonicSens.getParamID(), sensSlider);
        focusAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::spectralFocus.getParamID(), focusSlider);
        smoothAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::spectralSmoothing.getParamID(), smoothSlider);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20, 16);
        const int colW = area.getWidth() / 3;

        auto box1 = area.removeFromLeft(colW).reduced(12, 0);
        sensLabel.setBounds(box1.removeFromTop(16)); sensSlider.setBounds(box1);

        auto box2 = area.removeFromLeft(colW).reduced(12, 0);
        focusLabel.setBounds(box2.removeFromTop(16)); focusSlider.setBounds(box2);

        auto box3 = area.reduced(12, 0);
        smoothLabel.setBounds(box3.removeFromTop(16)); smoothSlider.setBounds(box3);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff12151d));
        g.setColour(juce::Colour(0xff222736));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f), 6.0f, 1.0f);

        g.setColour(juce::Colour(0xffa855f7));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("SPECTRAL & HARMONIC DECOMPOSITION (TONAL / HARMONIC VS NOISE / RESIDUAL)", getLocalBounds().removeFromTop(24), juce::Justification::centred);
    }

private:
    void setupDial(juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
        s.getProperties().set("accentColor", col.toString());
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    }

    juce::Slider sensSlider, focusSlider, smoothSlider;
    juce::Label sensLabel, focusLabel, smoothLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensAttachment, focusAttachment, smoothAttachment;
};

// 4. Phase & Spatial Splitting Panel
class PhaseSpatialPanelComponent : public juce::Component
{
public:
    PhaseSpatialPanelComponent(juce::AudioProcessorValueTreeState& apvts)
    {
        setupDial(threshSlider, threshLabel, "SPATIAL THRESH", juce::Colour(0xff10b981));
        setupDial(winSlider, winLabel, "WINDOW", juce::Colour(0xff34d399));
        setupDial(spreadSlider, spreadLabel, "SPREAD", juce::Colour(0xff10b981));

        threshAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::phaseSpatialThresh.getParamID(), threshSlider);
        winAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::phaseSpatialWindow.getParamID(), winSlider);
        spreadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::phaseSpatialSpread.getParamID(), spreadSlider);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20, 16);
        const int colW = area.getWidth() / 3;

        auto box1 = area.removeFromLeft(colW).reduced(12, 0);
        threshLabel.setBounds(box1.removeFromTop(16)); threshSlider.setBounds(box1);

        auto box2 = area.removeFromLeft(colW).reduced(12, 0);
        winLabel.setBounds(box2.removeFromTop(16)); winSlider.setBounds(box2);

        auto box3 = area.reduced(12, 0);
        spreadLabel.setBounds(box3.removeFromTop(16)); spreadSlider.setBounds(box3);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff12151d));
        g.setColour(juce::Colour(0xff222736));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f), 6.0f, 1.0f);

        g.setColour(juce::Colour(0xff10b981));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("PHASE & SPATIAL SPLITTING (IN-PHASE MONO VS SPATIAL DIFFUSE)", getLocalBounds().removeFromTop(24), juce::Justification::centred);
    }

private:
    void setupDial(juce::Slider& s, juce::Label& l, const juce::String& text, const juce::Colour& col)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
        s.getProperties().set("accentColor", col.toString());
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, TransientLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    }

    juce::Slider threshSlider, winSlider, spreadSlider;
    juce::Label threshLabel, winLabel, spreadLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> threshAttachment, winAttachment, spreadAttachment;
};
