#ifndef _UTILS_H_
#define _UTILS_H_

// HERE PUT wrappers for low level methods and other things

#include "Arduino.h"
#include "Definitions.h"    // Program constants and MACROS (including hardware stuff)

namespace Utils {

    // *　NOTE: to be able to use the namespace among several files, we need to
    // make the linkage of variables and methods external by declaring them by
    // using the "extern" keyword and define them in a separate .cpp file OR inline them - variables can be inlined since C++17, but not sure this what compiler I will be
    // using here. Otherwise we will see linker errors (multiple definitions)

	inline void waitBlink(uint8_t _times) {
		pinMode(PIN_LED_DEBUG, OUTPUT); //<-- to be able to call this without calling init first
		for (uint8_t i=0; i<_times; i++) {
			for (unsigned long i=0; i<1000000;i++) {
				digitalWrite(PIN_LED_DEBUG, LOW);
			}
			for (unsigned long i=0; i<1000000;i++) {
				digitalWrite(PIN_LED_DEBUG, HIGH);
			}
		}
	}
}

	


#endif
