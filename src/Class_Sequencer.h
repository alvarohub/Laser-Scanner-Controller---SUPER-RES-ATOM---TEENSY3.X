
#ifndef _Class_Sequencer_H_
#define _Class_Sequencer_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"

/* BASE CLASS InOutModule is used to be able to collect - as base class pointers - all the objects that
 have callable "setIn" or "getOut" methods (ex: for a laser object, we call setInput using the "bang"
 read out in the getOutputState from a Sequencer). For example:

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
    is Sequencer::activeSequencer. By the way, the only module whose "active/inactive" property is really useful is the clock (the others, when inactive will just pass the data through), so the clock can be activated/deactivated with a
    special serial command (SET_STATE_CLK, START_CLK, STOP_CLK)
    */

  public:
    Module()
    {
        Module::init();
    }
    virtual ~Module(){};

    virtual void init()
    {
        active = true; // default is true - in the case of the clock, it may be better to start with false.
        ptr_fromModule = NULL;
        reset();
    }

    void start() { active = true; }
    void stop() { active = false; }
    virtual void setActive(bool _active) { active = _active; } // note: this can be overloaded, so as to be able to
    // set the parameters when not active depending on the module (for instance, for the laser this is OFF)
    void toggleActive() { active = !active; }
    bool isActive() { return (active); }

    // Base class reset (the children can use it as default, or override it to complement it,
    // but still can call this one using Module::reset)
    // Reset parameters to default, reinitialize clock, etc.
    // (ATTN: the link is not reset!)
    virtual void reset()
    {
        // NOTE 1: sometimes modules are stateless.
        // NOTE 2: states could be more complex than a binary value of course. Also, children module can have they own
        // state variables of course, but this boolean variable will come handy in many cases.
        input = nextInput = false;
        output = nextOutput = false;
        state = nextState = false;
        firstTime = true; // this is used if something special has to happen the first time.
    }

    virtual String getName() = 0;
    virtual String getParamString() = 0;

    // Module equality (will be done through the name, but simple string comparison)
    bool isEqual(Module *_toPtr) { return (_toPtr->getName() == getName()); }

    void setInputLink(Module *_ptr_fromModule) { ptr_fromModule = _ptr_fromModule; }
    void clearInputLink() { ptr_fromModule = NULL; }
    Module *getPtrModuleFrom() { return (ptr_fromModule); }

    // Get the output of the module (for the time being, a boolean) for using it
    // in the next module in the chain.
    bool getOutputState() { return (output); }

    // NOTE 1: we need to first call update(), then refreshStates() for all modules in that order
    // (of course some modules do not need to implement some of those methods, since the (output)
    // state will be set by an external pin for instance).
    // NOTE 2: methods are not pure virtual because some classes won't need them, BUT the sequencer will call
    // update, action and refreshStates for all of them.

    // ***** UPDATE *****
    // By default, update uses the value of the previous module and the current state variables.
    // This method is *not* virtual and in general not overloaded.
    virtual void update()
    {
        if (active)
        {

            bool inputFrom = false;
            if (ptr_fromModule != NULL)                       // NOTE: if the module is not connected, inputFrom defaults to NULL.
                inputFrom = ptr_fromModule->getOutputState(); // get the output of the preceding module at time t-1

            computeNextState(inputFrom); // will compute nextOutput, nextInput and nextState

            if (firstTime)
            {
                firstTime = false;
                firstTimeAction();
            }
            else
            {
                // ATTN: perform the action all the time, or only when the output state changed?
                // For the time being, I will do the latest thing:
                //if (output != nextOutput)
                action();
            }

        } // otherwise state and output does not change, and action() is not performed.
    }

    // ***** EVOLVE *****
    virtual void computeNextState(bool _inputFrom) { nextOutput = _inputFrom; } // default to pass

    // ***** ACTION *****
    // Whatever the module needs to do (unrelated to the state evolution), using the input and any state variable.
    // Not pure virtual because some children won't use (but not non-virtual, otherwise calling from the
    // basew pointer will just exectute the one declared/defined here)
    virtual void action() {}
    virtual void firstTimeAction() { action(); } // by default, it is just the same than action()

    // ***** REFRESH *****
    //This method is necessary so that the modules are updated in time steps, but "in parallel".
    void refreshStates()
    {
        output = nextOutput;
        input = nextInput;
        state = nextState;
    }

  protected:
    bool firstTime;
    bool active;

    bool output, nextOutput;
    bool state, nextState;
    bool input, nextInput;

    Module *ptr_fromModule;
};

