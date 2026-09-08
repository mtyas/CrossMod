#include "MapperBlock.h"
#include <cmath>

namespace MidiFlux
{

MapperBlock::MapperBlock()
{
}

void MapperBlock::prepare(double, int)
{
}

void MapperBlock::reset()
{
}

void MapperBlock::allNotesOff(juce::MidiBuffer& outBuffer)
{
    MidiBlock::allNotesOff(outBuffer);
}

int MapperBlock::getNumParameters() const
{
    return 7;
}

const ParameterDefinition& MapperBlock::getParameterDef(int index) const
{
    static const std::array<ParameterDefinition, 7> defs = {{
        { "velCurve",   "Vel Curve",  0.0f, 6.0f, 0.0f, 1.0f, "", { "Linear", "Compress", "Expand", "Logarithmic", "Exponential", "Invert", "Random" } },
        { "velOutMin",  "Out Min",    1.0f, 127.0f, 1.0f, 1.0f, "", {} },
        { "velOutMax",  "Out Max",    1.0f, 127.0f, 127.0f, 1.0f, "", {} },
        { "ccRemap",    "CC Remap",   0.0f, 1.0f, 0.0f, 1.0f, "", { "Off", "On" } },
        { "ccSrc",      "CC In",      0.0f, 127.0f, 1.0f, 1.0f, "", {} },
        { "ccDst",      "CC Out",     0.0f, 127.0f, 74.0f, 1.0f, "", {} },
        { "ccScale",    "CC Scale",   -1.0f, 1.0f, 1.0f, 0.01f, "%", {} }
    }};
    return defs[juce::jlimit(0, 6, index)];
}

float MapperBlock::getParameterValue(int index) const
{
    switch (index)
    {
        case 0: return velCurve;
        case 1: return velOutMin;
        case 2: return velOutMax;
        case 3: return ccRemapActive;
        case 4: return ccSrc;
        case 5: return ccDst;
        case 6: return ccScale;
        default: return 0.0f;
    }
}

void MapperBlock::setParameterValue(int index, float value)
{
    switch (index)
    {
        case 0: velCurve = juce::jlimit(0.0f, 6.0f, value); break;
        case 1: velOutMin = juce::jlimit(1.0f, 127.0f, value); if (velOutMin > velOutMax) velOutMax = velOutMin; break;
        case 2: velOutMax = juce::jlimit(1.0f, 127.0f, value); if (velOutMax < velOutMin) velOutMin = velOutMax; break;
        case 3: ccRemapActive = juce::jlimit(0.0f, 1.0f, value); break;
        case 4: ccSrc = juce::jlimit(0.0f, 127.0f, value); break;
        case 5: ccDst = juce::jlimit(0.0f, 127.0f, value); break;
        case 6: ccScale = juce::jlimit(-1.0f, 1.0f, value); break;
    }
}

void MapperBlock::processBlock(const juce::MidiBuffer& inputMidi,
                               juce::MidiBuffer& outputMidi,
                               const BlockContext&)
{
    if (isBypassed())
    {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    int curve = static_cast<int>(std::round(velCurve));
    float oMin = velOutMin;
    float oMax = velOutMax;
    bool doCC = ccRemapActive > 0.5f;
    int srcCC = static_cast<int>(std::round(ccSrc));
    int dstCC = static_cast<int>(std::round(ccDst));

    for (const auto metadata : inputMidi)
    {
        auto msg = metadata.getMessage();
        int samplePos = metadata.samplePosition;
        int ch = msg.getChannel();

        if (msg.isNoteOn())
        {
            float norm = (float)msg.getVelocity() / 127.0f; // 0..1
            float transformed = norm;

            switch (curve)
            {
                case 0: // Linear
                    transformed = norm;
                    break;
                case 1: // Compress (sqrt)
                    transformed = std::sqrt(norm);
                    break;
                case 2: // Expand (x^2)
                    transformed = norm * norm;
                    break;
                case 3: // Logarithmic
                    transformed = std::log(1.0f + 9.0f * norm) / std::log(10.0f);
                    break;
                case 4: // Exponential
                    transformed = (std::exp(2.0f * norm) - 1.0f) / (std::exp(2.0f) - 1.0f);
                    break;
                case 5: // Invert
                    transformed = 1.0f - norm;
                    break;
                case 6: // Random
                    transformed = rng.nextFloat();
                    break;
            }

            float finalVel = oMin + transformed * (oMax - oMin);
            uint8_t velByte = (uint8_t)juce::jlimit(1.0f, 127.0f, std::round(finalVel));

            outputMidi.addEvent(juce::MidiMessage::noteOn(ch, msg.getNoteNumber(), velByte), samplePos);
            triggerActivity();
        }
        else if (msg.isController() && doCC)
        {
            if (msg.getControllerNumber() == srcCC)
            {
                float normCC = (float)msg.getControllerValue() / 127.0f;
                float mapped = (ccScale >= 0.0f) ? (normCC * ccScale) : (1.0f + normCC * ccScale);
                uint8_t outVal = (uint8_t)juce::jlimit(0.0f, 127.0f, std::round(mapped * 127.0f));
                outputMidi.addEvent(juce::MidiMessage::controllerEvent(ch, dstCC, outVal), samplePos);
            }
            else
            {
                outputMidi.addEvent(msg, samplePos);
            }
        }
        else
        {
            outputMidi.addEvent(msg, samplePos);
        }
    }
}

juce::String MapperBlock::getStatusDescription() const
{
    static const char* curves[] = { "Linear", "Compress", "Expand", "Log", "Exp", "Invert", "Random" };
    int cIdx = juce::jlimit(0, 6, static_cast<int>(std::round(velCurve)));
    return "Vel Curve: " + juce::String(curves[cIdx]) + " (" + juce::String(static_cast<int>(velOutMin)) + "-" + juce::String(static_cast<int>(velOutMax)) + ")";
}

} // namespace MidiFlux
