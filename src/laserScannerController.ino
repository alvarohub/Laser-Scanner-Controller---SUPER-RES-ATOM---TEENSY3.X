/*
  Program: Laser Scanner Controller for Super-resolution Microscope
  Author: Alvaro Cassinelli

  Version:
    [10/4/2018] MAJOR CHANGE: changing from the DUE to the Teensy (DUE Dacs are poorly documented and the libraries to set the
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
#include "Utils.h"                 // wrappers for low level methods and other things
#include "Class_P2.h"
#include "hardware.h"
#include "scannerDisplay.h"
//#include "renderer2D.h"

//  ================== SETUP ==================
void setup() {
// ===== INIT SERIAL COMMUNICATION:
  //Com::initSerialCom();
  initSerialCom();
  PRINTLN(">>REBOOTING... ");

  // ===== INIT HARDWARE
  Hardware::init();

  // ===== INIT DISPLAY ENGINE (default is stand by)
  DisplayScan::init();

  // Check FREE RAM in DEBUG mode:
  //PRINT("Free RAM: "); PRINT(Utils::freeRam()); PRINTLN(" bytes");

  PRINTLN(">>READY!");
}

// ================== MAIN LOOP ==================
void loop() {

  //updateSerialCom(); //no need, we are using a serial event handler!

//  analogWrite(PIN_ADCX, (uint16_t)(millis()%4096)); // do not increment current buffer head
//  analogWrite(PIN_ADCY, ((millis()+2048)%4096)); // also increment buffer head


}