/* ==================================================================================================
                            REAL MODULES (children of base class)
===================================================================================================*/

class Clock : public Module
{
    /*
     NOTE : This is basically a "slow" 50%-duty PWM generator.
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

    NOTE: the clock is an example of a module that *could* not use an internal state. However, I will use it in case
        we want to do some action only when there is a change of state.

    */

  public:
    Clock() { init(); }
    Clock(uint32_t _periodUs)
    {
        periodUs = _periodUs;
        init();
    }

    void init() override
    {
        myID = id_counter;
        id_counter++;
        myClassIndex = Definitions::ClassIndexes::CLASSID_CLK;
        myName = Definitions::classNames[myClassIndex];
        active = false; // default state is true for most modules, but the clock is special: we may want to start
                        // the clock in synch with other clocks. Btw, the clock can be TOGGLED BY AN INPUT TRIGGER!
    }

    void setPeriodUs(uint32_t _periodUs)
    {
        periodUs = _periodUs;
        reset();
    }
    uint32_t getperiodUs() { return (periodUs); }

    // ******************** Overriden/Overloaded METHODS OF THE BASE CLASS ***********************

    void reset() override
    {
        Module::reset();       // call the base reset()
        clockTimer = micros(); // reset of things proper to this child class.
    }

    String getName() { return (myName + "[" + String(myID) + "]"); }
    String getParamString()
    {
        return ("{state=" + Definitions::binaryNames[active] + ", period=" + String(periodUs) + "us}");
    }

    // **** EVOLUTION ****
    void computeNextState(bool _inputFrom) override
    {
        if (micros() - clockTimer > periodUs)
        {
            // we could do nextOutput =!output, but this is for clarity (other logical function could be performed):
            nextState = !state;

            clockTimer = micros();

            // And the nextOutput is:
            nextOutput = nextState;
        }
    }

    void action() override
    {
        digitalWrite(PIN_LED_DEBUG, output); // -test-
    }

    // ==================================================================================================

  private:
    String myName;
    uint8_t myClassIndex;
    uint8_t myID;
    static uint8_t id_counter;

    uint32_t periodUs = 1000; // in ms
    uint32_t clockTimer;      // in us
};

// NOTE: this is a stateless module, and it does not even depends on the previous module (btw, there
// should not be connected to anything before it...)
class InputTrigger : public Module
{
  public:
    InputTrigger() : inputTriggerPin(PIN_TRIGGER_INPUT) // default input pin (so we can use it right away)
    {
        // NOTE: the base class init method is called during construction.
        init();
    }
    InputTrigger(uint8_t _inputPin)
    {
        inputTriggerPin = _inputPin;
        init();
    }

    // Child class init method: only things other than what the base class init does, in particular the
    // myClassIndex and myName HAVE to be properly (re)set for each class type (see Utils::className).
    // => DO NOT FORGET to set the class index in the Laser class! (or any other not in Class_Sequencer.h that
    // is a child of Module and can be used in the sequencer pipeline)
    void init() override
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

        reset(); // in this case this is equal to Module::reset() since the reset method is not oveloading here.
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

    // **** EVOLUTION ****
    void computeNextState(bool _inputFrom) override // actually the input from won't be set from the input module,
    // here, unless (for some weird reason) the input trigger has an input module. In that case, the value is
    // ignored.
    {
        // everything stays the same, but nextOutput is updated (statelessly):
        nextOutput = readInput();
    }

    void action() override
    { // in this case, the action() method is mostly for tests.
        //digitalWrite(PIN_LED_MESSAGE, output);
    }

