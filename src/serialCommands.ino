
#include "Definitions.h"
#include "Utils.h"

#include "renderer2D.h"

// I include the following or low level stuff (but perhaps better to use Utils wrappers in the future):
#include "scannerDisplay.h"
#include "graphics.h"

// ==================== PARSER SETTINGS:
// Uncomment the following if you want the parser to continue parsing after a bad character (better not to do that)
//#define CONTINUE_PARSING

// ==================== COMMANDS:
// 1) Laser commands:

// a) Per-Laser:
#define SET_POWER_LASER "PWLASER"  // Param: laser index, 0 to MAX_LASER_POWER (0-4095, 12 bit res). Changes current state.
#define SET_SWITCH_LASER "SWLASER" // Param: laser index, [0-1],SWLASER. Set the chopping ultrafast switch. Changes current state.
#define SET_CARRIER "CARRIER"      // laser num + 0/1 where 0 means no carrier: when switch open, the laser shines continuously at the \
                                   // current power (filtered PWM), otherwise it will be a 50% PWM [chopping the analog power value

#define SET_PARAM_SEQUENCE "SET_SEQPARAM" // Param: laser index, t_delay, t_off (in us), trigger decimation
#define SET_SEQUENCER "SET_SEQ"           // Param: laser index, [0-1] to deactivate/activate sequencer
#define SET_SEQUENCER_ALL "SET_SEQ_ALL"   // Param: [0-1] to deactivate/activate all laser sequencers
#define SET_LASER_TRIGGER "SET_TRIG"      // Param: laser index, trigger number (-1 for external trigger, \
                                          // otherwise the index of another laser (0-3), trigger mode (0=RISE, 1=FALL, 2=CHANGE)
#define RESET_SEQUENCER "RST_SEQ"         // Param: none. This is not the same than switching off and on the sequencer: the later does not affect the \
                                          // actual sequencer/trigger states, but makes the laser unresponsive to the sequencer output.

// b) Simultaneously affecting all lasers:
#define SET_POWER_LASER_ALL "PWLASERALL"  // Param: 0 to MAX_LASER_POWER (0-4095, 12 bit res). TODO: per laser.
#define SET_SWITCH_LASER_ALL "SWLASERALL" // Param: [0-1],SWLASER. Will open/close the laser ultrafast switch.
#define SET_CARRIER_ALL "CARRIERALL"

#define TEST_LASERS "TSTLASERS" // no parameters. Will test each laser with a power ramp

// 2) Opto Tunners (note: these outputs are not "chopped" by a fast switch, i.e., there is no "carrier-mode"):
#define SET_POWER_OPTOTUNER_ALL "PWOPTOALL" // one value (power)
#define SET_POWER_OPTOTUNER "PWOPTO"        // two values (index of optotunner + power)

// 3) READOUT DIGITAL and ANALOG PINS:
#define SET_DIGITAL_A "WDIG_A"  // write digital pin A (pin 31). Parameter: 0,1
#define SET_DIGITAL_B "WDIG_B"  // write digital pin B (pin 32). Parameter: 0,1
#define READ_DIGITAL_A "RDIG_A" // read digital pin A (pin 31).
#define READ_DIGITAL_B "RDIG_B" // read digital pin B (pin 32).

#define SET_ANALOG_A "WANA_A"  // write ANALOG (pwm) pin A (pin 16). Parameter: 0-4095
#define SET_ANALOG_B "WANA_B"  // write ANALOG (pwm) pin B (pin 17). Parameter: 0-4095
#define READ_ANALOG_A "RANA_A" // read analog pin A (pin 16), 12 bit resolution.
#define READ_ANALOG_B "RANA_B" // read analog pin B (pin 17), 12 bit resolution.

// 4) Display Engine (on ISR) commands (namespace Hardware::Scan):
#define START_DISPLAY "START"   // start the ISR for the displaying engine
#define STOP_DISPLAY "STOP"     // stop the displaying ISR
#define SET_INTERVAL "DT"       // parameter: inter-point time in us (min about 20us)
#define DISPLAY_STATUS "STATUS" // show various settings. Note that the number of points in the \
                                // current blueprint (or "figure"), and the size of the \
                                // displaying buffer may differ because of clipping.
#define SET_SHUTTER "SHUTTER"

// 5) Figures and pose:
// * NOTE : each time these commands are called, the current figure (in blueprint) is
// re-rendered with the new transforms, in this order of transformation: rotation/scale/translation
#define RESET_POSE_GLOBAL "RSTPOSE" // set angle to 0, center to (0,0) and factor to 1
#define SET_ANGLE_GLOBAL "ANGLE"    // Param: angle (deg),ANGLE
#define SET_CENTER_GLOBAL "CENTER"  // Param: x,y,CENTER
#define SET_FACTOR_GLOBAL "FACTOR"  // Param: factor (0-...),FACTOR
#define SET_COLOR_GLOBAL "COLOR"    // TODO

