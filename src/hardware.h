#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"


//  ============ LOW LEVEL HARDWARE METHODS ================================
// * NOTE 1: make the critic code inline!

namespace Hardware {

	extern void init();

	namespace Lasers {
		extern void init();
		extern void test();

		extern void setSwitchRed(bool _state);
		extern void setPowerRed(uint16_t _power);

		// Composite setting:
		// extern void setPowerColor()
	}

	namespace Scanner {
		extern void init();

		// Mirror positioning (critical function!)
		inline void setMirrorsTo(int16_t _posX, int16_t _posY) {

			// ATTENTION !!: the (0,0) corresponds to the middle position of the mirrors:
			_posX += CENTER_MIRROR_ADX;
			_posY += CENTER_MIRROR_ADY;

			// constrainPos(_posx, _posy); // not using a function call is better [use a MACRO,
			// an inlined function or write the code here]:
			if (_posX > MAX_MIRRORS_ADX) _posX = MAX_MIRRORS_ADX;
			else if (_posX < MIN_MIRRORS_ADX) _posX = MIN_MIRRORS_ADX;
			if (_posY > MAX_MIRRORS_ADY) _posY = MAX_MIRRORS_ADY;
			else if (_posY < MIN_MIRRORS_ADY) _posY = MIN_MIRRORS_ADY;

			// NOTE: using alogWrite is far from optimal! in the future go more barebones!![note the CENTER_MIRROR_ADX/Y offset]
			// NOTE2: these DAC outputs are weird, and it is documented: voltage range from
			// about 0.55 to 2.75V (1/6 to 5/6 from VCC = 3.3V). This means that, at 12nit res,
			// the value 2047 is (1/6+(1/6+5/6)/2)*4.4 = 2/3*3.3 = 2.2V
			analogWrite( PIN_ADCX, _posX );
			analogWrite( PIN_ADCY, _posY );
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
		extern void testMirrorRange(uint16_t _durationMs);
		extern void testCircleRange(uint16_t _durationMs);

	}

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

		// Change PWM duty cycle: just use analogWrite() on Teensy. I will wrap it because we may need
		// to change the hardware, plus this will not be called very often (it is not the laser ON/OFF, but
		// it's power):
		inline void setPWMDuty(uint8_t _pin, uint16_t _duty) {analogWrite(_pin, _duty);}
	}

	// ========= OTHERS:
	// (a) Software reset: better than using the RST pin (noisy?)
	extern void resetBoard();

	inline void ledDebug(bool _state) { // overkill wrapper? no, it could do a blink or something like that,
		// using millis to be non blocking!
		digitalWrite(PIN_LED_DEBUG, _state);
	}

	inline void ledMessage(bool _state) { // overkill wrapper? no, it could do a blink or something like that,
		// using millis to be non blocking!
		digitalWrite(PIN_LED_MESSAGE, _state);
	}
}

#endif
