#ifndef _Class_Laser_H_
#define _Class_Laser_H_

#include "Arduino.h"
#include "Definitions.h"
#include <vector>
#include "Class_Sequencer.h"
#include "Utils.h"

// ===========================================================================================================

// There will be several lasers, so I do a class instead of a namespace.
// NOTE : the group of lasers is not an object itself, but a namespace. Anotehr option
// would have been to make the group of laser (array and methods) static methods and variables
// (in that case, the array could be dynamic - a vector)
class Laser
{
	static uint8_t myID; // automatically incremented at instantiation (so we can declare a laser array)

  public:
	// Public struct to store laser state (I will use a
	// stack to avoid having to save the state whenever we want to try something
	// or do some complex drawing - similar to "pushStyle" in OF or Processing)
	struct LaserState
	{
		uint16_t power;		// 0-MAX_LASER_POWER
		bool state;			// on/off
		bool carrierMode;   // chopper mode at FREQ_PWM_CARRIER
		bool sequencerMode; // this will activate the sequence mode, whose parameters are in the member variable mySequencer.
		// NOTE1: this variable seems redundant, but it is done to be able to quickly read the laser mode and all other laser states
		// NOTE2: carrier mode is independent of the sequence mode (meaning that in the ON state, the laser is still
		// modulated at the carrier frequency)
		int8_t triggerSource; // to identify the source to update myTrigger (0 for external, 1-4 for the other laser-states)
		bool blankingMode;	// blank between each figure (for the time being, end of trajectory buffer).
							  // NOTE: this is NOT the inter-point blanking, which - for the time being - is a property
							  // common to all lasers and could be a static class variable (but now is a DisplayScan variable).
	};

	Laser();
	Laser(uint8_t _pinPower, uint8_t _pinSwitch, int8_t _triggerSource);

	void init(uint8_t _pinPower, uint8_t _pinSwitch, uint8_t _triggerID);

	// Low level methods not affecting the current LaserState (myState) - useful for tests.
	void setSwitch(bool _state);
	void setPower(uint16_t _power);
	void setToCurrentState(); // in case we changed the state by directly accessing the myState variable (could made all private though)

	// Methods similar to the above, but affecting the current LaserState variable myState
	void setStateSwitch(bool _state);
	void setStatePower(uint16_t _power);
	void setState(LaserState _state);
	void resetState(); // reset to default state and activate it
	void setCarrierMode(bool _carrierMode);
	void setSequencerMode(bool _seqMode);
	void setBlankingMode(bool _blankingMode);

	void setTriggerSource(int8_t _triggerSource);
	void setTriggerMode(Trigger::TriggerMode _triggerMode); // for the time being, no getTriggerMode method
	void setTriggerOffset(uint8_t _offset);

	int8_t getTriggerSource();
	Trigger::TriggerMode getTriggerMode();
	uint8_t getTriggerOffset();

	void setSequencerParam(uint16_t _t_delay_us, uint16_t t_on_us, uint16_t _eventDecimation);

	bool getStateSwitch();
	uint16_t getStatePower();
	bool getSequencerMode();

	LaserState getLaserState();

	void pushState();
	void popState();
	void clearStateStack();

	// Update/read methods:
	bool updateReadTrigger(bool _newInput);
	bool updateReadSequencer(bool _event);
	void updateBlank(); // will switch off the laser if the blankingMode is true

	LaserState myState{defaultState}; // C++11 class member initialization (I define defaultState in case we want to revert to default):

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
		false, // switch (on/off)
		false, // carrier mode (on/off)
		false, // sequence mode (on/off)
		false  // blancking mode (on/off)
	};

	std::vector<LaserState> laserState_Stack;
	Trigger myTrigger;
	Sequencer mySequencer;

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
