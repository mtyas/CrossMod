#pragma once

#include <cmath>
#include <algorithm>

class Envelope
{
public:
    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    Envelope() = default;

    void setSampleRate(double newSampleRate)
    {
        sampleRate = std::max(1.0, newSampleRate);
        updateCoefficients();
    }

    void setParameters(float attackSec, float decaySec, float sustainLvl, float releaseSec)
    {
        attackTime = std::max(0.001f, attackSec);
        decayTime = std::max(0.001f, decaySec);
        sustainLevel = std::clamp(sustainLvl, 0.0f, 1.0f);
        releaseTime = std::max(0.001f, releaseSec);
        updateCoefficients();
    }

    void noteOn(float velocity = 1.0f)
    {
        noteVelocity = std::clamp(velocity, 0.0f, 1.0f);
        state = State::Attack;
        // Don't reset currentLevel to 0 to prevent clicks on fast retriggering
    }

    void noteOff()
    {
        if (state != State::Idle)
            state = State::Release;
    }

    void reset()
    {
        state = State::Idle;
        currentLevel = 0.0f;
    }

    bool isActive() const
    {
        return state != State::Idle;
    }

    State getState() const { return state; }
    float getVelocity() const { return noteVelocity; }
    float getCurrentLevel() const { return currentLevel; }

    inline float getNextSample()
    {
        switch (state)
        {
            case State::Idle:
                currentLevel = 0.0f;
                break;

            case State::Attack:
            {
                // Smooth convex attack curve
                currentLevel += attackRate * (1.1f - currentLevel);
                if (currentLevel >= 1.0f)
                {
                    currentLevel = 1.0f;
                    state = State::Decay;
                }
                break;
            }

            case State::Decay:
            {
                // Exponential decay towards sustain level
                currentLevel += decayRate * (sustainLevel - currentLevel);
                if (std::abs(currentLevel - sustainLevel) < 0.0005f)
                {
                    currentLevel = sustainLevel;
                    state = State::Sustain;
                }
                break;
            }

            case State::Sustain:
            {
                currentLevel = sustainLevel;
                break;
            }

            case State::Release:
            {
                // Exponential release towards 0.0
                currentLevel += releaseRate * (0.0f - currentLevel);
                if (currentLevel < 0.0001f)
                {
                    currentLevel = 0.0f;
                    state = State::Idle;
                }
                break;
            }
        }

        return currentLevel;
    }

private:
    void updateCoefficients()
    {
        // Calculate filter coefficients for smooth 60dB/90dB decay curves
        attackRate = 1.0f - std::exp(-1.0f / (attackTime * (float)sampleRate * 0.35f));
        decayRate = 1.0f - std::exp(-1.0f / (decayTime * (float)sampleRate * 0.3f));
        releaseRate = 1.0f - std::exp(-1.0f / (releaseTime * (float)sampleRate * 0.25f));
    }

    double sampleRate = 44100.0;
    float attackTime = 0.01f;
    float decayTime = 0.3f;
    float sustainLevel = 0.8f;
    float releaseTime = 0.2f;

    float attackRate = 0.01f;
    float decayRate = 0.01f;
    float releaseRate = 0.01f;

    float currentLevel = 0.0f;
    float noteVelocity = 1.0f;
    State state = State::Idle;
};
