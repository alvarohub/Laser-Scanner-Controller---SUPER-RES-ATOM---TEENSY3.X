//#include <Arduino.h>
#include "Utils.h"                 // wrappers for low level methods and other things
#include "Class_P2.h"
#include "hardware.h"
#include "scannerDisplay.h"

// Create an IntervalTimer object
IntervalTimer myTimer;

const int ledPin = LED_BUILTIN;  // the pin with a LED

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(38400);
  delay(2000);
  PRINTLN(">>REBOOTING... ");
  //myTimer.begin(blinkLED, 100000);  // blinkLED to run every xx ms
}

// The interrupt will blink the LED, and keep
// track of how many times it has blinked.
int ledState = LOW;
volatile unsigned long blinkCount = 0; // use volatile for shared variables

// functions called by IntervalTimer should be short, run as quickly as
// possible, and should avoid calling other functions if possible.
void blinkLED() {
  if (ledState == LOW) {
    ledState = HIGH;
    blinkCount = blinkCount + 1;  // increase when LED turns on
  } else {
    ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}

// The main program will print the blink count
// to the Arduino Serial Monitor
void loop() {
  unsigned long blinkCopy;  // holds a copy of the blinkCount

  // to read a variable which the interrupt code writes, we
  // must temporarily disable interrupts, to be sure it will
  // not change while we are reading.  To minimize the time
  // with interrupts off, just quickly make a copy, and then
  // use the copy while allowing the interrupt to keep working.
  noInterrupts();
  blinkCopy = blinkCount;
  interrupts();

  Serial.print("blinkCount = ");
  Serial.println(blinkCopy);
  delay(1000);
  PRINTLN(">>WORKING... ");
}
