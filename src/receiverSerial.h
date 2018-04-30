
#ifndef _process_serial_h
#define _process_serial_h

#define MAX_LENGTH_PACKET 200
#define END_PACKET '\n'

#include "Arduino.h"
#include "Definitions.h"

namespace ReceiverSerial {

	extern void init(uint16_t _baud);

	// This routine must be run between each time loop() runs. Multiple bytes of
	// data may be available.  NOTE: we could have used SerialEvent - occurs
	// whenever a new data comes in the hardware serial RX - but then it has to be
	// in the main. Same with Serial.bufferUntil(END_PACKET) - which would call
	// SerialEvent().
	extern bool updateReceive();

	// Data from the LAST RECEIVED PACKET (this info is updated when receiving something in the update() method)
	//char* getReceivedBufferString() { return(bufferStringReceive); }
	extern String getReceivedString();

	extern void setAckMode(bool _mode);
	extern bool getAckMode();

	// Anonymous namespace (the equivalent of a private member if it where a class)
	namespace {
		String receivedString;  // a String to hold incoming data
		bool requestACK;
	}

}  // namespace ReceiverSerial

// NOTE: HERE WE CAN HAVE OTHER NAMESPACES/OBJECTS to RECEIVE DATA! (BLE, Lora,
// OSC, ETHERNET....)
// ...

#endif
