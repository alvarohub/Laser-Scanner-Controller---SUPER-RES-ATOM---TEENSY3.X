
#include "messageParser.h" // TODO rename as "parser.h"
#include "dataCom.h"
#include "Definitions.h"

namespace Com
{

// NOTE: HERE WE CAN HAVE OTHER NAMESPACES/OBJECTS to RECEIVE DATA (BLE, Lora,
// OSC, ETHERNET....)
// ...

String receivedMessage;
bool requestACK = false;

// Common methods:
void update() {
  ReceiverSerial::receive();
  // TODO: other com methods (use an "#if def...)
}

void setAckMode(bool _mode) {requestACK = _mode; }
bool getAckMode() { return (requestACK); }

namespace ReceiverSerial
{

void init()
{
  Serial.begin(SERIAL_BAUDRATE);
  receivedMessage.reserve(MAX_LENGTH_MESSAGE);
  receivedMessage = "";

  // Default acknowledge mode:
  setAckMode(false);

   while (!Serial)
    ;

    PRINTLN("> SERIAL PORT READY");

}

void receive() // NOTE: no need to declare it before the setup, it is declared in Arduino.h
{
  // VERY TECHNICAL NOTE: the following will try to read everything in the buffer here as
  // multiple bytes may be available? If we do so, and since during parsing MORE bytes from
  // a new packet may be arriving we will not get outside the While loop if we send packets
  // continuously! This is ok since this means we are executing many commands.
  // Since there is nothing in the loop (ISR), everything is ok. However, in the future
  // it may be better to have a separate THREAD for this using an RTOS [something the
  // Arduino basic framework cannot do, but MBED and probably Teensy-Arduino can]
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    receivedMessage += inChar;

    if (inChar == END_MESSAGE_SERIAL) // NOTE: using the serial port, the only way to send many commands at once
    // is using a special character between the commands different from end of line, since the end of line will
    // send the data though the serial port (END_MESSAGE_SERIAL is enf of line!)
    {
      Parser::parseStringMessage(receivedMessage); //parse AND calls the appropiate functions
      receivedMessage = "";
    }
  }
}

} // namespace ReceiverSerial


} // namespace DataReceiver
