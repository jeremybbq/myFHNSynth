/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Synthesiser.h"

//==============================================================================
MyFHNSynthAudioProcessor::MyFHNSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    parameterTree(*this, nullptr, "parameterTreeID",
    {
        std::make_unique<juce::AudioParameterFloat>("directInput", "Direct Input", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("noiseAmp", "Noise Input", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("oscAmp", "Oscillator Amplitude", 0.0f, 1.0f, 0.5f),
    
        std::make_unique<juce::AudioParameterFloat>("modFreq", "Modulator Frequency", 0.0f, 0.5f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("modAmp", "Modulator Amplitude", 0.0f, 1.0f, 0.0f),
        
        std::make_unique<juce::AudioParameterFloat>("pulseWidth", "Pulse Width", 0.0f, 0.8f, 0.0f),
        
        std::make_unique<juce::AudioParameterChoice>("mainType", "Oscillator Type", juce::StringArray{"Sine", "Square", "Sawtooth"}, 0),
        std::make_unique<juce::AudioParameterChoice>("modType", "Modulator Type", juce::StringArray{"Sine", "Square"}, 0),
    
        std::make_unique<juce::AudioParameterFloat>("lfoFreq", "LFO Frequency", 0.0f, 20.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("lfoAmp", "LFO Amplitude", 0.0f, 1.0f, 0.0f),
        
        std::make_unique<juce::AudioParameterBool>("stereo", "Stereo", false),
        std::make_unique<juce::AudioParameterFloat>("detune", "Detune", 0.0f, 20.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("coupling", "Coupling", 0.0f, 1.0f, 0.0f),
    
        std::make_unique<juce::AudioParameterFloat>("cutoff", "Cutoff", 0.0f, 20000.0f, 20000.0f),
        std::make_unique<juce::AudioParameterFloat>("resonance", "Resonance", 0.0f, 20000.0f, 20000.0f),
        std::make_unique<juce::AudioParameterFloat>("strength", "Strength", 0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterChoice>("filterType", "Filter Type", juce::StringArray{"Low-Pass","High-Pass","Band-Pass"}, 0),
    
        std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.0f, 1.0f, 0.1f),
        std::make_unique<juce::AudioParameterFloat>("decay", "Decay", 0.0f, 1.0f, 0.1f),
        std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 1.0f),
        std::make_unique<juce::AudioParameterFloat>("release", "Release", 0.0f, 1.0f, 0.1f),
        
        std::make_unique<juce::AudioParameterFloat>("amp", "Overall Amp", 0.0f, 1.0f, 1.0f),
    })
#endif
{
    fhnSynth.addSound(new FHNSynthSound());
}

MyFHNSynthAudioProcessor::~MyFHNSynthAudioProcessor()
{
}

//==============================================================================
const juce::String MyFHNSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MyFHNSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MyFHNSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MyFHNSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MyFHNSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MyFHNSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MyFHNSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MyFHNSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MyFHNSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void MyFHNSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MyFHNSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    fhnSynth.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < voiceCount; i++)
    {
        fhnSynth.addVoice(new FHNSynthVoice(sampleRate));
    }
}

void MyFHNSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MyFHNSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MyFHNSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int i = 0; i < voiceCount; i++)
    {
        FHNSynthVoice* voice = dynamic_cast<FHNSynthVoice*>(fhnSynth.getVoice(i));
        voice->updateParameters(parameterTree);
    }
    
    fhnSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool MyFHNSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MyFHNSynthAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void MyFHNSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = parameterTree.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MyFHNSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameterTree.state.getType()))
        {
            parameterTree.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyFHNSynthAudioProcessor();
}
