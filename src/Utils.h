#ifndef _util_h_
#define _util_h_

#include "Arduino.h"

//NOTE: if I use #define instead of const... I avoid the problem of multiple definitions
// that would require "extern" declaration (or inline since C++17). But in principle #define(s)
// is bad (however this program is short)

#define DEBUG_MODE // by defining this, we can debug on the serial port

// ************************* OTHER CONSTANTS ******************************
/*
#define PI 3.14159265889
#define DEG_TO_RAD  (0.01745) // = PI/180.0:
#define RAD_TO_DEG (57.29578) // = 180.0/PI
*/
#define CW 1.0
#define CCW -1.0

// ******************************************************************************

#ifdef DEBUG_MODE
#define PRINT(...)      Serial.print(__VA_ARGS__)
#define PRINTLN(...)    Serial.println(__VA_ARGS__)
#define PRINT_CSTRING(...) for (uint8_t i=0; i<strlen(__VA_ARGS__); i++) Serial.print(__VA_ARGS__[i]); Serial.println()
#else
# define PRINT(...)
# define PRINTLN(...)
#define PRINT_CSTRING(...)
#endif

// ***********************************************************************************
// Size of an array of anything (careful: this doesn't work if array is allocated dynamically)
#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))

// ***********************************************************************************
// Handy "#define" methods:
#define INRANGE(x, a, b) ( ( (x)>(a) && (x)<(b) ) ? 1 : 0 )

// ***********************************************************************************

namespace Utils {

    // *　NOTE: to be able to use the namespace among several files, we need to
    // make the linkage of variables and methods external by declaring them by
    // using the "extern" keyword and define them in a separate .cpp file OR inline them - variables can be inlined since C++17, but not sure this what compiler I will be
    // using here. Otherwise we will see linker errors (multiple definitions)

}

#endif
