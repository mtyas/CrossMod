#include "CrossModVisualizer.h"
#include <cmath>
#include <algorithm>

CrossModVisualizer::CrossModVisualizer()
{
    preFxBufferL.assign(256, 0.0f);
    preFxBufferR.assign(256, 0.0f);
    postFxBufferL.assign(256, 0.0f);
    postFxBufferR.assign(256, 0.0f);
    startTimerHz(30);
}

CrossModVisualizer::~CrossModVisualizer()
{
    stopTimer();
}

void CrossModVisualizer::setParams(float k1to2, float k2to1, float coarse1, float coarse2, float fine1, float fine2, float voiceXMod)
{
    mod12 = k1to2;
    mod21 = k2to1;
    xVoiceMod = voiceXMod;

    float semiDiff = (coarse2 + fine2 * 0.01f) - (coarse1 + fine1 * 0.01f);
    ratio = std::pow(2.0f, semiDiff / 12.0f);
}

void CrossModVisualizer::setAudioWaveforms(const float* preFxDataL, const float* preFxDataR,
                                           const float* postFxDataL, const float* postFxDataR, int numSamples)
{
    if (preFxDataL != nullptr && numSamples > 0)
    {
        preFxBufferL.resize(numSamples);
        std::copy(preFxDataL, preFxDataL + numSamples, preFxBufferL.begin());
    }
    if (preFxDataR != nullptr && numSamples > 0)
    {
        preFxBufferR.resize(numSamples);
        std::copy(preFxDataR, preFxDataR + numSamples, preFxBufferR.begin());
    }
    if (postFxDataL != nullptr && numSamples > 0)
    {
        postFxBufferL.resize(numSamples);
        std::copy(postFxDataL, postFxDataL + numSamples, postFxBufferL.begin());
    }
    if (postFxDataR != nullptr && numSamples > 0)
    {
        postFxBufferR.resize(numSamples);
        std::copy(postFxDataR, postFxDataR + numSamples, postFxBufferR.begin());
    }
}

void CrossModVisualizer::timerCallback()
{
    animPhase += 0.04f;
    if (animPhase >= 6.2831853f)
        animPhase -= 6.2831853f;

    repaint();
}

void CrossModVisualizer::mouseDown(const juce::MouseEvent& e)
{
    float w = (float)getWidth();
    float tabW = w / 3.0f;

    if (e.y < 22)
    {
        if (e.x < tabW)
            setScopeMode(ScopeMode::XMod);
        else if (e.x < tabW * 2.0f)
            setScopeMode(ScopeMode::PreFx);
        else
            setScopeMode(ScopeMode::PostFx);
    }
    else
    {
        // Clicking on screen cycles to next mode
        int next = (static_cast<int>(currentMode) + 1) % 3;
        setScopeMode(static_cast<ScopeMode>(next));
    }
}

void CrossModVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Recessed Bezel & CRT dark glass
    g.setColour(juce::Colour(0xff121417));
    g.fillRoundedRectangle(bounds, 6.0f);

    auto screenBounds = bounds.reduced(3.0f);
    juce::ColourGradient crtGrad(juce::Colour(0xff091a13), screenBounds.getCentre(),
                                juce::Colour(0xff020805), screenBounds.getTopLeft(), true);
    g.setGradientFill(crtGrad);
    g.fillRoundedRectangle(screenBounds, 4.0f);

    // Bezel border
    g.setColour(juce::Colour(0xff333a42));
    g.drawRoundedRectangle(bounds, 6.0f, 1.2f);

    // 2. Mode Tabs at top
    float tabH = 17.0f;
    float tabW = screenBounds.getWidth() / 3.0f;
    float tabY = screenBounds.getY() + 1.0f;

    auto drawTab = [&](int idx, const juce::String& text, ScopeMode mode) {
        float tx = screenBounds.getX() + idx * tabW;
        bool active = (currentMode == mode);

        juce::Rectangle<float> tr(tx, tabY, tabW, tabH);
        if (active)
        {
            g.setColour(juce::Colour(0x352ecc71));
            g.fillRoundedRectangle(tr.reduced(1.0f, 1.0f), 2.0f);
            g.setColour(juce::Colour(0xff2ecc71));
            g.drawRoundedRectangle(tr.reduced(1.0f, 1.0f), 2.0f, 1.0f);

            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.setColour(juce::Colour(0xff80ffaa));
        }
        else
        {
            g.setFont(juce::FontOptions(8.0f, juce::Font::plain));
            g.setColour(juce::Colour(0xff55606d));
        }
        g.drawText(text, (int)tx, (int)tabY, (int)tabW, (int)tabH, juce::Justification::centred);
    };

    drawTab(0, "X-MOD", ScopeMode::XMod);
    drawTab(1, "PRE-FX", ScopeMode::PreFx);
    drawTab(2, "POST-FX", ScopeMode::PostFx);

    // Separator line under tabs
    g.setColour(juce::Colour(0x30333a42));
    g.drawLine(screenBounds.getX(), tabY + tabH, screenBounds.getRight(), tabY + tabH, 1.0f);

    // CRT Active Display Area
    auto displayArea = screenBounds;
    displayArea.removeFromTop(tabH + 2.0f);

    float cx = displayArea.getCentreX();
    float cy = displayArea.getCentreY();
    float rMax = std::min(displayArea.getWidth(), displayArea.getHeight()) * 0.44f;

    // Vintage CRT Reticle / Grid with concentric circles
    g.setColour(juce::Colour(0x182ecc71));
    g.drawLine(displayArea.getX() + 4.0f, cy, displayArea.getRight() - 4.0f, cy, 0.8f);
    g.drawLine(cx, displayArea.getY() + 4.0f, cx, displayArea.getBottom() - 4.0f, 0.8f);
    g.drawEllipse(cx - rMax, cy - rMax, rMax * 2.0f, rMax * 2.0f, 0.8f);
    g.drawEllipse(cx - rMax * 0.5f, cy - rMax * 0.5f, rMax, rMax, 0.6f);

    if (currentMode == ScopeMode::XMod)
    {
        // -------------------------------------------------------------
        // X-MOD: Cross-Modulation Synthesized Lissajous Curve
        // -------------------------------------------------------------
        constexpr int numPoints = 250;
        juce::Path p;
        constexpr float twoPi = 6.283185307179586f;

        float effectiveRatio = std::clamp(ratio, 0.125f, 8.0f);
        float beta12 = mod12 * 4.0f;
        float beta21 = mod21 * 4.0f;
        float extraMod = xVoiceMod * 2.0f;

        for (int i = 0; i <= numPoints; ++i)
        {
            float t = ((float)i / (float)numPoints) * twoPi;
            float theta1 = t + animPhase;
            float theta2 = t * effectiveRatio + animPhase * 1.5f;

            float s2_raw = std::sin(theta2);
            float s1 = std::sin(theta1 + beta21 * s2_raw + extraMod * std::sin(t * 3.0f));
            float s2 = std::sin(theta2 + beta12 * s1);

            float px = cx + s1 * (rMax * 0.9f);
            float py = cy - s2 * (rMax * 0.9f);

            if (i == 0)
                p.startNewSubPath(px, py);
            else
                p.lineTo(px, py);
        }

        // Green Phosphor glow
        g.setColour(juce::Colour(0x402ecc71));
        g.strokePath(p, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colour(0xd080ffaa));
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.strokePath(p, juce::PathStrokeType(0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        // -------------------------------------------------------------
        // PRE-FX or POST-FX: Real-Time Stereo Lissajous / Vector Phase Scope (L vs R)
        // -------------------------------------------------------------
        const auto& bufL = (currentMode == ScopeMode::PreFx) ? preFxBufferL : postFxBufferL;
        const auto& bufR = (currentMode == ScopeMode::PreFx) ? preFxBufferR : postFxBufferR;
        int n = std::min((int)bufL.size(), (int)bufR.size());

        juce::Path p;
        if (n > 2)
        {
            constexpr float scopeGain = 2.8f;
            for (int i = 0; i < n; ++i)
            {
                float valL = std::tanh(bufL[i] * scopeGain);
                float valR = std::tanh(bufR[i] * scopeGain);

                float px = cx + valL * (rMax * 0.92f);
                float py = cy - valR * (rMax * 0.92f);

                if (i == 0)
                    p.startNewSubPath(px, py);
                else
                    p.lineTo(px, py);
            }
        }
        else
        {
            p.startNewSubPath(cx - 2.0f, cy);
            p.lineTo(cx + 2.0f, cy);
        }

        // Distinct CRT colors for Pre vs Post
        juce::Colour glowCol = (currentMode == ScopeMode::PreFx) ? juce::Colour(0x352ecc71) : juce::Colour(0x35f39c12);
        juce::Colour beamCol = (currentMode == ScopeMode::PreFx) ? juce::Colour(0xd080ffaa) : juce::Colour(0xd0ffcc66);

        // Wide phosphor glow
        g.setColour(glowCol);
        g.strokePath(p, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Core phosphor beam
        g.setColour(beamCol);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Hot center
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.strokePath(p, juce::PathStrokeType(0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Subtitle indicator at bottom
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.setColour(juce::Colour(0x7080ffaa));
    juce::String subText = (currentMode == ScopeMode::XMod) ? "X-MOD LISSAJOUS"
                         : (currentMode == ScopeMode::PreFx) ? "PRE-FX LISSAJOUS"
                                                             : "POST-FX LISSAJOUS";
    g.drawText(subText, (int)displayArea.getX(), (int)displayArea.getBottom() - 12, (int)displayArea.getWidth(), 11, juce::Justification::centred);
}

