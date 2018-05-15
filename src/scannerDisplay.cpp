#include "scannerDisplay.h"

namespace DisplayScan {

  // Define the extern variables:
  //PointBuffer displayBuffer1, displayBuffer2;
  P2 displayBuffer1[MAX_NUM_POINTS];
  P2 displayBuffer2[MAX_NUM_POINTS];
  volatile P2 *ptrCurrentDisplayBuffer, *ptrHiddenDisplayBuffer;
  uint16_t readingHead,  newSizeBufferDisplay; // no need to be volatile
  volatile uint16_t sizeBufferDisplay;
  volatile bool needSwapFlag;

  IntervalTimer scannerTimer;
  uint32_t dt;
  bool running, interpointBlanking;
  StateDisplayEngine stateDisplayEngine;

  elapsedMicros delayMirrorsInterPointMicros, delayMirrorsInterFigureBlankingMicros;
  elapsedMicros delayInPoint;
  elapsedMicros delayLaserOnMicros, delayLaserOffMicros;

  void init() {

    // Set the displaying buffer pointer to the first ring buffer [filled with the
    // central point], and set swapping flag:

    // NOTE: I don't need to initialize the whole array because the ONLY place the
    // effective buffer size is changed is in the render() method (Renderer namespace),
    // which does fill the buffer as needed. HOWEVER, for some reason [maybe an error
    // on the indexes, will check later] accessing a not defined point will make
    // the program crash!!???
    for (uint16_t i=0; i<MAX_NUM_POINTS; i++) {
      displayBuffer1[i] = P2(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
      displayBuffer2[i] = P2(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
    }

    sizeBufferDisplay = newSizeBufferDisplay = 0;

    ptrCurrentDisplayBuffer = &(displayBuffer1[0]); // Note: displayBuffer1 is a const pointer to
    // the first element of the array displayBuffer1[].
    ptrHiddenDisplayBuffer = &(displayBuffer2[0]);
    readingHead = 0;
    needSwapFlag=false;

    // 3) Default scan parameters:
    dt = DEFAULT_RENDERING_INTERVAL;

    // 3) Start interrupt routine by default? YES
    scannerTimer.begin(displayISR, dt);
    running = true;

    // 4) Initialize waiting timers:
    resetWaitingTimers();

    // 5) initialize display engine state and variables:
    stateDisplayEngine = STATE_START;
    interpointBlanking = false;

    // Set ISR priority?
    // * NOTE 1 : lower numbers are higher priority, with 0 the highest and 255 the lowest.
    // Most other interrupts default to 128, millis() and micros() are priority 32.
    //  As a general guideline, interrupt routines that run longer should be given lower
    //　priority (higher numerical values).
    // * NOTE 2 : priority should be set after begin  - meaning it has to be set every time we stop
    // and restart??? I put it in "startDisplay()" method then.
    // Check here for details:
    //        - https://www.pjrc.com/teensy/td_timing_IntervalTimer.html
    //        - https://forum.pjrc.com/archive/index.php/t-26642.html
    // scannerTimer.priority(112); // lower than millis/micros but higher than "most others",
    // including
  }

uint32_t getInterPointTime() {return(dt);}

uint32_t getInterPointBlankingMode() {return(interpointBlanking);}

  void resetWaitingTimers() {
    delayMirrorsInterPointMicros=0;
    delayMirrorsInterFigureBlankingMicros=0;
    delayLaserOnMicros=0;
    delayLaserOffMicros=0;
    delayInPoint=0;
  }

  void startDisplay() {
    if (!running) { // otherwise do nothing, the ISR is already running
      if ( scannerTimer.begin(displayISR, dt) ) {
        scannerTimer.priority(112);
        running = true;
        resetWaitingTimers();
      } else {
        PRINTLN(">> ERROR: could not set ISR.");
      }
    }
  }

  void stopDisplay() {
    if (running) {
      scannerTimer.end(); // perhaps the condition is not necessary

    // Switch Off laser? (power *state* is not affected, and
    // will be retrieved when restarting the engine):
    Hardware::Lasers::switchOffAll(); // not "setStateSwitchAll(false)"

    // Also, set the PIN_INTENSITY_BLANKING to LOW (will be set to HIGH when retrieving the laser
    // state AND if at least one laser is not switched off):
    Hardware::Gpio::setIntensityBlanking(false);
  }
    running = false;
  }

  bool getRunningState() {return(running);}

  uint16_t getBufferSize() {return(sizeBufferDisplay);}

  void setInterPointTime(uint16_t _dt) {
    #ifdef BLOCKING_DELAYS
    // NOTE: in blocking mode, if the ISR last more than the sum of inter-times, it can hang the program!
    if (_dt < 10 + MIRROR_INTER_FIGURE_WAITING_TIME + MIRROR_INTER_POINT_WAITING_TIME + LASER_ON_WAITING_TIME + IN_NORMAL_POINT_WAIT)
    dt = _dt;
    #else
    dt=_dt;
    #endif
    // NOTE: there is a difference between "update" and "start",
    // check PJRC page. myTimer.update(microseconds);
    scannerTimer.update(dt);
  }

  void setInterPointBlankingMode(bool _mode) {
    interpointBlanking = _mode; // for the time being, this is not per-laser!
    //stateDisplayEngine = STATE_START; // important when changing variables that may affect
    // the ISR program flow!! (START of display engine clears also the laser "style" stack)

    // Needed or the reset to current state may not be called in the ISR
    if (!interpointBlanking) Hardware::Lasers::setToCurrentState();
  }

  void setDisplayBuffer(const P2 *_ptrFrameBuffer, uint16_t _size) {

    // note: can I use memcpy with size(P2)?? probably yes and much faster... TP TRY!!
    //memcpy (ptrHiddenDisplayBuffer, _ptrFrameBuffer, _size*sizeof(P2));
    for (uint16_t k=0; k<_size; k++) {
      ptrHiddenDisplayBuffer[k].x=_ptrFrameBuffer[k].x;
      ptrHiddenDisplayBuffer[k].y=_ptrFrameBuffer[k].y;
    }

    needSwapFlag = true;
    // The following is a critical piece of code and must be ATOMIC, otherwise
    // the flag may be reset
    // by the ISR before newSizeBufferDisplay is set.
    // Now, this means the displaying engine will briefly stop,
    // but really briefly, and moreover the resizing of the buffer is
    // only done at the end of a rendering figure: not very often...
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      //noInterrupts();
      newSizeBufferDisplay = _size;
      //interrupts();
    }
  }


  // =================================================================
  // =========== Mirror-psitioning ISR that is called every dt =======
  //==================================================================
  // * NOTE 1 : the ISR set and display ONE point at each entry
  // * NOTE 2 : this is perhaps the most critical method. And it has to be
  // as fast as possible!! (in the future, do NOT use analogWrite() !!)
  // * NOTE 3 : general guideline is to keep your function short and avoid
  // calling other functions if possible.
  void displayISR() {
    //  static uint8_t multiDisplay = 0;
    //  if (!readingHead) multiDisplay = (multiDisplay+1)%3;

    // First of all, regardless of the state of the displaying engine, exchange buffers
    // when finishing displaying the current buffer AND needSwapFlag is true - meaning
    // the rendering engine finished drawing a
    // new figure on the hidden buffer [check renderFigure() method]:
    if (needSwapFlag) {//&& (readingHead==0) ) {// need to swap AND we are in the start of buffer
      // * NOTE : the second condition is optional: we could start displaying from
      // the current readingHead - but this will deform the figure when
      // rendering too fast, basically rendering double buffering obsolete.

      // Now, IF we still want that [to smooth the *perceptual illusion* of deformation],
      // we also need to check readingHead is in the new range:
      readingHead = (readingHead % newSizeBufferDisplay);

      // * NOTE : The following variables are volatile - they
      //   need to be, because they are modified in the ISR:
      sizeBufferDisplay = newSizeBufferDisplay;

      volatile P2 *ptrAux = ptrCurrentDisplayBuffer;
      ptrCurrentDisplayBuffer = ptrHiddenDisplayBuffer;
      ptrHiddenDisplayBuffer = ptrAux;

      needSwapFlag = false;

      // NOTE : not using the style stack here is better for several reasons,
      // the most important being avoiding stack overflows when the program flow
      // changes as a result to some variables setting (ex: point-blanking)
      //Hardware::Lasers::clearStateStack();
      //Hardware::Lasers::resetToCurrentState();
    }

    // * NOTE 1: it is a detail, but in case there are no points
    // in the buffer [after clear for instance] we need to recenter
    // the mirrors for instance.
    // * NOTE 2: this will be done only after finishing the previous
    // figure... [nice or not?]
    if (sizeBufferDisplay==0) stateDisplayEngine = STATE_START;

    switch(stateDisplayEngine) {

      case STATE_START: // not the start of a figure, but a start of the display engine, including
      // the "restart" when the number of points is >0
      // NOTE: there is a difference between STOPPING the
      // display engine and CLEARING the blueprint which recenters the mirror, while
      // stopping the engine will pause on the last point being displayed
      //Hardware::Lasers::clearStateStack();

      //Hardware::Scanner::recenterPosRaw(); // recenter? no

      stateDisplayEngine = STATE_IDLE;
      //break; // proceed with the next case...

      case STATE_IDLE:
      {
        if (sizeBufferDisplay) {
          stateDisplayEngine = STATE_START_BLANKING;
          // Check and activate blanking if necessary:
          //Hardware::Lasers::pushState();
          for (uint8_t k = 0; k< NUM_LASERS; k++) {
            // switch laser off if blankingMode set (regardless of the mode - carrier or continuous)
            //if (Hardware::Lasers::LaserArray[k].myState.blankingMode) Hardware::Lasers::LaserArray[k].setSwitch(false);
            Hardware::Lasers::LaserArray[k].updateBlank(); // will only blank IF blanking mode true.
          }
        }
        // otherwise do nothing, but keep checking if the buffer gets filled with something.
      }
      break;

      case STATE_START_BLANKING:
      {
        // Position the mirrors to next point (this is, the first point in the trajectory, with readinHead = 0):
        int16_t adcX = static_cast<int16_t> ( ptrCurrentDisplayBuffer->x ) ;
        int16_t adcY = static_cast<int16_t> ( ptrCurrentDisplayBuffer->y );
        // NOTE:  avoid calling a function here if possible. It is okay if it is inline though!
        Hardware::Scanner::setPosRaw(adcX, adcY);

        delayMirrorsInterFigureBlankingMicros = 0;
        stateDisplayEngine = STATE_BLANKING_WAIT;
      }
      //... proceed to STATE_BLANKING_WAIT in the same call

      case STATE_BLANKING_WAIT:
      {
        #ifdef BLOCKING_DELAYS
        while (delayMirrorsInterFigureBlankingMicros < MIRROR_INTER_FIGURE_WAITING_TIME) {}
        #else // non blocking delay:
        if (delayMirrorsInterFigureBlankingMicros < MIRROR_INTER_FIGURE_WAITING_TIME)  break; // do nothing and break;
        #endif
        // End of mirror blanking waiting time: we are supposed to be in the right coordinates of first figure point: go directly to
        // laser on waiting state after setting the lasers ON (or whatever is needed)
        //Hardware::Lasers::popState(); // <-- IMPORTANT (do not forget or the stack will overflow).
        Hardware::Lasers::setToCurrentState();
        stateDisplayEngine = STATE_LASER_ON_WAITING;
      }
      // .. proceed!

      case STATE_START_NORMAL_POINT:
      {
        // Position mirrors  [ATTN: (0,0) is the center of the mirrors]
        int16_t adcX = static_cast<int16_t> ( (ptrCurrentDisplayBuffer + readingHead)->x ) ;
        int16_t adcY = static_cast<int16_t> ( (ptrCurrentDisplayBuffer + readingHead)->y );
        // NOTE:  avoid calling a function here if possible. It is okay if it is inline though!
        Hardware::Scanner::setPosRaw(adcX, adcY);

        stateDisplayEngine = STATE_TO_NORMAL_POINT_WAIT;
        delayMirrorsInterPointMicros = 0;
      }
      //... proceed

      case STATE_TO_NORMAL_POINT_WAIT:
      {
        #ifdef BLOCKING_DELAYS
        while (delayMirrorsInterPointMicros < MIRROR_INTER_POINT_WAITING_TIME) {}
        #else
        if (delayMirrorsInterPointMicros < MIRROR_INTER_POINT_WAITING_TIME) break;
        #endif
        // For the time being, color is not per-point, but uses the current state of the lasers which should
        // not have changed during the whole figure. By the way, the lasers can be switched off at the end the
        // normal point or not. If not [the default behaviour] the laser will be on during the jump to the next point.

        // SWITCH ON lasers IF interpointBlanking was true (this is COMMON for all lasers):
        // ATTN: interpointBlanking may have changed in the meantime, so that the laser will never
        // go back to the "current state". One solution is to do this setting all the time (no condition),
        // or call setToCurrentState() whenever we set interpointBlanking to false. I will use this last strategy.
        if (interpointBlanking) Hardware::Lasers::setToCurrentState(); //Hardware::Lasers::popState();
        stateDisplayEngine = STATE_LASER_ON_WAITING;
        delayLaserOnMicros = 0;
      }
      //... proceed to next case [STATE_LASER_ON_WAITING]

      case STATE_LASER_ON_WAITING:
      {
        #ifdef BLOCKING_DELAYS
        while (delayLaserOnMicros < LASER_ON_WAITING_TIME) {}
        #else
        if (delayLaserOnMicros < LASER_ON_WAITING_TIME ) break;
        #endif
        delayInPoint = 0;
        stateDisplayEngine = STATE_IN_NORMAL_POINT_WAIT;
      }
      // .. and then proceed

      case STATE_IN_NORMAL_POINT_WAIT: {
        #ifdef BLOCKING_DELAYS
        while (delayInPoint < IN_NORMAL_POINT_WAIT) {}
        #else
        if (delayInPoint < IN_NORMAL_POINT_WAIT ) break;
        #endif
        // Advance the readingHead on the round-robin buffer and check if we are in a normal cycle or final figure:
        // * NOTE 1 : no need to qualify readingHead it as volatile
        // since only the ISR will use it.
        // * NOTE 2 : if the second operand of / or % is zero the behavior is undefined in C++,
        // hence the condition on sizeBuffer size [but the check is done for this portion of
        // the ISR anyway]:
        //if (sizeBufferDisplay) readingHead = (readingHead + 1) % sizeBufferDisplay;
        readingHead = (readingHead + 1) % sizeBufferDisplay;
        if (readingHead==0) {
          // We WERE in the last point [TODO: this is not the right condition for a generic "end of figure"...]
          for (uint8_t k = 0; k< NUM_LASERS; k++) {
            // Switch laser off if blankingMode set [regardless of the mode - carrier or continuous]
            //if (Hardware::Lasers::LaserArray[k].myState.blankingMode) Hardware::Lasers::LaserArray[k].setSwitch(false);
            Hardware::Lasers::LaserArray[k].updateBlank(); // will only blank IF blanking mode true.
            // NOTE: we don't do inter-point blanking here! this is "true" blanking (between figures)
          }
          stateDisplayEngine = STATE_START_BLANKING;
        } else {
          // Switch OFF lasers
          if (interpointBlanking) {
            //Hardware::Lasers::pushState();
            Hardware::Lasers::switchOffAll(); // does not affect the current laser color/state
          }
          stateDisplayEngine = STATE_START_NORMAL_POINT;
        }
      }
      break;

    } // end switch state for the display engine modes

  } // end display ISR

}
