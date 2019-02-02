#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include "Arduino.h"
#include "Definitions.h"
#include "scannerDisplay.h"
#include "Class_Laser.h"
#include "Class_OptoTuner.h"
#include "Class_Sequencer.h"
#include "Utils.h"

#ifdef USING_SD_CARD
#include <SD.h>
#endif

#ifdef DEBUG_MODE_LCD
#include "Wire.h"
#include "rgb_lcd.h"
#endif

#ifdef DEBUG_MODE_TFT
#include <Adafruit_GFX.h>	// Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#endif

// ============ LOW LEVEL HARDWARE METHODS ================================
// * NOTE 1: make the critic code inline!

namespace Hardware
{

// Hardware initialization (Gpios, Laser and scanners:)
void init();

//Software reset: better than using the RST pin (noisy?)
extern void resetBoard();

extern void blinkLed(uint8_t _pinLed, uint8_t _times, uint32_t _periodMicros);
extern void blinkLedDebug(uint8_t _times, uint32_t _periodMicros = 1000000); // default period of 1s
extern void blinkLedMessage(uint8_t _times, uint32_t _periodMicros = 1000000);

extern void print(String _string);
extern void println(String _string);

namespace Gpio
{

void init();

inline void setShutter(bool _state)
{
	digitalWrite(PIN_SHUTTER, _state);
}

inline void setIntensityBlanking(bool _state)
{
	digitalWrite(PIN_INTENSITY_BLANKING, _state);
}

inline void setTriggerOut(bool _state)
{
	pinMode(PIN_TRIGGER_OUTPUT, OUTPUT);
	digitalWrite(PIN_TRIGGER_OUTPUT, _state);
}

inline bool readTriggerInput()
{
	pinMode(PIN_TRIGGER_OUTPUT, INPUT_PULLUP);
	return (digitalRead(PIN_TRIGGER_INPUT));
}

inline void setDigitalPin(uint8_t _pin, bool _state)
{
	// TODO: for the time being, it is up to the user to check if the pin is set as OUTPUT, capable of that
	// and not conflicting with the used pins? we could do such tests here.
	digitalWrite(_pin, _state);
}

// Wrappers for special pins (that appear on the D25 connector):
inline void setDigitalPinA(bool _state)
{
	pinMode(PIN_DIGITAL_A, OUTPUT); // it could have been used as input!
	digitalWrite(PIN_DIGITAL_A, _state);
}

inline bool readDigitalPinA()
{
	// INPUT or INPUT_PULLUP?
	pinMode(PIN_DIGITAL_A, INPUT); // it could have been used as output!
	return (digitalRead(PIN_DIGITAL_A));
}

inline void setAnalogPinA(uint16_t _duty)
{
	return (analogWrite(PIN_ANALOG_A, _duty));
}
inline uint16_t readAnalogPinA()
{
	return (analogRead(PIN_ANALOG_A));
}
inline void setDigitalPinB(bool _state)
{
	pinMode(PIN_DIGITAL_B, OUTPUT); // it could have been used as input!
	digitalWrite(PIN_DIGITAL_B, _state);
}

inline bool readDigitalPinB()
{
	// INPUT or INPUT_PULLUP?
	pinMode(PIN_DIGITAL_B, INPUT); // it could have been used as output!
	return (digitalRead(PIN_DIGITAL_B));
}
inline void setAnalogPinB(uint16_t _duty)
{
	return (analogWrite(PIN_ANALOG_B, _duty));
}
inline uint16_t readAnalogPinB()
{
	return (analogRead(PIN_ANALOG_B));
}

// Change PWM frequency:
extern void setPWMFreq(uint16_t _freq);

// Change PWM duty cycle: just use analogWrite() on Teensy. I will wrap it
// because we may need to change the hardware, plus this will not be called
// very often [it is not the laser ON/OFF, but its power]:
inline void setPWMDuty(uint8_t _pin, uint16_t _duty) { analogWrite(_pin, _duty); }

} // namespace Gpio

// ====================================================================================
// ============================ NAMESPACE CLOCKS ======================================
namespace Clocks
{
// there can be one or more clocks:
extern Clock arrayClock[NUM_CLOCKS];
// rem: no need to call to an init() clock function - this will be done using start()
extern void setStateAllClocks(bool _startStop);
extern void resetAllClocks();

} // namespace Clocks

// ====================================================================================
// ======================= NAMESPACE EXTERNAL TRIGGERS INPUT(S) =======================
// NOTE: for the time being, only one input and one output, but there can be more!
namespace ExtTriggers
{
extern InputTrigger arrayTriggerIn[NUM_EXT_TRIGGERS_IN];
extern OutputTrigger arrayTriggerOut[NUM_EXT_TRIGGERS_OUT];
} // namespace ExtTriggers

// ====================================================================================
// ======================= NAMESPACE TRIGGER EVENT DETECTORS ==========================
// NOTE: for the time being, only one input and one output, but there can be more!
namespace TriggerProcessors
{
extern TriggerProcessor arrayTriggerProcessor[NUM_TRG_PROCESSORS];
}

// ====================================================================================
// ============================ NAMESPACE PULSE SHAPERS ("Pulsars") ===================
namespace Pulsars
{
extern Pulsar arrayPulsar[NUM_PULSARS];
}

// ====================================================================================
// ============================ NAMESPACE SEQUENCER ===================================
/* This namespace contain methods to store and build the sequencer graph, as well as update and
refresh all its components. The "sequencer" contain an array of modules; the algorithm
basically goes through all the instantiated modules (lasers, clocks, etc) that are active
in the sequencer pipeline, call update(), followed by refresh().
Graphs can be non fully connected (i.e. this is similar to have several "sequencer" evolving at
the same time) and cyclic.
*/
namespace Sequencer
{

extern std::vector<Module> vectorModules;
extern bool activeSequencer; // this is useful to stop the whole sequencer instead of
// having to stop (i.e. bypass) each module.

extern Module *getModulePtr(uint8_t _classID, uint8_t _index);

extern void setState(bool _active); // activate/deactivate sequencer
extern bool getState();

extern void reset();

extern void addModulePipeline(Module *ptr_newModule);

extern void disconnectModules();

extern void clearPipeline();

extern void displaySequencerStatus();

extern void update();

} // namespace Sequencer

// ====================================================================================
// ========================  NAMESPACE LASERS =========================================
namespace Lasers
{

extern Laser laserArray[NUM_LASERS]; //= {"RED", "GREEN", "BLUE", "D-BLUE"};
									 // note: the laser IDs (used for sequence triggering) will be automatically created
									 // from the class static variable "myID" which is incremented at each instantiation.

// ****************** METHODS ********************
// NOTE: namespace methods correspond to static methods of the class Laser
void init();
extern void test();

inline void setStateSwitch(int8_t _laserIndex, bool _state)
{
	// NOTE: carrier mode overrides this.
	if (_laserIndex > 0) // otherwise do nothing
		laserArray[_laserIndex].setStateSwitch(_state);
}

inline void setStateSwitchAll(bool _switch)
{
	for (uint8_t i = 0; i < NUM_LASERS; i++)
		laserArray[i].setStateSwitch(_switch);
}

inline void switchOffAll()
{ // <<-- without affecting the state!
	for (uint8_t i = 0; i < NUM_LASERS; i++)
		laserArray[i].setSwitch(LOW);
}
inline void switchOnAll()
{ // <<-- without affecting the state!
	for (uint8_t i = 0; i < NUM_LASERS; i++)
		laserArray[i].setSwitch(HIGH);
}

inline void setStatePower(int8_t _laserIndex, uint16_t _power)
{
	if (_laserIndex > 0) // otherwise do nothing
		laserArray[_laserIndex].setStatePower(_power);
	//analogWrite(pinPowerLaser[_laser], _power);
}
inline void setStatePowerAll(uint16_t _power)
{
	for (uint8_t i = 0; i < NUM_LASERS; i++)
		laserArray[i].setStatePower(_power);
}

inline void setStateBlanking(int8_t _laserIndex, bool _blankingMode)
{
	if (_laserIndex > 0) // otherwise do nothing
		laserArray[_laserIndex].setStateBlanking(_blankingMode);
}

inline void setStateBlankingAll(bool _blankingMode)
{
	for (uint8_t i = 0; i < NUM_LASERS; i++)
		laserArray[i].setStateBlanking(_blankingMode);
}

inline void setStateCarrier(int8_t _laserIndex, bool _carrierMode)
{
	if (_laserIndex > 0)
		laserArray[_laserIndex].setStateCarrier(_carrierMode);
}

inline void setStateCarrierAll(bool _carrierMode)
{
	for (uint8_t i = 0; i < NUM_LASERS; i++)
		laserArray[i].setStateCarrier(_carrierMode);
}

// TODO: Composite colors (simultaneous laser manipulation)
// NOTE: in the future, use HSV (color wheel):
// inline void setLaserColor()

inline bool isSomeLaserOn()
{
	bool someLaserOn = false;
	for (uint8_t k = 0; k < NUM_LASERS; k++)
	{
		someLaserOn |= laserArray[k].getStateSwitch();
	}
	return (someLaserOn);
}

inline void updateIntensityBlanking()
{
	if (isSomeLaserOn())
		Hardware::Gpio::setIntensityBlanking(true);
	else
		Hardware::Gpio::setIntensityBlanking(false);
}

inline void setToCurrentState()
{
	for (uint8_t k = 0; k < NUM_LASERS; k++)
	{
		laserArray[k].setToCurrentState();
	}
	updateIntensityBlanking();
}

// And a stack for *all* the lasers:
inline void pushState()
{
	for (uint8_t k = 0; k < NUM_LASERS; k++)
		laserArray[k].pushState();
}
inline void popState()
{
	for (uint8_t k = 0; k < NUM_LASERS; k++)
		laserArray[k].popState();
}
inline void clearStateStack()
{
	for (uint8_t k = 0; k < NUM_LASERS; k++)
		laserArray[k].clearStateStack();
}

} // namespace Lasers

namespace OptoTuners
{
enum OptoTuneName
{
	OPTOTUNE_A = 0,
	OPTOTUNE_B
};

extern OptoTune OptoTuneArray[NUM_OPTOTUNERS];

// ****************** METHODS ********************
// NOTE: namespace methods correspond to static methods of the class OptoTune
void init();
extern void test();

inline void setStatePowerAll(uint16_t _power)
{
	for (uint8_t i = 0; i < NUM_OPTOTUNERS; i++)
		OptoTuneArray[i].setStatePower(_power);
}

// Without changing the power state:
inline void setPowerAll(uint16_t _power)
{
	for (uint8_t i = 0; i < NUM_OPTOTUNERS; i++)
		OptoTuneArray[i].setPower(_power);
}

// Wrappers for independent optotune control from this namespace:
inline void setStatePower(uint8_t _opto, uint16_t _power)
{
	if (_opto < NUM_OPTOTUNERS)
		OptoTuneArray[_opto].setStatePower(_power);
	//analogWrite(pinPowerOptoTune[_opto], _power);
}

inline void setPower(uint8_t _opto, uint16_t _power)
{
	if (_opto < NUM_OPTOTUNERS)
		OptoTuneArray[_opto].setPower(_power);
	//analogWrite(pinPowerOptoTune[_opto], _power);
}

inline void setToCurrentState()
{
	for (uint8_t k = 0; k < NUM_OPTOTUNERS; k++)
		OptoTuneArray[k].setToCurrentState();
}

// Other handy methods, more explicit control:
inline void setStatePowerOptoA(uint16_t _power) { OptoTuneArray[OPTOTUNE_A].setStatePower(_power); }
inline void setStatePowerOptoB(uint16_t _power) { OptoTuneArray[OPTOTUNE_B].setStatePower(_power); }

} // namespace OptoTuners

namespace Scanner
{
void init();

//Set mirrors - no constrain, no viewport transform [we assume the "renderer" did that already]:
inline void setPosRaw(int16_t _posX, int16_t _posY)
{
	// NOTE: using analogWrite is far from optimal! in the future go more barebones!!
	analogWrite(PIN_ADCX, (uint16_t)_posX);
#ifdef TEENSY_35_36
	analogWrite(PIN_ADCY, (uint16_t)_posY);
#endif
}

inline void recenterPosRaw()
{
	setPosRaw(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
}

inline void mapViewport(P2 &_point, float _minX, float _maxX, float _minY, float _maxY)
{
	_point.x = (_point.x - _minX) * (MAX_MIRRORS_ADX - MIN_MIRRORS_ADX) / (_maxX - _minX) + MIN_MIRRORS_ADX;
	// NOTE: the map function in arduino uses long(s), not floats!!
	_point.y = (_point.y - _minY) * (MAX_MIRRORS_ADY - MIN_MIRRORS_ADY) / (_maxY - _minY) + MIN_MIRRORS_ADY;
}

inline bool clipLimits(P2 &_point)
{
	bool clipped = false;
	if (_point.x > MAX_MIRRORS_ADX)
	{
		_point.x = MAX_MIRRORS_ADX;
		clipped = true;
	}
	else if (_point.x < MIN_MIRRORS_ADX)
	{
		_point.x = MIN_MIRRORS_ADX;
		clipped = true;
	}
	if (_point.y > MAX_MIRRORS_ADY)
	{
		_point.y = MAX_MIRRORS_ADY;
		clipped = true;
	}
	else if (_point.y < MIN_MIRRORS_ADY)
	{
		_point.y = MIN_MIRRORS_ADY;
		clipped = true;
	}
	return (clipped);
}

// Low level ADC test (also visual scanner range check).
// NOTE: this method does not uses any buffer, so it should be
// called when the DisplayScan is paused or stopped [if this is
// not done, it should work anyway but with a crazy back and forth
// of the galvano mirrors:
extern void testMirrorRange(uint16_t _durationSec);
extern void testCircleRange(uint16_t _durationSec);
extern void testCrossRange(uint16_t _durationSec);

} // namespace Scanner

namespace Lcd
{
#ifdef DEBUG_MODE_LCD

extern rgb_lcd lcd;
extern void init();
// wrappers for LCD display - could do more than just wrap the lcd methods, i.e, vertical scroll with interruption, etc.
extern void print(String text);
extern void println(String text);

#endif
} // namespace Lcd

namespace Tft
{
#ifdef DEBUG_MODE_TFT

extern void init();
extern void print(String text);
extern void println(String text);

#endif
} // namespace Tft

// *********** SD CARD STUFF *******************************
namespace SDCard
{

const int chipSelect = BUILTIN_SDCARD;
extern void init();
extern String readScript(const String _nameFile);

extern bool saveScript(String _nameFile);

extern void printDirectory(File dir, uint8_t numTabs);

} // namespace SDCard

} // namespace Hardware

#endif
