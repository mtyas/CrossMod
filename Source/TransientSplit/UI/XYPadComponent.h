#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ParameterIDs.h"
#include <vector>

struct XYMotionPoint
{
    float x = 0.0f; // -1.0 to 1.0
    float y = 0.0f; // -1.0 to 1.0
};

class XYPadComponent : public juce::Component,
                       private juce::Timer
{
public:
    explicit XYPadComponent(juce::AudioProcessorValueTreeState& apvts, int padIndex = 1);
    ~XYPadComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void updatePointFromMouse(const juce::MouseEvent& e);
    void setXYValues(float x, float y);

    juce::AudioProcessorValueTreeState& apvts;
    int padIdx = 1;

    juce::Label titleLabel;
    juce::Label coordsLabel;
    juce::TextButton recButton { "REC" };
    juce::TextButton loopButton { "LOOP" };
    juce::TextButton clearButton { "CLR" };
    juce::Slider speedSlider;
    juce::Label speedLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;

    juce::Rectangle<int> padBounds;

    float currentX = 0.0f; // -1.0 to 1.0
    float currentY = 0.0f; // -1.0 to 1.0

    bool isRecording = false;
    bool isLooping = false;
    bool isDragging = false;

    static constexpr int MaxRecordedPoints = 900; // 15 seconds at 60 Hz
    std::vector<XYMotionPoint> recordedMotion;
    float loopPlaybackIndex = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPadComponent)
};
