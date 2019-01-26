#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

//* NOTE1 : The file should be called instead "Constants" and should use const and "extern", but
// for the time being I use #define instead (to avoid the problem of multiple definitions
// requires "extern" declarations (or inline since C++17). TODO: change this, #define(s)
// is bad practice (however this program is short.

#include "Arduino.h" // <-- has a lot of #defines and instantiated variables already

/*
#define PI 3.14159265889
#define DEG_TO_RAD  (0.01745) // = PI/180.0
#define RAD_TO_DEG (57.29578) // = 180.0/PI
*/

// ========================= RENDERER ==========================================
// IMPORTANT: for the time being, we will NOT use a vector<> array, so we need
// to set maximum number of points (P2) larger than any figure size. If this is too large, compile will fail.
#define MAX_NUM_POINTS 5000

// ========================= WHICH HARDWARE ARE WE USING?  ========================
// * NOTE ATTN: This code is only for the Teensy 3.x and up.
// If using a teensy <3.5, then there is only one DAC.
#define TEENSY_35_36
//#define TEENSY_31_32
//#define TEENSY_LC

// ========================= GPIO PINS DEFINITION ==============================
// 1) Mirrors:
//  a) "real" DAC, 12 bit resolution
// On Teensy Teensy 3.5 and 3.6 the native DACs are on pins A21 and A22); on the
// Teensy 3.1/3.2 there is only one DAC on pin A14, same than LC on pin A12
#if defined TEENSY_35_36
#define PIN_ADCX	A21
#define PIN_ADCY 	A22
#elif defined TEENSY_31_32
#define PIN_ADCX	A14
#define PIN_ADCY 	3
#elif defined TEENSY_LC
#define PIN_ADCX	A12
#define PIN_ADCY 	3
#endif

//3) ======================= LED indicators (digital) ==========================
#define PIN_LED_DEBUG   13 // 13 is the built-in led
#define PIN_LED_MESSAGE 24

// ========================= GALVANOMETERS Mirrors =============================
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

// ======================== LASER STUFF ========================================
#define MAX_LASER_POWER 4095 // 12 bit PWM res. TODO: normalize for the control commands (wrapper)

// For setting the things below, check here:
// https://www.pjrc.com/teensy/td_pulse.html

#define FREQ_PWM_POWER      70000     // for filtering
#define FREQ_PWM_CARRIER    200000  // this is for chopping the signal (control fast switches)

#define RES_PWM  12 // for now, the PWM resolution is set for ALL PWM pins using the Arduino library,
// even though pwm pins can be divided in groups with different flexi-timer controllers.

#define NUM_LASERS 4 // larger in the future (in my circuit I already can control 4 lasers - quad op-apm...

//Power pins (PWM): analog value come from filtered PWM outputs
// PWM pins to control the laser power on flexitimer FTM0 (pins 5,6,9,10,20,21,22,23)
const uint8_t pinPowerLaser[NUM_LASERS]{5,6,9,10};//,20, 21, 22, 23};

// Digital pins to open/close each laser channel very fast. These pins should be used as DIGITAL OUTPUT or
// as PWM - to generate a carrier for instance. Using the lasers in "carrier" mode does not precludes
// inter-sprite or inter-point complete blanking.
// PWM pins on FTM3 (pins 2,7,8,14,38,37,36,35)
const uint8_t pinSwitchLaser[NUM_LASERS]{35, 36, 38, 37};// 14, 7, 2};

// ======================== OPTOTUNE stuff =====================================
// NOTE: these PWM pins will NOT be chopped by a fast ON/OFF switch pin (the "carrier")
#define NUM_OPTOTUNERS 2
const uint8_t pinPowerOptoTuner[NUM_OPTOTUNERS]{29, 30}; // these are PWM pins controlled by FTM2
#define FREQ_PWM_OPTOTUNE 65000  // PWM frequency for filtering
// #define RES_PWM_OPTOTUNE  12 // in the current Stoffregen library, the pwm resolution seems to
// affect ALL the timers... awkward, as it could be per-timer group.
#define MAX_OPTOTUNE_POWER 4095 // TODO: normalize for the control commands (wrapper)

