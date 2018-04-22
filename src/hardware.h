#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"
#include "scannerDisplay.h"


// ============ LOW LEVEL HARDWARE METHODS ================================
// * NOTE 1: make the critic code inline!

namespace Hardware {

	// Hardware initialization (Gpios, Laser and scanners:)
	extern void init();

	//Software reset: better than using the RST pin (noisy?)
	extern void resetBoard();

	extern void blinkLed(uint8_t _pinLed, uint8_t _times);
	extern void blinkLedDebug(uint8_t _times);
	extern void blinkLedMessage(uint8_t _times);

	namespace Gpio {

		void init();

		// Other digital pins
		inline void setDigitalPin(uint8_t _pin, bool _state) {
			// TODO: for the time being, it is up to the user to check if the pin is set as OUTPUT, capable of that
			// and not conflicting with the used pins? we could do such tests here.
			digitalWrite(_pin, _state);
		}

		// Change PWM frequency:
		extern void setPWMFreq(uint16_t _freq);

		// Change PWM duty cycle: just use analogWrite() on Teensy. I will wrap it
		// because we may need to change the hardware, plus this will not be called
		// very often [it is not the laser ON/OFF, but its power]:
		inline void setPWMDuty(uint8_t _pin, uint16_t _duty) {analogWrite(_pin, _duty);}
	}

	namespace Lasers {

		// ============== Laser pin declaration and #defines ========
		#define NUM_LASERS 5

		// * NOTE 1 : to switch on/off we could use just PWM, but it is better to have a digital "switch" so we conserve the value of current power;
		const uint8_t pinPowerLaser[5] = {5, 6, 7, 8, 9};//2, 3, 4, 5, 6};  // PWM capable:
		const uint8_t pinSwitchLaser[5] = {14, 15, 16, 17, 18};

		// Use an enum to identify lasers by a name [corresponds to array index]
		enum Laser {RED=0, GREEN = 1, BLUE = 2, YELLOW =3, MAGENTA = 4};

		extern void init();
		extern void test();

		void switchOffAll();
		void switchOnAll();
		void setPowerAll( uint16_t _power );

		inline void setSwitchLaser(uint8_t _laser, bool _state) {
			digitalWrite(pinSwitchLaser[_laser], _state);
		}

		inline void setPowerLaser(uint8_t _laser, uint16_t _power) {
			analogWrite(pinPowerLaser[_laser], _power);
		}

		// Composite setting:
		// TODO: extern void setColor();

		// Other methods for low level, more explicit control:
		inline void setSwitchRed(bool _state) {digitalWrite(pinSwitchLaser[RED], _state);}
		inline void setPowerRed(uint16_t _power) {analogWrite(pinPowerLaser[RED], _power);}

		inline void setSwitchGreen(bool _state) {digitalWrite(pinSwitchLaser[GREEN], _state);}
		inline void setPowerGreen(uint16_t _power) {analogWrite(pinPowerLaser[GREEN], _power);}

		inline void setSwitchBlue(bool _state) {digitalWrite(pinSwitchLaser[BLUE], _state);}
		inline void setPowerBlue(uint16_t _power) {analogWrite(pinPowerLaser[BLUE], _power);}

		inline void setSwitchYellow(bool _state) {digitalWrite(pinSwitchLaser[YELLOW], _state);}
		inline void setPowerYellow(uint16_t _power) {analogWrite(pinPowerLaser[YELLOW], _power);}

		inline void setSwitchMagenta(bool _state) {digitalWrite(pinSwitchLaser[MAGENTA], _state);}
		inline void setPowerMagenta(uint16_t _power) {analogWrite(pinPowerLaser[MAGENTA], _power);}


	}

	namespace Scanner {
		extern void init();

		//Set mirrors - no constrain, no viewport transform [we assume the "renderer" did that already]:
		inline void setPosRaw(int16_t _posX, int16_t _posY) {
			// NOTE: using analogWrite is far from optimal! in the future go more barebones!!
			analogWrite( PIN_ADCX, (uint16_t)_posX );
			#ifdef TEENSY_35_36
			analogWrite( PIN_ADCY, (uint16_t)_posY );
			#endif
		}

		inline void recenterPosRaw() {
			setPosRaw(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
		}

		inline void mapViewport(P2 &_point, float _minX, float _maxX, float _minY, float _maxY) {
			_point.x=map(_point.x, _minX, _maxX, MIN_MIRRORS_ADX, MAX_MIRRORS_ADX);
			_point.y=map(_point.y, _minY, _maxY, MIN_MIRRORS_ADY, MAX_MIRRORS_ADY);
		}

		inline bool clipLimits(P2 &_point) {
			bool clipped = false;
			if (_point.x > MAX_MIRRORS_ADX) {_point.x = MAX_MIRRORS_ADX; clipped = true;}
			else if (_point.x < MIN_MIRRORS_ADX)  {_point.x = MIN_MIRRORS_ADX; clipped = true;}
			if (_point.y > MAX_MIRRORS_ADY)  {_point.y = MAX_MIRRORS_ADY; clipped = true;}
			else if (_point.y < MIN_MIRRORS_ADY) { _point.y = MIN_MIRRORS_ADY; clipped = true;}
			return(clipped);
		}

		// Low level ADC test (also visual scanner range check).
		// NOTE: this method does not uses any buffer, so it should be
		// called when the DisplayScan is paused or stopped [if this is
		// not done, it should work anyway but with a crazy back and forth
		// of the galvano mirrors:
		extern void testMirrorRange(uint16_t _durationSec);
		extern void testCircleRange(uint16_t _durationSec);

	}

}

#endif
