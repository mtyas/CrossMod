#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Engine/PresetManager.h"

namespace MidiFlux
{

MidiFluxAudioProcessor::MidiFluxAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Load default factory preset: Generative Ambient Arp
    PresetManager::applyPreset(chain, 0);
}

MidiFluxAudioProcessor::~MidiFluxAudioProcessor()
{
}

const juce::String MidiFluxAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidiFluxAudioProcessor::acceptsMidi() const { return true; }
bool MidiFluxAudioProcessor::producesMidi() const { return true; }
bool MidiFluxAudioProcessor::isMidiEffect() const { return true; }
double MidiFluxAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MidiFluxAudioProcessor::getNumPrograms() { return 1; }
int MidiFluxAudioProcessor::getCurrentProgram() { return 0; }
void MidiFluxAudioProcessor::setCurrentProgram(int) {}
const juce::String MidiFluxAudioProcessor::getProgramName(int) { return {}; }
void MidiFluxAudioProcessor::changeProgramName(int, const juce::String&) {}

void MidiFluxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    chain.prepare(sampleRate, samplesPerBlock);
}

void MidiFluxAudioProcessor::releaseResources()
{
    chain.reset();
}

bool MidiFluxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Pass-through audio or disabled audio is fine
    return true;
}

void MidiFluxAudioProcessor::injectVirtualMidi(const juce::MidiMessage& msg)
{
    std::lock_guard<std::mutex> lock(virtualMidiMutex);
    virtualMidiQueue.addEvent(msg, 0);
}

void MidiFluxAudioProcessor::triggerPanic()
{
    std::lock_guard<std::mutex> lock(virtualMidiMutex);
    for (int ch = 1; ch <= 16; ++ch)
    {
        virtualMidiQueue.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
        virtualMidiQueue.addEvent(juce::MidiMessage::allSoundOff(ch), 0);
    }
}

void MidiFluxAudioProcessor::processBlock(juce::AudioBuffer<float>& audioBuffer, juce::MidiBuffer& midiMessages)
{
    // Clear audio outputs (it's a pure MIDI processor, pass through or silence audio)
    audioBuffer.clear();

    // Pull any virtual keyboard messages
    {
        std::lock_guard<std::mutex> lock(virtualMidiMutex);
        if (!virtualMidiQueue.isEmpty())
        {
            midiMessages.addEvents(virtualMidiQueue, 0, -1, 0);
            virtualMidiQueue.clear();
        }
    }

    // Extract playhead timing context
    BlockContext ctx;
    ctx.sampleRate = getSampleRate();
    ctx.numSamples = audioBuffer.getNumSamples();
    ctx.bpm = 120.0;
    ctx.isPlaying = false;
    ctx.ppqPosition = 0.0;

    if (auto* playHead = getPlayHead())
    {
        auto positionInfo = playHead->getPosition();
        if (positionInfo.hasValue())
        {
            if (positionInfo->getBpm().hasValue())
                ctx.bpm = *positionInfo->getBpm();

            if (positionInfo->getTimeSignature().hasValue())
            {
                ctx.timeSigNumerator = positionInfo->getTimeSignature()->numerator;
                ctx.timeSigDenominator = positionInfo->getTimeSignature()->denominator;
            }

            if (positionInfo->getPpqPosition().hasValue())
                ctx.ppqPosition = *positionInfo->getPpqPosition();

            ctx.isPlaying = positionInfo->getIsPlaying();
        }
    }

    chain.processMidi(midiMessages, ctx);
}

bool MidiFluxAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MidiFluxAudioProcessor::createEditor()
{
    return new MidiFluxAudioProcessorEditor(*this);
}

void MidiFluxAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto vt = chain.getState();
    std::unique_ptr<juce::XmlElement> xml(vt.createXml());
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void MidiFluxAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        juce::ValueTree vt = juce::ValueTree::fromXml(*xmlState);
        if (vt.isValid())
        {
            chain.setState(vt);
        }
    }
}

} // namespace MidiFlux

// JUCE Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiFlux::MidiFluxAudioProcessor();
}
