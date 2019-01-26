#ifndef _Class_OptoTune_H_
#define _Class_OptoTune_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"

// ===========================================================================================================

class OptoTune
{

	uint16_t power;

  public:
	OptoTune(){};
	OptoTune(uint8_t _pinPower)
	{
		init(_pinPower);
	}

	void init(uint8_t _pinPower)
	{
		// NOTE: be careful when assigning the PWM pins here: they should be on the
		// same flexi-timer so we can set the same PWM frequency. For instance, for pins
		// 29, 30, this is FTM2.
		//analogWriteFrequency(pinPower, FREQ_PWM_OPTOTUNE); // not done here: this is set in the
		// proper OptoTunes namespace.
		pinPower = _pinPower;
		setStatePower(defaultPower);
	}

	void setStatePower(uint16_t _power)
	{
		power = constrain(_power, 0, MAX_OPTOTUNE_POWER);
		analogWrite(pinPower, power);
	}

	void setPower(uint16_t _power)
	{ // without change the current state (this is to avoid the state stack)
		analogWrite(pinPower, constrain(_power, 0, MAX_OPTOTUNE_POWER));
	}

	void setToCurrentState()
	{
		setStatePower(power);
	}

  private:
	uint8_t pinPower;
	const uint16_t defaultPower = MAX_OPTOTUNE_POWER >> 1; // middle of range
};

#endif
