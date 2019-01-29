#include "Class_Laser.h"

uint8_t Laser::id_counter = 0;

Laser::Laser()
{
	init();
}

void Laser::init()
{
	myID = id_counter;
	id_counter++;
	myClassIndex = Definitions::ClassIndexes::CLASSID_LAS; // TODO: make these variables STATIC!!
	myName = Definitions::classNames[myClassIndex];

	// Default active = true for sequencer operation, but won't interfere with anything if the laser "module" is not linked
	// to anything in the sequencer pipeline:
	active = true;

	clearStateStack();

	// Set the default laser state:
	// setState(defaultState); //... now using C++11 member initialization method
}

Laser::Laser(uint8_t _pinPower, uint8_t _pinSwitch)
{
	init(_pinPower, _pinSwitch);
}

void Laser::init(uint8_t _pinPower, uint8_t _pinSwitch)
{

	pinPower = _pinPower;   // this is analog output (PWM). Does not need to be set (pinMode)
	pinSwitch = _pinSwitch; // this is a digital output, but can be used as PWM (carrier)
	init();
}

String Laser::getName() { return (myName+ "[" + String(myID) + "]("+Definitions::laserNames[myID]+")" ); }
// ATTN overloaded from Module base class for sequencer user: =================================
String Laser::getParamString() // used for sequencer stuff. TODO unify with the above...
{
	String param = "{ power=" + String(myState.power) + ", carrier=" + Definitions::binaryNames[myState.stateCarrier] + ", blank=" + Definitions::binaryNames[myState.stateBlanking] + "}";
	return (param);
}
//  ==========================================================================================

void Laser::setSwitch(bool _state)
{
	if (myState.stateCarrier)
	{
		if (!_state)
		{
			// Note: if in carrier mode, digitalWrite will be IGNORED unless we force
			// pinMode OUTPUT (it will reverse to PWM when issuing an analogWrite command)
			pinMode(pinSwitch, OUTPUT);
			digitalWrite(pinSwitch, LOW);
		}
		else
			analogWrite(pinSwitch, 0.58 * 2048);
	}
	else
	{
		digitalWrite(pinSwitch, _state);
	}
}

void Laser::setPower(uint16_t _power)
{
	analogWrite(pinPower, constrain(_power, 0, MAX_LASER_POWER));
}

void Laser::setStateSwitch(bool _state)
{
	if (myState.stateSwitch != _state)
	{ // we will only set the state if it changed, to avoid useless time consuming calls.
		myState.stateSwitch = _state;
		setSwitch(_state);
	}
}
bool Laser::getStateSwitch() { return (myState.stateSwitch); }

void Laser::toggleStateSwitch()
{
	myState.stateSwitch = !myState.stateSwitch;
	setSwitch(myState.stateSwitch);
}

void Laser::setStatePower(uint16_t _power)
{
	myState.power = constrain(_power, 0, MAX_LASER_POWER);
	analogWrite(pinPower, _power);
}
uint16_t Laser::getStatePower() { return (myState.power); }

void Laser::setStateCarrier(bool _stateCarrier)
{ // NOTE: the carrier is a square wave (pwm, 50% or adjusted at 58% or so
	// to account for an asymmetry between the time it takes to switch ON and switch OFF of the lasers,
	// with a pwm frequency that is tuneable - by default 100kHz).
	// It "modulates" the current laser power (another PWM per-laser at FREQ_PWM_CARRIER of
	// about 70kHz, with a low pass filter)
	myState.stateCarrier = _stateCarrier;
	setSwitch(myState.stateSwitch);
}
bool Laser::getStateCarrier() { return (myState.stateCarrier); }

void Laser::setStateBlanking(bool _blankingMode) { myState.stateBlanking = _blankingMode; }
bool Laser::getStateBlanking() { return (myState.stateBlanking); }
void Laser::updateBlank()
{
	if (myState.stateBlanking)
	{
		setSwitch(LOW);
	}
	// ATTN: no change to current state, so setToCurrentState will make it go back to PWM carrier if necessary
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
