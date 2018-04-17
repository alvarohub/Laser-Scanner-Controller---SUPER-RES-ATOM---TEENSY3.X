
#ifndef _SCANNER_H_
#define _SCANNER_H_

// REM1: a namespace instead of a class, because there is only one "instance" of the DisplayScan
// REM2: normally only used by the laserDisplay translation unit, but we may want to
// have low level positioning methods in Utils and in the main for
// instance; therefore everything should have external linkage [otherwise
// there will be link errors as there will be multiple definitions)]
// REM: another option that only works for methods is to make them "inline"

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"
#include "Class_P2.h"
#include "hardware.h"

// We need to use ATOMIC_BLOCK (critical sections stopping the interrupts):
 #include <util/atomic.h> // not for the Arduino DUE !!!

// *********** ISR DISPLAYING parameters **********************
// NOTE: analogWrite - for the ADC - takes ~10us? (is it blocking?)

// ISR timer interval for rendering each point [non-blocking of course]
#define DEFAULT_RENDERING_INTERVAL 100 // in microseconds

//  Delay after sending a position to the ADC, in order to account for mirror inertia.
#define DELAY_POSITIONING_MIRRORS_US 10  // in us.

namespace DisplayScan { // note: this namespace contains methods that are beyond the low level hardware ones for controlling the
	// mirrors: it is actually the diaplaying engine!

	// ======================= SCANNER CONTROL methods  =======================
	extern void init();

	extern void startDisplay();
	extern void stopDisplay();
	extern bool getRunningState();

    // The following corresponds in OpenGL to the sending of the "rendered" vertex array
    // to the framebuffer...
    extern void setDisplayBuffer(const P2 *ptrBlueprint, uint16_t _sizeBlueprint);

	extern uint16_t getBufferSize();

	extern void setInterPointTime(uint16_t _dt);
	extern void setBlankingRed(bool _val);

	extern void writeOnHiddenBuffer(uint16_t _absIndex, const P2& _point);
	// NOTE: to go faster, we could direcly access the buffers and not use this
	// method, but it will be only called when rendering a figure by the renderer,
	// which will not happen too fast. This is safer!

	// * NOTE: Even if this is not a class, I can make variables or methods
 	// "private" by using an anonymous namespace:
	//namespace {

		// =============DOUBLE RING BUFFERS =================================
		// * NOTE: Double buffering is VERY USEFUL to avoid seeing the
		// mirrors stops while rendering a figure, or having a deformed figure.
		// extern PointBuffer displayBuffer1, displayBuffer2; <-- unclear, prefer:
        extern P2 displayBuffer1[MAX_NUM_POINTS]; // displayBuffer1 is a const pointer
        // to the P2 object displayBuffer1[0]. Using displayBuffer1[i] or *(displayBuffer1+i) is
        // exactly the same (pointer arithmetics works because the compiler knows it is a
        // pointer to objects of type P2)
        extern P2 displayBuffer2[MAX_NUM_POINTS]; // or P2 displayBuffer1* and use
        // dynamic allocation with displayBuffer1 = new P2[MAX_NUM_POINTS]
		extern volatile bool needSwapFlag;
		extern uint16_t newSizeBufferDisplay; // no need to be volatile
		extern uint16_t readingHead;

		// The following variables must be qualified volatile, as they may be modificated
		// outside the section of code where they appear [because of the ISR]
		extern volatile P2 *ptrCurrentDisplayBuffer, *ptrHiddenDisplayBuffer;
		extern volatile uint16_t sizeBufferDisplay;
		extern volatile bool resizeFlag;

		// TIMER INTERRUPT for scanner positionning. IntervalTimer is supported only on 32 bit
		// boards: Teensy LC, 3.0, 3.1, 3.2, 3.5 & 3.6. Up to 4 IntervalTimer objects may be active
		// simultaneuously on Teensy 3.0 - 3.6. Teensy LC has only 2 timers for IntervalTimer.
        // NOTE: Teensy (Cortex M4) is capable of *priorites* in the interrupts (16 nested levels).
        //       I will set the IntervalTimer to a higher priority than millis() and micros() which
        //       are set to priority 32 (there are 256 levels, arranged in 16 groups).
        //       For that, we use the "priority(..)" method of IntervalTimer.
        //       (Check Paul Stoffregen notes).
		extern IntervalTimer scannerTimer; // check: https://www.pjrc.com/teensy/td_timing_IntervalTimer.html
		extern void displayISR();
		extern uint32_t dt;
		extern bool running;

		// ======================= OTHERS  =================================
		extern bool blankingFlag; // inter-point laser on/off (true means off or "blankingFlag")
	//}
}

#endif
