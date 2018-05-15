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

	public:

		// I will create a struct to store laser state: the reason is I will use a
		// stack to avoid having to save the state whenever we want to try something
		// or do some complex drawing (very similar to "pushStyle" in OF or
		// Processing)
		struct LaserState {
		uint16_t power;	// 0-MAX_LASER_POWER
		bool state; 		// on/off

		bool carrierMode;   // chopper mode at FREQ_PWM_CARRIER
		bool blankingMode;  // blank between each figure (for the time being, end of trajectory buffer).
		// NOTE: this is NOT the inter-point blanking, which - for the time being - is a property
		// common to all lasers and could be a static class variable (but now is a DisplayScan variable).
	};

	Laser() {};
	Laser(uint8_t _pinPower, uint8_t _pinSwitch) {
		init(_pinPower,_pinSwitch);
	}

	void init(uint8_t _pinPower, uint8_t _pinSwitch) {
		pinPower = _pinPower;
	  pinSwitch = _pinSwitch;

		//pinMode(_pinPower, OUTPUT); // <<-- no need, it will be used exclusively as a PWM signal
		//pinMode(_pinSwitch, OUTPUT); // <--- done in setCarrierMode method

		// setCarrierMode(false); // by default it will *not* be set to "carrier" mode, so the pinSwitch
		// will be set as OUTPUT

		//setStatePower(MAX_LASER_POWER/2); // half power...
		//setStateSwitch(false);			 // but switched off

		// Set the default laser state:
		setState(defaultState);

		// clear the
		clearStateStack();
	}

	// Low level methods not affecting the current LaserState (myState):
	void setSwitch(bool _state) {
		// Note: if in carrier mode, this action will be IGNORED unless we force
		// pinMode OUTPUT (it will reverse to PWM when issuing an analoWrite command)
		pinMode(pinSwitch, OUTPUT);
		digitalWrite(pinSwitch, _state);
	}

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

	void setCarrierMode( bool _carrierMode) {
		myState.carrierMode = _carrierMode;
		if (!_carrierMode) {
			pinMode(pinSwitch, OUTPUT); // by doing this, we re-enable digital control.
			//digitalWrite(pinSwitch, LOW); // set to LOW when stopping PWM cycles?
		} else {
			analogWrite(pinSwitch, 0.58*MAX_LASER_POWER);//MAX_LASER_POWER>>1); // restart a 50% PWM generation if needed
		}
	}

	void setBlankingMode( bool _blankingMode) {
		myState.blankingMode = _blankingMode;
	}

	void updateBlank() {
		if (myState.blankingMode) {
			pinMode(pinSwitch, OUTPUT); //setToCurrentState will make it go back to PWM if necessary
			digitalWrite(pinSwitch, LOW);
		// ATTN: no change to current state!!
	}
	}

	void setState(LaserState _state) {
		myState=_state;
		setToCurrentState();
	}

	LaserState getLaserState() {return(myState);}

	bool setToCurrentState() {
		digitalWrite(pinSwitch, myState.state);
		analogWrite(pinPower, myState.power);
		setCarrierMode(myState.carrierMode);
		setBlankingMode(myState.blankingMode);
		return(myState.state);
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

	LaserState myState;

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

	// Default laser state: [ half power, switched ON, carrier OFF, inter-figure blanking ON]
	// NOTE: for the time being, inter-point blanking is a variable of DisplayScan, so it concern
	// all the lasers at the same time.
	const LaserState defaultState = {2000, true, false, true};

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
