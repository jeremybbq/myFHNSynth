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
    InputProcessor(float _sampleRate)
        : mainOsc(std::make_unique<SinOsc>()), 
        modOsc(std::make_unique<SinOsc>())
    {
        sampleRate = _sampleRate;
        mainOsc->setSampleRate(sampleRate);
        modOsc->setSampleRate(sampleRate);
    }
    
    void resetMainType(float mainType)
    {
        if (!mainType)
            mainOsc = std::make_unique<SinOsc>();
        else if (mainType == 1)
            mainOsc = std::make_unique<SquareOsc>();
        else if (mainType == 2)
            mainOsc = std::make_unique<SawToothOsc>();
        mainOsc->setSampleRate(sampleRate);
    }
    
    void resetModType(float modType)
    {
        if (!modType)
            modOsc = std::make_unique<SinOsc>();
        else
            modOsc = std::make_unique<SquareOsc>();
        modOsc->setSampleRate(sampleRate);
    }
    
    void updateParam(float newMainAmp, float newModFreq, float newModAmp, float newNoiseAmp, float pw)
    {
        modFreq = newModFreq;
        mainAmp = newMainAmp;
        modAmp = newModAmp;
        noiseAmp = newNoiseAmp;
        updatePulseWidth(pw);
    }
    
    void updatePulseWidth(float pw)
    {
        if (SquareOsc* mainSquare = dynamic_cast<SquareOsc*>(mainOsc.get()))
            mainSquare->setPulseWidth(pw);
        if (SquareOsc* modSquare = dynamic_cast<SquareOsc*>(modOsc.get()))
            modSquare->setPulseWidth(pw);
    }
    
    void resetPhase()
    {
        mainOsc->resetPhase();
        modOsc->resetPhase();
    }
    
    float processInput(float directInput, float frequency)
    {
        modOsc->setFrequency(frequency * (std::pow(2, modFreq) - 1));
        mainOsc->setFrequency(frequency);
        
        auto phaseOffset = modOsc->processOscillator() * modAmp;
        mainOsc->setPhaseOffset(phaseOffset);
        
        auto oscInput = mainOsc->processOscillator() * mainAmp;
        auto noiseInput = (noise.nextFloat() - 0.5f) * noiseAmp * 2;
        return directInput + oscInput + noiseInput;
    }
    
    
private:
    juce::Random noise;
    std::unique_ptr<Phasor> mainOsc; // std::unique_ptr
    std::unique_ptr<Phasor> modOsc;
    
    float sampleRate;
    float mainAmp, modFreq, modAmp, noiseAmp;
    
};

#endif /* InputProcessor.h */
