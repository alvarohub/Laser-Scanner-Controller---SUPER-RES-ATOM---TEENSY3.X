#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

//* NOTE1 : The file should be called instead "Constants" and should use const and "extern", but
// for the time being I use #define instead (to avoid the problem of multiple definitions
// requires "extern" declarations (or inline since C++17). TODO: change this, #define(s)
// is bad practice (however this program is short)

#include "Arduino.h" // <-- has a lot of #defines and instantiated variables already

#define DEBUG_MODE// by defining this, we can debug on the serial port

#ifdef DEBUG_MODE
#define PRINT(...)      Serial.print(__VA_ARGS__)
#define PRINTLN(...)    Serial.println(__VA_ARGS__)
#define PRINT_CSTRING(...) for (uint8_t i=0; i<strlen(__VA_ARGS__); i++) Serial.print(__VA_ARGS__[i]); Serial.println()
#else
# define PRINT(...)
# define PRINTLN(...)
#define PRINT_CSTRING(...)
#endif

/*
#define PI 3.14159265889
#define DEG_TO_RAD  (0.01745) // = PI/180.0:
#define RAD_TO_DEG (57.29578) // = 180.0/PI
*/

// *****************************************************************************
// Size of an array of anything (careful: this doesn't work if array is allocated dynamically)
#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))

// *****************************************************************************
// Handy "#define" methods:
#define INRANGE(x, a, b) ( ( (x)>(a) && (x)<(b) ) ? 1 : 0 )

// ========================= RENDERER ==========================================
// IMPORTANT: for the time being, we will NOT use a vector<> array, so we need
// to set maximum number of points (P2) larger than any figure size. If this is too large, compile will fail.
#define MAX_NUM_POINTS 1000

// ========================= WHICH HARDWAARE WE USING?  ========================
// * NOTE ATTN: This code is only for the Teensy 3.x and up.
// If using a teensy <3.5, then there is only one DAC.
//#define TEENSY_35_36
#define TEENSY_31_32
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

//  b) PWM pins to do hardware offset [can be set near 0 and calibrate center by softare...]. NOT USED for the time being - should not be necessary. 
#define PIN_OFFSETX  8
#define PIN_OFFSETY  7

// Min and max: (in ADC units):
#define MAX_LASER_POWER 4095 // 12 bit PWM res; maybe not used for now
// min laser power is 0.
#define FREQ_PWM 40000  // This will set the PWM pins frequency to 40kHz, resulting in an 11 bit resolution


//3) ============== LED indicators (digital) ==================
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

#endif
