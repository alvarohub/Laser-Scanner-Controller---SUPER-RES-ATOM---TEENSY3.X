
    // HIGHLIGHTING (CSS decoration styles)
  

// * If the incoming character is a packet terminator, proceed to parse the data.
    // The advantages of doing this instead of parsing for EACH command, is that we can
    // have a generic string parser [we can then use ANY other protocol to form the
    // message string, say: OSC, Lora, TCP/IP, etc]. This effectively separates the
    // serial receiver from the parser, and it simplifies debugging.


bool debugSerialActive = true;
bool debugLCDActive = false;
bool debugTFTActive = false;

extern void setDebugSerial(bool _active);
extern void setDebugLCD(bool _active);
extern void setDebugTFT(bool _active);

void buildPipeline() // must be called each time we add or modify a module interconnection
{

	vectorPtrModules.clear();

	// Clocks:
	for (uint8_t k = 1; k < NUM_PULSARS; k++)
	{
		if (arrayClock[k].isActive())
			vectorPtrModules.push_back(&(arrayClock[k]));
	}

	// al triggers:
	for (uint8_t k = 1; k < NUM_EXT_TRIGGERS_IN; k++)
	{
		if (arrayTriggerIn[k].isActive())
			vectorPtrModules.push_back(&(arrayTriggerIn[k]));
	}

	for (uint8_t k = 1; k < NUM_EXT_TRIGGERS_OUT; k++)
	{
		if (arrayTriggerOut[k].isActive())
			vectorPtrModules.push_back(&(arrayTriggerOut[k]));
	}

	// Trigger event detectors:
	for (uint8_t k = 1; k < NUM_TRG_PROCESSORS; k++)
	{
		if (arrayTriggerProcessor[k].isActive())
			vectorPtrModules.push_back(&(arrayTriggerProcessor[k]));
	}

	// Pulsars:
	for (uint8_t k = 1; k < NUM_PULSARS; k++)
	{
		if (arrayPulsar[k].isActive())
			vectorPtrModules.push_back(&(arrayPulsar[k]));
	}

	// Lasers:
	for (uint8_t k = 1; k < NUM_LASERS; k++)
	{
		if (laserArray[k].isActive())
			vectorPtrModules.push_back(&(laserArray[k]));
	}
}
