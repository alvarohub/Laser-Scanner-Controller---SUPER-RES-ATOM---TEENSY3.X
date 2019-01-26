#ifndef _UTILS_H_
#define _UTILS_H_

// HERE PUT wrappers for low level methods and other things

#include "Arduino.h"
#include "Definitions.h" // Program constants and MACROS (including hardware stuff)

// Use digitalWriteFast instead of digitalWrite? (uncomment to use default one in Arduino.h)
#define digitalWrite(PIN, VAL) ( __builtin_constant_p(PIN) ? digitalWriteFast(PIN, (VAL)) : (digitalWrite)(PIN, (VAL)) )

#define DEFAULT_VERBOSE_MODE true // can be changed by sofware (using Utils:setVerboseMode(...))

// Useful serial print methods:
#define PRINT(STRING) (Hardware::print(STRING))
#define PRINTLN(STRING) (Hardware::println(STRING))

#define DEBUG_MODE_SERIAL // by defining this, we can debug on the serial port
//#define DEBUG_MODE_LCD  // for using the LCD panel
//#define DEBUG_MODE_TFT // for using the TFT panel

#define USING_SD_CARD // normally, we will use the Teensy3.6, so the SD card reader should be always there. \
					  // but we may need to compile from a compatible microcontroller without this peripheric

namespace Utils
{

// NOTE: to be able to use the namespace among several files, we need to
// make the linkage of variables and methods external by declaring them by
// using the "extern" keyword and define them in a separate .cpp file OR inline them
// (variables can be inlined since C++17, but not sure this what compiler I will be
// using here). Otherwise we will see linker errors (multiple definitions)

extern bool verboseMode;
extern bool recordingScript;

extern void setVerboseMode(bool _active);

extern bool isNumber(String _str);
extern bool isDigit(char _val);
extern bool isSmallCaps(String _arg);

} // namespace Utils
#endif
