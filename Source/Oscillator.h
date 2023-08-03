/*
  ==============================================================================

    Oscillator.h
    Created by tmudd on 03/02/2020.
    Modified by jeremybai on 09/12/2022.
    Copyright Â© 2020 tmudd. All rights reserved.

  ==============================================================================
*/

#ifndef Oscillator_h
#define Oscillator_h

/**
 Base oscillator class
 
 outputs the phase directly in the range: 0-1
 */
class Phasor
{
public:
    
    /// empty destructor function
    virtual ~Phasor() {};
    
    // Our parent oscillator class does the key things required for most oscillators:
    // -- handles phase
    // -- handles setters and getters for frequency and samplerate
    
    // update the phase and output the next sample from the oscillator
    float processOscillator()
    {
        phase += phaseDelta;
        
        if (phase > 1.0f)
            phase -= 1.0f;
        
        return output(phase + phaseOffset);
    }

    // force reset phase to start next period
    void resetPhase()
    {
        phase = 0;
    }
    
    // this function is the one that we will replace in the classes that inherit from Phasor
    virtual float output(float p)
    {
        return p;
    }
    
    /**
     set the sample rate in Hz (e.g. 44100). Must be set BEFORE calling setFrequency()!!!
     
     @param sr sample rate in Hz
     */
    void setSampleRate(float sr)
    {
        sampleRate = sr;
    }
    
    /**
     sets the oscillator frequency in Hz (e.g. 440). Must be set AFTER calling setSampleRate()!!!
     
     @param freq oscillator frequency in Hz
     */
    void setFrequency(float freq)
    {
        frequency = freq;
        phaseDelta = frequency / sampleRate;
    }
    
    /// for phase modulation:
    void setPhaseOffset(float _phaseOffset)
    {
        phaseOffset = _phaseOffset;
    }
    
private:
    float frequency;
    float sampleRate;
    float phase = 0.0f;
    float phaseDelta;
    
    float phaseOffset = 0.0;        // for phase modulation
};

/**
 Sine Oscillator built on Phasor base class
 */
class SinOsc : public Phasor
{
    float output(float p) override  { return std::sin(p * 2.0 * 3.1415926); }
};

/**
 Squarewave Oscillator built on Phasor base class
 
 Includes setPulseWidth to change the waveform shape
 */
class SquareOsc : public Phasor
{
public:
    float output(float p) override  { return (p > pulseWidth) ? -1.0f : 1.0f; }
    
    /// set square wave pulse width (0-1))
    void setPulseWidth(float pw)    { pulseWidth = pw; }
private:
    float pulseWidth = 0.5f;
};


/**
 Sawtooth Oscillator built on Phasor base class
 */
class SawToothOsc : public Phasor
{
public:
    float output(float p) override  { return p * 2.0f - 1.0f; }
};

#endif /* Oscillator.h */
