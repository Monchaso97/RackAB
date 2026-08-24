#include "PluginProcessor.h"
#include "PluginEditor.h"

RackABProcessor::RackABProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void RackABProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    chain.prepare (sampleRate, samplesPerBlock);
}

void RackABProcessor::releaseResources()
{
    chain.releaseResources();
}

bool RackABProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void RackABProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int ns = buffer.getNumSamples();
    inLevel.store (juce::jmax (buffer.getMagnitude (0, 0, ns),
                               buffer.getMagnitude (juce::jmin (1, buffer.getNumChannels() - 1), 0, ns)));

    chain.process (buffer, midi);

    outLevel.store (juce::jmax (buffer.getMagnitude (0, 0, ns),
                                buffer.getMagnitude (juce::jmin (1, buffer.getNumChannels() - 1), 0, ns)));
}

juce::AudioProcessorEditor* RackABProcessor::createEditor()
{
    return new RackABEditor (*this);
}

void RackABProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    chain.getState (destData);
}

void RackABProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    chain.setState (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RackABProcessor();
}
