/*
  ==============================================================================

    InputProcessor.h
    Created: 26 Jul 2023 12:09:43am
    Author:  Jeremy Bai

  ==============================================================================
*/

#ifndef Input_Processor_h
#define Input_Processor_h

#include <JuceHeader.h>
#include "Oscillator.h"

class InputProcessor
{
    
public:
    InputProcessor(float sampleRate)
        : mainOsc(new SinOsc), modOsc(new SinOsc)
    {
        mainOsc->setSampleRate(sampleRate);
        modOsc->setSampleRate(sampleRate);
    }
    
    void resetMainType(float mainType)
    {
        delete mainOsc;
        switch (static_cast<int>(mainType))
        {
            case 0:
                mainOsc = new SinOsc;
                break;
            case 1:
                mainOsc = new SquareOsc;
                break;
            case 2:
                mainOsc = new SawToothOsc;
                break;
            default:
                mainOsc = nullptr;
                break;
        }
    }
    
    void resetModType(float modType)
    {
        delete modOsc;
        if (!modType)
            modOsc = new SinOsc;
        else
            modOsc = new SquareOsc;
    }
    
    void updateParam(float newMainAmp, float newModFreq, float newModAmp, float newNoiseAmp, float pw)
    {
        modOsc->setFrequency(newModFreq);
        mainAmp = newMainAmp;
        modAmp = newModAmp;
        noiseAmp = newNoiseAmp;
        updatePulseWidth(pw);
    }
    
    void updatePulseWidth(float pw)
    {
        if (SquareOsc* mainSquare = dynamic_cast<SquareOsc*>(mainOsc))
            mainSquare->setPulseWidth(pw);
        if (SquareOsc* modSquare = dynamic_cast<SquareOsc*>(modOsc))
            modSquare->setPulseWidth(pw);
    }
    
    void resetPhase()
    {
        mainOsc->resetPhase();
        modOsc->resetPhase();
    }
    
    float processInput(float directInput, float frequency)
    {
        mainOsc->setFrequency(frequency);
        
        auto phaseOffset = modOsc->processOscillator() * modAmp;
        mainOsc->setPhaseOffset(phaseOffset);
        
        auto oscInput = mainOsc->processOscillator() * mainAmp;
        auto noiseInput = (noise.nextFloat() - 0.5f) * noiseAmp * 2;
        return directInput + oscInput + noiseInput;
    }
    
    
private:
    juce::Random noise;
    Phasor* mainOsc;
    Phasor* modOsc;
    
    float mainAmp, modAmp, noiseAmp;
    
};

#endif /* InputProcessor.h */