// ======================== INTERNAL CLOCKS  =====================================
// NOTE: the number is arbitrary. I Use 4 just in case we want to use one for each laser
// and one for the camera (there could be more, in case of more complex pipeline graphs)
#define NUM_CLOCKS 4 // (just in case, )

// ======================== EXTERNAL TRIGGERS (input and outputs)  ===============
// Only one of each for the time being are available on hardware
#define NUM_EXT_TRIGGERS_IN 4
#define NUM_EXT_TRIGGERS_OUT 4

// ======================== TRIGGER EVENT PROCESSORS  ============================
// Arbitrary number (like the clocks):
#define NUM_TRG_PROCESSORS 4

// ======================== PULSARS (PULSE SHAPERS) ==============================
// Arbitrary number...
#define NUM_PULSARS 4

// =========== OTHER PWM pins exposed in the D25 connector  ====================
// NOTE : there is yet no wrapper associated to these pins: they are
// just exposed in the connector. In the case of digital pins, they can be switched on/off
// by the generic command: pin,0/1,SETPIN
// TODO : we could have commands for reading pins too (digital and analog)

// * ANALOG PINS (for reading and writing, on TPM1 timer)
#define PIN_ANALOG_A 16
#define PIN_ANALOG_B 17

// * DIGITAL PINS:
#define PIN_DIGITAL_A 31
#define PIN_DIGITAL_B 32

// * TRIGGER PIN (bidirectional?)
#define PIN_TRIGGER_INPUT   22 // pin 12 in D25 ILDA connector
#define PIN_TRIGGER_OUTPUT  23 // pi 21 in D25 ILDA connector (this is DB-, but it should then not be connected to GND)

// * INTENSITY/BLANKING
// * NOTE: This is set automatically to LOW when NO laser is ON, and OFF otherwise by software; however, it would be
// much better if this was done electronically, not using the relatively complex - and easy to broke - logic of my program!
#define PIN_INTENSITY_BLANKING  15 // in D25 ILDA connector, it goes to pin 3 (pin 16 is normally tied to ground then!)

// * SHUTTER PIN: should put 5V when drawing and lasers ON, and 0 otherwise.
#define PIN_SHUTTER	            14 // this is pin 13 in D25 ILDA connector

// ======================== SIMPLE I/O INTERFACE (LCD, buttons) ================
// 1] LCD Grove RGB display on Teensy SDA0/SCL0 )using Wire library=
//NOTE: these pins are listed here for reference, but they are the
//default pins using in Wire.h.

#define LCD_SDA     18
#define LCD_SCL     19

// 2] TFT display (Adafruit_ST7735)
#define TFT_SCLK    25    // set these to be whatever pins you like, if using software SPI
#define TFT_MOSI    26   // set these to be whatever pins you like, if using software SPI
#define TFT_CS      27
#define TFT_DC      28
#define TFT_RST     0 // not used for now, the RST is connected to 5V (Vin)

// ======= Useful string definitions to access hardware elements ================
// NOTE1: I aggregate these here in case we want to change the keywords quickly
// NOTE2: They could be in the hardware namespace or perhaps even better in the messageParser
// namespace, but I may use these string names throught the program.
namespace Definitions {


// * LASERS:
const String laserNames[NUM_LASERS]{"red", "green", "blue", "deep_blue"};

// * SEQUENCER MODULES:
// NOTE: unfortunately there is no implementation of stl::map in Arduino
//const std::map<String, int> classMap = {{"clock", 0}, {"in", 1}, {"out", 2}, {"laser", 3}, {"prc", 4}, {"trg", 5}};
// ... so I will need to traverse the array to find the index:
#define NUM_MODULE_CLASSES 6
const String classNames[NUM_MODULE_CLASSES]{"clk","in", "out", "las", "prc", "trg"};

// * TRIGGER MODES:
#define NUM_TRIG_MODES 3
const String trgModeNames[NUM_TRIG_MODES]{"rise", "fall", "change"};

// * binary values ON/OFF:
const String binaryNames[2]{"on", "off"};

}
//=========================== OTHER USEFUL MACROS ===============================
// Size of an array of anything (careful: this doesn't work if array is allocated dynamically)
#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))
#define INRANGE(x, a, b) ( ( (x)>(a) && (x)<(b) ) ? 1 : 0 )

#endif
