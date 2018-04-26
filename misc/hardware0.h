#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"
#include "scannerDisplay.h"

#ifdef DEBUG_MODE_LCD
#include "Wire.h"
#include "rgb_lcd.h"
#endif

#ifdef DEBUG_MODE_TFT
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#endif

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

	extern void print(String _string);
	extern void println(String _string);

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

		// * NOTE 1 : to switch on/off we could use just PWM, but it is better to have a digital "switch" so we conserve
		// the value of current power.
		// * NOTE 2 : pwm frequency change in Gpio init affects PWM capable pins: 5, 6, 9, 10, 20, 21, 22, 23
		const uint8_t pinPowerLaser[5] = {5, 6, 9, 10, 20};
		const uint8_t pinSwitchLaser[5] = {33,34,35,36,37};

		// Use an enum to identify lasers by a name [corresponds to array index]
		enum Laser {RED_LASER=0, GREEN_LASER = 1, BLUE_LASER = 2, YELLOW_LASER =3, MAGENTA_LASER = 4};

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
		inline void setSwitchRed(bool _state) {digitalWrite(pinSwitchLaser[RED_LASER], _state);}
		inline void setPowerRed(uint16_t _power) {analogWrite(pinPowerLaser[RED_LASER], _power);}

		inline void setSwitchGreen(bool _state) {digitalWrite(pinSwitchLaser[GREEN_LASER], _state);}
		inline void setPowerGreen(uint16_t _power) {analogWrite(pinPowerLaser[GREEN_LASER], _power);}

		inline void setSwitchBlue(bool _state) {digitalWrite(pinSwitchLaser[BLUE_LASER], _state);}
		inline void setPowerBlue(uint16_t _power) {analogWrite(pinPowerLaser[BLUE_LASER], _power);}

		inline void setSwitchYellow(bool _state) {digitalWrite(pinSwitchLaser[YELLOW_LASER], _state);}
		inline void setPowerYellow(uint16_t _power) {analogWrite(pinPowerLaser[YELLOW_LASER], _power);}

		inline void setSwitchMagenta(bool _state) {digitalWrite(pinSwitchLaser[MAGENTA_LASER], _state);}
		inline void setPowerMagenta(uint16_t _power) {analogWrite(pinPowerLaser[MAGENTA_LASER], _power);}


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
			_point.x=(_point.x - _minX) * (MAX_MIRRORS_ADX - MIN_MIRRORS_ADX) / (_maxX - _minX )  + MIN_MIRRORS_ADX;
			// NOTE: the map function in arduno uses long(s), not floats!!
			_point.y=(_point.y - _minY) * (MAX_MIRRORS_ADY - MIN_MIRRORS_ADY) / (_maxY - _minY )  + MIN_MIRRORS_ADY;
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


	namespace Lcd {
		#ifdef DEBUG_MODE_LCD

		extern rgb_lcd lcd;
		extern void init();
		// wrappers for LCD display - could do more than just wrap the lcd methods, i.e, vertical scroll with interruption, etc.
		extern void print(String text);
		extern void println(String text);

		#endif
	}

	namespace Tft {
#ifdef DEBUG_MODE_TFT

		extern void init();
		extern void print(String text);
		extern void println(String text);

#endif
	}

}

#endif
