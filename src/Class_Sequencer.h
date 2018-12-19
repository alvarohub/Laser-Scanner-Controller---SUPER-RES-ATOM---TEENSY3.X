
#ifndef _Class_Sequencer_H_
#define _Class_Sequencer_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"

class Trigger
{
private:
    bool state; // current state of the "button"
    uint8_t triggerState = Utils::TriggerState::TRIG_EVENT_NONE; 
public:
    Trigger() {}
    Trigger(bool _startState) : state(_startState) {};

     // I will use pulling in the main loop to check and update the trigger state, but in the future we could
    // do it using an external interrupt (with a lower priority with respect to the scanner display periodic soft interruption)
    Utils::TriggerState update(bool _newState)
    {
        if (_newState) {
            if (state)
                triggerState = Utils::TriggerState::TRIG_EVENT_NONE; 
            else
                triggerState = Utils::TriggerState::TRIG_FALL;
        }
        else
        {
         if (state)
                triggerState = Utils::TriggerState::TRIG_RISE; 
            else
                triggerState = Utils::TriggerState::TRIG_EVENT_NONE;
        }
        return (triggerState);
    }

    Utils::TriggerState getTriggerState() { return (triggerState); }
   
    void setTriggerState(Utils::TriggerState _triggerState) { triggerState = _triggerState; } // can be useful for reset to a particular state
  
};

class Sequencer {

    Sequencer() {};
    Sequencer(SequenceParam _seq) : state(_seq) {};

	void setSequencer(SequenceParam _seq) {
		// NOTE: setting the sequence does not activate the sequence mode: this is done in by setting the proper
		// member: myState.sequenceMode
		mySequence = _seq;
	}

  private:
    bool state; // output of the sequencer: ON or OFF (in the future, it can be a struct also containing power)
    Trigger trigger;
    uint32_t timer; // reset to micros() each time we receive a trigger signal
};

#endif
