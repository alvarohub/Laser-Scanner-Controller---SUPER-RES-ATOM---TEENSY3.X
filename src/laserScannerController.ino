/*
   Program: Laser Scanner Controller for Super-resolution Microscope
   Author: Alvaro Cassinelli

   Version:
   [10/4/2018] MAJOR CHANGE: changing from the DUE to the Teensy - DUE Dacs are poorly documented and the libraries to set the
   PWM frequencies on the other pins either interfere with the dacs or don't work properly.
   [3/4/2018] Quick version not using classes but already having some important features:
   ++NEW:
 + New robust non-blocking Serial protocol commands
 + Timer interrupts for galvos positioning at precise timing
 + Double buffering using two Ring Buffers to avoid artefacts and delays when re-rendering the figure (ex: moving or rotating)
 + PWM frequency rised to 32kHz to be able to use a low pass filter (but not a DC filter!) so the laser power and the op-amp offsets can be changed much faster (PWM freq by default is about 490Hz)
 + An initial set of figure primitives (circle, rectangle and line)
 + Programmable inter-point blanking and non-blocking delays (more parameters needed here, like laser on-switch delay)
 + Simplified OpenGL-like pose state variables for translation, resizing and rotation
 + Software reset (using the RST pin wastes a pin, needs a pullup resistor and can be sensitive to noise)
   --YET TO DO:
   - use a command dictionnary/serialiser/parse library (or perhaps at least parseInt or parseFloat? maybe not the last)
   - use a more advanced OpenGL-like renderer (using homography tansformations)
   - use classes (like the github code I shared with Nicolas a month ago)
 */

// INCLUDES (in Arduino framework, we need to #include the libs in the "main" compile unit too...)
#include "Arduino.h"
#include "Definitions.h"    // Program constants and MACROS (including hardware stuff)
#include "Utils.h"          // wrappers for low level methods and other things
#include "Class_P2.h"
#include "hardware.h"
#include "scannerDisplay.h"

// for tests in the loop:
#include "renderer2D.h"
#include "graphics.h"

//  ================== SETUP ==================
void setup() {

        // 1] INIT SERIAL COMMUNICATION:
        initSerialCom(); // <-- TODO: make a "Com" namespace to use other communication protocols/hardware

        // 2] INIT HARDWARE
        Hardware::init();

        // 3] INIT DISPLAY ENGINE (default is not stand by, but running)
        DisplayScan::init();

        PRINTLN("==SYSTEM READY==");
        //PRINT(">> ");

        // 4] Blink led (needs to be called after setting pin modes)
        Hardware::blinkLedDebug(2);

        // Check FREE RAM in DEBUG mode:
        //PRINT("Free RAM: "); PRINT(Utils::freeRam()); PRINTLN(" bytes");

        // FOR TESTS:
        // Graphics::clearScene();
        // Graphics::drawCircle(40.0, 60);
        // Renderer2D::renderFigure();
        // DisplayScan::startDisplay();
        // Hardware::Lasers::setCarrierModeAll(true);

}

// ================== MAIN LOOP ==================
void loop()
{

        //  updateSerialCom(); //no need if using a serial event handler!

        // Update laser sequence (for the lasers that have the sequencer active).
        // I do this with polling, but in the future I could use an interrupt (with lower priority than 
        // the periodic software interrupt for the scanner display)
        Hardware::Lasers::updateLaserSequencers();

        //TEST:

        // float t= 1.0*millis()/1000;
        // Graphics::setAngle(45.0*t); // in deg (10 deg/sec)
        // Graphics::setCenter(20.0*cos(2.0*t), 30.0*sin(3.0*t));
        //  Graphics::setScaleFactor(0.2+.3*(1.0+cos(2.5*t)));
        // Renderer2D::renderFigure();

        //delay(200);
}
