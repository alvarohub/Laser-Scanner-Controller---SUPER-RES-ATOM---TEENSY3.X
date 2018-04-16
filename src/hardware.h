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
		const uint8_t pinPowerLaser[5] = {2,3,4,5,6};  // PWM capable:
		const uint8_t pinSwitchLaser[5] = {36, 37, 38, 39, 40};

		// Use an enum to identify lasers by a name [corresponds to array index]
		enum {RED=0, GREEN = 1, BLUE = 2, YELLOW =3, MAGENTA = 4};

		extern void init();
		extern void test();
		void switchOffAll();
		void switchOnAll();
		void setPowerAll( uint16_t _power );

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

		// Composite setting:
		// extern void setColor();
	}

	namespace Scanner {
		extern void init();


		inline void setMirrorsTo(int16_t _posX, int16_t _posY) {

			// ATTENTION : (0,0) corresponds to the middle position of the mirrors!
			_posX += CENTER_MIRROR_ADX;
			_posY += CENTER_MIRROR_ADY;

			// constrainPos(_posx, _posy); // not using a function call is
			// better [use a MACRO, an inlined function or write the code here]:
			if (_posX > MAX_MIRRORS_ADX) _posX = MAX_MIRRORS_ADX;
			else if (_posX < MIN_MIRRORS_ADX) _posX = MIN_MIRRORS_ADX;
			if (_posY > MAX_MIRRORS_ADY) _posY = MAX_MIRRORS_ADY;
			else if (_posY < MIN_MIRRORS_ADY) _posY = MIN_MIRRORS_ADY;

			// NOTE: using alogWrite is far from optimal! in the future go more barebones!!
			analogWrite( PIN_ADCX, (uint16_t)_posX );
			#ifdef TEENSY_35_36
			analogWrite( PIN_ADCY, (uint16_t)_posY );
			#endif
		}

		inline void recenterMirrors() {
			setMirrorsTo(0,0);
			//setMirrorsTo(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
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
