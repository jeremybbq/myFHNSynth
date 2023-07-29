/*
  ==============================================================================

    Synthesiser.h
    Created by tmudd on 07/07/2020.
    Modified by jeremybai on 09/12/2022.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Oscillator.h"
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
    {
        sinOsc.setSampleRate(sampleRate);
        squareOsc.setSampleRate(sampleRate);
        sawtoothOsc.setSampleRate(sampleRate);
        
        sinMod.setSampleRate(sampleRate);
        squareMod.setSampleRate(sampleRate);
        
        envelope.setSampleRate(sampleRate);
        envelope.setParameters(envelopeParam);
    }

    /**
     Set mixing parameters of sinewave, sawwave and noise.
    */
    void setMixParameters(std::atomic<float>* _sinMix,
        std::atomic<float>* _sawMix,
        std::atomic<float>* _noiseMix,
        std::atomic<float>* _subOscMix)
    {
        sinMix = *_sinMix;
        sawMix = *_sawMix;
        noiseMix = *_noiseMix;
        subOscMix = *_subOscMix;
    }

    /**
     Set the ADSR for envelope.
    */
    void setEnvelopeParameters(std::atomic<float>* a,
        std::atomic<float>* d,
        std::atomic<float>* s,
        std::atomic<float>* r)
    {
        envelopeParam.attack = *a;
        envelopeParam.decay = *d;
        envelopeParam.sustain = *s;
        envelopeParam.release = *r;

        envelope.setParameters(envelopeParam);
    }

    /**
     Set pulse width for main and sub pulse oscillators.
    */
    void setPulseWidth(std::atomic<float>* pw)
    {
        squareWave.setPulseWidth(*pw);
    }

    /**
     Set detune on frequencies of main oscillators on both left and right channels.
    */
    void setDetune(std::atomic<float>* detune)
    {
        sawDetune = *detune;
    }

    /**
     Deviate oscillator frequency from note frequency and allow for hard sync.
    */
    void setHardSync(std::atomic<float>* hs)
    {
        hardSync = *hs;
    }

    /**
     Set sub oscillator type.
    */
    void setSubOscType(std::atomic<float>* sub)
    {
        subOscType = *sub;
    }

    /*
     Set IIR filter options and coefficients.
    */
    void setIIRFilter(std::atomic<float>* _cutoff,
        std::atomic<float>* _resonance,
        std::atomic<float>* _keytrack,
        std::atomic<float>* _strength,
        std::atomic<float>* _filterType)
    {
        cutoff = *_cutoff + *_keytrack * noteFrequency;
        resonance = *_resonance;
        filterType = *_filterType;
        strength = *_strength;

        if (filterType > 1)
        {
            coeff = juce::IIRCoefficients::makeBandPass(getSampleRate(), cutoff);
        }
        else if (filterType < 1)
        {
            coeff = juce::IIRCoefficients::makeLowPass(getSampleRate(), cutoff);
        }
        else
        {
            coeff = juce::IIRCoefficients::makeHighPass(getSampleRate(), cutoff);
        }
        
        filter.setCoefficients(coeff);
    }

    /**
     Set LFO parameters.
    */
    void setLFO(std::atomic<float>* _switch,
        std::atomic<float>* _type,
        std::atomic<float>* _target,
        std::atomic<float>* _freq,
        std::atomic<float>* _scale)
    {
        lfoSwitch = *_switch;
        lfoType = *_type;
        lfoTarget = *_target;
        lfoFrequency = pow(10, *_freq);
        lfoAmplitude = *_scale;
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
            for (int sampleIndex = startSample;   sampleIndex < (startSample+numSamples);   sampleIndex++)
            {

                // get envelope sample
                float envelopeVal = envelope.getNextSample();

                //=========================================================================================

                // set up lfo and modifying parameters
                if (lfoSwitch)
                {
                    // get lfo sample depending on lfo type
                    if (lfoType < 1)
                    {
                        sinlfo.setFrequency(lfoFrequency);
                        lfoSample = sinlfo.process();
                    }
                    else if (lfoType > 1)
                    {
                        sawlfo.setFrequency(lfoFrequency);
                        lfoSample = sawlfo.process();
                    }
                    else
                    {
                        sqrlfo.setFrequency(lfoFrequency);
                        lfoSample = sqrlfo.process();
                    }

                    // compute modifying parameters from lfo sample
                    if (lfoTarget < 1)
                    {
                        pitchMod = pow(2, lfoSample * lfoAmplitude);
                        ampMod = 1;
                        hardSyncMod = 1;
                    }
                    else if (lfoTarget > 1)
                    {
                        hardSyncMod = lfoSample * lfoAmplitude + 1;
                        pitchMod = 1;
                        ampMod = 1;
                    }
                    else
                    {
                        ampMod = lfoSample * lfoAmplitude + 1;
                        pitchMod = 1;
                        hardSyncMod = 1;
                    }
                }
                else
                {
                    // make sure parameters resets to default when lfo off
                    pitchMod = 1;
                    ampMod = 1;
                    hardSyncMod = 1;
                }

                // get main oscillators samples
                oscFrequency = noteFrequency * std::pow(2, hardSync * hardSyncMod);
                sqrwave.setFrequency(oscFrequency * pitchMod);
                sawwave.setFrequency(oscFrequency * pitchMod);
                sawwave2.setFrequency(oscFrequency * pitchMod * (1 + sawDetune));

                float pulseSample = sqrwave.process() * sinMix;
                float sawSample = (sawwave.process() + sawwave2.process()) / 2 * sawMix;
                float noiseSample = noise.nextFloat() * noiseMix;
                
                // sub oscillator depends on type
                float subSample = 0;
                if (subOscType < 1)
                {
                    subSinewave.setFrequency(noteFrequency * pitchMod / 2);
                    subSample = subSinewave.process();
                }
                else if (subOscType > 1)
                {
                    subSawwave.setFrequency(noteFrequency * pitchMod / 2);
                    subSample = subSawwave.process();
                }
                else
                {
                    subSqrwave.setFrequency(noteFrequency * pitchMod / 2);
                    subSample = subSqrwave.process();
                }
                subSample = subSample * subOscMix;
                
                //=========================================================================================
                
                float currentSample = pulseSample + sawSample + noiseSample + subSample;
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
                        phasor2.resetPhase();
                        sqrwave.resetPhase();
                        sawwave.resetPhase();
                        sawwave2.resetPhase();
                        subSqrwave.resetPhase();
                        subSawwave.resetPhase();
                        subSinewave.resetPhase();
                        sqrlfo.resetPhase();
                        sawlfo.resetPhase();
                        sinlfo.resetPhase();
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

    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParam;

    // main oscillators and lfos
    juce::Random noise;
    SinOsc sinOsc, sinMod;
    SquareOsc squareOsc, squareMod;
    SawToothOsc sawtoothOsc;

    // main params
    float directInput, modAmp, noiseAmp;
    float noteFrequency, detune;
    float oscType, modType;
    
    // IIR filter
    juce::IIRFilter filter;
    juce::IIRCoefficients coeff;
    float cutoff, resonance, strength, filterType;
    
    // lfo params and mod params
    bool lfoSwitch = false;
    float lfoType, lfoTarget, lfoFrequency, lfoAmplitude, lfoSample;
    float pitchMod, ampMod, hardSyncMod;

};
