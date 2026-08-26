#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <atomic>
#include <vector>
#include <memory>
#include "RandomUtils.h"
#include "ScaleTheory.h"

namespace MidiFlux
{

struct BlockContext
{
    double sampleRate = 44100.0;
    double bpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    double ppqPosition = 0.0;
    bool isPlaying = false;
    int numSamples = 512;
    int rootKey = 0;   // 0 = C, 1 = C#, etc.
    int scaleType = 0; // ScaleType enum from ScaleTheory
};

struct ParameterDefinition
{
    juce::String id;
    juce::String name;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float step = 0.01f;
    juce::String suffix = "";
    std::vector<juce::String> options = {}; // If discrete choices
};

class MidiBlock
{
public:
    virtual ~MidiBlock() = default;

    virtual juce::String getTypeId() const = 0;
    virtual juce::String getDisplayName() const = 0;
    virtual juce::String getCategory() const = 0;
    virtual juce::Colour getAccentColor() const = 0;

    virtual void prepare(double sampleRate, int maxSamplesPerBlock) = 0;
    virtual void reset() = 0;
    virtual void allNotesOff(juce::MidiBuffer& outBuffer)
    {
        for (int ch = 1; ch <= 16; ++ch)
        {
            outBuffer.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
            outBuffer.addEvent(juce::MidiMessage::allSoundOff(ch), 0);
        }
    }

    virtual void processBlock(const juce::MidiBuffer& inputMidi,
                              juce::MidiBuffer& outputMidi,
                              const BlockContext& ctx) = 0;

    bool isBypassed() const { return bypassed.load(std::memory_order_relaxed); }
    void setBypassed(bool b) { bypassed.store(b, std::memory_order_relaxed); }

    bool isSoloed() const { return soloed.load(std::memory_order_relaxed); }
    void setSoloed(bool s) { soloed.store(s, std::memory_order_relaxed); }

    void triggerActivity()
    {
        activityCount.store(15, std::memory_order_relaxed);
    }

    bool hasActivityAndDecay()
    {
        int current = activityCount.load(std::memory_order_relaxed);
        if (current > 0)
        {
            activityCount.store(current - 1, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    // Parameter abstraction
    virtual int getNumParameters() const = 0;
    virtual const ParameterDefinition& getParameterDef(int index) const = 0;
    virtual float getParameterValue(int index) const = 0;
    virtual void setParameterValue(int index, float value) = 0;

    virtual juce::String getParameterText(int index) const
    {
        if (index < 0 || index >= getNumParameters())
            return "";

        const auto& def = getParameterDef(index);
        float val = getParameterValue(index);

        if (!def.options.empty())
        {
            int optIdx = juce::jlimit(0, static_cast<int>(def.options.size()) - 1, static_cast<int>(std::round(val)));
            return def.options[optIdx];
        }

        if (def.step >= 1.0f)
            return juce::String(static_cast<int>(std::round(val))) + def.suffix;

        return juce::String(val, 2) + def.suffix;
    }

    virtual void randomize()
    {
        FastRandom rng(static_cast<uint64_t>(juce::Time::getHighResolutionTicks()));
        for (int i = 0; i < getNumParameters(); ++i)
        {
            const auto& def = getParameterDef(i);
            if (!def.options.empty())
            {
                int opt = rng.nextInt(0, static_cast<int>(def.options.size()) - 1);
                setParameterValue(i, static_cast<float>(opt));
            }
            else
            {
                float r = rng.nextFloat(def.minValue, def.maxValue);
                if (def.step > 0.0f)
                    r = std::round((r - def.minValue) / def.step) * def.step + def.minValue;
                setParameterValue(i, r);
            }
        }
    }

    virtual juce::ValueTree getState() const
    {
        juce::ValueTree vt("Block");
        vt.setProperty("type", getTypeId(), nullptr);
        vt.setProperty("bypassed", isBypassed(), nullptr);

        for (int i = 0; i < getNumParameters(); ++i)
        {
            const auto& def = getParameterDef(i);
            vt.setProperty(def.id, getParameterValue(i), nullptr);
        }
        return vt;
    }

    virtual void setState(const juce::ValueTree& vt)
    {
        if (vt.hasProperty("bypassed"))
            setBypassed(static_cast<bool>(vt.getProperty("bypassed")));

        for (int i = 0; i < getNumParameters(); ++i)
        {
            const auto& def = getParameterDef(i);
            if (vt.hasProperty(def.id))
            {
                float val = static_cast<float>(vt.getProperty(def.id));
                setParameterValue(i, val);
            }
        }
    }

protected:
    std::atomic<bool> bypassed{ false };
    std::atomic<bool> soloed{ false };
    std::atomic<int> activityCount{ 0 };
};

} // namespace MidiFlux