    // *************************************************************************************************

  private:
    String myName;
    uint8_t myClassIndex;
    uint8_t myID;
    static uint8_t id_counter;
    uint8_t inputTriggerPin;
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

    void init() override
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
    void setOutput(bool _out)
    {
        digitalWrite(outputTriggerPin, _out);
        digitalWrite(PIN_LED_MESSAGE, _out); // -test-
    }

    // ******************** OVERRIDDEN METHODS OF THE BASE CLASS **************************************
    String getName() { return (myName + "[" + String(myID) + "]"); }
    String getParamString() { return ("{ pin=" + String(outputTriggerPin) + "}"); }

    // **** EVOLUTION ****
    void computeNextState(bool _inputFrom) override
    {
        nextOutput = _inputFrom;
    }

    void action() override
    { // NOTE: state can be used to avoid calling the action all the time, by checking if the output has actually
        // changed:
        //if (state!=nextState)
        setOutput(output);
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

    void init() override
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

    void reset() override
    {
        Module::reset();
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
        return ("{mode=" + Definitions::trgModeNames[mode] + ", burst=" + String(burstLength) + ", skip=" + String(skipLength) + ", offset=" + String(offsetEvents) + "}");
    }

    void action() override
    {
        // digitalWrite(PIN_LED_MESSAGE, output); // -test-
    }

    void computeNextState(bool _inputFrom) override
    {
        nextInput = _inputFrom;

        // ATTN: remember we want a "bang" only, so if nothing happens,
        // the next output state HAS to be false:
        nextOutput = false;

        //  First, detect an event ("event" is a detected rise, fall or change on state of input module)
        bool event = false;
        switch (mode)
        {
        case 0: // RISE
            event = (!input) && nextInput;
            break;
        case 1: // FALL
            event = input && (!nextInput);
            break;
        case 2: // CHANGE
            // "^" is an XOR (same than "!="" for binary arguments)
            event = (input ^ nextInput);
            break;
        default:
            break;
        }

        // Then, the output ("output" is the result of the processed event stream (burst, skip and offset)
        if (event)
        {
            // Serial.println("number rises: " + String(counterEvents + 1));

            if (stateMachine == BURST_STATE)
            {
                if (counterEvents < burstLength)
                {
                    nextOutput = true;
                }
                else
                {
                    // Serial.println("END BURST"); // -test-
                    stateMachine = SKIP_STATE;
                    nextOutput = false; //<-- not necessary... but I leave it for clarity.
                    counterEvents = 0;  // NOTE offset is only applied at start (or reset).
                }
            }

            // NOTE: state could have changed here, hence I don't use a switch/case!

            if (stateMachine == SKIP_STATE)
            {
                if (counterEvents < skipLength)
                {
                    nextOutput = false; //<-- not necessary... but I leave it for clarity.
                }
                else
                {
                    stateMachine = BURST_STATE;
                    nextOutput = true;
                    counterEvents = 0;
                }
            }

            // Increment number events detected by the trigger:
            counterEvents++;
        }
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

    void init() override
    {
        myID = id_counter;
        id_counter++;
        myClassIndex = Definitions::ClassIndexes::CLASSID_PUL;
        myName = Definitions::classNames[myClassIndex];
        reset();
    }

    void reset() override
    {
        Module::reset();
        timerPulsar = millis(); // +t_off_ms + t_on_ms; // this is to avoid
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

    // ******************** OVERRIDEN METHODS OF THE BASE CLASS ***********************
    void computeNextState(bool _inputFrom) override
    {
        if (_inputFrom) // reset timer (only!)
            timerPulsar = millis();

        // Output of the Pulsar: ON or OFF (in the future, it can be a struct also
        // containing an analog value - e.g. power ramps)
        uint32_t timePassed = millis() - timerPulsar;
        nextOutput = (timePassed > t_off_ms) && (timePassed <= (t_off_ms + t_on_ms));
    }

    void action() override
    {
        digitalWrite(PIN_LED_MESSAGE, output); // -test-
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
