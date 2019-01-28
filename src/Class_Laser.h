#ifndef _Class_Laser_H_
#define _Class_Laser_H_

#include "Arduino.h"
#include "Definitions.h"
#include <vector>
#include "Class_Sequencer.h"
#include "Utils.h"

// ===========================================================================================================

// There will be several lasers, so I do a class instead of a namespace.
// NOTE : the group of lasers is not an object itself, but a namespace. Another option
// would have been to make the group of laser (array and methods) static methods and variables
// (in that case, the array could be dynamic - a vector)
class Laser : public Module
{

  public:
	// Public struct to store laser state (I will use a
	// stack to save/retrieve the state whenever we want to try something
	// or do some complex drawing - similar to "pushStyle" in OF or Processing)
	struct LaserState
	{
		uint16_t power;	// 0-MAX_LASER_POWER
		bool stateSwitch;  // on/off
		bool stateCarrier; // chopper mode at FREQ_PWM_CARRIER
		// NOTE: carrier mode is independent of the sequence mode (meaning that in the ON state, the laser is still
		// modulated at the carrier frequency)
		bool stateBlanking; // blank between each figure (for the time being, end of trajectory buffer).
							// NOTE: this is NOT the inter-point blanking, which - for the time being - is a property
							// common to all lasers and could be a static class variable (but now is a DisplayScan variable).
	};

	Laser();
	Laser(uint8_t _pinPower, uint8_t _pinSwitch);
	void init(uint8_t _pinPower, uint8_t _pinSwitch);

	// ATTN overloaded from Module base class for sequencer user: =================================
	virtual String getParamString();
	virtual bool getState() { return (myState.stateSwitch); } // the same than getStateSwitch() in fact
	virtual void action() { setStateSwitch(state); }		  // reminder: state is a variable of the base class
	//  ==========================================================================================

	// Low level methods not affecting the current LaserState (myState) - useful for tests.
	void setSwitch(bool _state);
	void toggleStateSwitch(); // will be useful for the sequencer.
	void setPower(uint16_t _power);
	void setToCurrentState(); // in case we changed the state by directly accessing the myState variable
							  //(could made all private though)

	// Methods similar to the above, but affecting the current LaserState variable myState
	void setStateSwitch(bool _state);
	void setStatePower(uint16_t _power);
	void setStateCarrier(bool _stateCarrier);
	void setStateSequencer(bool _seqMode);
	void setStateBlanking(bool _blankingMode);

	// Setting all the variables simultaneously:
	void setState(LaserState _state);
	void resetState(); // reset to default state and activate it

	bool getStateSwitch();
	uint16_t getStatePower();
	bool getStateCarrier();
	bool getStateSequencer();
	bool getStateBlanking();

	LaserState getCurrentState();

	void pushState();
	void popState();
	void clearStateStack();

	// Update/read methods:
	void updateBlank(); // will switch off the laser if the stateBlanking is true

  private:
	LaserState myState{defaultState}; // C++11 class member initialization (I define defaultState in case we want to revert to default):

	uint8_t pinPower, pinSwitch;

	// Default laser state:
	// NOTE: for the time being, inter-point blanking is a variable of DisplayScan, so it concern
	// all the lasers at the same time.
	const LaserState defaultState = {
		2000,  // power (0-4095)
		false, // switch (on/off)
		false, // carrier mode (on/off)
		false  // blancking mode (on/off)
	};

	std::vector<LaserState> laserState_Stack;

	static uint8_t id_counter; // automatically incremented at instantiation

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
