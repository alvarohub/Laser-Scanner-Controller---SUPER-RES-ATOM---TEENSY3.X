
#ifndef _Class_Sequencer_H_
#define _Class_Sequencer_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"

/* BASE CLASS InOutModule is used to be able to collect - as base class pointers - all the objects that
 have callable "setIn" or "getOut" methods (ex: for a laser object, we call setInput using the "bang"
 read out in the getOutput from a Sequencer). For example:

      [ (no setIn) Clock 1 ] --> [ Trigger Detector 1 ] --> [ Sequencer 1 ] --> [ Laser 1 ]

or

      [ Clock 1]  -->  [ Trigger Detector 2 ] --> [ Laser 3 ]
                                             |
                                             --> [Trigger Output]
      (both laser and trigger output connected to the same trigger detector)

or

      [ Trigger Input ] --> [ Trigger Detector 4] --> [ Sequencer 3 ] --> [ Laser 2 ] --> [ Trigger Output ]
                                                  |
                                                  --> [Laser 1]
      (note that the trigger output could be directly connected to the sequencer 3, but in the future the
      lasers could change their state for other reasons than a "bang")

 Of course, several non-connected pipelines can run concurrently. Cyclic graphs are also possible (the information will
 advance by time steps, as we update all the nodes simultaneously using their previous values in the main loop).

 Setting/getting methods should have the same names on all the children objects, so they are declared
 as virtual methods in the base class. (There could be other ways to do this, such as having indexes or member
 names to identify the objects, or implementing a more complicated way to access class member functions through
 pointers... not sure about this last possibility without the use of a base class though; moreover, perhaps
 the only advantage would be to avoid forcing the same name for the setters and getters, which seems not really worth the additional complexity).

 NOTE: in the future I can add nodes with several inputs or outputs, and more complex (cyclic) graphs. Just like the "gluons". This can be particularly useful for doing logical operations between the signals (would be easy to implement some basic operations, unary or binary, such as NOT, AND, OR, XOR...).
*/

class Module
{
    // ATTENTION: to make this class polymorphic (so we can call methods from derived classes using
    // a pointer to a base class object), we need to declare ALL these methods as virtual in the base class.
    // These methods are here: reset(), update(), getState(), action() and refresh();
    // (otherwise we would need to do a static or dynamic cast to convert the pointer to the correspondent
    // derived class, thus loosing the interest of having an array of base class pointers to process all the modules)

    // NOTE: for the time being, I won't care setting each module as active/inactive: the only variable that
    // commands the state of the whole sequencer is Sequencer::activeSequencer. Therefore the default active value of
    // set in the constructor is "true"

  public:
    Module() : ptr_fromModule(NULL), active(true), state(false), myName("*") {}
    virtual ~Module(){};

    virtual String getName() { return (""); } // not pure virtual because of the intermediate class receiverModule
    virtual String getParamString() { return (""); }

    // Note: some modules won't use this, but is simpler to make it a variable of the base class so we don't need
    // to check if the modules have or have not inputs.
    void setInputLink(Module *_ptr_fromModule) { ptr_fromModule = _ptr_fromModule; }

    // Get the currect state of the module (for the time being, a boolean)
    // NOTE: the method may be overriden in many cases, like the external trigger input or clock
    virtual bool getState() { return (state); }

    // NOTE 1: methods are not pure virtual because some classes won't need them, BUT the sequencer will call
    // update, action and refresh for all of them.
    // NOTE 2: we need to call action() for all the modules first, then update() for all modules, and
    // finally refresh() for all modules too, in that order (of course some modules do not need to implement
    // some of those methods, since the (output) state will be set by an external pin for instance)
    virtual void action() {}
    virtual void update() {}
    virtual void refresh() {}

    bool isActive() { return (active); }

    void setState(bool _active) { active = _active; }

    void start()
    {
        active = true;
        //reset();
    }

    void stop()
    {
        active = false;
    }

    virtual void reset() {} // not pure virtual, because some objects don't need it,
                            // but all of them will call start and stop

    Module *getPtrModuleFrom() { return (ptr_fromModule); }

  protected:
    Module *ptr_fromModule;
    bool active;
    bool state;
    String myName;
};

// ==================================================================================================
//                                      MODULES WITH ONLY OUTPUTS
// ==================================================================================================

