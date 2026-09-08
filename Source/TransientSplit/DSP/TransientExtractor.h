#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

enum class SplitSoloMode
{
    Normal = 0,
    SoloTransient = 1,
    SoloSustain = 2,
    MuteTransient = 3,
    MuteSustain = 4
};

enum class DetectionMode
{
    Transient = 0, // Snappy micro-transient spikes (drums, percussive hits)
    Attack = 1     // Extended attack duration (strummed guitar, synth pluck, vocal onset)
};

class TransientExtractor
{
public:
    TransientExtractor() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        
        // Lookahead buffer: ~3ms max lookahead (192 samples at 48kHz, up to 768 at 192kHz)
        maxLookaheadSamples = static_cast<int>(currentSampleRate * 0.005) + 8;
        lookaheadBufferL.assign(maxLookaheadSamples, 0.0f);
        lookaheadBufferR.assign(maxLookaheadSamples, 0.0f);
        lookaheadWritePos = 0;

        reset();
        updateCoefficients();
    }

    void reset()
    {
        std::fill(lookaheadBufferL.begin(), lookaheadBufferL.end(), 0.0f);
        std::fill(lookaheadBufferR.begin(), lookaheadBufferR.end(), 0.0f);
        lookaheadWritePos = 0;

        envFastL = 0.0f;
        envFastR = 0.0f;
        envSlowL = 0.0f;
        envSlowR = 0.0f;
        smoothWeightL = 0.0f;
        smoothWeightR = 0.0f;

        meterTransient = 0.0f;
        meterSustain = 0.0f;
    }

    void setParameters(float attackMs, float releaseMs, float sensitivity, 
                       float thresholdDb, DetectionMode mode, int lookaheadSamples,
                       float transientGainDb, float sustainGainDb,
                       float transientPan, float sustainPan,
                       SplitSoloMode soloMode)
    {
        attackTimeMs = std::clamp(attackMs, 1.0f, 300.0f);
        releaseTimeMs = std::clamp(releaseMs, 10.0f, 600.0f);
        detectionSensitivity = std::clamp(sensitivity, 0.1f, 3.0f);
        splitThresholdLinear = std::pow(10.0f, std::clamp(thresholdDb, -60.0f, 0.0f) * 0.05f);
        currentMode = mode;
        
        currentLookahead = std::clamp(lookaheadSamples, 0, maxLookaheadSamples - 1);

        transGainLinear = (transientGainDb <= -59.5f) ? 0.0f : std::pow(10.0f, transientGainDb * 0.05f);
        sustGainLinear = (sustainGainDb <= -59.5f) ? 0.0f : std::pow(10.0f, sustainGainDb * 0.05f);

        transPanVal = std::clamp(transientPan, -1.0f, 1.0f);
        sustPanVal = std::clamp(sustainPan, -1.0f, 1.0f);

        currentSoloMode = soloMode;

        updateCoefficients();
    }

    int getLatencySamples() const noexcept
    {
        return currentLookahead;
    }

    float getTransientMeterLevel() const noexcept { return meterTransient; }
    float getSustainMeterLevel() const noexcept { return meterSustain; }

    void process(const float* inL, const float* inR,
                 float* transOutL, float* transOutR,
                 float* sustOutL, float* sustOutR,
                 int numSamples,
                 float* delayedDryOutL = nullptr, float* delayedDryOutR = nullptr)
    {
        if (numSamples <= 0) return;

        // Equal power panning factors (normalized so center pan = 1.0)
        constexpr float sqrt2 = 1.414213562373095f;
        const float tPanL = std::cos((transPanVal + 1.0f) * 0.25f * 3.14159265f) * sqrt2;
        const float tPanR = std::sin((transPanVal + 1.0f) * 0.25f * 3.14159265f) * sqrt2;
        const float sPanL = std::cos((sustPanVal + 1.0f) * 0.25f * 3.14159265f) * sqrt2;
        const float sPanR = std::sin((sustPanVal + 1.0f) * 0.25f * 3.14159265f) * sqrt2;

        float peakTrans = 0.0f;
        float peakSust = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float xL = inL ? inL[i] : 0.0f;
            const float xR = inR ? inR[i] : 0.0f;

            // Write into lookahead ring buffer
            lookaheadBufferL[lookaheadWritePos] = xL;
            lookaheadBufferR[lookaheadWritePos] = xR;

            // Calculate delayed sample index
            int readPos = lookaheadWritePos - currentLookahead;
            if (readPos < 0)
                readPos += maxLookaheadSamples;

            const float delayedL = lookaheadBufferL[readPos];
            const float delayedR = lookaheadBufferR[readPos];

            if (delayedDryOutL != nullptr) delayedDryOutL[i] = delayedL;
            if (delayedDryOutR != nullptr) delayedDryOutR[i] = delayedR;

            lookaheadWritePos = (lookaheadWritePos + 1) % maxLookaheadSamples;

            // Amplitude envelope tracking (instantaneous absolute input)
            const float absL = std::abs(xL);
            const float absR = std::abs(xR);

            // Fast follower update
            if (absL > envFastL)
                envFastL = alphaFastAttack * envFastL + (1.0f - alphaFastAttack) * absL;
            else
                envFastL = alphaFastRelease * envFastL + (1.0f - alphaFastRelease) * absL;

            if (absR > envFastR)
                envFastR = alphaFastAttack * envFastR + (1.0f - alphaFastAttack) * absR;
            else
                envFastR = alphaFastRelease * envFastR + (1.0f - alphaFastRelease) * absR;

            // Slow follower update
            if (absL > envSlowL)
                envSlowL = alphaSlowAttack * envSlowL + (1.0f - alphaSlowAttack) * absL;
            else
                envSlowL = alphaSlowRelease * envSlowL + (1.0f - alphaSlowRelease) * absL;

            if (absR > envSlowR)
                envSlowR = alphaSlowAttack * envSlowR + (1.0f - alphaSlowAttack) * absR;
            else
                envSlowR = alphaSlowRelease * envSlowR + (1.0f - alphaSlowRelease) * absR;

            // Differential calculation: amount fast exceeds slow
            const float diffL = std::max(0.0f, envFastL - envSlowL);
            const float diffR = std::max(0.0f, envFastR - envSlowR);

            // Ratio metric with floor & threshold
            float rawWeightL = 0.0f;
            float rawWeightR = 0.0f;

            if (envFastL > splitThresholdLinear)
            {
                const float normL = diffL / (envFastL + 1e-6f);
                rawWeightL = std::clamp(normL * detectionSensitivity, 0.0f, 1.0f);
            }

            if (envFastR > splitThresholdLinear)
            {
                const float normR = diffR / (envFastR + 1e-6f);
                rawWeightR = std::clamp(normR * detectionSensitivity, 0.0f, 1.0f);
            }

            // Curve shaping depending on mode
            if (currentMode == DetectionMode::Transient)
            {
                // Quadratic curve for snappy transients
                rawWeightL = rawWeightL * rawWeightL;
                rawWeightR = rawWeightR * rawWeightR;
            }
            else // Attack mode
            {
                // S-curve for smoother attack envelopment
                rawWeightL = rawWeightL * (3.0f - 2.0f * rawWeightL);
                rawWeightR = rawWeightR * (3.0f - 2.0f * rawWeightR);
            }

            // Low-pass smooth the separation weights to prevent any high-frequency switching chatter
            smoothWeightL = alphaWeightSmooth * smoothWeightL + (1.0f - alphaWeightSmooth) * rawWeightL;
            smoothWeightR = alphaWeightSmooth * smoothWeightR + (1.0f - alphaWeightSmooth) * rawWeightR;

            const float wTL = std::clamp(smoothWeightL, 0.0f, 1.0f);
            const float wTR = std::clamp(smoothWeightR, 0.0f, 1.0f);
            const float wSL = 1.0f - wTL;
            const float wSR = 1.0f - wTR;

            // Split delayed signals
            float tL = delayedL * wTL;
            float tR = delayedR * wTR;
            float sL = delayedL * wSL;
            float sR = delayedR * wSR;

            // Apply Gains & Panning
            tL *= transGainLinear * tPanL;
            tR *= transGainLinear * tPanR;
            sL *= sustGainLinear * sPanL;
            sR *= sustGainLinear * sPanR;

            // Solo / Mute logic
            switch (currentSoloMode)
            {
                case SplitSoloMode::SoloTransient:
                    sL = 0.0f;
                    sR = 0.0f;
                    break;
                case SplitSoloMode::SoloSustain:
                    tL = 0.0f;
                    tR = 0.0f;
                    break;
                case SplitSoloMode::MuteTransient:
                    tL = 0.0f;
                    tR = 0.0f;
                    break;
                case SplitSoloMode::MuteSustain:
                    sL = 0.0f;
                    sR = 0.0f;
                    break;
                case SplitSoloMode::Normal:
                default:
                    break;
            }

            transOutL[i] = tL;
            transOutR[i] = tR;
            sustOutL[i] = sL;
            sustOutR[i] = sR;

            peakTrans = std::max(peakTrans, std::max(std::abs(tL), std::abs(tR)));
            peakSust = std::max(peakSust, std::max(std::abs(sL), std::abs(sR)));
        }

        // Smooth visual meter levels with decay
        const float decay = 0.92f;
        meterTransient = std::max(peakTrans, meterTransient * decay);
        meterSustain = std::max(peakSust, meterSustain * decay);
    }

