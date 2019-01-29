
#ifndef _Class_Sequencer_H_
#define _Class_Sequencer_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"

/* BASE CLASS InOutModule is used to be able to collect - as base class pointers - all the objects that
 have callable "setIn" or "getOut" methods (ex: for a laser object, we call setInput using the "bang"
 read out in the getOutput from a Sequencer). For example:

        [ (no setIn) Clock 1 ] --> [ Trigger Processor 1 ] --> [ Pulser 1 ] --> [ Laser 1 ]

or even multi-branch sequencers:

        [ Clock 1]  -->  [ Trigger Processor 2 ]  --> [ Laser 3 ]
                                                 |
                                                  --> [ Pulse Shaper 1 ]--> ["Trigger" Output]

      (both laser and pulse shaper output connected to the same trigger detector/processor)

or even cascades:

         [ "Trigger" Input ] --> [ Trigger Processor 1 ] --> [ Pulse Shaper 3 ] --> [ Laser 1 ] --> [ Trigger Output ]
                                                        |
                                                         --> [Laser 2] --> [ Trigger Processor 2] -- >[Laser 3]
                                                                      |
                                                                       --> [ Trigger Processor 3] --> [Laser 4]

    NOTE: here, "trigger" is connected to the laser: the state of the laser could change for other reasons
       than the sequencer - so if this is active (the update function is called), it will interfere with it.

 Of course, several non-connected pipelines can run concurrently from input triggers or different clocks:

        [ "Trigger" Input ] --> [ Trigger Processor 1 ] --> [ Pulse Shaper 1 ] --> [ Trigger Output ]
        [ Clock 0 ] --> [ Pulse Shaper 2 ] --> [ Laser 1 ]
        [ Clock 1 ] --> [ Pulse Shaper 3 ] --> [ Laser 2 ]

    NOTE: To be sure that these clocks start at the same time, we need to call on,START_ALL_CLK.
          Another way is to start all the clock simultaneously using an external clock for instance (the
          state of the clocks will be toggled)

        [ "Trigger" Input ] [ Trigger Processor ]  --> [ Clock 0 ] --> [ Pulse Shaper 2 ] --> [ Laser 1 ]
                                                  |
                                                   --> [ Clock 1 ] --> [ Trigger Processor ] --> [ Laser 2 ]

    TODO: - Add blinking led nodes (for debug and message), logical modules (and, or, xor), and most interestingly,
          make inter-module messages that are "analog" (ramps, etc) that could be useful to control the microscope state.
          - Cyclic graphs would be possible (the information wil advance by clocked or triggered time steps, as we update
           all the nodes *simultaneously* using their previous values in the main loop). This presents a slight problem
           though: what to do when the inputs are different? I would use an OR or AND function; it won't be done for the
           time being.

         [ "Trigger" Input ] --> [ Pulse Shaper 2 ]  --> [ Laser 1 ] -- >  [ Trigger Processor 1] -->
                                                    |                                                |
                                                     <-------------------- < ------------------------
*/

class Module
{
    /*
    NOTE: to make this class polymorphic (so we can call methods from derived classes using
    a pointer to a base class object), we need to declare ALL these methods as virtual in the base class.
    (As a side note, it is not possible to overload methods because they are called from the base class object - then
    we should do a static or dynamic cast to convert the pointer to the corresponding tothe  derived class,
    thus loosing the interest of having an array of base class pointers to process all the modules)

    NOTE: All the modules have input - even the input trigger, because there can be special actions made by the trigger
    input (for debugging for example) and the pipeline could have feedback. Also, the input could be used to disable the
    module function, inverting the signal, changing parameters (I won't go so far now)

    NOTE: Default active state is "true. Indeed, for the time being, I won't care setting each
    module as active/inactive: the only variable that commands the state of the whole sequencer
    is Sequencer::activeSequencer.
    The only module whose "active/inactive" property is really useful is the clock (the others,
    when inactive will just pass the data through), so the clock can be activated/deactivated with a
    special serial command (SET_STATE_CLK, START_CLK, STOP_CLK)
    */

  public:
    Module()
    {
        baseInit(); // the base class init() is called before the child constructor, so we don't need to set some things.
    }
    virtual ~Module(){};

    void start() { active = true; }
    void stop() { active = false; }
    void setState(bool _active) { active = _active; }
    void toggleState() { active = !active; }
    bool isActive() { return (active); }