class Clock : public Module
{
    /*
     NOTE 1: this is basically a "slow" 50%-duty PWM generator.
     NOTE 1: We could use an IntervalTimer to geneate clock Pulsars, but then a clocked trigger
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

    NOTE 2: to generate a PWM with a duty cycle (to control the camera exposure in EDGE mode), use a sequencer.
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
        myID = id_count;
        id_count++;
        reset();
    }

    String getName()
    {
        return ("CLK[" + String(myID) + "]");
    }

    String getParamString() { return ("{" + String(periodMs) + "ms}"); }

    void setPeriodMs(uint32_t _periodMs)
    {
        periodMs = _periodMs;
        reset();
    }
    uint32_t getPeriodMs() { return (periodMs); }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS ***********************

    void reset() { clockTimer = millis(); }

    bool getState()
    {
        if (active && (millis() - clockTimer > periodMs))
        {
            state = !state;
        } // otherwise don't change the current state
        return (state);
    }

    // NOTE: action(), update() and refresh() do not need to be overriden here
    // ==================================================================================================

  private:
    uint32_t periodMs = 10; // in ms
    uint32_t clockTimer;
    static uint8_t id_count;
    uint8_t myID;
};

class InputTrigger : public Module
{
  public:
    InputTrigger() : inputTriggerPin(PIN_TRIGGER_INPUT)
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
        myID = id_count;
        id_count++;
        pinMode(inputTriggerPin, INPUT_PULLUP); // or pulldown? or floating? (make a parameter?)
    }

    String getName()
    {
        return ("IN[" + String(myID) + "]");
    }

    void setInputPin(uint8_t _inputPin)
    {
        inputTriggerPin = _inputPin;
        init();
    }
    uint8_t getInputPin() { return (inputTriggerPin); }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS **************************************
    bool getState()
    {
        // There could be some sort of processing (filter? software schmitt trigger?),
        // checking or something (led debug?)
        return (digitalRead(inputTriggerPin));
    }
    // NOTE: reset(), action(), update() and refresh() do not need to be overridden here.
    // *************************************************************************************************

  private:
    static uint8_t id_count;
    uint8_t myID;
    uint8_t inputTriggerPin;
};

// ==================================================================================================
//                                      MODULES WITH INPUT
// --> These modules have an additional method implemented, "computeNextState", as well as variables
// to record previous states.
// ==================================================================================================

class receiverModule : public Module
{
  public:
    receiverModule() : nextState(false) {}
    receiverModule(Module *_ptr_fromModule) : nextState(false)
    {
        setInputLink(_ptr_fromModule);
    }
    virtual ~receiverModule(){};

    bool getState() { return (state); }

    void update()
    {
        if (active)
        {
            bool inputVal = ptr_fromModule->getState(); // get the state of the preceding module at time t-1
            nextState = computeNextState(inputVal);     // compute the next state

            // or, if we use instead a different technique setting the next module, we would do:
            //nextState = computeNextState(inputVal);
            //ptr_toModule->setState(nextState)
        }
    }

    void refresh()
    {
        state = nextState; // update the state of the module
    }

    // ****** THIS METHOD WILL BE OVERRIDEN by some CHILDREN of receiverModule ********
    // The following method will compute the value of the nextState boolean variable:
    virtual bool computeNextState(bool _inputVal) { return (false); }
    // **********************************************************************************

  protected:
    bool nextState;
};

class OutputTrigger : public receiverModule
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
        myID = id_count;
        id_count++;
        pinMode(outputTriggerPin, OUTPUT);
    }

    String getName()
    {
        return ("OUT[" + String(myID) + "]");
    }

    void setOutputPin(uint8_t _outputPin)
    {
        outputTriggerPin = _outputPin;
        init();
    }
    uint8_t getOutputPin() { return (outputTriggerPin); }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS **************************************
    //  getState() is not overridden: we don't read the trigger output state (in principle!), neither compute
    // update or refresh.

    void action()
    {
        digitalWrite(outputTriggerPin, state);
    }

    // ************************************************************************************************

  private:
    uint8_t outputTriggerPin;
    static uint8_t id_count;
    uint8_t myID;
};

class TriggerProcessor : public receiverModule
{

  public:
    TriggerProcessor()
    {
        init();
        reset();
    }
    TriggerProcessor(uint8_t _mode,
                     uint16_t _continuousNumEvents,
                     uint16_t _skipNumEvents,
                     uint16_t _offsetEvents)
    {
        init();
        // Note: to have full continuous events, do continuous = 1, and skip = 0
        setParam(_mode, _continuousNumEvents, _skipNumEvents, _offsetEvents);
    }

    void init()
    {
        myID = id_count;
        id_count++;
    }

    String getName()
    {
        return ("TRG[" + String(myID) + "]");
    }

    String getParamString()
    {
        return ("{" + Definitions::trgModeNames[mode] + ", " + String(continuousNumEvents) + ", " + String(skipNumEvents) + ", " + String(offsetEvents) + "}");
    }

    void reset()
    {
        state = false;                 //better not to do it?
        counterEvents = -offsetEvents; // IMPORTANT! start (negative if offset) only the FIRST time
        stateMachine = NO_SKIPPING_STATE;
    }

    void setParam(uint8_t _mode,
                  uint16_t _continuousNumEvents,
                  uint16_t _skipNumEvents,
                  uint16_t _offsetEvents)
    {
        setMode(_mode);
        continuousNumEvents = _continuousNumEvents;
        skipNumEvents = _skipNumEvents;
        offsetEvents = _offsetEvents;

        reset();
    }

    void setMode(uint8_t _mode)
    {
        mode = _mode;
        reset();
    }

    void setBurst(uint16_t _continuousNumEvents)
    {
        continuousNumEvents = _continuousNumEvents;
        reset();
    }
    void setSkip(uint16_t _skipNumEvents)
    {
        skipNumEvents = _skipNumEvents;
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
        return (continuousNumEvents);
    }
    uint16_t getSkip()
    {
        return (skipNumEvents);
    }
    uint16_t getOffset()
    {
        return (offsetEvents);
    }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS **************************************

    bool computeNextState(bool _inputVal)
    {
        bool event = false;
        bool output = false;

        // Then, detect event depending on the selected mode:
        switch (mode)
        {
        case 0:
            event = (!state) && _inputVal;
            break;
        case 1:
            event = state && (!_inputVal);
            break;
        case 2:
            // "^" is an XOR (same than "!="" for binary arguments)
            event = (state ^ _inputVal);
            break;
        default:
            break;
        }

        if (event)
        {
            counterEvents++;

            switch (stateMachine)
            {
            case NO_SKIPPING_STATE:
                if (counterEvents < continuousNumEvents)
                {
                    output = true;
                }
                else
                {
                    stateMachine = SKIPPING_STATE;
                    counterEvents = 0;
                    output = false;
                }
                break;
            case SKIPPING_STATE:
                if (counterEvents >= skipNumEvents)
                {
                    stateMachine = NO_SKIPPING_STATE;
                    counterEvents = 0;
                    output = true;
                }
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
        NO_SKIPPING_STATE,
        SKIPPING_STATE
    };

    uint8_t mode = 0; // default is RISE
    // NOTE: by setting continuousNumEvents to 1 and skipNumEvents to 0, there is
    // no skip and the TriggerProcessor is continuous.
    uint16_t continuousNumEvents = 1;
    uint16_t skipNumEvents = 0;
    uint16_t offsetEvents = 0;
    StateMachine stateMachine = NO_SKIPPING_STATE;

    int32_t counterEvents = 0; // must be a signed integer

    static uint8_t id_count;
    uint8_t myID;
};

class Pulsar : public receiverModule
{
    // TODO: the "pulse shaper" (or "pulsar") has a simple binary output;
    // however, it would not be too difficult to have an analog output (ramps, triangle
    // signal, even sinusoid. This may be very useful for steering the microscope stage!)

  public:
    Pulsar()
    {
        timerPulsar = millis() + t_off_ms + t_on_ms;
        init();
        reset();
    }
    Pulsar(uint32_t _t_off_ms, uint32_t _t_on_ms)
    {
        init();
        setParam(_t_off_ms, _t_on_ms);
    }

    void init()
    {
        myID = id_count;
        id_count++;
    }

    String getName()
    {
        return ("PUL[" + String(myID) + "]");
    }

    String getParamString()
    {
        return ("{" + String(t_off_ms) + "ms, " + String(t_on_ms) + "ms}");
    }

    void setParam(uint32_t _t_off_ms, uint32_t _t_on_ms)
    {
        t_off_ms = _t_off_ms;
        t_on_ms = _t_on_ms;
        reset();
    }

    void reset()
    {
        timerPulsar = millis() + t_off_ms + t_on_ms;
    }

    // ******************** OVERRIDEN METHODS OF THE BASE CLASS ***********************
    // void action() {}

    bool computeNextState(bool _inputVal)
    {
        if (_inputVal)
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

    static uint8_t id_count;
    uint8_t myID;
};

#endif
