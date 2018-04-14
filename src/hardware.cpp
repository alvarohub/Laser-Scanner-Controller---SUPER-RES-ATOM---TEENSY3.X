#include "hardware.h"

namespace Hardware {

	namespace Gpio {

		void init() { // set default modes, output values, etc of pins, other that the laser and scanner:

			// ========= Setting Digital pins
			// * NOTE : we set here only the pins that do not belong to specific hardware;
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

			// Power: will use the PWM pins. No need to set as output, plus its frequency
			// is set in the Gpio init().

			// Switch: set as digital outputs:
			for (uint8_t i=0; i<NUM_LASERS; i++) pinMode(pinSwitchLaser[i], OUTPUT);

			setPowerAll(0);
			switchOffAll();

			PRINTLN(">> LASERS READY");
		}

		void test() {

		}

		extern void switchOffAll() {
			for (uint8_t i=0; i<NUM_LASERS; i++) digitalWrite(pinSwitchLaser[i], LOW);
		}
		extern void swithOnAll() { // NOTE: power is set independently
			for (uint8_t i=0; i<NUM_LASERS; i++) digitalWrite(pinSwitchLaser[i], LOW);
		}

		extern void setPowerAll(uint16_t _power) {
			for (uint8_t i=0; i<NUM_LASERS; i++) analogWrite(pinPowerLaser[i], _power);
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
			elapsedMicros usec =0;
			uint32_t startScanning = millis();
			while (millis() - startScanning <_durationMs) {
				for (uint16_t y = MIN_MIRRORS_ADY; y < MAX_MIRRORS_ADY; y+=10) {
					for (uint16_t x = MIN_MIRRORS_ADX; x < MAX_MIRRORS_ADX; x+=10) {
						analogWrite( PIN_ADCX, x );
						#ifdef TEENSY_35_36
						analogWrite( PIN_ADCX, y );
						#endif
						while (usec < 100) ; // wait
						usec = 0;
					}
					while (usec < 1000) ; // wait
					usec = 0;
				}
			}
		}

		void testCircleRange(uint16_t _durationMs) {
			elapsedMicros usec =0;
			uint32_t startScanning = millis();
			float phi;
			while (millis()-startScanning<_durationMs) {
				for (uint16_t k=0;k<100;k++) {
					phi = DEG_TO_RAD*k;
					uint16_t x=(uint16_t)(  1.0*CENTER_MIRROR_ADX*(1.0+cos(phi))  );
					uint16_t y=(uint16_t)(  1.0*CENTER_MIRROR_ADY*(1.0+sin(phi))  );
					analogWrite( PIN_ADCX, x );
					#ifdef TEENSY_35_36
					analogWrite( PIN_ADCX, y );
					#endif
					while (usec < 100) ; // wait
					usec = 0;
				}
				while (usec < 1000) ; // wait
				usec = 0;
			}
		}
	}

	// ======== OTHERS GENERIC HARWARE ROUTINES:
	void blinkLed(uint8_t _pinLed, uint8_t _times) {
		// Non blocking blink! Avoid to use delay() too
		elapsedMicros usec = 0;
		for (uint8_t i=0; i<_times; i++) {

			// Use a loop, or elapsedMicros [see: https://www.pjrc.com/teensy/teensy31.html]
			// for (unsigned long i=0; i<1000000;i++) { digitalWrite(PIN_LED_DEBUG, LOW);}
			// for (unsigned long i=0; i<1000000;i++) { digitalWrite(PIN_LED_DEBUG, HIGH);}

			digitalWrite(_pinLed, HIGH);
			while (usec<500000) {} // half a second
			usec = 0;
			digitalWrite(_pinLed, LOW);
			while (usec<500000) {} // half a second
			usec = 0;
		}
	}

	void blinkLedDebug(uint8_t _times) {
		blinkLed(PIN_LED_DEBUG, _times);
	}
	void blinkLedMessage(uint8_t _times) {
		blinkLed(PIN_LED_MESSAGE, _times);
	}

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
