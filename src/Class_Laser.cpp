#include "Class_Laser.h"

uint8_t Laser::myID = 0; // we should start by 1 (I will use this for trigger selection)

Laser::Laser() { myID++; };
Laser::Laser(uint8_t _pinPower, uint8_t _pinSwitch)
{
	init(_pinPower, _pinSwitch);
	myID++;
}

void Laser::init(uint8_t _pinPower, uint8_t _pinSwitch)
{
	pinPower = _pinPower;   // this is analog output (PWM). Does not need to be set (pinMode)
	pinSwitch = _pinSwitch; // this is a digital output, but can be used as PWM (carrier)

	//pinMode(_pinPower, OUTPUT);  // <-- no need, it will be used exclusively as a PWM signal
	//pinMode(_pinSwitch, OUTPUT); // <--- done in setStateCarrier method (if carrier off)

	// Set the default laser state:
	// setState(defaultState); //... now using C++11 member initialization method

	resetSequencer();
	clearStateStack();
}

void Laser::restartSequencer() {
	resetTrigger();
	resetSequencer();
}

void Laser::setSwitch(bool _state)
{
	if (!_state)
	{
		// Note: if in carrier mode, digitalWrite will be IGNORED unless we force
		// pinMode OUTPUT (it will reverse to PWM when issuing an analoWrite command)
		if (myState.stateCarrier)
			pinMode(pinSwitch, OUTPUT);
		digitalWrite(pinSwitch, LOW);
	}
	else
	{
		// in case digital carrier is on, the following is ignored but this is what we want (modulation on). 
		// If carrier is OFF, then it means we are in pinmode OUTPUT already, so this will not be ignored. 
		digitalWrite(pinSwitch, HIGH);
	}
}

void Laser::setPower(uint16_t _power) { analogWrite(pinPower, _power); }

void Laser::setStateSwitch(bool _state)
{
	// Note: if in carrier mode, this action will be IGNORED (but the state changes)
	myState.stateSwitch = _state;
	setSwitch(_state);
}
bool Laser::getStateSwitch() { return (myState.stateSwitch); }

void Laser::setStatePower(uint16_t _power)
{
	myState.power = _power;
	analogWrite(pinPower, _power);
}
uint16_t Laser::getStatePower() { return (myState.power); }

void Laser::setStateCarrier(bool _stateCarrier)
{ // note: the carrier is a square wave (pwm, 50% or adjusted at 58% or so
	// to account for an asymetry between the time it takes to switch ON and switch OFF of the lasers,
	// with a pwm frequency that is tunneable - by default 100kHz).
	// It "modulates" the current laser power (another PWM per-laser at FREQ_PWM_CARRIER of
	// about 70kHz, with a low pass filter)

	myState.stateCarrier = _stateCarrier; // to be able to quickly read the laser mode from its struct state

	if (!_stateCarrier)
	{
		pinMode(pinSwitch, OUTPUT); // by doing this, we re-enable digital control.
		digitalWrite(pinSwitch, myState.stateSwitch);
	}
	else
	{
		// Note: the resolution of all PWM signals is RES_PWM, defaulting to 12 (see "Definitions.h"), or 4095
		analogWrite(pinSwitch, 0.58 * 2048); // not exactly 50% because of the different time it takes to switch laser ON or OFF
	}
}
bool Laser::getStateCarrier() { return (myState.stateCarrier); }

void Laser::setStateSequencer(bool _seqState) { myState.stateSequencer = _seqState; }
bool Laser::getStateSequencer() { return (myState.stateSequencer); }

void Laser::setStateBlanking(bool _blankingMode) { myState.stateBlanking = _blankingMode; }
bool Laser::getStateBlanking() { return (myState.stateBlanking); }
void Laser::updateBlank()
{
	if (myState.stateBlanking)
	{
		// ATTN: no change to current state, so setToCurrentState will make it go back to PWM carrier if necessary
		pinMode(pinSwitch, OUTPUT); 
		digitalWrite(pinSwitch, LOW);
	}
}

void Laser::setState(LaserState _state)
{
	myState = _state;
	setToCurrentState();
}

void Laser::resetState()
{ // revert to default state (without clearing the state stack)
	setState(defaultState);
}

void Laser::setToCurrentState()
{
	setStateSwitch(myState.stateSwitch);
	setStateCarrier(myState.stateCarrier);
	setStatePower(myState.power);
	setStateSequencer(myState.stateSequencer);
	setStateBlanking(myState.stateBlanking);
}
Laser::LaserState Laser::getCurrentState() { return (myState); }

void Laser::pushState() { laserState_Stack.push_back(myState); }
void Laser::popState()
{
	setState(laserState_Stack.back());
	laserState_Stack.pop_back();
}
void Laser::clearStateStack() { laserState_Stack.clear(); }
