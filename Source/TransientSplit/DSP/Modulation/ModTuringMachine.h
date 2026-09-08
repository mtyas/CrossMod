#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <random>

class ModTuringMachine
{
public:
    ModTuringMachine()
    {
        shiftRegister = 0b1010011011001010;
    }

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        reset();
    }

    void reset()
    {
        clockPhase = 0.0f;
        currentVoltage = 0.0f;
        targetVoltage = 0.0f;
        currentStep = 0;
        lastPpqStep = -1;
    }

    void setBpm(double bpm)
    {
        currentBpm = (bpm > 20.0) ? bpm : 120.0;
    }

    void setParameters(float mutationParam, int lengthIndex, bool isSynced, int syncSubdiv, float rateParam, float glideParam)
    {
        mutationProb = std::clamp(mutationParam, 0.0f, 1.0f);
        
        switch (lengthIndex)
        {
            case 0: loopLength = 4; break;
            case 1: loopLength = 8; break;
            case 2: loopLength = 16; break;
            case 3: loopLength = 32; break;
            default: loopLength = 16; break;
        }

        syncEnabled = isSynced;
        syncIndex = syncSubdiv;
        glideAmount = std::clamp(glideParam, 0.0f, 0.99f);

        if (syncEnabled)
        {
            const double beatsPerSec = currentBpm / 60.0;
            double beats = 0.5;
            switch (syncIndex)
            {
                case 0: beats = 0.125; break; // 1/32
                case 1: beats = 0.25;  break; // 1/16
                case 2: beats = 0.5;   break; // 1/8
                case 3: beats = 0.75;  break; // 1/8d
                case 4: beats = 1.0;   break; // 1/4
                case 5: beats = 2.0;   break; // 1/2
                default: beats = 0.5;  break;
            }
            stepBeats = beats;
            clockFreqHz = static_cast<float>(beatsPerSec / beats);
        }
        else
        {
            // Free rate: 0.2 Hz to 50.0 Hz
            clockFreqHz = 0.2f * std::pow(250.0f, std::clamp(rateParam, 0.0f, 1.0f));
        }

        phaseInc = static_cast<float>(clockFreqHz / currentSampleRate);
    }

    void syncToHostPosition(double ppqPosition, bool isPlaying)
    {
        if (syncEnabled && isPlaying && ppqPosition >= 0.0)
        {
            const int64_t totalStep = static_cast<int64_t>(std::floor(ppqPosition / stepBeats));
            if (totalStep != lastPpqStep)
            {
                if (lastPpqStep >= 0)
                {
                    const int64_t numStepsToAdvance = std::min<int64_t>(totalStep - lastPpqStep, 16);
                    for (int64_t s = 0; s < numStepsToAdvance; ++s)
                        stepClock();
                }
                else
                {
                    stepClock();
                }
                lastPpqStep = totalStep;
            }

            const double stepPos = std::fmod(ppqPosition, stepBeats) / stepBeats;
            clockPhase = static_cast<float>(stepPos);
            currentStep = static_cast<int>(totalStep % loopLength);
            if (currentStep < 0) currentStep += loopLength;
        }
    }

    float getNextSample()
    {
        clockPhase += phaseInc;
        if (clockPhase >= 1.0f)
        {
            clockPhase -= 1.0f;
            currentStep = (currentStep + 1) % loopLength;
            stepClock();
        }

        // Apply glide / slew
        currentVoltage += (1.0f - glideAmount) * (targetVoltage - currentVoltage);
        return currentVoltage;
    }

    uint32_t getShiftRegister() const noexcept { return shiftRegister; }
    int getLoopLength() const noexcept { return loopLength; }
    int getCurrentStep() const noexcept { return currentStep; }
    float getCurrentVoltage() const noexcept { return currentVoltage; }
    float getTargetVoltage() const noexcept { return targetVoltage; }

private:
    void stepClock()
    {
        // Get the oldest bit at position (loopLength - 1)
        const uint32_t oldestBit = (shiftRegister >> (loopLength - 1)) & 1;

        // Invert oldest bit (standard Turing Machine behavior) or decide on mutation
        uint32_t newBit = oldestBit ^ 1;

        // Probabilistic mutation / random bit flip
        const float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        if (r < mutationProb)
        {
            newBit = (std::rand() % 2 == 0) ? 1 : 0;
        }

        // Shift register left by 1 and put newBit at LSB
        shiftRegister = ((shiftRegister << 1) | newBit) & 0xFFFFFFFF;

        // Convert the lowest 8 bits of shift register to a scaled voltage [-1.0, 1.0]
        const uint32_t dacValue = shiftRegister & 0xFF;
        targetVoltage = (static_cast<float>(dacValue) / 255.0f) * 2.0f - 1.0f;
    }

    double currentSampleRate = 48000.0;
    double currentBpm = 120.0;

    uint32_t shiftRegister = 0b1010011011001010;
    int loopLength = 16;
    int currentStep = 0;
    float mutationProb = 0.0f;

    bool syncEnabled = true;
    int syncIndex = 2; // 1/8
    double stepBeats = 0.5;
    float clockFreqHz = 4.0f;
    float clockPhase = 0.0f;
    float phaseInc = 0.0f;
    int64_t lastPpqStep = -1;

    float glideAmount = 0.1f;
    float targetVoltage = 0.0f;
    float currentVoltage = 0.0f;
};
