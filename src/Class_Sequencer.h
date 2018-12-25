
#ifndef _Class_Sequencer_H_
#define _Class_Sequencer_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"

class Trigger
{

  public:

    Trigger() {}
    Trigger(int8_t _source, uint8_t _mode, uint16_t _skipNumEvents, uint16_t _offsetEvents) {
        setTriggerParam(_source, _mode, _skipNumEvents, _offsetEvents);
    }

    void setTriggerParam(int8_t _source, uint8_t _mode, uint16_t _skipNumEvents, uint16_t _offsetEvents) {
        source = _source;
        mode = _mode;
        skipNumEvents = _skipNumEvents; 
        offsetEvents =  _offsetEvents;
        counterEvents = -offsetEvents;
    }

    void setTriggerSource(int8_t _source) { source = _source; }
    void setTriggerMode(uint8_t _mode = 0) { mode = _mode; }
    void setTriggerSkipNumEvents(uint16_t _skipNumEvents) {
        skipNumEvents = _skipNumEvents;
        counterEvents = -offsetEvents;
    }
    void setTriggerOffsetEvents(uint16_t _offsetEvents) { 
        offsetEvents =  _offsetEvents;
        counterEvents = -offsetEvents;
    }

    int8_t getTriggerSource() { return (source); }
    uint8_t getTriggerMode() { return(mode); }
    uint16_t getTriggerSkipNumEvents() { return (skipNumEvents); }
    uint16_t getTriggerOffsetEvents() { return(offsetEvents); }

    // I will use pulling in the main loop to check and update the trigger state from the selected input (pin or another boolean),
    // but in the future we could do it using an external interrupt (with a lower priority with respect to the scanner display
    // periodic soft interruption).
    auto updateReadTrigger(bool _newInput)
    {
        bool event = false;
        bool output = false;

        // First, detect event depending on the selected mode:
        switch (mode)
        {
        case 0: // RISE
            event = (!oldInput) && _newInput;
            break;
        case 1: // FALL
            event = oldInput && (!_newInput);
            break;
        case 2: // CHANGE
            event = (oldInput ^ _newInput); // "^" is an XOR (same than "!="" for binary arguments)
            break;
        default:
            break;
        }

        if (event) counterEvents++;

        if (counterEvents == skipNumEvents + 1) {
            output = true;
            counterEvents = 0;
        }

        oldInput = _newInput;

        return (output); 
    }

    void resetTrigger() {
        oldInput = false; //better not to do it?
        counterEvents = -offsetEvents;
    }

  private:
    int8_t source = -1; // trigger source (-1 for external input, [0-3] for other things, like laser states)
    uint8_t mode = 0;   // 0 = RISE, 1 = FALL, 2 = CHANGE
    uint16_t skipNumEvents = 0; // number of events (RISE, FALL or FALL) to ignore before triggering output to high
    uint16_t offsetEvents = 0;   // only used because I have a resetTrigger() method
    
    int32_t counterEvents = 0;
    bool oldInput = false;
};

// ****************************************************************************************************************

class Sequencer
{

  public:
    Sequencer() { timerSequencer = millis() + t_delay_ms + t_on_ms; }
    Sequencer(uint32_t _t_delay_ms, uint32_t _t_on_ms) {
        setSequencerParam(_t_delay_ms, _t_on_ms);
        timerSequencer = millis() + t_delay_ms + t_on_ms; 
    }

    void setSequencerParam(uint32_t _t_delay_ms, uint32_t _t_on_ms)
    {
        t_delay_ms = _t_delay_ms;
        t_on_ms = _t_on_ms;
        timerSequencer = millis() + _t_delay_ms + _t_on_ms;
    }

    auto updateReadSequencer(bool _triggerEvent) // executed when there is a trigger event (trigger output "true")
    {
        if (_triggerEvent)  timerSequencer = millis(); // reset timer
        
        // Output of the sequencer: ON or OFF (in the future, it can be a struct also containing an analog value - e.g. power ramps)
        bool output = ((timerSequencer > t_delay_ms) && (timerSequencer <= t_delay_ms + t_on_ms));
        
        return (output);
    }

     void resetSequencer() {
        timerSequencer = millis() + t_delay_ms + t_on_ms;
    }


   // Sequence parameters (eventually t_off too, or a more complicated sequece using an array)
    uint32_t t_delay_ms = 0;
    uint32_t t_on_ms =    50; 

  private:
    
    uint32_t timerSequencer; // reset to millis() each time we receive a trigger signal
 
};

#endif
