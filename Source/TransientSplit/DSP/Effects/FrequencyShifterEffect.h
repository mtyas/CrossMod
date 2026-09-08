#pragma once

#include "BaseEffect.h"
#include <cmath>
#include <array>
#include <algorithm>

class FrequencyShifterEffect : public BaseEffect
{
public:
    FrequencyShifterEffect() = default;

    EffectType getType() const override { return EffectType::FrequencyShifter; }
    juce::String getName() const override { return "Bode Frequency Shifter"; }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset() override
    {
        oscPhaseL = 0.0f;
        oscPhaseR = 0.0f;
        feedbackL = 0.0f;
        feedbackR = 0.0f;
        toneStateL = 0.0f;
        toneStateR = 0.0f;

        xStateL.fill(0.0f);
        yStateL.fill(0.0f);
        xStateR.fill(0.0f);
        yStateR.fill(0.0f);
    }

    void setParameters(float p1, float p2, float p3, float p4, float p5, float mix) override
    {
        // P1: Coarse Shift (-2000 Hz to +2000 Hz)
        const float coarseShift = -2000.0f + std::clamp(p1, 0.0f, 1.0f) * 4000.0f;

        // P2: Fine Shift (-50 Hz to +50 Hz)
        const float fineShift = -50.0f + std::clamp(p2, 0.0f, 1.0f) * 100.0f;

        shiftHz = coarseShift + fineShift;

        // P3: Stereo Mode (0: Up-Shift, 1: Down-Shift, 2: Stereo Up/Down)
        mode = std::clamp(static_cast<int>(std::floor(p3 * 2.99f)), 0, 2);

        // P4: Feedback (0% to 75%)
        feedbackAmount = std::clamp(p4, 0.0f, 0.75f);

        // P5: Tone Filter (600Hz to 16000Hz)
        const double cutoffHz = 600.0 * std::pow(26.66, std::clamp(p5, 0.0f, 1.0f));
        toneCoeff = static_cast<float>(1.0 - std::exp(-6.2831853 * cutoffHz / currentSampleRate));

        wetMix = std::clamp(mix, 0.0f, 1.0f);

        phaseInc = static_cast<float>(6.283185307179586 * shiftHz / currentSampleRate);
    }

    void process(float* bufferL, float* bufferR, int numSamples) override
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = bufferL[i];
            const float dryR = bufferR ? bufferR[i] : dryL;

            const float inL = dryL + feedbackL * feedbackAmount;
            const float inR = dryR + feedbackR * feedbackAmount;

            oscPhaseL += phaseInc;
            if (oscPhaseL >= 6.2831853f) oscPhaseL -= 6.2831853f;
            if (oscPhaseL < 0.0f) oscPhaseL += 6.2831853f;

            oscPhaseR += (mode == 2) ? -phaseInc : phaseInc;
            if (oscPhaseR >= 6.2831853f) oscPhaseR -= 6.2831853f;
            if (oscPhaseR < 0.0f) oscPhaseR += 6.2831853f;

            float iL = 0.0f, qL = 0.0f;
            float iR = 0.0f, qR = 0.0f;
            splitPhase(inL, xStateL, yStateL, iL, qL);
            splitPhase(inR, xStateR, yStateR, iR, qR);

            const float cosL = std::cos(oscPhaseL);
            const float sinL = std::sin(oscPhaseL);
            const float cosR = std::cos(oscPhaseR);
            const float sinR = std::sin(oscPhaseR);

            float sL = 0.0f, sR = 0.0f;
            if (mode == 0)
            {
                sL = iL * cosL - qL * sinL;
                sR = iR * cosR - qR * sinR;
            }
            else if (mode == 1)
            {
                sL = iL * cosL + qL * sinL;
                sR = iR * cosR + qR * sinR;
            }
            else
            {
                sL = iL * cosL - qL * sinL;
                sR = iR * cosR + qR * sinR;
            }

            toneStateL += toneCoeff * (sL - toneStateL);
            toneStateR += toneCoeff * (sR - toneStateR);

            feedbackL = toneStateL;
            feedbackR = toneStateR;

            bufferL[i] = dryL * (1.0f - wetMix) + toneStateL * wetMix;
            if (bufferR)
                bufferR[i] = dryR * (1.0f - wetMix) + toneStateR * wetMix;
        }
    }

private:
    void splitPhase(float in, std::array<float, 4>& xState, std::array<float, 4>& yState, float& iOut, float& qOut)
    {
        const float alphaX[4] = { 0.161758f, 0.733029f, 0.945350f, 0.990598f };
        const float alphaY[4] = { 0.479401f, 0.872496f, 0.977251f, 0.997499f };

        float x = in;
        for (int k = 0; k < 4; ++k)
        {
            float y = alphaX[k] * (x - xState[k]) + xState[k];
            xState[k] = x;
            x = y;
        }
        iOut = x;

        float yIn = in;
        for (int k = 0; k < 4; ++k)
        {
            float y = alphaY[k] * (yIn - yState[k]) + yState[k];
            yState[k] = yIn;
            yIn = y;
        }
        qOut = yIn;
    }

    double currentSampleRate = 48000.0;
    float shiftHz = 0.0f;
    int mode = 0;
    float feedbackAmount = 0.0f;
    float toneCoeff = 0.85f;
    float wetMix = 1.0f;

    float phaseInc = 0.0f;
    float oscPhaseL = 0.0f;
    float oscPhaseR = 0.0f;
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
    float toneStateL = 0.0f;
    float toneStateR = 0.0f;

    std::array<float, 4> xStateL { 0.0f }, yStateL { 0.0f };
    std::array<float, 4> xStateR { 0.0f }, yStateR { 0.0f };
};
