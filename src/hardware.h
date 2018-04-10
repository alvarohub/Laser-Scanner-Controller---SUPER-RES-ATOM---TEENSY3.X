#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "Arduino.h"
#include "Utils.h"

// ========================= WHICH HARDWA ARE WE USING?  =======================
// * NOTE ATTN: This code is only for the Teensy 3.x and up.
// If using a teensy <3.5, then there is only one DAC.
//#define TEENSY_35_36
#define TEENSY_31_32
//#define TEENSY_LC

// ========================= GPIO PINS DEFINITION ==============================
// 1) Mirrors:
//  a) "real" DAC, 12 bit resolution
// On Teensy Teensy 3.5 and 3.6 the native DACs are on pins A21 and A22),
// A14 on the Teensy 3.1/3.2, and A12 on the Teensy LC.
#if defined TEENSY_35_36
#define PIN_ADCX	A21
#elif defined TEENSY_31_32
#define PIN_ADCX	A14
#elif defined TEENSY_LC
#define PIN_ADCX	A12
#endif

#define PIN_ADCY A22 // this is only for the Teensy 3.5 and 3.6

//  b) PWM pins to do hardware offset (can be set near 0 and calibrate center by softare...)
#define PIN_OFFSETX  8
#define PIN_OFFSETY  7

//2) Lasers
#define FREQ_PWM 40000  // This will set the PWM pins frequency to 40kHz, resulting in an 11 bit resolution
#define MAX_DUTY_CYCLE  2047 // 2^11-1 = 2047
#define PIN_SWITCH_RED  36 // we could use just PWM, but it is better to have a digital "switch"
#define PIN_PWM_RED 2 // PWM capable [DUE pwm capable pin: 2 to 13]

//3) LED indicators:
#define PIN_LED_DEBUG   13
#define PIN_LED_MESSAGE 14

// ************************* CONSTANT HARDWARE PARAMETERS ******************************
#define MAX_MIRRORS_ADX	4095
#define MIN_MIRRORS_ADX	0
#define MAX_MIRRORS_ADY 4095
#define MIN_MIRRORS_ADY 0

// NOTE: there are two methods to produce the offset. One is to set a numeric ADC offset,
// the other is to use two analog pins (PWM) and op-amps. In BOTH cases we have have half the
// resolution of the DAC (0 to 2047). I will use BOTH (the one previously used in Nicolas's
// code corresponds to setting OFFSETADX/Y to 0).
#define CENTER_MIRROR_ADX  2047
#define CENTER_MIRROR_ADY  2047

// LASER (in ADC units):
#define MAX_LASER_POWER 4095 // we can limit this!
// (min is 0)

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
