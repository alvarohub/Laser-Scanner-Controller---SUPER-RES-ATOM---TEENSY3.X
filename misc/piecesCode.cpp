
if (ptr_module->getName()=="clk[0]")
Serial.print("A:");
            Serial.print(Definitions::binaryNames[state] + " ");
            Serial.println(Definitions::binaryNames[nextState]);

// NOTE/TODO: serialEvent is a function callback in core Arduino Serial: it has to be here. In the future,
// properly integrate this in Com namespace.
void serialEvent()
{
  // SerialEvent occurs whenever a new data comes in the hardware serial RX.
  // It is NOT an interrupt routine: the function gets called at the end of each loop()
  // iteration if there is something in the serial buffer - in others
  // words, it is equivalent to a call using "if (Serial.available()) ...".
  // This means we have to avoid saturating the buffer during the loop... this won't happen
  // unless the PC gets really crazy or you use an (horrible) delay in the loop.
  // It won't happen as I use an ISR for the mirror, and the commands are very short).
  Com::ReceiverSerial::receive();
}

  private: // by making these variables private and not protected, we ensure we don't make mistakes (forgetting
    // to declare the corresponding variables in the child class)
    // ATTN each class muist have a different static counter identifier! problem is, a static
    // variable of the base class cannot be "overloaded". So we need to use different names or caller functions.
    // (C++ has no virtual static data members).
    // Some workaround: //stackoverflow.com/questions/1390913/are-static-variables-in-a-base-class-shared-by-all-derived-classes
    // (but in my case it is simple to just change names, and add myID = id_counterXX and id_counterXX++ in each
    // child class). The base class myID will however tell how many objects have been instantiated, which
    // can be interesting anyway.
    uint8_t myID;
    static uint8_t id_counter;


  // NOTE also, if we want to keep myID different for each class, and since we call methods of the children
    // through a pointer of the base class, we should make it private! But this is not enough: when checking myID
    // from the base class pointer, it will give the base myID!! Let's just use different names, and a virtual getName
    // or overloaded function.


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