    // reset() and init() are not pure virtual, because some objects just don't need them
    virtual void baseInit()
    {

        //ATTN myClassIndex and myName HAVE to be properly (re)set for each class type (see Utils::className).
        // => DO NOT FORGET to set the class index in the Laser class! (or any other not in Class_Sequencer.h that
        // is a child of Module and can be used in the sequencer pipeline)

        // myID in the base class can be handy to know the index of creation of each object (but then
        // we need a virtual method to get the value and use cast... Let's forget about an ID for the base class)

        ptr_fromModule = NULL;

        active = true;

        state = false;
        nextState = false;

        firstTime = true; // this is used to ensure that something special happens the same time,
        // and that it will no depend on the casual difference between the initial state and the
        // computed nextState.
    }

    // Reset parameters to default, reinitialize clock, etc.
    virtual void reset()
    {
        state = nextState = false;
        firstTime = true;
    }

    virtual String getName() = 0;
    virtual String getParamString() = 0;

    bool isEqual(Module *_toPtr) { return (_toPtr->getName() == getName()); } // simple comparison of String objects

    void setInputLink(Module *_ptr_fromModule) { ptr_fromModule = _ptr_fromModule; }
    Module *getPtrModuleFrom() { return (ptr_fromModule); }

    // Get the currect state of the module (for the time being, a boolean) in particular for using it
    // in the next module in the chain.
    // NOTE: the method may be overriden in many cases, like the external trigger input or clock, and not
    // use the "state" variable of the module at all.
    virtual bool getState() { return (state); }

    // NOTE 1: we need to first call update(), then refresh() for all modules in that order
    // (of course some modules do not need to implement some of those methods, since the (output)
    // state will be set by an external pin for instance).
    // NOTE 2: methods are not pure virtual because some classes won't need them, BUT the sequencer will call
    // update, action and refresh for all of them.

    // ***** UPDATE *****
    // By default, update uses the value of the previous module to update the nextState. However, some modules
    // (such as the // clock or the input trigger) will override it. A better solution though is to make the
    // computeNextState such that it simply does not depends on the previous state, but from a timer function or a
    // digital input.
    virtual void update()
    {
        if (active && (ptr_fromModule != NULL))
        {
            bool stateFrom = ptr_fromModule->getState(); // get the state of the preceding module at time t-1
            nextState = computeNextState(stateFrom);     // compute the next state
        }
    }

    // ***** NEXT STATE *****
    // The following method will compute the value of the nextState boolean variable.
    // NOTE: by default (if not overriden) the input value will just pass through to the output.
    virtual bool computeNextState(bool _stateFrom) { return (_stateFrom); }

    // ***** REFRESH *****
    //This method is necessary so that the modules are updated in time steps, but "in parallel".
    // It will call "action()" on state change (or continuously if one wants too by overriing it)
    // NOTE if one wants to start by an effective action, then we need to set state and nextState with
    // different values.
    virtual void refresh()
    {
        if (firstTime)
        {
            firstTime = false;
            // set state different or equal from nextState so the action is performed or not the
            // first time?
            state = !nextState; // ensures the action is performed the first time.
        }

        if (state != nextState) // the first time is useful for
        {                       // change-conditional action
            state = nextState;

            action();
        }
    }

    // ***** ACTION *****
    // Whatever the module needs to do (unrelated to the state evolution), using the state variable.
    // note pure virtual because some children won't use (but not non-virtual, otherwise calling from the
    // basew pointer will just exectute the one declared/defined here)
    virtual void action() {}

  protected:
    bool active;
    bool state;
    bool nextState;
    bool firstTime;
    Module *ptr_fromModule;
};

/* ==================================================================================================
                            REAL MODULES (children of base class)
===================================================================================================*/

class InputTrigger : public Module
{
  public:
    InputTrigger() : inputTriggerPin(PIN_TRIGGER_INPUT) // default input pin (so we can use it right away)
    {
        init();
    }
    InputTrigger(uint8_t _inputPin)
    {
        inputTriggerPin = _inputPin;
        init();
    }

    void init()
    {
        myID = id_counter;
        id_counter++;
        myClassIndex = Definitions::ClassIndexes::CLASSID_IN;
        myName = Definitions::classNames[myClassIndex];
        setInputPin(inputTriggerPin);
    }

