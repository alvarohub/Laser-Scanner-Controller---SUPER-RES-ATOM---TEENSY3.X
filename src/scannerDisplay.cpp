#include "scannerDisplay.h"

namespace DisplayScan {

    // Define the extern variables:
    PointBuffer displayBuffer1, displayBuffer2;
    volatile PointBuffer *ptrCurrentDisplayBuffer, *ptrHiddenDisplayBuffer;
    uint16_t readingHead,  newSizeBuffers;
    volatile uint16_t sizeBuffers;
    bool canSwapFlag;
    volatile bool resizeFlag;

    // ======================= OTHERS  =================================
    bool blankingFlag; // inter-point laser on/off (true means off or "blankingFlag")

    void init() {

        // Init hardware stuff:
        Hardware::Scanner::init();


        // Set the displaying buffer pointer to the first ring buffer, and set swapping flags:
        displayBuffer1[0]=P2(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
        displayBuffer2[0]=P2(CENTER_MIRROR_ADX, CENTER_MIRROR_ADY);
        sizeBuffers = newSizeBuffers = 0;
        ptrCurrentDisplayBuffer = &displayBuffer1;
        ptrHiddenDisplayBuffer = &displayBuffer2;
        readingHead = 0;
        canSwapFlag=true;
        resizeFlag=false;

        // 3) Default scan parameters:
        blankingFlag = true;
        setInterPointTime(DEFAULT_RENDERING_INTERVAL);

        // 3) Set interrupt routine (NOTE: "scannerTimer" is a #define using Timer1)
        scannerTimer.begin(displayISR, microseconds);
        stopDisplay();
    }

    void startDisplay() {
        scannerTimer.start(); // we assume period set
    }
    void stopDisplay() {
        scannerTimer.stop();
        // Reset mirrors to center position? Not necessarily!
    }

    void pauseDisplay() {scannerTimer.pause();}
    void resumeDisplay() {scannerTimer.resume();}

    bool getRunningState() { return( scannerTimer.getRunningState() ); }
    bool getPauseState() {return(scannerTimer.getPauseState());}

    void stopSwapping() {canSwapFlag=false;}
    void startSwapping() {canSwapFlag=true;}

    void resizeBuffer(uint16_t _newSize) {
        // This is a critical, and must be ATOMIC, otherwise the flag may be reset
        // by the ISR before newSizeBuffers is set. Now, this means the displaying
        // engine will briefly stop - but really briefly, and moreover the resizing
        // of the buffer is only done at the end of a rendering figure: not very often.
        // ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { // <-- NOT FOR ARDUINO DUE
            PRINT("NEW SIZE TO SET: "); PRINTLN(_newSize);
            noInterrupts();
            resizeFlag = true;
            newSizeBuffers = _newSize;
            interrupts();
            // }
        }

        uint16_t getBufferSize() {return(sizeBuffers);}

        void setInterPointTime(uint16_t _dt) {
            scannerTimer.setPeriod(_dt);
        }
        void setBlankingRed(bool _newBlankState) {
            blankingFlag = _newBlankState;
        }

        void writeOnHiddenBuffer(uint16_t _absIndex, const P2& _point) {
            //(*ptrHiddenDisplayBuffer)[_absIndex].set(_point);
            (*ptrHiddenDisplayBuffer)[_absIndex].x=_point.x;
            (*ptrHiddenDisplayBuffer)[_absIndex].y=_point.y;
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
            int16_t ADCX = (int16_t)((*ptrCurrentDisplayBuffer)[readingHead].x);
            int16_t ADCY = (int16_t)((*ptrCurrentDisplayBuffer)[readingHead].y);
            Hardware::Scanner::setMirrorsTo(ADCX, ADCY);

            // After setting, advance the readingHead on the round-robin buffer:
            // ATTN: if the second operand of / or % is zero the behavior is undefined in C++!!
            if (!sizeBuffers)
            readingHead = (readingHead + 1) % sizeBuffers; // no real need to qualify it as volatile
            // since only the ISR will use it.

            if (blankingFlag) {
                // TODO: delay for mirror positioning?
                Hardware::Lasers::setSwitchRed(HIGH);
            }

            // Exchange buffers when finishing displaying the current buffer,
            // but ONLY if canSwapFlag is true, meaning the rendering engine is not
            // still writing in the hidden buffer [check renderFigure() method]


            if ( canSwapFlag && !readingHead ) {
                if (resizeFlag) {
                    sizeBuffers = newSizeBuffers; // NOTE: sizeBuffers should be volatile
                    resizeFlag = false;           // NOTE: resizeFlag should be volatile
                    // Restart readingHead? no need, since it IS already zero here:
                    //if (readingHead>=sizeBuffers) readingHead = 0;
                }
                // NOTE 1: the second condition is optional: we could start displaying from
                // the current readingHead - but this will deform the figure when
                // rendering too fast.
                // NOTE 2: ptrCurrentDisplayBuffer and ptrHiddenDisplayBuffer are volatile!
                volatile PointBuffer *ptrAux = ptrCurrentDisplayBuffer; // could be cast non volatile?
                ptrCurrentDisplayBuffer = ptrHiddenDisplayBuffer;
                ptrHiddenDisplayBuffer = ptrAux;
            }


        }

    }
