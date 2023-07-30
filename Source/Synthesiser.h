/*
  ==============================================================================

    Synthesiser.h
    Created by tmudd on 07/07/2020.
    Modified by jeremybai on 09/12/2022.

  ==============================================================================
*/

#ifndef Synthesiser_h
#define Synthesiser_h

#include <JuceHeader.h>
#include "Oscillator.h"
#include "InputProcessor.h"
#include "FHNSolver.h"

class FHNSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int midiNoteNumber) override      { return true; }
    bool appliesToChannel (int midiChannel) override      { return true; }
};

/*!
 @class FHNSynthVoice
 @abstract A synth voice that creates sounds utilising FHN solver.
 
 @namespace none
 */
class FHNSynthVoice : public juce::SynthesiserVoice
{
public:
    /**
     Initialise voice and pass sample rate to oscillators and envelope
     
     @param sampleRate float type sample rate
     */
    FHNSynthVoice(float sampleRate)
      : lfo(new SinOsc(sampleRate)),
        leftInput(new InputProcessor(sampleRate)), rightInput(new InputProcessor(sampleRate)),
        oscType(0), modType(0),
        leftSolver(new FHNSolver(sampleRate)), rightSolver(new FHNSolver(sampleRate))
        
    {
        envelope.setSampleRate(sampleRate);
    }

    /**
     Update synth parameters from parameter tree's current state, called per buffer
    */
    void updateParameters(const juce::AudioProcessorValueTreeState& apvts)
    {
        // update main params
        directInput = *apvts.getRawParameterValue("directInput");
        oscAmp = *apvts.getRawParameterValue("oscAmp");
        noiseAmp = *apvts.getRawParameterValue("noiseAmp");
        
        modFreq = *apvts.getRawParameterValue("modFreq");
        modAmp = *apvts.getRawParameterValue("modAmp");
        
        pulseWidth = *apvts.getRawParameterValue("pulseWidth");
        
        // check input processor osc type change and update params
        float newOscType = *apvts.getRawParameterValue("oscType");
        float newModType = *apvts.getRawParameterValue("modType");
        
        if (oscType != newOscType)
        {
            oscType = newOscType;
            leftInput->resetOscType(oscType);
            rightInput->resetOscType(oscType);
        }
        if (modType != newModType)
        {
            modType = newModType;
            leftInput->resetModType(modType);
            rightInput->resetModType(modType);
        }
        
        leftInput->updateParam(oscAmp, modFreq, modAmp, noiseAmp, pulseWidth);
        rightInput->updateParam(oscAmp, modFreq, modAmp, noiseAmp, pulseWidth);
        
        lfoFreq = *apvts.getRawParameterValue("lfoFreq");
        lfoAmp = *apvts.getRawParameterValue("lfoAmp");
        lfo->setFrequency(lfoFreq);
        
        stereo = *apvts.getRawParameterValue("stereo");
        detune = *apvts.getRawParameterValue("deturn");
        coupling = *apvts.getRawParameterValue("coupling");
        
        // update filter
        cutoff = *apvts.getRawParameterValue("cutoff");
        resonance = *apvts.getRawParameterValue("resonance");
        strength = *apvts.getRawParameterValue("strength");
        
        filterType = *apvts.getRawParameterValue("filterType");
        switch (filterType)
        {
            case 0:
                coeff = juce::IIRCoefficients::makeLowPass(getSampleRate(), cutoff, resonance);
                leftFilter.setCoefficients(coeff);
                rightFilter.setCoefficients(coeff);
                break;
            case 1:
                coeff = juce::IIRCoefficients::makeHighPass(getSampleRate(), cutoff, resonance);
                leftFilter.setCoefficients(coeff);
                rightFilter.setCoefficients(coeff);
                break;
            case 2:
                coeff = juce::IIRCoefficients:makeBandPass(getSampleRate(), cutoff, resonance);
                leftFilter.setCoefficients(coeff);
                rightFilter.setCoefficients(coeff);
                break;
            default:
                leftFilter.makeInactive();
                rightFilter.makeInactive();
                break;
        }
        
        // update ADSR
        envelopeParam.attack = * apvts.getRawParameterValue("attack");
        envelopeParam.decay = * apvts.getRawParameterValue("decay");;
        envelopeParam.sustain = * apvts.getRawParameterValue("sustain");;
        envelopeParam.release = * apvts.getRawParameterValue("release");;
        envelope.setParameters(envelopeParam);
    }

