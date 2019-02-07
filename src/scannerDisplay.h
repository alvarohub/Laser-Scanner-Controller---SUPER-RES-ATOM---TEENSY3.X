
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
#define DEFAULT_ISR_PERIOD_RENDER 20 // in microseconds

// ISR internal delays:
// NOTE 1 : to have any effect, the waiting times below shuold all be larger than the rendering interval - the effective
//          waiting will be a multiple of the rendering interval.
//          Another option is to do blocking delays INSIDE the ISR (in which case the sum should be << than the ISR period).
//            In the last case, the main thing to remember iss that while you are in an interrupt routine the clock isn't ticking:
//            millis() won't change and micros() will initially change, but once it goes past a millisecond where a
//            millisecond tick is required to update the counter, it all falls apart. SO, if used properly, micros()
//            can work inside the ISR to do a blocking delay.

#define BLOCKING_DELAYS // uncomment to have non-blocking delays (ISR period should be as small as possible)). With blocking delays, the timings will be precise and can reach the minimum, since in non-blocking mode the delays (inter point,
// etc) are measured by pooling from one interrupt to another, so it will be MULTIPLES of DT.

// NOTE 2 : do this variable, with setters and getters for the following:
#define MIRROR_INTER_FIGURE_WAITING_TIME 0 // in microseconds, delay to give time to the galvos to reach start \
                                           //new figure, or the start of the same figure (in case it is not closed)
#define MIRROR_INTER_POINT_WAITING_TIME 10 // 15 / delay to give time to the mirrors to reach the current point

//#define LASER_OFF_WAITING_TIME            0     // in microseconds - or assumed to be really fast anyway.
#define LASER_ON_WAITING_TIME 0 // time waiting for correct laser power when reaching the current point position
#define IN_NORMAL_POINT_WAIT 10 // 15 / Time it passes EXACTLY on the current point with lasers ON

namespace DisplayScan
{
// note: this namespace contains methods that are beyond the low level hardware ones for controlling the
// mirrors: it is actually the displaying engine!

enum StateDisplayEngine
{
  STATE_START = 0,
  STATE_IDLE,
  STATE_START_BLANKING,
  STATE_BLANKING_WAIT,
  STATE_START_NORMAL_POINT,
  STATE_TO_NORMAL_POINT_WAIT,
  STATE_IN_NORMAL_POINT_WAIT,
  STATE_LASER_ON_WAITING
};

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
extern uint32_t getInterPointTime(); // {return(dt);}

extern void setInterPointBlankingMode(bool _mode);
extern uint32_t getInterPointBlankingMode(); // {return(interpointBlanking);}

// * NOTE: Even if this is not a class, I can make variables or methods
// "private" by using an anonymous namespace:
//namespace {

inline void resetWaitingTimers();

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

// TODO: make the display buffers from Laser POINTS (LP), containing P2i and also color (and detected intensity?)
//struct LP { // a laser point (for now, using a float P2, but in the future let's use uint16_t)
//P2 point;
//}

// Note: variables cannot be inlined (<C++11)
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
extern elapsedMicros delayMirrorsInterPointMicros, delayMirrorsInterFigureBlankingMicros;
extern elapsedMicros delayInPoint;
extern elapsedMicros delayLaserOnMicros, delayLaserOffMicros;
extern bool running;
extern bool interpointBlanking;
extern StateDisplayEngine stateDisplayEngine;
//    }

} // namespace DisplayScan

#endif
