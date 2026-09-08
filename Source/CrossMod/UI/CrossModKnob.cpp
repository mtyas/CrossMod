#include "CrossModKnob.h"
#include <cmath>

CrossModSlider::CrossModSlider()
    : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow)
{
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe5dfd2));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void CrossModSlider::setParamInfo(const juce::String& paramID, juce::RangedAudioParameter* param, MidiLearnManager* mlm)
{
    targetParamID = paramID;
    rangedParam = param;
    midiLearnManager = mlm;
}

void CrossModSlider::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }

    juce::Slider::mouseDown(e);
}

void CrossModSlider::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (rangedParam != nullptr)
    {
        setValue(rangedParam->getDefaultValue(), juce::sendNotificationAsync);
    }
}

void CrossModSlider::showContextMenu()
{
    if (midiLearnManager == nullptr || targetParamID.isEmpty())
        return;

    juce::PopupMenu menu;

    juce::String title = rangedParam != nullptr ? rangedParam->getName(30) : targetParamID;
    menu.addSectionHeader(title);

    bool isLearningThis = midiLearnManager->isLearningParam(targetParamID);
    int mappedCC = -1;
    int mappedCh = 0;
    bool isMapped = midiLearnManager->getMappingForParam(targetParamID, mappedCC, mappedCh);

    if (isLearningThis)
    {
        menu.addItem(1, "Cancel MIDI Learn");
    }
    else
    {
        menu.addItem(1, "MIDI Learn (Move a Controller Knob)");
    }

    if (isMapped)
    {
        juce::String chStr = (mappedCh == 0) ? "Omni" : ("Ch " + juce::String(mappedCh));
        menu.addItem(2, "Assigned: CC " + juce::String(mappedCC) + " (" + chStr + ")", false, false);
        menu.addItem(3, "Unassign CC " + juce::String(mappedCC));
    }

    menu.addSeparator();
    menu.addItem(4, "Clear All MIDI Mappings...");
    if (rangedParam != nullptr)
    {
        menu.addItem(5, "Reset to Default Value");
    }

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, isLearningThis, isMapped](int result)
        {
            if (result == 1)
            {
                if (isLearningThis)
                    midiLearnManager->cancelLearning();
                else
                    midiLearnManager->startLearning(targetParamID);
                repaint();
            }
            else if (result == 3)
            {
                midiLearnManager->removeMappingForParam(targetParamID);
                repaint();
            }
            else if (result == 4)
            {
                midiLearnManager->clearAllMappings();
                repaint();
            }
            else if (result == 5)
            {
                if (rangedParam != nullptr)
                    setValue(rangedParam->getDefaultValue(), juce::sendNotificationAsync);
            }
        });
}

void CrossModSlider::paint(juce::Graphics& g)
{
    juce::Slider::paint(g);

    // Draw pulsating indicator if currently learning
    if (midiLearnManager != nullptr && midiLearnManager->isLearningParam(targetParamID))
    {
        auto bounds = getLocalBounds().toFloat();
        float textBoxH = 16.0f;
        auto dialBounds = bounds.withTrimmedBottom(textBoxH);
        float diameter = std::min(dialBounds.getWidth(), dialBounds.getHeight()) - 4.0f;
        auto center = dialBounds.getCentre();

        float timeMs = (float)(juce::Time::getMillisecondCounter() % 1000);
        float phase = (timeMs / 1000.0f) * juce::MathConstants<float>::twoPi;
        float pulseAlpha = 0.5f + 0.45f * std::sin(phase);

        // Halo circle
        g.setColour(juce::Colour(0xff29b6f6).withAlpha(pulseAlpha));
        g.drawEllipse(center.x - diameter * 0.5f, center.y - diameter * 0.5f, diameter, diameter, 2.5f);

        // "LEARN" badge
        auto badgeRect = juce::Rectangle<float>(center.x - 22.0f, center.y - 8.0f, 44.0f, 16.0f);
        g.setColour(juce::Colour(0xd0101418));
        g.fillRoundedRectangle(badgeRect, 3.0f);
        g.setColour(juce::Colour(0xff29b6f6));
        g.drawRoundedRectangle(badgeRect, 3.0f, 1.0f);

        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(juce::Colours::white.withAlpha(pulseAlpha));
        g.drawText("LEARN", badgeRect, juce::Justification::centred, false);
    }
}

void CrossModKnob::init(juce::Component& parent,
                        juce::AudioProcessorValueTreeState& apvts,
                        MidiLearnManager& mlm,
                        const juce::ParameterID& pid,
                        const juce::String& text,
                        VintageLookAndFeel::KnobColor color,
                        const juce::String& suffix)
{
    slider = std::make_unique<CrossModSlider>();
    slider->getProperties().set("KnobAccent", static_cast<int>(color));
    if (!suffix.isEmpty())
        slider->setTextValueSuffix(suffix);

    auto* param = apvts.getParameter(pid.getParamID());
    slider->setParamInfo(pid.getParamID(), param, &mlm);

    parent.addAndMakeVisible(slider.get());

    label = std::make_unique<juce::Label>("lbl", text);
    label->setFont(juce::FontOptions(10.5f, juce::Font::bold));
    label->setColour(juce::Label::textColourId, juce::Colour(0xffc8c2b5));
    label->setJustificationType(juce::Justification::centred);
    parent.addAndMakeVisible(label.get());

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pid.getParamID(), *slider);
}

void CrossModKnob::setBounds(int x, int y, int w, int h)
{
    int labelH = 15;
    if (label != nullptr)
        label->setBounds(x, y, w, labelH);
    if (slider != nullptr)
        slider->setBounds(x, y + labelH - 2, w, h - labelH + 2);
}
