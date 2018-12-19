#include "Class_Laser.h"
uint8_t Laser::myID = 0; // we should start by 1 (I will use this for trigger selection)

Laser::Laser() { myID++; };
Laser::Laser(uint8_t _pinPower, uint8_t _pinSwitch)
{
	init(_pinPower, _pinSwitch);
	trigger.setTriggerState(Utils::TriggerState::TRIG_EVENT_NONE);
	myID++;
}

void Laser::setSwitch(bool _state)
{
	// Note: if in carrier mode, this action will be IGNORED unless we force
	// pinMode OUTPUT (it will reverse to PWM when issuing an analoWrite command)
	pinMode(pinSwitch, OUTPUT);
	digitalWrite(pinSwitch, _state);

	// *** TODO ***
	// separate properly the carrier switch from the on/off switch!!!
}

void Laser::init(uint8_t _pinPower, uint8_t _pinSwitch)
{
	pinPower = _pinPower;   // this is analog output (PWM). Does not need to be set (pinMode)
	pinSwitch = _pinSwitch; // this is a digital output, but can be used as PWM (carrier)

	//pinMode(_pinPower, OUTPUT); // <<-- no need, it will be used exclusively as a PWM signal
	//pinMode(_pinSwitch, OUTPUT); // <--- done in setCarrierMode method (if carrier off)

	// setCarrierMode(false); // by default it will *not* be set to "carrier" mode, so the pinSwitch
	// will be set as OUTPUT

	//setStatePower(MAX_LASER_POWER/2); // half power...
	//setStateSwitch(false);			 // but switched off

	// Set the default laser state:
	// setState(defaultState); //... now using C++11 member initialization method

	// clear the state stack
	clearStateStack();
}

void Laser::setCarrierMode(bool _carrierMode)
{   // note: the carrier is a square wave (pwm, 50% or adjusted at 58% or so
	// to account for an asymetry between the time it takes to switch ON and switch OFF of the lasers,
	// with a pwm frequency that is tunneable - by default 100kHz).
	// It "modulates" the current laser power (another PWM per-laser at FREQ_PWM_CARRIER of
	// about 70kHz, with a low pass filter)

	myState.carrierMode = _carrierMode; // to be able to quickly read the laser mode from its struct state
	
	if (!_carrierMode)
	{
		pinMode(pinSwitch, OUTPUT); // by doing this, we re-enable digital control.
		//digitalWrite(pinSwitch, LOW); // set to LOW when stopping PWM cycles? ** TO DO ** : separate carrier from switching
		digitalWrite(pinSwitch, myState.state);
	}
	else
	{
		// note: the resolution of all PWM signals is RES_PWM, defaulting to 12 (see "Definitions.h"), or 4095
		analogWrite(pinSwitch, 0.58 * 2048); // not exactly 50% because of the different time it takes to switch laser ON or OFF
	}
}

void setSequenceMode(bool _seqMode)
	{
		myState.sequenceMode = _seqMode; // to be able to quickly read the laser mode from its struct state
		mySequencer.setRunningMode(_seqMode);
	}

void Laser::updateBlank()
{
	if (myState.blankingMode)
	{
		pinMode(pinSwitch, OUTPUT); //setToCurrentState will make it go back to PWM if necessary
		digitalWrite(pinSwitch, LOW);
		// ATTN: no change to current state!!
	}
}

void Laser::setToCurrentState()
{
	setCarrierMode(myState.carrierMode); // done before setting the switch state, as it will do pinMode output IF carrier off
	setSequenceMode(myState.sequenceMode)
	digitalWrite(pinSwitch, myState.state);
	analogWrite(pinPower, myState.power);
	setBlankingMode(myState.blankingMode);
}

