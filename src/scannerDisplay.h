
#ifndef _SCANNER_H_
#define _SCANNER_H_

// REM1: a namespace instead of a class, because there is only one "instance" of the DisplayScan
// REM2: normally only used by the laserDisplay translation unit, but we may want to
// have low level positioning methods in Utils and in the main for
// instance; therefore everything should have external linkage [otherwise
// there will be link errors as there will be multiple definitions)]
// REM: another option that only works for methods is to make them "inline"

#include "Arduino.h"
#include "Utils.h"
#include "Class_P2.h"
#include "hardware.h"

// We need to use ATOMIC_BLOCK (critical sections stopping the interrupts):
// #include <util/atomic.h> // not for the Arduino DUE !!!

// *********** ISR DISPLAYING parameters **********************
#define DEFAULT_RENDERING_INTERVAL 1000 // in microseconds [ATTN: analogWrite takes ~10us]


namespace DisplayScan { // note: this namespace contains methods that are beyond the low level hardware ones for controlling the
	// mirrors: it is actually the diaplaying engine!

	// ======================= SCANNER CONTROL methods  =======================
	extern void init(); // init hardware (if necessary)

	extern void startDisplay();
	extern void stopDisplay();
	extern bool getRunningState();

	extern void stopSwapping();
	extern void startSwapping();

	// IMPORTANT: if, in Renderer2D, the number of points change, you may want
	// to stop the displaying engine. This is ok, as it will be done automatically
	// in the ISR by NOT exchanging buffers until finishing the change [actually it
	// will be extremely fast, so it may not even stop once].
	void resizeBuffer(uint16_t _newSize); // this is delicate (check implementation)

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
		extern PointBuffer displayBuffer1, displayBuffer2;
		extern bool canSwapFlag;
		extern uint16_t newSizeBuffers;
		extern uint16_t readingHead;

		// The following variables must be qualified volatile, as they may be modificated
		// outside the section of code where they appear [because of the ISR]
		extern volatile PointBuffer *ptrCurrentDisplayBuffer, *ptrHiddenDisplayBuffer;
		extern volatile uint16_t sizeBuffers;
		extern volatile bool resizeFlag;

		// TIMER INTERRUPT for scanner positionning. IntervalTimer is supported only on 32 bit
		// boards: Teensy LC, 3.0, 3.1, 3.2, 3.5 & 3.6. Up to 4 IntervalTimer objects may be active
		// simultaneuously on Teensy 3.0 - 3.6. Teensy LC has only 2 timers for IntervalTimer.
		extern IntervalTimer scannerTimer; // check: https://www.pjrc.com/teensy/td_timing_IntervalTimer.html
		extern void displayISR();
		extern uint16_t dt;
		extern bool running;

		// ======================= OTHERS  =================================
		extern bool blankingFlag; // inter-point laser on/off (true means off or "blankingFlag")
	//}
}

#endif