private:
    void updateCoefficients()
    {
        const double sr = currentSampleRate;

        // Ultra fast attack for the fast follower: ~0.5 ms
        const double fastAttackTimeSec = 0.0005;
        alphaFastAttack = static_cast<float>(std::exp(-1.0 / (fastAttackTimeSec * sr)));

        // Fast release is governed by the attack window parameter (2ms to 300ms)
        double fastReleaseTimeSec = (attackTimeMs * 0.001);
        if (currentMode == DetectionMode::Attack)
            fastReleaseTimeSec *= 2.0; // Extend duration for attack mode
        alphaFastRelease = static_cast<float>(std::exp(-1.0 / (fastReleaseTimeSec * sr)));

        // Slow follower attack: ~25ms (transient mode) or ~70ms (attack mode)
        const double slowAttackTimeSec = (currentMode == DetectionMode::Transient) ? 0.025 : 0.070;
        alphaSlowAttack = static_cast<float>(std::exp(-1.0 / (slowAttackTimeSec * sr)));

        // Slow follower release: ~releaseTimeMs
        const double slowReleaseTimeSec = releaseTimeMs * 0.001;
        alphaSlowRelease = static_cast<float>(std::exp(-1.0 / (slowReleaseTimeSec * sr)));

        // Weight transition smoothing: ~0.8ms filter
        const double weightSmoothSec = 0.0008;
        alphaWeightSmooth = static_cast<float>(std::exp(-1.0 / (weightSmoothSec * sr)));
    }

    double currentSampleRate = 48000.0;
    int maxLookaheadSamples = 1024;
    int currentLookahead = 64; // ~1.3ms default
    int lookaheadWritePos = 0;

    std::vector<float> lookaheadBufferL;
    std::vector<float> lookaheadBufferR;

    float attackTimeMs = 20.0f;
    float releaseTimeMs = 150.0f;
    float detectionSensitivity = 1.0f;
    float splitThresholdLinear = 0.001f;
    DetectionMode currentMode = DetectionMode::Transient;

    float transGainLinear = 1.0f;
    float sustGainLinear = 1.0f;
    float transPanVal = 0.0f;
    float sustPanVal = 0.0f;
    SplitSoloMode currentSoloMode = SplitSoloMode::Normal;

    float alphaFastAttack = 0.0f;
    float alphaFastRelease = 0.0f;
    float alphaSlowAttack = 0.0f;
    float alphaSlowRelease = 0.0f;
    float alphaWeightSmooth = 0.0f;

    float envFastL = 0.0f;
    float envFastR = 0.0f;
    float envSlowL = 0.0f;
    float envSlowR = 0.0f;
    float smoothWeightL = 0.0f;
    float smoothWeightR = 0.0f;

    float meterTransient = 0.0f;
    float meterSustain = 0.0f;
};
