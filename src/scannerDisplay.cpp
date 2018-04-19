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

    bool blankingFlag;

    IntervalTimer scannerTimer;
    uint32_t dt;
    bool running;

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
        blankingFlag = true;
        dt = DEFAULT_RENDERING_INTERVAL;

        // 3) Start interrupt routine by default? No.
        //scannerTimer.begin(displayISR, dt);
        running = false;

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

    void startDisplay() {
        if ( (!running) && (scannerTimer.begin(displayISR, dt)) ) {
            scannerTimer.priority(112);
            running = true;
        }
        else {
            PRINTLN(">> ERROR: could not set ISR.");
        }
    }
    void stopDisplay() {
        if (running) scannerTimer.end(); // perhaps the condition is not necessary
        running = false;
    }

    bool getRunningState() {return(running);}

    uint16_t getBufferSize() {return(sizeBufferDisplay);}

    void setInterPointTime(uint16_t _dt) {
        dt=_dt;
        // NOTE: there is a difference between "update" and "start", check PJRC page. myTimer.update(microseconds);
        scannerTimer.update(dt);
    }
    void setBlankingRed(bool _newBlankState) {
        blankingFlag = _newBlankState;
    }

    void writeOnHiddenBuffer(uint16_t _absIndex, const P2& _point) {
        // NOTE: in the future, the display buffer should contain ints,
        // but the blueprint floats to make the transformations and rendering.

        //(*ptrHiddenDisplayBuffer)[_absIndex].set(_point); // better not to make a call
        ptrHiddenDisplayBuffer[_absIndex].x=_point.x; // note: *(ptr+i) = ptr[i]
        (ptrHiddenDisplayBuffer+_absIndex)->y=_point.y;
    }

    void requestBufferSwap() {needSwapFlag = true;}

    void resizeBuffer(uint16_t _newSize) {
        //PRINT("NEW SIZE BUFFER: "); PRINTLN(_newSize);
        // The following is a critical piece of code and must be ATOMIC, otherwise the flag may be reset
        // by the ISR before newSizeBufferDisplay is set. Now, this means the displaying
        // engine will briefly stop - but really briefly, and moreover the resizing
        // of the buffer is only done at the end of a rendering figure: not very often.
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { // <-- NOT FOR ARDUINO DUE
            //noInterrupts();
            newSizeBufferDisplay = _newSize;
            //interrupts();
        }
    }

    // =================================================================
    // =========== Mirror-psitioning ISR that is called every dt =======
    //==================================================================
    // * NOTE 1 : this is perhaps the most critical method. And it has to be
    // as fast as possible!! (in the future, do NOT use analogWrite() !!)
    // * NOTE 2 : general guideline is to keep your function short and avoid
    // calling other functions if possible.
    void displayISR() {

        // Exchange buffers when finishing displaying the current buffer
        // AND needSwapFlag is true - meaning the rendering engine finished drawing a
        // new figure on the hidden buffer [check renderFigure() method]:
        if ( needSwapFlag && !readingHead ) {
            // i.e. if we need to swap and we are in the start of buffer
            // * NOTE : the second condition is optional: we could start displaying from
            // the current readingHead - but this will deform the figure when
            // rendering too fast.

            // NOTE: resizing may or may not have taken place while rendering a
            // new scene/figure, but instead of having a flag, let's just use
            // the value of newSizeBufferDisplay which is set in the renderer method.

            // * NOTE : The following variables are volatile - they
            //need to be, because they are modified in the ISR:
            sizeBufferDisplay = newSizeBufferDisplay;
            volatile P2 *ptrAux = ptrCurrentDisplayBuffer;
            ptrCurrentDisplayBuffer = ptrHiddenDisplayBuffer;
            ptrHiddenDisplayBuffer = ptrAux;
            needSwapFlag = false;
        }

        if (blankingFlag) {
            Hardware::Lasers::setSwitchRed(LOW); // avoid calling a function here if possible... (TODO)
            // TODO: also add delay for laser off? (TODO)
        }

        // DISPLAY point by point at each ISR ENTRY
        // * NOTE 1: it is a detail, but in case there are no points
        // in the buffer [after clear for instance] we need to recenter
        // the mirrors for instance.
        // * NOTE 2: this will be done only after finishing the previous
        // figure... [nice or not?]
        if (sizeBufferDisplay) {

            // Position mirrors  [ATTN: (0,0) is the center of the mirrors]
            int16_t ADCX = static_cast<int16_t> ( (ptrCurrentDisplayBuffer + readingHead)->x ) ;
            int16_t ADCY = static_cast<int16_t> ( (ptrCurrentDisplayBuffer + readingHead)->y );

            // PRINT(ADCX);PRINT(" ");PRINTLN(ADCY);

            // NOTE:  avoid calling a function here if possible. It is okay if it is inline though!
            Hardware::Scanner::setMirrorsTo(ADCX, ADCY);

            // After setting, advance the readingHead on the round-robin buffer:
            // * NOTE 1 : no need to qualify readingHead it as volatile
            // since only the ISR will use it.
            // * NOTE 2 : if the second operand of / or % is zero the behavior is undefined in C++,
            // hence the condition on sizeBuffer size [but the check is done for this portion of
            // the ISR anyway]:
            //if (sizeBufferDisplay) readingHead = (readingHead + 1) % sizeBufferDisplay;
            readingHead = (readingHead + 1) % sizeBufferDisplay;
        } else {
            // Force recentering after finishing figure and no points in the
            // blueprint? there is a difference between STOPPING the
            // display engine and CLEARING the blueprint!!
            Hardware::Scanner::recenterMirrors();
        }

        if (blankingFlag) {
            // TODO: non-blocking delay for mirror positioning before switching
            // the laser on? ...for the time being: blocking (for tests)
            elapsedMicros pause = 0;
            while (pause<DELAY_POSITIONING_MIRRORS_US);

            Hardware::Lasers::setSwitchRed(HIGH); // avoid calling a function here if possible (TODO)

        }
    }
}
