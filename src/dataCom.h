
#ifndef _process_serial_h
#define _process_serial_h

#include "Arduino.h"
#include "Definitions.h"

#define MAX_LENGTH_MESSAGE 200

// Better use const in each namespace? (but puting it in the start of the file is better for easy changes)
#define END_MESSAGE_SERIAL '\n'
#define SERIAL_BAUDRATE 38400

// =======================================================================================
// Data Communication namespace:
namespace Com
{

extern String receivedMessage; // holds the received message (one or more commands)
extern bool requestACK;
extern void update();
extern void setAckMode(bool _mode);
extern bool getAckMode();

namespace ReceiverSerial
{

extern void init();
extern void receive();

} // namespace ReceiverSerial

} // namespace Communication

#endif
