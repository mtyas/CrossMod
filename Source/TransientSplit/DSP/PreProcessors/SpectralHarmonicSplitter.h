#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

class SpectralHarmonicSplitter
{
public:
    SpectralHarmonicSplitter() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        currentMaxBlock = std::max(64, maxBlockSize);

        // 8 Logarithmically spaced tracking frequencies (70 Hz to 12 kHz)
        const float freqs[8] = { 80.0f, 180.0f, 420.0f, 950.0f, 2100.0f, 4600.0f, 8500.0f, 13000.0f };
        for (int b = 0; b < 8; ++b)
        {
            const float clampedF = std::clamp(freqs[b], 20.0f, static_cast<float>(currentSampleRate * 0.45));
            const float g = std::tan(3.14159265358979323846f * clampedF / static_cast<float>(currentSampleRate));
            const float k = 1.0f / 3.0f; // Q = 3.0
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;

            gCoeff[b] = g;
            kCoeff[b] = k;
            a1Coeff[b] = a1;
            a2Coeff[b] = a2;
            a3Coeff[b] = a3;
        }

        reset();
    }

    void reset() noexcept
    {
        for (int b = 0; b < 8; ++b)
        {
            s1L[b] = 0.0f; s2L[b] = 0.0f;
            s1R[b] = 0.0f; s2R[b] = 0.0f;
        }
        harmonicEnvL = 0.0f;
        harmonicEnvR = 0.0f;
    }

    void setParameters(float sensitivity, float spectralFocus, float smoothingMs) noexcept
    {
        sens = std::clamp(sensitivity, 0.0f, 1.0f);
        focus = std::clamp(spectralFocus, 0.0f, 1.0f);

        const double smoothSec = std::clamp(smoothingMs, 1.0f, 100.0f) * 0.001;
        alphaSmooth = static_cast<float>(1.0 - std::exp(-1.0 / (smoothSec * currentSampleRate)));
    }

    void process(const float* inL, const float* inR,
                 float* harmonicL, float* harmonicR,
                 float* noiseL, float* noiseR,
                 int numSamples)
    {
        if (numSamples <= 0 || !inL) return;

        for (int i = 0; i < numSamples; ++i)
        {
            const float l = inL[i];
            const float r = inR ? inR[i] : l;

            float maxBandL = 0.0f;
            float maxBandR = 0.0f;

            // Zero-Delay Feedback (ZDF) 2nd-order State Variable Filter Bank
            for (int b = 0; b < 8; ++b)
            {
                // Left channel
                const float bpL = a2Coeff[b] * (l - s1L[b]) + a1Coeff[b] * s2L[b];
                const float lpL = a3Coeff[b] * (l - s1L[b]) + a2Coeff[b] * s2L[b] + s1L[b];
                s1L[b] = 2.0f * lpL - s1L[b];
                s2L[b] = 2.0f * bpL - s2L[b];
                maxBandL = std::max(maxBandL, std::abs(bpL));

                // Right channel
                const float bpR = a2Coeff[b] * (r - s1R[b]) + a1Coeff[b] * s2R[b];
                const float lpR = a3Coeff[b] * (r - s1R[b]) + a2Coeff[b] * s2R[b] + s1R[b];
                s1R[b] = 2.0f * lpR - s1R[b];
                s2R[b] = 2.0f * bpR - s2R[b];
                maxBandR = std::max(maxBandR, std::abs(bpR));
            }

            const float totalL = std::abs(l) + 1e-5f;
            const float totalR = std::abs(r) + 1e-5f;

            // Instantaneous harmonic crest factor: ratio of peak resonant band to wideband energy
            const float rawRatioL = std::clamp(maxBandL / totalL, 0.0f, 1.2f);
            const float rawRatioR = std::clamp(maxBandR / totalR, 0.0f, 1.2f);

            harmonicEnvL += alphaSmooth * (rawRatioL - harmonicEnvL);
            harmonicEnvR += alphaSmooth * (rawRatioR - harmonicEnvR);

            // Calculate harmonic gain with sensitivity threshold and focus curve
            const float thresh = (1.0f - sens) * 0.45f;
            float normL = (harmonicEnvL - thresh) / (1.0f - thresh + 1e-4f);
            float normR = (harmonicEnvR - thresh) / (1.0f - thresh + 1e-4f);
            normL = std::clamp(normL, 0.0f, 1.0f);
            normR = std::clamp(normR, 0.0f, 1.0f);

            // Shape with Hermite / Focus exponent
            const float p = 1.0f + (1.0f - focus) * 1.5f;
            const float gainHarmL = std::pow(normL * normL * (3.0f - 2.0f * normL), p);
            const float gainHarmR = std::pow(normR * normR * (3.0f - 2.0f * normR), p);

            const float gainNoiseL = 1.0f - gainHarmL;
            const float gainNoiseR = 1.0f - gainHarmR;

            if (harmonicL) harmonicL[i] = l * gainHarmL;
            if (harmonicR) harmonicR[i] = r * gainHarmR;
            if (noiseL) noiseL[i] = l * gainNoiseL;
            if (noiseR) noiseR[i] = r * gainNoiseR;
        }
    }

private:
    double currentSampleRate = 48000.0;
    int currentMaxBlock = 512;

    float sens = 0.5f;
    float focus = 0.5f;
    float alphaSmooth = 0.05f;

    std::array<float, 8> gCoeff;
    std::array<float, 8> kCoeff;
    std::array<float, 8> a1Coeff;
    std::array<float, 8> a2Coeff;
    std::array<float, 8> a3Coeff;

    std::array<float, 8> s1L;
    std::array<float, 8> s2L;
    std::array<float, 8> s1R;
    std::array<float, 8> s2R;

    float harmonicEnvL = 0.0f;
    float harmonicEnvR = 0.0f;
};