    /**
     What should be done when a note starts

     @param midiNoteNumber
     @param velocity
     @param SynthesiserSound unused variable
     @param / unused variable
     */
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        playing = true;
        ending = false;

        noteFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        
        envelope.reset();
        envelope.noteOn();
    }
    
    /// Called when a MIDI noteOff message is received
    /**
     What should be done when a note stops

     @param / unused variable
     @param allowTailOff bool to decie if the should be any volume decay
     */
    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        envelope.noteOff();
        ending = true;
    }
    
    //--------------------------------------------------------------------------
    /**
     Main DSP Block
     
     If the sound that the voice is playing finishes during the course of this rendered block, it must call clearCurrentNote(), to tell the synthesiser that it has finished

     @param outputBuffer pointer to output
     @param startSample position of first sample in buffer
     @param numSamples number of smaples in output buffer
     */
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (playing) // check to see if this voice should be playing
        {
            // iterate through the necessary number of samples (from startSample up to startSample + numSamples)
            for (int sampleIndex = startSample;   sampleIndex < (numSamples);   sampleIndex++)
            {

                // get envelope sample
                float envelopeVal = envelope.getNextSample();

                lfo->setFrequency(lfoFreq);
                auto leftFrequency = noteFrequency * std::pow(2, lfo.processOscillator() * lfoAmp);
                auto rightFrequency = leftFrequency + detune;
                
                auto left = leftInput->processInput(directInput, leftFrequency);
                auto right = rightInput->processInput(directInput, rightFrequency);

                //auto k;
                
                leftSolver->setTemporalScale(k);
                rightSolver->setTemporalScale(k);
                
                auto currentLeft = leftSolver->getCurrentState();
                auto currentRight = rightSolver->getCurrentState();
                
                auto leftSample = leftSolver->processSystem(left + coupling * currentRight);
                auto rightSample = rightSolver->processSystem(right + coupling * currentLeft);
                
                float filteredSample = filter.processSingleSampleRaw(currentSample);
                float finalSample = currentSample * (1 - strength) + filteredSample * strength;
                
                // for each channel, write the currentSample float to the output
                for (int chan = 0; chan<outputBuffer.getNumChannels(); chan++)
                {
                    // The output sample is scaled by 0.2 so that it is not too loud by default
                    outputBuffer.addSample (chan, sampleIndex, finalSample * envelopeVal * ampMod * 0.2);
                }

                if (ending)
                {
                    if (envelopeVal < 0.0001f)
                    {
                        clearCurrentNote();
                        playing = false;

                        // reset oscillators to avoid clipping when starting next note
                        phasor.resetPhase();
                    }
                }
            }
        }
    }
    
    void pitchWheelMoved(int) override {}

    void controllerMoved(int, int) override {}
    
    /**
     Can this voice play a sound.

     @param sound a juce::SynthesiserSound* base class pointer
     @return sound cast as a pointer to an instance of FHNSynthSound
     */
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<FHNSynthSound*> (sound) != nullptr;
    }
    //--------------------------------------------------------------------------
private:
    //--------------------------------------------------------------------------
    // Set up any necessary variables here
    
    bool playing = false;
    bool ending = false;
    bool stereo = false;

    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParam;

    // main oscillators and modules
    SinOsc* lfo;
    InputProcessor* leftInput, rightInput;
    FHNSolver* leftSolver, rightSolver;


    // main params
    float directInput, oscAmp, noiseAmp, modFreq, modAmp, pulseWidth;
    float noteFrequency, detune, coupling, lfoFreq, lfoAmp;
    float oscType, modType;
    
    // IIR filter
    juce::IIRFilter leftFilter, rightFilter;
    juce::IIRCoefficients coeff;
    float cutoff, resonance, keytrack, strength, filterType;

};

#endif /* Synthesiser.h */
