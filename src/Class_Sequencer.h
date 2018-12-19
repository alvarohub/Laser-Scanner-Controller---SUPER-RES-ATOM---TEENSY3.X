
#ifndef _Class_Sequencer_H_
#define _Class_Sequencer_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"

class Trigger
{

  public:
    enum TriggerMode
    {
        TRIG_RISE,
        TRIG_FALL,
        TRIG_CHANGE
    };

    Trigger() {}
    Trigger(bool _startState) : state(_startState){};


    void setTriggerID(uint8_t _triggerID) { triggerID = _triggerID; }
    uint8_t getTriggerID() { return (triggerID); }

    
    bool getTriggerState() { return (triggerState); }
    // Forcing the current trigger state to a particular value may be useful:
    void setTriggerState(bool _triggerState) { triggerState = _triggerState; }

    void setTriggerMode(TriggerMode _triggerMode) { triggerMode = _triggerMode; }

    // I will use pulling in the main loop to check and update the trigger state from the selected input (pin or another boolean),
    // but in the future we could do it using an external interrupt (with a lower priority with respect to the scanner display
    // periodic soft interruption).
    auto update(bool _newInput)
    {

        switch (triggerMode)
        {
        case TRIG_RISE:
            triggerState = (!oldInput) && _newInput;
            break;
        case TRIG_FALL:
            triggerState = oldInput && (!_newInput);
            break;
        case TRIG_CHANGE:
            triggerState = (oldInput != _newInput);
            break;
        default:
            break;
        }

        oldInput = _newInput;

        return (triggerState);
    }

  private:
    TriggerMode triggerMode = TRIG_RISE;
    bool oldInput = false;
    uint8_t triggerState = false;
    uint8_t triggerID; // an identifier of the external pin or laser state to look for (checked outside the object)
};

// ****************************************************************************************************************

class Sequencer
{

  private:
    // Sequence parameters to use when in sequence mode. Note that the updateSequence method is a method of the
    //namespace Hardware::Lasers, because we may need to check the states of all the laser objects instantiated.
    struct SequenceParam
    {
        uint16_t t_delay, t_on;     // eventually t_off too
        uint16_t triggerDecimation; // the number of trigger pulses that correspond to one cycle of the sequence
    };

    const SequenceParam defaultSequence = {
        0, // delay time (in ms)
        5, // time on (in ms)
        1  // number of trigger events to restart the sequence (decimation)
    };

    SequenceParam mySequence{defaultSequence};
    uint16_t counterTrigger = 0;
    uint32_t timerSequencer; // reset to micros() each time we receive a trigger signal
    bool output;             // output of the sequencer: ON or OFF (in the future, it can be a struct also containing an analog value - e.g. power)
    bool running = false;

  public:
    Sequencer(){};
    Sequencer(SequenceParam _seqInit) : ouput(_seqInit), running(false) {}

    void setRunningMode(bool _running)
    {
        if (_running) {
            if (!running) { // restart the sequencer parameters
                counterTrigger = 0;
                timerSequencer = micros();
            }
        }
        running = _running;
    }

    void set(SequenceParam _seqParam)
    {
        // NOTE: setting the sequence does not activate the sequence mode: this is done in by setting the proper
        // member: myState.sequenceMode
        mySequence = _seqParam;
    }

    void set(uint16_t _t_delay, uint16_t t_on, uint16_t _triggerDecimation)
    {
        mySequence.t_delay = _t_delay;
        mySequence.t_on = _t_on;
        mySequence.triggerDecimation = _triggerDecimation;
    }

    auto eventUpdate() // to be called when there is an event update (from a trigger)
    {

        if (running)
        { // otherwise don't change the output nor advance the trigger counter

            counterTrigger = (counterTrigger + 1) % mySequence.triggerDecimation;

            if (!counterTrigger)
                timerSequencer = micros();

            output = ((timerSequencer > mySequence.t_delay) && (timerSequencer <= mySequence.t_delay + mySequence.t_on));
        }

        return (output);
    }
};

#endif
