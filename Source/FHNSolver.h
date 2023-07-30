/*
  ==============================================================================

    FHNSolver.h
    Created: 17 Jul 2023 12:09:43am
    Author:  Jeremy Bai

  ==============================================================================
*/

#ifndef FHN_Solver_h
#define FHN_Solver_h

class FhnSolver
{
public:
    
    FhnSolver(float samplerate) : dt(1.0f/samplerate) {}
    ~FhnSolver() {}
    
    struct State
    {
        float v;
        float w;
        
        State() : v(0.0f), w(0.0f) {}
    };
    
    struct Delta
    {
        float dv;
        float dw;
        
        Delta() : dv(0.0f), dw(0.0f) {}
        Delta(float p, float q) : dv(p), dw(q) {}
        
        Delta operator/(const float x) const
        {
            return Delta(dv/x, dw/x);
        }
        
        Delta operator+(const Delta& d) const
        {
            return Delta(dv + d.dv, dw + d.dw);
        }
    };
    
    void setCurrentState(float newv, float neww)
    {
        currentState.v = newv;
        currentState.w = neww;
    }
    
    State updateCurrentState(Delta delta)
    {
        State newState;
        newState.v = currentState.v + delta.dv;
        newState.w = currentState.w + delta.dw;
        return newState;
    }
    
    void setParameter(float newa, float newb, float newc)
    {
        a = newa;
        b = newb;
        c = newc;
    }
    
    void setTemporalScale(float newk)
    {
        k = newk;
    }
    
    void setDt(float newdt)
    {
        dt = newdt;
    }
    
    Delta dy(State state)
    {
        Delta newDelta;
        newDelta.dv = (state.v - 25.0f/12.0f * std::pow(state.v, 3) - 0.4f * state.w + 0.4f * currentInput) * dt * k;
        newDelta.dw = (2.5f * state.v + a - b * state.w) * c * dt * k;
        return newDelta;
    }
    
    float processSystem(float input)
    {
        currentInput = input;
        k1 = dy(currentState);
        k2 = dy(updateCurrentState(k1/2));
        k3 = dy(updateCurrentState(k2/2));
        k4 = dy(updateCurrentState(k3));
        currentState = updateCurrentState((k1 + k2/2 + k3/2 + k4)/6);
        return getCurrentState();
    }
    
    float getCurrentState()
    {
        return currentState.v;
    }

private:
    State currentState;
    Delta k1, k2, k3, k4;
    float currentInput = 0.0f;
    float dt;
    float a = 0.7f, b = 0.8f, c = 0.1f, k = 1.0f;
    
};

#endif /* FHNSolver.h */

