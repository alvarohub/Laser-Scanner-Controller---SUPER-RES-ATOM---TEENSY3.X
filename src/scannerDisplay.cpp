#include "scannerDisplay.h"

namespace DisplayScan {

    // Define the extern variables:
    //PointBuffer displayBuffer1, displayBuffer2;
    P2 displayBuffer1[MAX_NUM_POINTS];
    P2 displayBuffer2[MAX_NUM_POINTS];
    volatile P2 *ptrCurrentDisplayBuffer, *ptrHiddenDisplayBuffer;
    uint16_t readingHead,  newSizeBuffers;
    volatile uint16_t sizeBuffers;
    bool canSwapFlag;
    volatile bool resizeFlag;

    bool blankingFlag;

    IntervalTimer scannerTimer;
    uint16_t dt;
    bool running;

    void init() {

        // Set the displaying buffer pointer to the first ring buffer [filled with the
        // central point], and set swapping flags:
        displayBuffer1[0]=P2(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
        displayBuffer2[0]=P2(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
        // NOTE: I don't need to initialize the whole array because the ONLY place the
        // effective buffer size is changed is in the render() method (Renderer namespace),
        // which does fill the buffer as needed.

        sizeBuffers = newSizeBuffers = 0;

        ptrCurrentDisplayBuffer = &(displayBuffer1[0]); // Note: displayBuffer1 is a const pointer to
        // the first element of the array displayBuffer1[].
        ptrHiddenDisplayBuffer = &(displayBuffer2[0]);
        readingHead = 0;
        canSwapFlag=true;
        resizeFlag=false;

        // 3) Default scan parameters:
        blankingFlag = true;
        dt = DEFAULT_RENDERING_INTERVAL;

        // 3) Set interrupt routine by default? No.
        //scannerTimer.begin(displayISR, dt);
        running = false;
    }

    void startDisplay() {
        scannerTimer.begin(displayISR, dt);
        running = true;
    }
    void stopDisplay() {
        scannerTimer.end();
        running = true;
    }

    bool getRunningState() {return(running);}


    uint16_t getBufferSize() {return(sizeBuffers);}

    void setInterPointTime(uint16_t _dt) {
        dt=_dt;
        scannerTimer.update(dt);
    }
    void setBlankingRed(bool _newBlankState) {
        blankingFlag = _newBlankState;
    }

    void writeOnHiddenBuffer(uint16_t _absIndex, const P2& _point) {
        //(*ptrHiddenDisplayBuffer)[_absIndex].set(_point); // better not to make a call
        ptrHiddenDisplayBuffer[_absIndex].x=_point.x; // note: *(ptr+i) = ptr[i]
        (ptrHiddenDisplayBuffer+_absIndex)->y=_point.y;
    }

    void stopSwapping() {canSwapFlag=false;}
    void startSwapping() {canSwapFlag=true;}

    void resizeBuffer(uint16_t _newSize) {
        PRINT("NEW SIZE BUFFER: "); PRINTLN(_newSize);
        // This is a critical, and must be ATOMIC, otherwise the flag may be reset
        // by the ISR before newSizeBuffers is set. Now, this means the displaying
        // engine will briefly stop - but really briefly, and moreover the resizing
        // of the buffer is only done at the end of a rendering figure: not very often.
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { // <-- NOT FOR ARDUINO DUE
            //noInterrupts();
            resizeFlag = true; // <-- this is to be able to finish the previous figure
            newSizeBuffers = _newSize;
            //interrupts();
        }
    }

    // =================================================================
    // =========== Mirror-psitioning ISR that is called every dt =======
    //==================================================================
    // * NOTE: this is perhaps the most critical method. And it has to be
    // as fast as possible!! (in the future, do NOT use analogWrite() !!)
    void displayISR() {

        if (blankingFlag) {
            Hardware::Lasers::setSwitchRed(LOW);
            // TODO: also add delay for laser off? (TODO)
        }

        // Position mirrors:
        int16_t ADCX = (int16_t)( (ptrCurrentDisplayBuffer + readingHead)->x )  ;
        int16_t ADCY = (int16_t)( (ptrCurrentDisplayBuffer + readingHead)->y );
        Hardware::Scanner::setMirrorsTo(ADCX, ADCY);

        // After setting, advance the readingHead on the round-robin buffer:
        // ATTN: if the second operand of / or % is zero the behavior is undefined in C++,
        // hence the condition on sizeBuffer size:
        if (!sizeBuffers) readingHead = (readingHead + 1) % sizeBuffers; // no need to qualify it as volatile
        // since only the ISR will use it.

        if (blankingFlag) {
            // TODO: delay for mirror positioning?
            Hardware::Lasers::setSwitchRed(HIGH);
        }

        // Exchange buffers when finishing displaying the current buffer,
        // but ONLY if canSwapFlag is true, meaning the rendering engine is not
        // still writing in the hidden buffer [check renderFigure() method]:
        if ( canSwapFlag && !readingHead ) { // i.e. if we can swap and we are in the (re)start of buffer
            // * NOTE : the second condition is optional: we could start displaying from
            // the current readingHead - but this will deform the figure when
            // rendering too fast.
            if (resizeFlag) {
                sizeBuffers = newSizeBuffers; // NOTE: sizeBuffers should be volatile
                resizeFlag = false;           // NOTE: resizeFlag should be volatile
                // Restart readingHead? no need, since it IS already zero here:
                //if (readingHead>=sizeBuffers) readingHead = 0;
            }
            // * NOTE : ptrCurrentDisplayBuffer and ptrHiddenDisplayBuffer are volatile!
            volatile P2 *ptrAux = ptrCurrentDisplayBuffer; // could be cast non volatile?
            ptrCurrentDisplayBuffer = ptrHiddenDisplayBuffer;
            ptrHiddenDisplayBuffer = ptrAux;
        }

    }

}