//6) Scene clearing and blanking between objects (only useful when having many figures simultanesouly)
#define CLEAR_SCENE "CLEAR" // clear the blueprint, and also stop the display
#define CLEAR_MODE "CLMODE" // [0-1],CLMODE. When set to 0, if we draw a figure it will                 \
                            // be ADDED to the current scene. Otherwise drawing first clear the current \
                            // scene and make a new figure.

// The following commands affect all lasers simultaneously [TODO: per laser]
#define SET_BLANKING_ALL "BLANKALL" //Figure-to-figure blanking. Param: [0/1]. Affects all lasers.
#define SET_BLANKING "BLANK"        // per laser fig-to-fig blanking

#define SET_INTER_POINT_BLANK "PTBLANK" // pt-to-pt blanking. ALWAYS affects all lasers for the time being.

// 7) Figure primitives:
#define MAKE_LINE "LINE"      // Param: width,height,numpoints,LINE [from (0,0)] or posX,posY,length,height,numpoint,LINE
#define MAKE_CIRCLE "CIRCLE"  // Param: radius,numpoints,CIRCLE ou X,Y,radius,numpoints,CIRCLE
#define MAKE_RECTANGLE "RECT" // Param: width,height,numpointX,numPointX,RECT ou X,Y,width,height,numpointsX, numpointY,RECT
#define MAKE_SQUARE "SQUARE"  // Param: size of side,numpoints side,RECT ou X,Y,size-of-side,RECT
#define MAKE_ZIGZAG "ZIGZAG"  // Param: width,height,numpoints X,numpoints Y,ZIGZAG or with position first
#define MAKE_SPIRAL "SPIRAL"  // Param: length-between-arms, num-tours, numpoints, SPIRAL

// 8) TEST FIGURES:
#define LINE_TEST "LITEST"
#define CIRCLE_TEST "CITEST"  // no parameters: makes a circle.
#define SQUARE_TEST "SQTEST"  // no parameters: makes a square
#define COMPOSITE_TEST "MIRE" // no parameters: cross and squares (will automatically launch START)
// NOTE: we can see the blanking problem here because it is a composite figure

// 9) LOW LEVEL FUNCTIONS and CHECKING COMMANDS:
#define TEST_MIRRORS_RANGE "SQRANGE" // time of show in seconds, SQRANGE (square showing the limits of galvos)
#define TEST_CIRCLE_RANGE "CIRANGE"  // time of show in seconds, CIRANGE (CIRCLE taille de diametre 200 centered on (0,0))
#define TEST_CROSS_RANGE "CRRANGE"   // time of show in seconds, CRRANGE (CROSS centered on 0,0)
#define SET_DIGITAL_PIN "SETPIN"     // pin number, state(true/false), SETPIN
#define RESET_BOARD "RESET"          // RESET the board (note: this will disconnect the serial port)

// =============================================================================
String messageString;
#define MAX_LENGTH_STACK 20
String argStack[MAX_LENGTH_STACK]; // number stack storing numeric parameters for commands (for RPN-like parser).
                                   //Note that the data is saved as a String - it will be converted to int, float or whatever
                                   // by the specific method associated with the correnspondent command.
String cmd;                        // we will parse one command at a time

enum stateParser
{
  START,
  NUMBER,
  SEPARATOR,
  CMD
} myState;
//Note: the name of an unscoped enumeration may be omitted: such declaration only introduces the enumerators into the enclosing scope.

// Initialization:
void initSerialCom()
{
  Serial.begin(SERIAL_BAUDRATE);
  messageString.reserve(200);
  messageString = "";
}

// ====== GATHER BYTES FROM SERIAL PORT ========================================
// * NOTE: This will fill the messageString until finding a packet terminator
void serialEvent()
{
  // SerialEvent occurs whenever a new data comes in the hardware serial RX.
  // It is NOT an interrupt routine: the function gets called at the end of each loop()
  // iteration if there is something in the serial buffer - in others
  // words, it is equivalent to a call using "if (Serial.available()) ...".
  // This means we have to avoid saturating the buffer during the loop... this won't happen
  // unless the PC gets really crazy or you use an (horrible) delay in the loop.
  // It won't happen as I use an ISR for the mirror, and the commands are very short).

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
    messageString += inChar;
    // * If the incoming character is a packet terminator, proceed to parse the data.
    // The advantages of doing this instead of parsing for EACH command, is that we can
    // have a generic string parser [we can then use ANY other protocol to form the
    // message string, say: OSC, Lora, TCP/IP, etc]. This effectively separates the
    // serial receiver from the parser, and it simplifies debugging.
    if (inChar == END_PACKET)
    {
      parseStringMessage(messageString); // parse AND calls the appropiate functions
      messageString = "";
    }
  }
}

