#include "hardware.h"

namespace Hardware {

	namespace Gpio {

		void init() { // set default modes, output values, etc of pins, other that the laser and scanner:

			// ========= Setting Digital pins
			// * NOTE 1: if we want to really speed up things on pin xx, we could use on the DUE:
			// g_APinDescription[ xx ].pPort -> PIO_SODR = g_APinDescription[8].ulPin;
			// g_APinDescription[ xx ].pPort -> PIO_CODR = g_APinDescription[8].ulPin;
			// [Under the hood, Arduino IDE uses Atmel’s CMSIS compliant libraries]
			// [it seems that this methods in a loop() generate a 16MHz square signal while
			// the use of the digitalWrite methods can only go up to 200Hz...]
			pinMode(PIN_LED_DEBUG, OUTPUT);   digitalWrite(PIN_LED_DEBUG, LOW);     // for debug, etc
			pinMode(PIN_LED_MESSAGE, OUTPUT); digitalWrite(PIN_LED_MESSAGE, LOW); // to signal good message reception

			// ========= Configure PWM frequency and resolution
			// Resolution [available on Teensy LC, 3.0 - 3.6]
			analogWriteResolution(12);

			// Frequency PWM pins:
			// * NOTE: the PWM signals are created by hardware timers. PWM pins common to each timer always
			// have the same frequency, so if you change one pin's PWM frequency, all other pins for the same timer change:
			setPWMFreq(FREQ_PWM);
			// Duty cycle: I guess by default it is 0. Otherwise make a const array and reset all the powers. This will
			// be done anyway for the lasers in the Lasers namespace init()

			// ========= DAC: of course, the DACs are not PWM pins. They not need setting.
			// On Teensy Teensy 3.5 and 3.6 the native DACs are on pins A21 and A22), A14 on the
			// Teensy 3.1/3.2, and A12 on the Teensy LC.

			PRINTLN(">> GPIOs READY");
		}

		// Set frequency in HZ on pins 5, 6, 9, 10, 20, 21, 22, 23 (works on 3.1 to 3.6)
		void setPWMFreq(uint16_t _freq) {
			analogWriteFrequency(2, _freq);
		}
	}

	namespace Lasers {
		void init() {
			pinMode(PIN_SWITCH_RED, OUTPUT);  digitalWrite(PIN_SWITCH_RED, LOW);

			// Power: will use the PWM pins. No need to set as output, plus its frequency
			// is set in the Gpio init().

			//Default values:
			setSwitchRed(LOW); // no need to have a switch state for the time being (digital output works as a boolean state)
			setPowerRed(MAX_LASER_POWER);    // half power (rem: no need to have a "power" variable for the time being - PWM is done by hardware)

			PRINTLN(">> LASERS READY");
		}

		void test() {

		}

		void setSwitchRed(bool _state) { // I make a method here in case there will be some need to do more than digitalWrite
			digitalWrite(PIN_SWITCH_RED, _state);
		}

		void setPowerRed(uint16_t _power) {
			// * NOTE 1: analogWrite(PIN_PWM_RED, _power) does not works properly plus the carrier is 1kHz.
			// * NOTE 2: the 11 bit constraint is done in the setPWMDuty method: mo need to do it here.
			Gpio::setPWMDuty(PIN_PWM_RED, _power);
		}
	}

	namespace Scanner {

		void init() {
			// RENCENTER mirror and fill BOTH buffers with the central position too:
			recenterMirrors();
			PRINTLN(">> SCANNERS READY");
		}

		//inline void setMirrorsTo(uint16_t _posX, uint16_t _posY);
		//inline void recenterMirrors();

		void testMirrorRange(uint16_t _durationMs) {
			uint32_t startTime = millis();
			while (millis()-startTime<_durationMs) {
				for (uint16_t x = MIN_MIRRORS_ADX; x < MAX_MIRRORS_ADX; x+=10) {
					for (uint16_t y = MIN_MIRRORS_ADY; y < MAX_MIRRORS_ADY; y+=10) {
						analogWrite( PIN_ADCX, x );
						analogWrite( PIN_ADCY, y );
						delay(1); // in ms (ATTN: of course delay() not be used in the ISR stuff)
					}
				}
			}
		}

		void testCircleRange(uint16_t _durationMs) {
			uint16_t startTime = millis();
			float phi;
			while (millis()-startTime<_durationMs) {

				for (uint16_t k=0;k<360;k++) {
					phi = DEG_TO_RAD*k;
					int16_t x=(int16_t)(  1.0*CENTER_MIRROR_ADX*(1.0+cos(phi))  );
					int16_t y=(int16_t)(  1.0*CENTER_MIRROR_ADY*(1.0+sin(phi))  );
					analogWrite( PIN_ADCX, x );
					analogWrite( PIN_ADCY, y );
					delay(1);
				}

			}
		}
	}

	// ======== OTHERS GENERIC HARWARE ROUTINES:
	void init() {
		//initSerial(); // make a namespace for serial? TODO
		Gpio::init();
		Lasers::init();
		Scanner::init();
	}

	//Software reset: better than using the RST pin (noisy?)
	void resetBoard() {
		SCB_AIRCR = 0x05FA0004; // software reset on Teensy 3.X
	}


}
