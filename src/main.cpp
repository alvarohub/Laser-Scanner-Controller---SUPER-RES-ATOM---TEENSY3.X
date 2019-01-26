/*
   Program: Laser Scanner Controller for Super-resolution Microscope
   Author: Alvaro Cassinelli

   Version:
   [18/1/2019] HUGE CHANGES:
      + more versatile communication protocol (preparing for ethernet)
      + A pipeline based (binary) sequencer capable of handling branching and parallel execution, with
        clocks, pulse shapers, trigger detectors and delay lines. Adding more modules would be easy, which
        means we could have synchronized analog signals (such as ramps, to control the microscope stage)
      + Possibility to record command scripts
      + Use of the micro SD File System to load/save command scripts.

   [10/4/2018] MAJOR CHANGE:
      + Changing from the DUE to the Teensy - DUE Dacs are poorly documented and the libs to set the
        PWM frequencies on the other pins either interfere with the dacs or don't work properly.
      + Code properly refactored using namespaces and C++ classes

   [3/4/2018] Quick version not using classes but already having some important features:
      + New robust non-blocking Serial protocol commands
      + Timer interrupts for galvos positioning at precise timing
      + Double buffering using two Ring Buffers to avoid artefacts and delays when re-rendering the figure (ex:
        moving or rotating)
      + PWM frequency raised to 32kHz to be able to use a low pass filter (but not a DC filter!) so the laser power and
        the op-amp offsets can be changed much faster (PWM freq by default is about 490Hz)
      + An initial set of figure primitives (circle, rectangle and line)
      + Programmable inter-point blanking and non-blocking delays (more parameters needed here, like laser on-switch delay)
      + Simplified OpenGL-like pose state variables for translation, resizing and rotation
      + Software reset (using the RST pin wastes a pin, needs a pullup resistor and can be sensitive to noise)

   --YET TO DO:
   - use a command dictionnary/serialiser/parse library (or perhaps at least parseInt or parseFloat? maybe not the last)
   - use a more advanced OpenGL-like renderer (using homography tansformations)

 */

#include <Arduino.h>
#include "hardware.h"
#include "Utils.h" // wrappers for low level methods and other things
#include "messageParser.h"
#include "dataCom.h"

void initSerialCom();

//  ================== SETUP ==================
void setup()
{

  // 1] INIT SERIAL COMMUNICATION:
  Com::ReceiverSerial::init();

  // 2] INIT HARDWARE
  Hardware::init();

  // 3] INIT DISPLAY ENGINE (default is not stand by, but running)
  DisplayScan::init();

  PRINTLN("==SYSTEM READY==");
  //PRINT(">> ");

  // 4] Blink led to show everything went fine(needs to be called after setting pin modes)
  Hardware::blinkLedDebug(4,250000); // period in us

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

  // Update sequencer (if it is inactive, the call will return immediately)
  //Hardware::Sequencer::update();

  //TEST:
  // float t= 1.0*millis()/1000;
  // Graphics::setAngle(45.0*t); // in deg (10 deg/sec)
  // Graphics::setCenter(20.0*cos(2.0*t), 30.0*sin(3.0*t));
  //  Graphics::setScaleFactor(0.2+.3*(1.0+cos(2.5*t)));
  // Renderer2D::renderFigure();

  //delay(200);
}

// ====== GATHER BYTES FROM SERIAL PORT ========================================
// * NOTE: This will fill the messageString until finding a packet terminator
String messageString = "";

void serialEvent() {
   // SerialEvent occurs whenever a new data comes in the hardware serial RX.
  // It is NOT an interrupt routine: the function gets called at the end of each loop()
  // iteration if there is something in the serial buffer - in others
  // words, it is equivalent to a call using "if (Serial.available()) ...".
  // This means we have to avoid saturating the buffer during the loop... this won't happen
  // unless the PC gets really crazy or you use an (horrible) delay in the loop.
  // It won't happen as I use an ISR for the mirror, and the commands are very short).
  Com::ReceiverSerial::receive();
}