// =============================================================================================
// ======================== PARSE THE MESSAGE ==================================================
// NOTE: I tested this throughly and it seems to work faultlessly as for 5.April.2018
String cmdString;
uint8_t numArgs;
bool cmdExecuted;

void resetParser()
{ // without changing the parsing index [in the for loop]:
  numArgs = 0;
  for (uint8_t i = 0; i < MAX_LENGTH_STACK; i++)
    argStack[i] = "";
  cmdString = "";
  cmdExecuted = false; // = true; // we will "AND" this with every command correctly parsed in the string
  myState = START;
}

bool parseStringMessage(const String &_messageString)
{
  static String oldMessageString = ""; // oldMessageString is for repeating last GOOD string-command

  String messageString = _messageString;

  resetParser();

  // Note: messageString contains the END_PACKET char, otherwise we wouldn't be here;
  // So, going through the for-loop with the condition i < messageString.length()
  // is basically the same as using the condition: _messageString[i] != END_PACKET
  for (uint8_t i = 0; i < messageString.length(); i++)
  {
    char val = _messageString[i];

    //PRINT(">"); PRINT((char)val);PRINT(" : "); PRINTLN((uint8_t)val);

    // Put ASCII characters for a number (base 10) in the current argument in the stack.
    // => Let's not permit to write a number AFTER a command.
    if ((val >= '-') && (val <= '9') && (val != '/')) //this is '0' to '9' plus '-' and '.' to form floats and negative numbers
    {
      if ((myState == START) || (myState == SEPARATOR))
        myState = NUMBER;
      if (myState == NUMBER)
      { // it could be in CMD state...
        argStack[numArgs] += val;
        //  PRINTLN(" (data)");
      }
      else
      { // actually this just means myState == CMD
        PRINTLN("> BAD FORMED PACKET");
#ifdef CONTINUE_PARSING
        resetParser(); // reset the parser, but continue from next string character.
#else
        break;
#endif
      }
    }

    // Put letter ('A' to 'Z') in cmdString.
    // => Let's not permit to start writing a command if we did not finish a number or
    // nothing was written before.
    else if (((val >= 'A') && (val <= 'Z')) || (val == '_') || (val == ' ')) // accept composing commands with "_" (ex: MAKE_CIRCLE)
    {

      if ((myState == START) || (myState == SEPARATOR))
        myState = CMD;
      if (myState == CMD)
      { // Could be in NUMBER state
        cmdString += val;
        //   PRINTLN(" (command)");
      }
      else
      {
        PRINTLN("> BAD FORMED PACKET");
#ifdef CONTINUE_PARSING
        resetParser(); // reset the parser, but continue from next string character.
#else
        break;     // abort parsing - we could also restart it from here with resetParser()
#endif
      }
    }

    // Store numeric argument when receiving a NUMBER_SEPARATOR
    else if (val == NUMBER_SEPARATOR)
    {
      // NOTE: do not permit a number separator AFTER a command, another separator,
      // or nothing: only a separator after a number is legit
      if (myState == NUMBER) // no need to test: }&&(argStack[numArgs].length() > 0)) {
      {
        //PRINTLN(" (separator)");
        //PRINT("> ARG n."); PRINT(numArgs); PRINT(" : "); PRINTLN(argStack[numArgs]);
        numArgs++;
        myState = SEPARATOR;
      }
      else
      { // number separator without previous data, another number separator or command
        PRINTLN("> BAD FORMED PACKET");
        //PRINT_LCD("> BAD FORMED ARG LIST");
#ifdef CONTINUE_PARSING
        resetParser(); // reset parsing data, and continue from next character.
#else
        break;     // abort parsing (we could also restart it from here with resetParser())
#endif
      }
    }

    else if ((val == END_PACKET) || (val == COMMAND_SEPARATOR))
    { // or another CMD terminator code...
      if (myState == START)
      { // implies cmdString.length() equal to zero
        // END of packet received WITHOUT a command:
        // repeat command IF there was a previous good command:
        // NOTE: for the time being, this discards the rest of the message
        // (in case it came from something else than a terminal of course)
        if (val == END_PACKET)
        {
          if (oldMessageString != "")
          {
            PRINTLN(oldMessageString);
            parseStringMessage(oldMessageString); // <<== ATTN: not ideal perhaps to use recurrent
            //  call here.. but when using in command line input, the END_PACKET is the
            // last character, so there is no risk of deep nested calls (on return the parser
            // will end in the next loop iteration)
          }
          else
          {
            PRINTLN("> NO PREVIOUS COMMAND TO REPEAT");
#ifdef CONTINUE_PARSING
            resetParser(); // reset parsing data, and continue from next character.
#else
            break; // abort parsing (we could also restart it from here with resetParser())
#endif
          }
        }
        else
        { // concatenator without previous CMD
          PRINTLN("> BAD FORMED PACKET");
#ifdef CONTINUE_PARSING
          resetParser(); // reset parsing data, and continue from next character.
#else
          break;   // abort parsing (we could also restart it from here with resetParser())
#endif
        }
      }
      else if (myState == CMD)
      { // anything else before means bad formed!
        // * Note 1 : state == CMD here implies cmdString.length() > 0
        // * Note 2 : we don't check argument number, can be anything including nothing.

        PRINT("> EXEC: ");
        for (uint8_t k = 0; k < numArgs; k++)
        {
          PRINT(argStack[k]);
          PRINT(",");
        }
        PRINTLN(cmdString);

        //PRINT("> trying execution... ");
        // *********************************************************
        cmdExecuted = interpretCommand(cmdString, numArgs, argStack);
        // *********************************************************

        if (cmdExecuted)
        { // save well executed command string:
          oldMessageString = messageString;
          PRINTLN("> OK");
          //PRINT("> ");
          //Hardware::blinkLedMessage(2);
        }
        else
        {
          PRINTLN("> FAIL");
          //PRINT("> ");
        }

        // NOTE: commands can be concatenated AFTER and END_PACKET in case of input from something else than a terminal,
        // so we need to restart parsing from here (will only happen when input string is stored in memory or sent from OSC, etc)
        resetParser();
      }
      else
      {
        PRINTLN("> BAD FORMED PACKET");
#ifdef CONTINUE_PARSING
        resetParser(); // reset parsing data, and continue from next character.
#else
        break;     // abort parsing (we could also restart it from here with resetParser())
#endif
      }
    }
    else if (val == LINE_FEED_IGNORE)
    {
      // do nothing with this, continue parsing
    }

    else
    { // this means we received something else (not a number, not a letter from A-Z,
      // not a number, packet terminator, not a concatenator)
      PRINTLN("> BAD FORMED PACKET");
//PRINT("> ");
#ifdef CONTINUE_PARSING
      resetParser(); // reset parsing data, and continue from next character.
#else
      break;       // break the for-loop of parsing
#endif
    }
  } // end parse for-loop

  return (cmdExecuted);
}