    void setInputPin(uint8_t _inputPin)
    {
        inputTriggerPin = _inputPin;
        pinMode(_inputPin, INPUT_PULLUP); // or pulldown? or floating? (make a parameter?)
    }
    uint8_t getInputPin() { return (inputTriggerPin); }

    // Separating this method from action() is for debugging purposes, or for using the
    // trigger somewhere else (not in the sequencer, with the sequencer inactive):
    bool readInput()
    {
        return (digitalRead(inputTriggerPin));
    }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS **************************************
    String getName() { return (myName + "[" + String(myID) + "]"); } // by overriding this method,
                                                                     //we ensure that we are using the myID of the derived class!
    String getParamString() { return ("{ pin=" + String(inputTriggerPin) + "}"); }

    bool getState()
    {
        // There could be some sort of processing (filter? software schmitt trigger?),
        // checking or something (led debug?)
        return (readInput());
    }
    // NOTE: reset(), action(), update() and refresh() do not need to be overridden here.
    // NOTE: If the input trigger is in the middle of the pipeline, the data received will be overriden
    // by the state of the digital pin, regarless of the state or nextState value.
    // *************************************************************************************************

  private:
    String myName;
    uint8_t myClassIndex;
    uint8_t myID;
    static uint8_t id_counter;
    uint8_t inputTriggerPin;
};

class Clock : public Module
{
    /*
     NOTE : This is basically a "slow" 50%-duty PWM generator.
     NOTE:  The clock is a child of "receiverModule" because it can be started/stopped (actually toggled)
            by an input bang (an external trigger for instance). See "action()" method.
     NOTE : We could use an IntervalTimer to geneate clock Pulsars, but then a clocked trigger
            should work as either a callback function, or use another IntervalTimer with higher rate.
            However, this strategy won't work for the triggers controlled by changes in laser state (
            unless, again, the sample rate is fast or a pin change will produce a callback. This is risky, as
            it may introduce unacceptable delays to set the laser state). In case of an IntervalTimer controlled
            trigger, the problem is that there may be jitter (timer periods are arbitrary). Therefore, I will
            not use IntervalTimer, but a simple pulling method (that could eventually be triggered by an
            interval timer if things are not critical).
            Another possibility is to use a PWM signal (tone function!), which, using analogWriteFrequency
            has a lower limit of a few Hz. But I guess it may be possible that we need to integrate the camera
            image for larger periods, while it will never be too fast (up to 1kHz perhaps?), so the
            best solution is to do a very simple Clock object.

    NOTE : To generate a PWM with a duty cycle (to control the camera exposure in EDGE mode), use a sequencer.
           It is better this way because using a Trigger object, we can decimate and also add dead times.
    */

  public:
    Clock() { init(); }
    Clock(uint32_t _periodMs)
    {
        periodMs = _periodMs;
        init();
    }

    void init()
    {
        myID = id_counter;
        id_counter++;
        myClassIndex = Definitions::ClassIndexes::CLASSID_CLK;
        myName = Definitions::classNames[myClassIndex];
        active = false; // default state is true for most modules, but the clock is special: we may want to start
                        // the clock in synch with other clocks. Btw, the clock can be TOGGLED BY AN INPUT TRIGGER!
        reset();
    }

    void setPeriodMs(uint32_t _periodMs)
    {
        periodMs = _periodMs;
        reset();
    }
    uint32_t getPeriodMs() { return (periodMs); }

    // ******************** Overriden/Overloaded METHODS OF THE BASE CLASS ***********************

    String getName() { return (myName + "[" + String(myID) + "]"); }
    String getParamString() { return ("{ period=" + String(periodMs) + "ms}"); }

    void reset() { clockTimer = millis(); }

    bool getState()
    {
        // getState can be called at any time, from everywhere in the program (not in main loop
        // for instance), but then don't forget to call "update" right before or the state won't change.
        // To avoid this problem, we call update here:
        update();
        return (state);
    }

    void update() // very special in the case of the clock:
    {
        // previous module can activate/deactivate the clock:
        if (ptr_fromModule != NULL)
        {
            active = ptr_fromModule->getState();
            // reset(); ?
        }
        else if (active && (millis() - clockTimer > periodMs))
        { // otherwise don't change the current state
            state = !state;
            clockTimer = millis();
            digitalWrite(PIN_LED_DEBUG, state); // -test-
        }
    }

