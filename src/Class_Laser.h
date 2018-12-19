#ifndef _Class_Laser_H_
#define _Class_Laser_H_

#include "Arduino.h"
#include "Definitions.h"
#include <vector>
#include "Utils.h"

// ===========================================================================================================

// There will be several lasers, so I do a class instead of a namespace.
// NOTE : the group of lasers is not an object itself, but a namespace. Anotehr option
// would have been to make the group of laser (array and methods) static methods and variables
// (in that case, the array could be dynamic - a vector)
class Laser {
	static uint8_t myID;

  public:
	// I will create a struct to store laser state: the reason is I will use a
	// stack to avoid having to save the state whenever we want to try something
	// or do some complex drawing (very similar to "pushStyle" in OF or
	// Processing)
	struct LaserState {
		uint16_t power;		// 0-MAX_LASER_POWER
		bool state; 		// on/off
		bool carrierMode;   // chopper mode at FREQ_PWM_CARRIER
		bool sequenceMode; 	// this will activate the sequence mode, whose parameters are in a variable sequenceParam
		bool blankingMode;  // blank between each figure (for the time being, end of trajectory buffer).
		// NOTE: this is NOT the inter-point blanking, which - for the time being - is a property
		// common to all lasers and could be a static class variable (but now is a DisplayScan variable).
	};

	// Sequence parameters to use when in sequence mode. Note that the updateSequence method is a method of the
	//namespace Hardware::Lasers, because we may need to check the states of all the laser objects instantiated.
	struct SequenceParam {
		uint16_t t_delay, t_on; // eventually t_off too
		uint8_t triggerID; // the trigger to 
		Utils::TriggerMode triggerMode;
		uint16_t triggerDecimation; // the number of trigger pulses that correspond to one cycle of the sequence
	};

	Laser() {myID++;};
	Laser(uint8_t _pinPower, uint8_t _pinSwitch) {
		init(_pinPower,_pinSwitch);
		trigger.setTriggerState(Utils::TriggerState::TRIG_EVENT_NONE);
		myID++;
	}

	void init(uint8_t _pinPower, uint8_t _pinSwitch);

	// Low level methods not affecting the current LaserState (myState):
	void setSwitch(bool _state);

	void setPower(uint16_t _power) {
		analogWrite(pinPower, _power);
	}

	void setStateSwitch(bool _state) {
		// Note: if in carrier mode, this action will be IGNORED (but the state changes)
		myState.state = _state;
		setSwitch(_state);
	}

	bool readStateSwitch() {
		return(myState.state);
	}

	void setStatePower(uint16_t _power) {
		myState.power = _power;
		analogWrite(pinPower, _power);
	}

	void setCarrierMode( bool _carrierMode);

	void setBlankingMode( bool _blankingMode) {
		myState.blankingMode = _blankingMode;
	}

	void updateBlank();

	void setState(LaserState _state) {
		myState =_state;
		setToCurrentState();
	}
	
	bool setToCurrentState();

	void resetState() { // revert to default state (without clearing the state stack)
		setState(defaultState);
	}

	LaserState getLaserState() {
		return(myState);
	}

	void pushState() {
		laserState_Stack.push_back(myState);
	}
	void popState() {
		setState(laserState_Stack.back());
		laserState_Stack.pop_back();
	}

	void clearStateStack() {
		laserState_Stack.clear();
	}

	void setSequenceMode(bool _seqMode) {
		 // NOTE: carrier mode is independent of the sequence mode (meaning that in the 
		 // ON state, the laser is still modulated at the carrier frequency)
		 myState.sequenceMode = _seqMode; 
	}

	LaserState myState{defaultState};  // C++11 class member initialization (I  define defaultState in case we want to revert to default):
	Sequencer mySequencer{defaultSequence};
	
  private:

	// NOTE: methods that affect all the objects from this class could access
	// all the objects by having a static vector<> or a static array and a static
	// index counter. However, since all the Hardware namespaces are in one place,
	// instead of using static class methods, I will have extern functions in the
	// Laser namespace (hence the commenting of the following lines)
	// // Set Power PWM frequency and resolution:
	// static void setPowerFrequencyPWM(uint16_t); // affect all power-pwm frequencies
	// static void setPowerResolutionPWM(uint8_t); // affects all power-pwm lasers
	//
	// // Carrier (when used). NOTE: it could be something different from a square wave, 50% duty ratio!
	// static void setCarrierFrequencyPWM(uint16_t);
	// static void setCarrierResolutionPWM(uint8_t _resBits = 10); // in bits.
	// static void setCarrierDutyCycle(uint8_t _val = 512);

	uint8_t pinPower, pinSwitch;

	// Default laser state: [ half power, switched OFF, carrier OFF, inter-figure blanking ON]
	// NOTE: for the time being, inter-point blanking is a variable of DisplayScan, so it concern
	// all the lasers at the same time.
	const LaserState defaultState = {
		2000,  // power (0-4095)
		false, // carrier mode (on/off)
		false, // sequence mode (on/off)
		false  // blancking mode (on/off)
		};

	const SequenceParam defaultSequence = {
		0, // delay time (in ms)
		5, // time on (in ms)
		0, // trigger ID (0 for the camera, 1, 2, 3 for the lasers - )
		Utils::TRIG_RISE,
		1 // number of trigger events to restart the sequence
	};

	std::vector<LaserState> laserState_Stack;

	/* NOTES:
	- Even if the laser has analog control, a digital pin may be used for fast
	blanking or PWM modulation at 50% duty cycle (carrier), hence it would be
	good in the future to have two pins for the laser.
	- If the modulation is based on PWM, then pinDAC will not be used - analog
	level is retrieved by filtering the square signal.
	- In PWM "analog" mode, there could be an additional carrier frequency, at
	a much higher frequency; it would be complicated to do this using just one
	Timer, better to use TWO pwm signals and a simple electronic switch to chop
	the signal (when ON). This is reminiscent of IrDA protocol.
	*/
};

#endif