// =============================================================================
// ========== COMMAND INTERPRETER ==============================================
// NOTE: Here you can add commands at will; but I would wait until I can do a
// proper command dictionnary so as to simplify the way you write the new ones.
// =============================================================================
bool interpretCommand(String _cmdString, uint8_t _numArgs, String argStack[])
{
  bool execFlag = false;

  //==========================================================================
  //========== LASER COMMANDS ================================================
  //==========================================================================
  if (_cmdString == SET_POWER_LASER_ALL)
  { // Param: 0 to 4096 (12 bit res).
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStatePowerAll(constrain(argStack[0].toInt(), 0, MAX_LASER_POWER));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_POWER_LASER)
  { // Param: laser number, power (0 to 4096, 12 bit res).
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStatePower(argStack[0].toInt(), constrain(argStack[1].toInt(), 0, MAX_LASER_POWER));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SWITCH_LASER_ALL)
  { // Param: 0 to 4096 (12 bit res).
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStateSwitchAll(argStack[0].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SWITCH_LASER)
  {
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStateSwitch(argStack[0].toInt(), argStack[1].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_CARRIER_ALL)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStateCarrierAll(argStack[0].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_CARRIER)
  {
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStateCarrier(argStack[0].toInt(), argStack[1].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SHUTTER)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setShutter(argStack[0].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SEQUENCER)
  {
    // Param: - laser index,
    //        - 0/1 to deactivate/activate sequencer
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStateSequencer(argStack[0].toInt(), argStack[1].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }
  else if (_cmdString == SET_SEQUENCER_ALL)
  {
    // Param:  0/1 to deactivate/activate sequencer for all lasers
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStateSequencerAll(argStack[0].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_PARAM_SEQUENCE)
  {
    // Param: (1) laser index
    //        (2) t_delay
    //        (3) t_off (in us)
    if (_numArgs == 3)
    {
      //PRINTLN("> EXECUTING... ");
      uint8_t laserIndex = argStack[0].toInt();
      Hardware::Lasers::LaserArray[laserIndex].setSequencerParam(argStack[1].toInt(),
                                                                 argStack[2].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_LASER_TRIGGER)
  {
    // Params: (1) laser index
    //         (2) trigger source (-1 for external trigger, otherwise the index of another laser (0-3)
    //         (3) trigger mode (0=RISE, 1=FALL, 2=CHANGE)
    //         (4) trigger decimation (>0)
    //         (5) trigger offset
    if (_numArgs == 5)
    {
      //PRINTLN("> EXECUTING... ");

      //Laser* laser_ptr = &(Hardware::Lasers::LaserArray[argStack[0].toInt()]);
      //laser_ptr->setTriggerSource(argStack[1].toInt());
      //laser_ptr->setTriggerMode(argStack[2].toInt());
      //laser_ptr->setTriggerDecimation(argStack[3].toInt());
      //laser_ptr->setTriggerOffset(argStack[4].toInt());

      Hardware::Lasers::LaserArray[argStack[0].toInt()].setTriggerParam(argStack[1].toInt(),  // int8_t _source
                                                                        argStack[2].toInt(),  // uint8_t _mode
                                                                        argStack[3].toInt(),  // uint16_t _decimation
                                                                        argStack[4].toInt()); //uint16_t _offset

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == RESET_SEQUENCER)
  {
    // Param: (1) laser index
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::LaserArray[argStack[0].toInt()].restartSequencer();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_LASERS)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::test();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  //=============== OPTOTUNNER COMMANDS  =====================================
  //==========================================================================
  else if (_cmdString == SET_POWER_OPTOTUNER_ALL) // Param: 0 to 4096 (12 bit res)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::OptoTuners::setStatePowerAll(constrain(argStack[0].toInt(), 0, MAX_OPTOTUNE_POWER));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_POWER_OPTOTUNER) // Param: laser number, power (0 to 4096, 12 bit res)
  {
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::OptoTuners::setStatePower(argStack[0].toInt(),
                                          constrain(argStack[1].toInt(), 0, MAX_OPTOTUNE_POWER));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  //============== SCANNER COMMANDS  =========================================
  //==========================================================================
  else if (_cmdString == START_DISPLAY)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::startDisplay();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == STOP_DISPLAY)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::stopDisplay();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_INTERVAL)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::setInterPointTime((uint16_t)atol(argStack[0].c_str())); // convert c-string to long, then cast to unsigned int
                                                                           // the method strtoul needs a c-string, so we need to
                                                                           // convert the String to that.
      //DisplayScan::setInterPointTime(strtoul(argStack[0].c_str(),NULL,10); // base 10
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == DISPLAY_STATUS)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");

      PRINT(" 1-CLEAR MODE: ");
      if (Graphics::getClearMode())
        PRINTLN("ON");
      else
        PRINTLN("OFF");

      PRINT(" 2-SCENE PTS: ");
      PRINTLN(Renderer2D::getSizeBlueprint());

      PRINT(" 3-DISPLAY ISR: ");
      if (DisplayScan::getRunningState())
        PRINT("ON");
      else
        PRINT("OFF");
      PRINT(" / PERIOD: ");
      PRINT(DisplayScan::getInterPointTime());
      PRINT(" us");
      PRINT(" / BUFFER: ");
      PRINT(DisplayScan::getBufferSize());
      PRINTLN(" points");

      PRINT(" 6-INTERPOINT BLANKING: ");
      if (DisplayScan::getInterPointBlankingMode())
        PRINTLN("ON");
      else
        PRINTLN("OFF");

      PRINTLN(" 7-LASERS : ");
      Laser::LaserState laserState;
      for (uint8_t k = 0; k < NUM_LASERS; k++)
      {
        laserState = Hardware::Lasers::LaserArray[k].getCurrentState();
        PRINT("     ");
        PRINT(Hardware::Lasers::laserNames[k]);
        PRINT("\t LASER [ power = ");
        PRINT(laserState.power);
        PRINT(", switch = ");
        PRINT(laserState.stateSwitch);
        PRINT(", carrier = ");
        PRINT(laserState.stateCarrier);
        PRINT(", sequencer = ");
        PRINT(laserState.stateSequencer);
        PRINT(", blanking = ");
        PRINT(laserState.stateBlanking);
        PRINTLN("]");

        Laser *laser = &(Hardware::Lasers::LaserArray[k]);

        PRINT("\t SEQUENCE (ms) [ t_delay = ");
        PRINT(laser->t_delay_ms);
        PRINT(", t_on = ");
        PRINT(laser->t_on_ms);
        PRINTLN("]");

        PRINT("\t TRIGGER [ source = ");
        PRINT(laser->getTriggerSource());
        PRINT(", mode = ");
        PRINT(laser->getTriggerMode());
        PRINT(", decimation = ");
        PRINT(laser->getTriggerSkipNumEvents());
        PRINT(", offset = ");
        PRINT(laser->getTriggerOffsetEvents());
        PRINTLN("]");
      }
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  // ========== POSE PARAMETERS ("OpenGL"-like state machine) ================
  //==========================================================================
  //  * NOTE 1 : This is a very simplified "open-gl" like rendering engine, but
  //    should be handy anyway. It works as follows: the pose parameters are applied
  //    whenever we draw a figure; that's why it is interesting to have parameters-less,
  //    normalized primitives: when modifying the angle, center and scale, EVERYTHING
  //    being displayed is rescaled, rotated and/or translated.
  //  * NOTE 2 : In the future, use a MODELVIEW MATRIX instead, and make some
  //    methods to set "passive transforms" (as in my laserSensingDisplay code)
  //  * NOTE 3 : Instead of doing Graphics::... we could just do:
  //                     using namespace Graphics;
  //    However, I prefer the suffix for clariry (my Object Oriented biais...)
  //==========================================================================
  else if (_cmdString == RESET_POSE_GLOBAL)
  { // Param: none
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::resetGlobalPose();
      // As explained above, we need to RE-RENDER the display buffer:
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_ANGLE_GLOBAL) // Param: angle in DEG (float)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setAngle(argStack[0].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_CENTER_GLOBAL) // Param: x,y
  {
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setCenter(argStack[0].toFloat(), argStack[1].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_FACTOR_GLOBAL) // Param: scale
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setScaleFactor(argStack[0].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_COLOR_GLOBAL) // Param: color bool [TODO: real colors]
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setColorRed(argStack[0].toInt());
      Renderer2D::renderFigure(); // <<== this will really be useful soon, as we will have a per-point color
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  //================ FIGURES (check Graphics namespace) ======================
  // * NOTE 1 : after all the figure composition, it is
  // imperative to call to the method Renderer2D::renderFigure().
  // * NOTE 2 : The pose parameters are COMPOSED with the global ones.
  // * NOTE 3 : Depending on the number of args, different pre-sets are used
  //==========================================================================

  // == CLEAR SCENE and CLEAR MODE ==========================================
  else if (_cmdString == CLEAR_SCENE)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      // The sequence order and items is arbitrary:
      Graphics::clearScene();
      // clear also the pose parameters - otherwise there is a lot of confusion:
      Graphics::resetGlobalPose();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == CLEAR_MODE)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setClearMode(argStack[0].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_BLANKING_ALL)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      // This is delicate: we need to stop the displaying engine, and reset it (in particular
      // the style stack, or we may run into overflows because the variable affects the program flow)
      Hardware::Lasers::setStateBlankingAll(argStack[0].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }
  else if (_cmdString == SET_BLANKING)
  {
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      // This is delicate: we need to stop the displaying engine, and reset it (in particular
      // the style stack, or we may run into overflows because the variable affects the program flow),
      // OR, we don't use the style stack (I decided for the later for the time being)
      Hardware::Lasers::setStateBlanking(argStack[0].toInt(), argStack[1].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_INTER_POINT_BLANK)
  { // for the time being, this is for ALL lasers:
    // for the time being, this is a "DisplayScan"
    // method [in the future, a per-laser method?]
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::setInterPointBlankingMode(argStack[0].toInt() > 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // == MAKE LINE ==========================================
  else if (_cmdString == MAKE_LINE)
  {
    switch (_numArgs)
    {
    case 3: //origin at (0,0)
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawLine(
          argStack[0].toFloat(), argStack[1].toFloat(),
          argStack[2].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
      break;
    case 5:
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      // from point, lenX, lenY, num points
      P2 startP2(argStack[0].toFloat(), argStack[1].toFloat());
      Graphics::drawLine(
          startP2,
          argStack[2].toFloat(), argStack[3].toFloat(),
          argStack[4].toInt());
      Renderer2D::renderFigure();
    }
      execFlag = true;
      break;
    default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  // == MAKE CIRCLE ==========================================
  // a) Depending on the number of arguments, we do something different:
  //    - with one parameter (nb points), we draw a circle in (0,0) with unit radius
  //      [of course, the radius is multiplied by the currrent scaling factor]
  else if (_cmdString == MAKE_CIRCLE)
  {
    switch (_numArgs)
    {
    case 2: // radius + num points [centered]
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawCircle(argStack[0].toFloat(), argStack[1].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
      break;
    case 4:
    { // center point, radius, num points
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      P2 centerP2(argStack[0].toFloat(), argStack[1].toFloat());
      Graphics::drawCircle(centerP2, argStack[2].toFloat(), argStack[3].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    break;
    default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  // == MAKE RECTANGLE ==========================================
  else if (_cmdString == MAKE_RECTANGLE)
  {
    switch (_numArgs)
    {
    case 4: // [centered]
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawRectangle(
          argStack[0].toFloat(), argStack[1].toFloat(),
          argStack[2].toInt(), argStack[3].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
      break;
    case 6:
    { // From lower left corner:
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      P2 fromP2(argStack[0].toFloat(), argStack[1].toFloat());
      Graphics::drawRectangle(
          fromP2,
          argStack[2].toFloat(), argStack[3].toFloat(),
          argStack[4].toInt(), argStack[5].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    break;
    default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  // == MAKE SQUARE ==========================================
  else if (_cmdString == MAKE_SQUARE)
  {
    switch (_numArgs)
    {
    case 2: // side, num point [centered]
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawSquare(argStack[0].toFloat(), argStack[1].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
      break;
    case 4:
    { // center point, radius, num points
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      P2 fromP2(argStack[0].toFloat(), argStack[1].toFloat());
      Graphics::drawSquare(fromP2, argStack[2].toFloat(), argStack[3].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    break;
    default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  else if (_cmdString == MAKE_ZIGZAG)
  {
    switch (_numArgs)
    {
    case 4: // Centered on [0,0]
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawZigZag(
          argStack[0].toFloat(), argStack[1].toFloat(),
          argStack[2].toInt(), argStack[3].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
      break;
    case 6:
    { // from left bottom corner:
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      P2 fromP2(argStack[0].toFloat(), argStack[1].toFloat());
      Graphics::drawZigZag(
          fromP2,
          argStack[2].toFloat(), argStack[3].toFloat(),
          argStack[4].toInt(), argStack[5].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    break;
    default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  else if (_cmdString == MAKE_SPIRAL)
  {
    switch (_numArgs)
    {
    case 5:
    {
      Graphics::updateScene();
      P2 center(argStack[0].toFloat(), argStack[1].toFloat());
      Graphics::drawSpiral(
          center,
          argStack[2].toFloat(), // radius arm [ r= radiusArm * theta ]
          argStack[3].toFloat(), // num tours (float)
          argStack[4].toInt()    // num points
      );
      Renderer2D::renderFigure();
      execFlag = true;
    }
    break;
    case 3:
      Graphics::updateScene();
      Graphics::drawSpiral(
          argStack[0].toFloat(), // radius arm [ r= radiusArm * theta ]
          argStack[1].toFloat(),
          argStack[2].toInt());
      Renderer2D::renderFigure();
      execFlag = true;
      break;
    default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  // ....

  // F) TEST FIGURES [this is a test: scene is CLEARED whatever the clear state,
  // and displaying is STARTED whatever the previous state]. Laser Color and mode
  // is given by the currest state of lasers.

  // a) LINE TEST:
  else if (_cmdString == LINE_TEST)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::clearScene();

      Graphics::drawLine(P2(-50.0, -30), 100, 60, 50); // fisrt argument is the radius
      // REM: equal to: Graphics::setScaleFactor(500); Graphics::drawCircle(100);

      // NOTE: the color attributes will be used by the renderer in the future.
      // USE CURRENT PARAMETERS:
      //Hardware::Lasers::setStatePowerRed(1000);
      //Hardware::Lasers::setStateSwitchRed(true);

      Renderer2D::renderFigure();

      DisplayScan::startDisplay(); // start engine, whatever the previous state

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // b) CIRCLE, NO PARAMETERS [500 ADC units radius circle, centered, 100 points]
  else if (_cmdString == CIRCLE_TEST)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::clearScene();

      Graphics::drawCircle(50.0, 100); // fisrt argument is the radius
      // REM: equal to: Graphics::setScaleFactor(500); Graphics::drawCircle(100);

      // NOTE: the color attributes will be used by the renderer in the future.
      // USE CURRENT PARAMETERS:
      //Hardware::Lasers::setStatePowerRed(1000);
      //Hardware::Lasers::setStateSwitchRed(true);

      Renderer2D::renderFigure();

      DisplayScan::startDisplay(); // start engine, whatever the previous state

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // c) : SQUARE, NO PARAMETERS [500 ADC units side, centered, 10 points/side]
  else if (_cmdString == SQUARE_TEST)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::clearScene();
      Graphics::drawSquare(100, 50.0); //length of side, num points per side

      // NOTE: the color attributes will be used by the renderer in the future.
      // USE CURRENT PARAMETERS:
      //Hardware::Lasers::setStatePowerRed(1000);
      //Hardware::Lasers::setStateSwitchRed(true);

      Renderer2D::renderFigure();

      DisplayScan::startDisplay();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // d) : SQUARE + CIRCLE TEST
  else if (_cmdString == COMPOSITE_TEST)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      float radius = 75;
      Graphics::clearScene();
      Graphics::drawSquare(2 * radius, 50.0);
      Graphics::drawCircle(radius, 100.0);
      Graphics::drawSquare(1.414 * radius, 50.0);
      Graphics::drawLine(P2(-90, 0), 180, 0, 50.0);
      Graphics::drawLine(P2(0, -90), 0, 180, 50.0);

      // NOTE: the color attributes will be used by the renderer in the future.
      // USE CURRENT PARAMETERS:
      //Hardware::Lasers::setStatePowerRed(1000);
      //Hardware::Lasers::setStateSwitchRed(true);

      Renderer2D::renderFigure();

      DisplayScan::startDisplay();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // .........................................................................
  // ... BUILD HERE WHAT YOU NEED
  // .........................................................................

  //==========================================================================
  //=====================  LOW LEVEL COMMANDS ================================
  //==========================================================================
  else if (_cmdString == SET_DIGITAL_PIN)
  { // Param:pin, state
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setDigitalPin(argStack[0].toInt(), argStack[1].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // Wrappers for special pins (exposed in the D25 connector):
  else if (_cmdString == SET_DIGITAL_A)
  { // Param: state
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setDigitalPinA(argStack[0].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_DIGITAL_B)
  { // Param: state
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setDigitalPinB(argStack[0].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == READ_DIGITAL_A)
  { // Param: none
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      bool val = Hardware::Gpio::readDigitalPinA();
      execFlag = true;
      PRINT("> ");
      PRINTLN((val > 0 ? "1" : "0"));
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == READ_DIGITAL_B)
  { // Param: none
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      bool val = Hardware::Gpio::readDigitalPinB();
      execFlag = true;
      PRINT("> ");
      PRINTLN((val > 0 ? "1" : "0"));
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_ANALOG_A)
  { // Param: duty cycle (0-4095)
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setAnalogPinA(argStack[0].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_ANALOG_B)
  { // Param: duty cycle (0-4095)
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setAnalogPinB(argStack[0].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == READ_ANALOG_A)
  { // Param: none
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      uint16_t val = Hardware::Gpio::readAnalogPinA();
      execFlag = true;
      PRINT("> ");
      PRINTLN(val);
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == READ_ANALOG_B)
  { // Param: none
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      uint16_t val = Hardware::Gpio::readAnalogPinB();
      execFlag = true;
      PRINT("> ");
      PRINTLN(val);
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == RESET_BOARD)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      delay(500);
      Hardware::resetBoard();
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_MIRRORS_RANGE)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      // ...

      // * NOTE 1 : testMirroRange() will stop the display engine, but put it again
      //            if it was working. A the LIFO stack for ISR state could come handy...
      // * NOTE 2 : power and switch of lasers is modified. Again, a stack for laser
      //            attributes would be great.
      Hardware::Lasers::pushState();
      Hardware::Lasers::setStateSwitchRed(true);
      Hardware::Lasers::setStatePowerRed(1000);
      Hardware::Lasers::setStateSwitchGreen(true);
      Hardware::Lasers::setStatePowerGreen(1000);
      Hardware::Lasers::setStateSwitchBlue(true);
      Hardware::Lasers::setStatePowerBlue(1000);
      Hardware::Scanner::testMirrorRange(argStack[0].toInt());
      Hardware::Lasers::popState();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_CIRCLE_RANGE)
  {

    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::pushState();
      Hardware::Lasers::setStateSwitchRed(true);
      Hardware::Lasers::setStatePowerRed(1000);
      Hardware::Lasers::setStateSwitchGreen(true);
      Hardware::Lasers::setStatePowerGreen(1000);
      Hardware::Lasers::setStateSwitchBlue(true);
      Hardware::Lasers::setStatePowerBlue(1000);
      Hardware::Scanner::testCircleRange(argStack[0].toInt());
      Hardware::Lasers::popState();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_CROSS_RANGE)
  {

    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");

      Hardware::Lasers::pushState();
      Hardware::Lasers::setStateSwitchRed(true);
      Hardware::Lasers::setStatePowerRed(1000);
      Hardware::Lasers::setStateSwitchGreen(true);
      Hardware::Lasers::setStatePowerGreen(1000);
      Hardware::Lasers::setStateSwitchBlue(true);
      Hardware::Lasers::setStatePowerBlue(1000);
      Hardware::Scanner::testCrossRange(argStack[0].toInt());
      Hardware::Lasers::popState();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else
  { // unkown command or bad parameters ==> bad command (in the future, use a CmdDictionnay)
    PRINTLN("> BAD COMMAND");
    execFlag = false;
  }

  return execFlag;
}