    // ==================================================================================================

  private:
    String myName;
    uint8_t myClassIndex;
    uint8_t myID;
    static uint8_t id_counter;

    uint32_t periodMs = 1000; // in ms
    uint32_t clockTimer;
};

class OutputTrigger : public Module
{
  public:
    OutputTrigger() : outputTriggerPin(PIN_TRIGGER_OUTPUT)
    {
        init();
    }
    OutputTrigger(uint8_t _outputPin)
    {
        outputTriggerPin = _outputPin;
        init();
    } // in the future, there could be be several triggers

    void init()
    {
        myID = id_counter;
        id_counter++;
        myClassIndex = Definitions::ClassIndexes::CLASSID_OUT;
        myName = Definitions::classNames[myClassIndex];
        setOutputPin(outputTriggerPin);
    }

    void setOutputPin(uint8_t _outputPin)
    {
        outputTriggerPin = _outputPin;
        pinMode(outputTriggerPin, OUTPUT);
    }
    uint8_t getOutputPin() { return (outputTriggerPin); }

    // Separating this method from action() is for debugging purposes, or for using the
    // trigger somewhere else (not in the sequencer, with the sequencer inactive):
    void setOutput(bool _state)
    {
        digitalWrite(outputTriggerPin, _state);
        digitalWrite(PIN_LED_MESSAGE, state); // -test-
    }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS **************************************
    String getName() { return (myName + "[" + String(myID) + "]"); }
    String getParamString() { return ("{ pin=" + String(outputTriggerPin)); }

    void action()
    {
        setOutput(state);
    }

    // ************************************************************************************************

  private:
    String myName;
    uint8_t myClassIndex;
    uint8_t myID;
    static uint8_t id_counter;

    uint8_t outputTriggerPin;
};

class TriggerProcessor : public Module
{

  public:
    TriggerProcessor()
    {
        init();
    }
    TriggerProcessor(uint8_t _mode,
                     uint16_t _burstLength,
                     uint16_t _skipLength,
                     uint16_t _offsetEvents)
    {
        init();
        // Note: to have full continuous events, do continuous = 1, and skip = 0
        setParam(_mode, _burstLength, _skipLength, _offsetEvents);
    }

    void init()
    {
        myID = id_counter;
        id_counter++;
        myClassIndex = Definitions::ClassIndexes::CLASSID_TRG;
        myName = Definitions::classNames[myClassIndex];

        state = true; // NOTE: trigger processors are always active: they produce output when they receive input.
                      // It will be quite useless to deactivate them, but one could do it if desired (command not implemented though)

        mode = 0; // default is RISE
        // NOTE: by setting burstLength to 1 and skipLength to 0, there is
        // no skip and the TriggerProcessor is continuous:
        burstLength = 1;
        skipLength = 0;
        offsetEvents = 0;

        reset();
    }

    void reset()
    {
        counterEvents = -offsetEvents; // IMPORTANT! start (negative if offset) only the FIRST time
        stateMachine = BURST_STATE;
    }

    void setParam(uint8_t _mode,
                  uint16_t _burstLength,
                  uint16_t _skipLength,
                  uint16_t _offsetEvents)
    {
        mode = _mode;
        burstLength = _burstLength;
        skipLength = _skipLength;
        offsetEvents = _offsetEvents;

        reset();
    }

    void setMode(uint8_t _mode)
    {
        mode = _mode;
        reset();
    }
    void setBurst(uint16_t _burstLength)
    {
        burstLength = _burstLength;
        reset();
    }
    void setSkip(uint16_t _skipLength)
    {
        skipLength = _skipLength;
        reset();
    }
    void setOffset(uint16_t _offsetEvents)
    {
        offsetEvents = _offsetEvents;
        reset();
    }

    String getMode()
    {
        return (Definitions::trgModeNames[mode]);
    }
    uint16_t getBurst()
    {
        return (burstLength);
    }
    uint16_t getSkip()
    {
        return (skipLength);
    }
    uint16_t getOffset()
    {
        return (offsetEvents);
    }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS **************************************

    String getName() { return (myName + "[" + String(myID) + "]"); }
    String getParamString()
    {
        return ("{ mode=" + Definitions::trgModeNames[mode] + ", burst=" + String(burstLength) + ", skip=" + String(skipLength) + ", offset=" + String(offsetEvents) + "}");
    }

