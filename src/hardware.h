#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"
#include "scannerDisplay.h"
#include "Class_Laser.h"

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

		enum LaserName {RED_LASER=0, GREEN_LASER, BLUE_LASER, YELLOW_LASER, CYAN_LASER};

		extern Laser LaserArray[NUM_LASERS];

		// ****************** METHODS ********************
		// NOTE: namespace methods correspond to static methods of the class Laser
		extern void init();
		extern void test();

		inline void switchOffAll() { // <<-- without affecting the state!
			for (uint8_t i=0; i<NUM_LASERS; i++) LaserArray[i].setSwitch(LOW);
		}

		inline void switchOnAll() { // <<-- without affecting the state!
			for (uint8_t i=0; i<NUM_LASERS; i++) LaserArray[i].setSwitch(HIGH);
		}
		// Methods affecting ALL the lasers in the LaserArray
		inline void setStateSwitchAll(bool _switch) {
			for (uint8_t i=0; i<NUM_LASERS; i++) LaserArray[i].setStateSwitch(_switch);
		}

		inline void setStatePowerAll(uint16_t _power) {
			for (uint8_t i=0; i<NUM_LASERS; i++) LaserArray[i].setStatePower(_power);
		}

		// Wrappers for independent laser control:
		inline void setStateSwitch(uint8_t _laser, bool _state) { // carrier mode overrides this
			LaserArray[_laser].setStateSwitch(_state);
			//digitalWrite(pinSwitchLaser[_laser], _state);
		}

		inline void setStatePower(uint8_t _laser, uint16_t _power) {
			LaserArray[_laser].setStatePower(_power);
			//analogWrite(pinPowerLaser[_laser], _power);
		}

		inline void setBlankingModeAll(bool _blankingMode) {
			for (uint8_t i=0; i<NUM_LASERS; i++) LaserArray[i].setBlankingMode(_blankingMode);
		}
		inline void setBlankingMode(uint8_t _laser, bool _blankingMode) {
			LaserArray[_laser].setBlankingMode(_blankingMode);
		}

		inline void setCarrierModeAll(bool _carrierMode) {
			for (uint8_t i=0; i<NUM_LASERS; i++) LaserArray[i].setCarrierMode(_carrierMode);
		}
		inline void setCarrierMode(uint8_t _laser, bool _carrierMode) {
			LaserArray[_laser].setCarrierMode(_carrierMode);
		}

		// Other handy methods, more explicit control:
		inline void setStateSwitchRed(bool _state) {LaserArray[RED_LASER].setStateSwitch(_state);}
		inline void setStatePowerRed(uint16_t _power) {LaserArray[RED_LASER].setStatePower(_power);}

		inline void setStateSwitchGreen(bool _state) {LaserArray[BLUE_LASER].setStateSwitch(_state);}
		inline void setStatePowerGreen(uint16_t _power) {LaserArray[BLUE_LASER].setStatePower(_power);}

		inline void setStateSwitchBlue(bool _state) {LaserArray[GREEN_LASER].setStateSwitch(_state);}
		inline void setStatePowerBlue(uint16_t _power) {LaserArray[GREEN_LASER].setStatePower(_power);}

		inline void setStateSwitchYellow(bool _state) {LaserArray[YELLOW_LASER].setStateSwitch(_state);}
		inline void setStatePowerYellow(uint16_t _power) {LaserArray[YELLOW_LASER].setStatePower(_power);}

		inline void setStateSwitchCyan(bool _state) {LaserArray[CYAN_LASER].setStateSwitch(_state);}
		inline void setStatePowerCyan(uint16_t _power) {LaserArray[CYAN_LASER].setStatePower(_power);}

		// TODO: Composite colors (simultaneous laser manipulation)
		// NOTE: in the future, use HSV (color wheel):
		// inline void setLaserColor()

		inline void setToCurrentState() {
			for (uint8_t k = 0; k< NUM_LASERS; k++) LaserArray[k].setToCurrentState();
		}

		// And a stack for *all* the lasers:
		inline void pushState() {
			for (uint8_t k = 0; k< NUM_LASERS; k++) LaserArray[k].pushState();
		}
		inline void popState() {
			for (uint8_t k = 0; k< NUM_LASERS; k++) LaserArray[k].popState();
		}
		inline void clearStateStack() {
			for (uint8_t k = 0; k< NUM_LASERS; k++) LaserArray[k].clearStateStack();
		}

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
		extern void testCrossRange(uint16_t _durationSec);

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
