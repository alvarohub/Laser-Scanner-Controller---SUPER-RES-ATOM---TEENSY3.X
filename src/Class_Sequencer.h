
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

    void setTriggerMode(TriggerMode _triggerMode = TRIG_RISE) { triggerMode = _triggerMode; }
    bool checkEvent() { return (event); }

    // I will use pulling in the main loop to check and update the trigger state from the selected input (pin or another boolean),
    // but in the future we could do it using an external interrupt (with a lower priority with respect to the scanner display
    // periodic soft interruption).
    // ATTENTION: event won't change until the next call to update()
    auto update(bool _newInput)
    {

        switch (triggerMode)
        {
        case TRIG_RISE:
            event = (!oldInput) && _newInput;
            break;
        case TRIG_FALL:
            event = oldInput && (!_newInput);
            break;
        case TRIG_CHANGE:
            event = (oldInput ^ _newInput); // "^" is an XOR (same than "!="" for binary arguments)
            break;
        default:
            break;
        }

        oldInput = _newInput;

        return (event);
    }

  private:
    TriggerMode triggerMode = TRIG_RISE;
    bool oldInput = false;
    bool event = false;
};

// ****************************************************************************************************************

class Sequencer
{
    // Sequence parameters to use when in sequence mode. Note that the updateSequence method is a method of the
    //namespace Hardware::Lasers, because we may need to check the states of all the laser objects instantiated.
    struct SequenceParam
    {
        // note: timings are in microseconds!!
        uint16_t t_delay_us, t_on_us; // eventually t_off too
        uint16_t eventDecimation;     // the number of trigger pulses that correspond to one cycle of the sequence
    };

  public:
    Sequencer() : mySequence(defaultSequence), running(false){};
    Sequencer(SequenceParam _seqParam) : mySequence(_seqParam), running(false) {}

    void setRunningMode(bool _running)
    {
        if (_running)
        {
            if (!running)
            { // restart the sequencer parameters
                eventCounter = 0;
                timerSequencer = micros();
            }
        }
        running = _running;
    }

    bool getRunningMode()
    {
        return (running);
    }

    void set(SequenceParam _seqParam)
    {
        // NOTE: setting the sequence does not change the running state
        mySequence = _seqParam;
    }

    void set(uint16_t _t_delay_us, uint16_t _t_on_us, uint16_t _eventDecimation)
    {
        mySequence.t_delay_us = _t_delay_us;
        mySequence.t_on_us = _t_on_us;
        mySequence.eventDecimation = _eventDecimation;
    }

    auto update(bool _event) // executed when there is an event (trigger output "true")
    {

        if (running && _event)
        { // otherwise don't change the output nor advance the event counter

            eventCounter = (eventCounter + 1) % mySequence.eventDecimation;

            if (!eventCounter) // overflow
                timerSequencer = micros();

            output = ((timerSequencer > mySequence.t_delay_us) && (timerSequencer <= mySequence.t_delay_us + mySequence.t_on_us));
        }

        return (output);
    }

  private:
    const SequenceParam defaultSequence = {
        0,     // delay time (in ms) - note: camera frame rate is about 100Hz, or 10ms period.
        50000, // time on (in ms)
        1      // (vent decimation: number of events needed to launch the sequence
    };

    SequenceParam mySequence{defaultSequence};
    uint16_t eventCounter = 0;
    uint32_t timerSequencer; // reset to micros() each time we receive a trigger signal
    bool output;             // output of the sequencer: ON or OFF (in the future, it can be a struct also containing an analog value - e.g. power)
    bool running = false;
};

#endif
