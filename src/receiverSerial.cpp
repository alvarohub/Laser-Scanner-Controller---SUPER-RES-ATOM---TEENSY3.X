
#include "receiverSerial.h"

namespace ReceiverSerial {

void init(uint16_t _baud) {
  Serial.begin(_baud);
  receivedString.reserve(MAX_LENGTH_PACKET);
  // Default acknowlege mode:
  setAckMode(false);
}

// This routine must be run between each time loop() runs. Multiple bytes of
// data may be available.  NOTE: we could have used SerialEvent - occurs whenever
// a new data comes in the hardware serial RX - but then it has to be in the
// main.
bool updateReceive() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    receivedString += inChar;
    if (inChar == END_PACKET) return true;
    // NOTE: receivedString contains the END_PACKET char, important for parsing
  }
  return false;
}

String useReceivedString() {
  String auxString = receivedString;
  receivedString = "";
  return auxString;
  }

  // requestACK mode won't be saved on EEPROM (we don't want to have inconsistent modes between gluons at start)
  void setAckMode(bool _mode) { requestACK = _mode; }
  bool getAckMode() { return(requestACK); }


}  // namespace ReceiverSerial