    void action()
    {
        // TODO: this could signal somehow the state of the trigger processor
        // (the state machine state, the number of detected events, etc)
    }

    bool computeNextState(bool _stateFrom)
    {

        // "event" is a detected rise, fall or change on state of input module
        bool event = false;

        // "output" is the result of the processed event stream (burst, skip and offset)
        bool output = false;

        switch (mode)
        {
        case 0: // RISE
            event = (!state) && _stateFrom;
            break;
        case 1: // FALL
            event = state && (!_stateFrom);
            break;
        case 2: // CHANGE
            // "^" is an XOR (same than "!="" for binary arguments)
            event = (state ^ _stateFrom);
            break;
        default:
            break;
        }

        // then, update the state of this module to match the value of the input module
        state = _stateFrom;

        if (event)
        {
            // digitalWrite(PIN_LED_DEBUG, state); // -test-

            // Increment number events detected by the trigger:
            counterEvents++;

            switch (stateMachine)
            {
            case BURST_STATE:
                if (counterEvents <= burstLength)
                {
                    output = true;
                }
                else
                {
                    // Serial.println("END BURST"); // -test-
                    output = false;
                    stateMachine = SKIP_STATE;
                    counterEvents = 0; // NOTE offset is not applied anymore.
                }
                break;
            case SKIP_STATE:
                if (counterEvents > skipLength)
                {
                    output = true;
                    stateMachine = BURST_STATE;
                    counterEvents = 0;
                } // else output remains false
                break;
            default:
                break;
            }
        }
        return (output);
    }

  private:
    // will put this in Definitions namespace in case we can change the keywords
    // const String trgModeNames[3]{"rise", "fall", "change"};

    enum StateMachine
    {
        BURST_STATE,
        SKIP_STATE
    };

    uint8_t mode;
    uint16_t burstLength;
    uint16_t skipLength;
    uint16_t offsetEvents;
    StateMachine stateMachine;

    int32_t counterEvents; // ATTN: must be a signed integer

    String myName;
    uint8_t myClassIndex;
    uint8_t myID;
    static uint8_t id_counter;
};

class Pulsar : public Module
{
    // TODO: the "pulse shaper" (or "pulsar") has a simple binary output;
    // however, it would not be too difficult to have an analog output (ramps, triangle
    // signal, even sinusoid. This may be very useful for steering the microscope stage!)

  public:
    Pulsar()
    {
        init();
    }
    Pulsar(uint32_t _t_off_ms, uint32_t _t_on_ms)
    {
        setParam(_t_off_ms, _t_on_ms);
        init();
    }

    void init()
    {
        myID = id_counter;
        id_counter++;
        myClassIndex = Definitions::ClassIndexes::CLASSID_PUL;
        myName = Definitions::classNames[myClassIndex];
        reset();
    }

    void setParam(uint32_t _t_off_ms, uint32_t _t_on_ms)
    {
        t_off_ms = _t_off_ms;
        t_on_ms = _t_on_ms;
        reset();
    }

    String getName() { return (myName + "[" + String(myID) + "]"); }
    String getParamString()
    {
        return ("{" + String(t_off_ms) + "ms, " + String(t_on_ms) + "ms}");
    }

    void reset()
    {
        timerPulsar = millis(); // +t_off_ms + t_on_ms; // this is to avoid
    }

    // ******************** OVERRIDEN METHODS OF THE BASE CLASS ***********************
    bool computeNextState(bool _stateFrom)
    {
        if (_stateFrom)
            reset(); // reset timer
        // Output of the Pulsar: ON or OFF (in the future, it can be a struct also
        // containing an analog value - e.g. power ramps)
        uint32_t timePassed = millis() - timerPulsar;
        bool output = (timePassed > t_off_ms) && (timePassed <= (t_off_ms + t_on_ms));
        return (output);
    }
    // ********************************************************************************

  private:
    // Pulse parameters (eventually t_off_ms too, or a more complicated shape using an array)
    uint32_t t_off_ms = 0;
    uint32_t t_on_ms = 50;
    uint32_t timerPulsar; // reset to millis() each time we receive a trigger signal

    String myName; // TODO: make these variables STATIC!
    uint8_t myClassIndex;

    uint8_t myID;
    static uint8_t id_counter;
};

#endif
