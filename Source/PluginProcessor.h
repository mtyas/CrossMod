#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <clap-juce-extensions/clap-juce-extensions.h>
#include "Engine/MidiChainProcessor.h"
#include <mutex>

namespace MidiFlux
{

class MidiFluxAudioProcessor : public juce::AudioProcessor,
                               public clap_juce_extensions::clap_properties
{
public:
    MidiFluxAudioProcessor();
    ~MidiFluxAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>& audioBuffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    MidiChainProcessor& getChain() { return chain; }

    // Queue virtual keyboard MIDI messages from UI thread
    void injectVirtualMidi(const juce::MidiMessage& msg);

    void triggerPanic();

private:
    MidiChainProcessor chain;

    // Lock-free / mutex queue for UI virtual keyboard notes
    std::mutex virtualMidiMutex;
    juce::MidiBuffer virtualMidiQueue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFluxAudioProcessor)
};

} // namespace MidiFlux
