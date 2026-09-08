#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

enum class ScopeMode
{
    XMod = 0,
    PreFx = 1,
    PostFx = 2
};

class CrossModVisualizer : public juce::Component, public juce::Timer
{
public:
    CrossModVisualizer();
    ~CrossModVisualizer() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setParams(float k1to2, float k2to1, float coarse1, float coarse2, float fine1, float fine2, float voiceXMod);
    void setAudioWaveforms(const float* preFxDataL, const float* preFxDataR,
                           const float* postFxDataL, const float* postFxDataR, int numSamples);

    void setScopeMode(ScopeMode mode) { currentMode = mode; repaint(); }
    ScopeMode getScopeMode() const { return currentMode; }

private:
    ScopeMode currentMode = ScopeMode::XMod;

    float mod12 = 0.0f;
    float mod21 = 0.0f;
    float ratio = 1.0f;
    float xVoiceMod = 0.0f;
    float animPhase = 0.0f;

    std::vector<float> preFxBufferL;
    std::vector<float> preFxBufferR;
    std::vector<float> postFxBufferL;
    std::vector<float> postFxBufferR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrossModVisualizer)
};
