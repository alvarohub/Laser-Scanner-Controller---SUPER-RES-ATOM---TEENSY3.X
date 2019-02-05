#include "messageParser.h"

namespace Parser
{

bool recordingScript = false;
String scriptStringInMemory;

enum stateParser
{
  START,
  NUMBER,
  SEPARATOR,
  CMD
} myState;
//Note: the name of an un-scoped enumeration may be omitted:
// such declaration only introduces the enumerators into the enclosing scope.

// =============================================================================
//  ======== USEFUL CONVERSION METHODS =========================================
int8_t toBool(const String _str)
{
  int8_t val = -1;
  if (Utils::isNumber(_str))
    val = _str.toInt() > 0;
  else if (_str == "on")
    val = 1;
  else if (_str == "off")
    val = 0;

  return (val);
}

int8_t toClassID(const String _str)
{
  int8_t val(0); // return -1 if class not found

  if (Utils::isNumber(_str))
    val = _str.toInt();
  else if (Utils::isSmallCaps(_str))
  {
    for (int8_t k = 0; k < NUM_MODULE_CLASSES; k++)
    {
      if (_str == Definitions::classNames[k])
      {
        val = k;
        break;
      }
    }
  }
  if (val < 0 && val >= NUM_MODULE_CLASSES)
    val = -1;
  return (val);
}

int8_t toLaserID(const String _str)
{
  int8_t val = -1;

  if (Utils::isNumber(_str))
    val = _str.toInt();
  else if (Utils::isSmallCaps(_str))
  {
    for (int8_t k = 0; k < NUM_LASERS; k++)
    {
      if (_str == Definitions::laserNames[k])
      {
        val = k;
        break;
      }
    }
  }
  //PRINTLN("Switching laser: "+String(val));
  return (val);
}

int8_t toTrgMode(String _str)
{
  int8_t val = -1;

  if (Utils::isNumber(_str))
    val = _str.toInt();
  else if (Utils::isSmallCaps(_str))
  {
    for (int8_t k = 0; k < NUM_TRIG_MODES; k++)
    {
      if (_str == Definitions::trgModeNames[k])
      {
        val = k;
        break;
      }
    }
  }
  return (val);
}

// =============================================================================
// ======== PARSE THE MESSAGE ==================================================

void beginRecordingScript()
{
  scriptStringInMemory = "";
  recordingScript = true;
}
void endRecordingScript()
{
  recordingScript = false;
}

void addRecordingScript()
{
  recordingScript = true;
}

// The following function will read the file and produce the messageString to
// send to the parser, exactly as if it where typed on the serial port.
String readScript(String _nameFile)
{
  _nameFile += ".txt";
  PRINTLN("-- READING : " + _nameFile);

  File myFile;
  myFile = SD.open(_nameFile.c_str());

  String msgString = "";
  if (myFile)
  {
    // Read ALL the characters in the file and put them into a msgString:
    while (myFile.available())
      msgString += (char)myFile.read();

    // close the file ( only one file can be open at a time,
    // so you have to close this one before opening another.)
    myFile.close();
  }
  else
  {
    // if the file didn't open correctly, return a special message:
    msgString = "error";
  }
  return (msgString);
}

bool saveScript(String _nameFile)
{
  _nameFile += ".txt";

  bool execOk = false;
  File myFile;
  myFile = SD.open(_nameFile.c_str(), FILE_WRITE);

  // if the file opened okay, write verbatim what we have in
  if (myFile)
  {
    for (uint32_t k = 0; k < scriptStringInMemory.length(); k++)
    {
      myFile.print((char)scriptStringInMemory[k]);
    }
    //myFile.print(END_CMD);
    myFile.close();
    execOk = true;
  }
  else
  {
    // if the file didn't open, print an error:
    PRINTLN("> ERROR writing file!");
  }
  return (execOk);
}

// ***************************************************************************************************************
// RPN PARSER ****************************************************************************************************
String oldAtomicCommandString = ""; // oldAtomicCommandString is for repeating last GOOD string-command
bool parseStringMessage(const String &_messageString)
{
  // NOTE: can contain more than one command; however, after every command there must be an END_COMMAND
  String messageString;

  String cmdString; // one command executed at a time.
  uint8_t numArgs;
  String argStack[SIZE_CMD_STACK]; // number stack storing numeric parameters for commands (for RPN-like parser).
                                   // Note that the data is saved as a String - it will be converted to int, float or
                                   // whatever by the specific method associated with the correspondent command.

  bool cmdExecuted;

  /* Lambda function (C++ does not allows nested functions unless they are in a struct or class).
   I need this because if we call parseStringMessage in a recursive way (for instance, to execute a script,
   and then continue parsing the original message), the variables need to be local (and I want to reset them
   without the need to do it "by hand" all the time).
   NOTE: Usually you put [=] or [&] as captures. [=] means that you capture all variables in the scope in which the value is defined by value, which means that they will keep the value that they had when the lambda was declared. [&] means that you capture all variables in the scope by reference, which means that they will always have their current value, but if they are erased from memory the program will crash. */
  auto resetParser = [&]() {
    numArgs = 0;
    for (uint8_t i = 0; i < SIZE_CMD_STACK; i++)
      argStack[i] = "";
    cmdString = "";
    myState = START;
  };

  // ================= START PARSING ========================
  //PRINTLN("START PARSING MESSAGE");
  messageString = _messageString;

  resetParser(); // reset parsing the first time (attn: there can be many commands INSIDE the messageString,
                 // so the collected arguments and command should be reset after each individual command interpretation and
                 // execution!)

  bool scriptExecuted = true; // initialized to true before string parsing : it will be "ANDed" with each cmdExecuted.
                              // NOTE: cmdExecuted does not need to be set here, it will be set when a single command is executed (correctly or not)

  // Note: messageString contains the END_CMD (char), otherwise we wouldn't be here;
  // So, going through the for-loop with the condition i < messageString.length()
  // is basically the same as using the condition: _messageString[i] != END_CMD
  for (uint8_t i = 0; i < messageString.length(); i++)
  {
    char val = _messageString[i];

    // PRINT("Received message: ");
    // if (val == END_CMD)
    //   PRINT("CR (end command)");
    // else if (val == LINE_FEED_IGNORE)
    //   PRINT("LF (ignore)");
    // else
    //   PRINT((char)val);
    // PRINT(" : ");
    // PRINTLN((uint8_t)val);

    // ************ GATHER ARGUMENTS:
    // Put ASCII characters for a number (base 10) plus '-' and '.' to form floats and negative numbers ('/' must be
    // excluded) in the current argument in the stack, as well as non-capital letters (can be parameters too)
    if ((((val >= '-') && (val <= '9')) || ((val >= 'a') && (val <= 'z'))) && (val != '/'))
    {
      if ((myState == START) || (myState == SEPARATOR))
        myState = NUMBER;

      if (myState == NUMBER)
      { // it could be in CMD state...
        argStack[numArgs] += val;
        //  PRINTLN(" (data)");
      }
      else // actually this just means myState == CMD
      {
        PRINTLN("> BAD FORMED PACKET");
        break;
      }
    }

    // ************ GATHER COMMAND:
    // Put letter ('A' to 'Z') in cmdString.
    // => Let's not permit to start writing a command if we did not finish a number or nothing was written before.
    else if (((val >= 'A') && (val <= 'Z')) || (val == '_'))
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
        break; // abort parsing - we could also restart it from here with resetParser()
      }
    }

    // Store argument and increment argument counter when receiving a ARG_SEPARATOR:
    else if (val == ARG_SEPARATOR)
    {
      // NOTE: do not permit a number separator AFTER a command, another separator,
      // or nothing: only a separator after a number is legit
      if (myState == NUMBER)
      { // no need to test: }&&(argStack[numArgs].length() > 0)) {
        //PRINTLN(" (separator)");
        //PRINT("> ARG n."); PRINT(numArgs); PRINT(" : "); PRINTLN(argStack[numArgs]);
        numArgs++;
        myState = SEPARATOR;
      }
      else
      { // number separator without previous data, another number separator or command:
        // through "bad formed" or ignore?
        PRINTLN("> BAD FORMED PACKET");
        //PRINT_LCD("> BAD FORMED ARG LIST");
        break;
      }
    }

    else if (val == END_CMD)
    { // THIS INDICATES END OF MESSAGE STRING or ATOMIC COMMAND

      if (myState == START)
      { // implies cmdString.length() equal to zero, i.e, END of packet received WITHOUT a command:
        // => REPEAT command IF there was a previous good command?
        // NOTE: for the time being, this discards the rest of the message
        // (in case it came from something else than a terminal of course)
        if (val == END_CMD)
        {
          //NOTE: I will not do repeat, unless explicitly asked with a command. This way the line feed is
          // just ignored if there is no command string (better to write readable scripts)
          PRINTLN(">");

          /* IF REPEAT by LINE FEED:
          if (oldAtomicCommandString != "")
          {
            PRINTLN("> [REPEAT]");
            parseStringMessage(oldAtomicCommandString); // <<== ATTN: not ideal perhaps to use recurrent
            // call here.. but when using in command line input, the END_CMD is the
            // last character, so there is no risk of deep nested calls (on return the parser
            // will end in the next loop iteration)
          }
          else
          {
            PRINTLN("> [NO PREVIOUS COMMAND TO REPEAT]");
            break; // abort parsing (we could also restart it from here with resetParser())
          }
          */
        }
        else
        { // "concatenator" without previous CMD
          PRINTLN("> BAD FORMED PACKET");

          break; // abort parsing (we could also restart it from here with resetParser())
        }
      }
      else if (myState == CMD)
      { // anything else before means bad formed!
        // * Note 1 : state == CMD here implies cmdString.length() > 0
        // * Note 2 : we don't check argument number, can be anything including nothing.

        cmdExecuted = false;
        //PRINTLN(" ");
        PRINT("> EXEC: ");

        // Retrieve the whole atomic command string (for checking and for saving into oldAtomicCommandString
        // if appropiate - ie, different from certain special commands such as start/end recording scripts)
        String atomicCommandString = "";
        for (uint8_t k = 0; k < numArgs; k++)
        {
          atomicCommandString += argStack[k] + ",";
        }
        atomicCommandString += cmdString + END_CMD; // NOTE: INCLUDE the END_CMD character

        // Show it on the console:
        PRINT(atomicCommandString);

        // *********************************************************
        // *********************************************************
        cmdExecuted = interpretCommand(cmdString, numArgs, argStack);
        // *********************************************************
        // *********************************************************

        if (cmdExecuted) // if THIS particular command was succesfully executed:
        {
          scriptExecuted = scriptExecuted && cmdExecuted; // this way we can know if there was an error in the middle of
          // a script, without stopping it.
          // NOTE: ignore and don't record START_REC_SCRIPT and END_REC_SCRIPT or ADD_REC_SCRIPT, nor
          // the REPEAT_COMMAND commands! (could be useful, but handling that becomes convoluted)
          if ((cmdString != REPEAT_COMMAND) && (cmdString != START_REC_SCRIPT) && (cmdString != END_REC_SCRIPT) && (cmdString != ADD_REC_SCRIPT))
          {
            // 1) Save properly parsed and executed command string in oldAtomicCommandString for repetition upon "ENTER":
            oldAtomicCommandString = atomicCommandString;

            // 2) Also, save it in the recording string if in recording mode:
            if (recordingScript)
            {
              scriptStringInMemory += atomicCommandString; // NOTE: each message has an END_CMD, which is a newlin (\n),
              // so each command in the saved file will be in a different row.
            }
          }
          PRINTLN("> OK");
          //PRINT("> ");
          Hardware::blinkLedMessage(1, 1000); // short period to avoid slowing down continuous execution of serial commands
        }
        else
        {
          PRINTLN("> FAILED EXECUTION");
          //PRINT("> ");
        }

        // NOTE: commands can be concatenated AFTER and END_CMD in case of input from something else than a terminal,
        // so we need to restart parsing from here (will only happen when input string is stored in RAM, the file
        // system (SD card) or sent from MQTT, Ethernet, OSC, etc)
        resetParser();
      }
      else
      {
        PRINTLN("> BAD FORMED PACKET");
        break;
      }
    }
    // ignore these characters:
    else if ((val == LINE_FEED_IGNORE) || (val == ' '))
    {
      // do nothing with this, continue parsing
    }

    else
    { // this means we received something else (not a number, not a letter from A-Z,
      // not a number, packet terminator, not a concatenator)
      PRINTLN("> BAD FORMED PACKET");
      //PRINT("> ");

      break; // break the for-loop of parsing
    }
  } // end parse for-loop

  return (scriptExecuted); // NOTE: if there is only one command, this is equal to return(cmdExecuted)
}

// =============================================================================
// ========== COMMAND INTERPRETER ==============================================
// NOTE: Here you can add commands yourself; but I would wait until I can do a
// proper COMMAND DICTIONNARY (or use a good command parser/serialiser library)
// so as to simplify the way one can write new ones.
// =============================================================================
bool interpretCommand(String _cmdString, uint8_t _numArgs, String argStack[])
{
  bool execFlag = false;

  //PRINTLN("START INTERPRETATION");

  //==========================================================================
  //====== MISC ================================================

  if (_cmdString == REPEAT_COMMAND)
  { // Param: 0 to 4096 (12 bit res).
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      if (oldAtomicCommandString != "")
      {
        PRINTLN("> REPEAT");
        parseStringMessage(oldAtomicCommandString); // <<== ATTN: not ideal perhaps to use recurrent
        // call here.. but when using in command line input, the END_CMD is the
        // last character, so there is no risk of deep nested calls (on return the parser
        // will end in the next loop iteration)
      }
      else
      {
        PRINTLN("> [NO PREVIOUS COMMAND TO REPEAT]");
      }
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  // A) ====== LASER COMMANDS ================================================
  //==========================================================================
  else if (_cmdString == SET_POWER_LASER_ALL)
  { // Param: 0 to 4096 (12 bit res).
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStatePowerAll(argStack[0].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_POWER_LASER)
  { // Param: laser number or name, power (0 to 4096, 12 bit res).
    if ((_numArgs == 2) && Utils::isNumber(argStack[1]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStatePower(toLaserID(argStack[0]), argStack[1].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SWITCH_LASER_ALL)
  { // Param: 0/1 or "on"/"off"
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setStateSwitchAll(toBool(argStack[0]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SWITCH_LASER)
  { // Param: laser number or name, 0/1 or "on"/"off"

    if (_numArgs == 2)
    //PRINTLN("> EXECUTING... ");
    {
      Hardware::Lasers::setStateSwitch(toLaserID(argStack[0]), toBool(argStack[1]));
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
      Hardware::Lasers::setStateCarrierAll(toBool(argStack[0]));
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
      Hardware::Lasers::setStateCarrier(toLaserID(argStack[0]), argStack[1]);
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
      Hardware::Gpio::setShutter(argStack[0]);
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
  // 2) ====== SEQUENCER  ====================================================
  //==========================================================================

  // CLOCK parameter configuration:
  //#define SET_CLOCK_PERIOD "SET_PERIOD_CLK" // Param: {clk_id, period in ms}
  else if (_cmdString == SET_CLOCK_PERIOD)
  {
    if ((_numArgs == 2) && Utils::isNumber(argStack[0]) && Utils::isNumber(argStack[1]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Clocks::arrayClock[argStack[0].toInt()].setPeriodMs(argStack[1].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //#define START_CLOCK	         "START_CLK"            // Param: {clk_id}
  else if (_cmdString == START_CLOCK)
  {
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Clocks::arrayClock[argStack[0].toInt()].start();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //#define STOP_CLOCK	         "STOP_CLK"             // Param: {clk_id}
  else if (_cmdString == STOP_CLOCK)
  {
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Clocks::arrayClock[argStack[0].toInt()].stop();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_CLOCK_STATE)
  {
    if ((_numArgs == 2) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Clocks::arrayClock[argStack[0].toInt()].setActive(toBool(argStack[1]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }
  else if (_cmdString == SET_CLOCK_STATE_ALL)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Clocks::setStateAllClocks(toBool(argStack[0]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }
  else if (_cmdString == RST_CLOCK)
  {
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Clocks::arrayClock[argStack[0].toInt()].reset();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }
  else if (_cmdString == RST_ALL_CLOCKS)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Clocks::resetAllClocks();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // TRIGGER PROCESSOR parameter configuration:
  //#define SET_TRG "SET_TRG"
  // Param: { trg_id = [0-NUM_TRG_PROCESSORS],
  //          mode trigger=[0,1,2] (or "rise", "fall", "change")
  //          burst=[0...],        (positive integer)
  //          skip=[0...],         (positive integer)
  //          delay=[0...]         (positive integer)
  // }
  else if (_cmdString == SET_TRIGGER_PROCESSOR)
  {
    using namespace Hardware::TriggerProcessors;
    if (_numArgs == 5)
    {
      //PRINTLN("> EXECUTING... ");
      // Get pointer to the processor object:
      TriggerProcessor *ptr_trgProc = &(arrayTriggerProcessor[argStack[0].toInt()]);
      // Set parameters:
      //ptr_trgProc->setMode(static_cast<TrgMode>(argStack[1].toInt()));
      ptr_trgProc->setMode(toTrgMode(argStack[1])); // can be numeric or string
      ptr_trgProc->setBurst(argStack[2].toInt());
      ptr_trgProc->setSkip(argStack[3].toInt());
      ptr_trgProc->setOffset(argStack[4].toInt());

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // PULSE SHAPER parameter configuration:
  //  #define SET_PULSE_SHAPER "SET_PUL" // Param: {pul_id, time off (ms), time on (ms)}
  else if (_cmdString == SET_PULSE_SHAPER)
  {
    if (_numArgs == 3)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Pulsars::arrayPulsar[argStack[0].toInt()].setParam(argStack[1].toInt(), argStack[2].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // SEQUENCER activation/deactivation:
  // #define SET_SEQUENCER_STATE   "SET_STATE_SEQ"   // Param: {0/1}. Deactivate/activate sequencer.
  else if (_cmdString == SET_SEQUENCER_STATE)
  {
    if (_numArgs == 1)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Sequencer::setState(toBool(argStack[0]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // #define START_SEQUENCER	      "START_SEQ"
  else if (_cmdString == START_SEQUENCER)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Sequencer::setState(true);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //#define STOP_SEQUENCER	      "STOP_SEQ"
  else if (_cmdString == STOP_SEQUENCER)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Sequencer::setState(false);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == RESET_SEQUENCER)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Sequencer::reset();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // a) Adding a module to the sequencer pipeline:
  // #define ADD_SEQUENCER_MODULE "ADD_SEQ_MODULE" // Param: {mod_from class code [0-5] and index in the class}
  else if (_cmdString == ADD_SEQUENCER_MODULE)
  {
    using namespace Hardware::Sequencer;
    if (_numArgs == 2)
    {
      //PRINTLN("> EXECUTING... ");
      Module *ptr_newModule = getModulePtr(toClassID(argStack[0]), argStack[1].toInt()); // note: this function is overloaded to
      // take strings or numbers parameters
      addModulePipeline(ptr_newModule); // checks if already there...
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // a) Interconnect two modules: mod_from (must have an output) to mod_to (must accept input)
  // #define SET_SEQUENCER_LINK "SET_LNK_SEQ" // Param: {mod_from class code [0-5] and index in the class [depends],
  //                                                      mod_to class code [0-5] and index in the class [depends]}
  else if (_cmdString == SET_SEQUENCER_LINK)
  {
    using namespace Hardware::Sequencer;
    if (_numArgs == 4)
    {
      //PRINTLN("> EXECUTING... ");
      Module *ptr_ModuleFrom = getModulePtr(toClassID(argStack[0]), argStack[1].toInt());
      Module *ptr_ModuleTo = getModulePtr(toClassID(argStack[2]), argStack[3].toInt());

      // Add both modules to the pipeline automatically or it is better to add it first and then make the links?
      // I will add them automatically for now since we won't be doing soon branching pipelines (TODO: better handling of
      // multiple or branching pipelines):
      addModulePipeline(ptr_ModuleFrom);          // checks if already there...
      addModulePipeline(ptr_ModuleTo);            // checks if already there...
      ptr_ModuleTo->setInputLink(ptr_ModuleFrom); // no need to check, but value of input will be overwritten

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // b) Create a chain of interconnected modules at once:
  // NOTE: it does NOT delecte whatever was before
  //#define SET_SEQUENCER_CHAIN "SET_CHAIN_SEQ" // Param: {module1 class and index, module two class and index, ...}
  else if (_cmdString == SET_SEQUENCER_CHAIN)
  {
    using namespace Hardware::Sequencer;
    if (!(_numArgs % 2)) // number of arguments must be even (there are more tests to do on the ranges, but at least that)
    {
      //PRINTLN("> EXECUTING... ");
      for (uint8_t k = 0; k < _numArgs / 2 - 1; k++)
      {
        Module *ptr_ModuleFrom = getModulePtr(toClassID(argStack[2 * k]), argStack[2 * k + 1].toInt());
        Module *ptr_ModuleTo = getModulePtr(toClassID(argStack[2 * k + 2]), argStack[2 * k + 3].toInt());

        // these methods add to the vector of pointers only if the pointers where not there
        addModulePipeline(ptr_ModuleFrom);
        addModulePipeline(ptr_ModuleTo);

        ptr_ModuleTo->setInputLink(ptr_ModuleFrom);
      }
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // c) Disconnect ALL links (aka, delete the sequencer pipeline). Not the same than reset or deactivate the sequencer!
  //#define CLEAR_SEQUENCER "CLEAR_SEQ"
  else if (_cmdString == CLEAR_SEQUENCER)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Sequencer::clearPipeline();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // d) Display sequencer pipeline status:
  //#define DISPLAY_SEQUENCER_STATUS "STATUS_SEQ"
  else if (_cmdString == DISPLAY_SEQUENCER_STATUS)
  {
    if (_numArgs == 0)
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Sequencer::displaySequencerStatus();
      //PRINTLN(msg);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //#define SET_STATE_MODULE	"SET_STATE"  // Param: {module1 class, index, on/off}
  // Setting modules active/inactive independently is useful for debugging at least,
  // but can have other practical uses (stop one laser but not the other without changing the,
  // sequencer pipeline, etc):
else if (_cmdString == SET_STATE_MODULE)
  {
    if (_numArgs == 3)
    {
      //PRINTLN("> EXECUTING... ");
      Module *ptr_Module = Hardware::Sequencer::getModulePtr(toClassID(argStack[0]), argStack[1].toInt());
      ptr_Module->setActive(toBool(argStack[2])); // <-- NOTE: it is then up to the user to set the state it "ends"
      // being at during the stop (this will depend on the module: for the laser, I think it is better to set it off,
      // so I will overload the base method "setActive()")
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }


  //==========================================================================
  // 3) ====== OPTOTUNER COMMANDS  ===========================================
  //==========================================================================
  else if (_cmdString == SET_POWER_OPTOTUNER_ALL)
  { // Param: 0 to 4096 (12 bit res).
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::OptoTuners::setStatePowerAll(argStack[0].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_POWER_OPTOTUNER)
  { // Param: laser number, power (0 to 4096, 12 bit res).
    if ((_numArgs == 2)&& Utils::areNumbers(_numArgs,argStack))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::OptoTuners::setStatePower(argStack[0].toInt(), argStack[1].toInt());
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  // C) ====== SCANNER COMMANDS  =============================================
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
    if ((_numArgs == 1)&& Utils::isNumber( argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::setInterPointTime(argStack[0].toInt());
      //DisplayScan::setInterPointTime((uint16_t)atol(argStack[0].c_str()));
      // convert c-string to long, then cast to unsigned int
      // the method strtoul needs a c-string, so we need to convert the String to that:
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

      PRINTLN(" 7-LASERS [power, state, carrier, inter-fig blank]: ");
      Laser::LaserState laserState;
      for (uint8_t k = 0; k < NUM_LASERS; k++)
      {
        PRINT("     ");
        PRINT(Hardware::Lasers::laserArray[k].getName());
        laserState = Hardware::Lasers::laserArray[k].getCurrentState();
        PRINT("\t[");
        PRINT(laserState.power);
        PRINT(", ");
        PRINT(laserState.stateSwitch > 0 ? "on" : "off");
        PRINT(", ");
        PRINT(laserState.stateCarrier > 0 ? "on" : "off");
        PRINT(", ");
        PRINT(laserState.stateBlanking > 0 ? "on" : "off");
        PRINTLN("]");
      }

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  // D) ======= POSE PARAMETERS ("OpenGL"-like state machine) ================
  //==========================================================================
  //  * NOTE 1 : This is a very simplified "open-gl" like rendering engine, but
  //    should be handy anyway. It works as follows: the pose parameters are applied
  //    whenever we draw a figure; that's why it is interesting to have parameters-less,
  //    normalized primitives: when modifying the angle, center and scale, EVERYTHING
  //    being displayed is re-scaled, rotated and/or translated.
  //  * NOTE 2 : In the future, use a MODELVIEW MATRIX instead, and make some
  //    methods to set "passive transforms" (as in my laserSensingDisplay code)
  //  * NOTE 3 : Instead of doing Graphics::... we could just do:
  //                     using namespace Graphics;
  //    However, I prefer the suffix for clarity (my Object Oriented bias...)
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

  else if (_cmdString == SET_ANGLE_GLOBAL)
  { // Param: angle in DEG (float)
    if ((_numArgs == 1)&& Utils::isNumber( argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setAngle(argStack[0].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_CENTER_GLOBAL)
  { // Param: x,y
    if ((_numArgs == 2)&& Utils::areNumbers(_numArgs, argStack))
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setCenter(argStack[0].toFloat(), argStack[1].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SCALE_GLOBAL)
  { // Param: scale
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Graphics::setScaleFactor(argStack[0].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_COLOR_GLOBAL)
  { // Param: color bool [TODO: real colors]
    if ((_numArgs == 1)&&Utils::isNumber(argStack[0]))
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
  // E) ============ FIGURES (check Graphics namespace) ======================
  // * NOTE 1 : after all the figure composition, it is
  // imperative to call to the method Renderer2D::renderFigure().
  // * NOTE 2 : The pose parameters are COMPOSED with the global ones.
  // * NOTE 3 : Depending on the number of arguments, different pre-sets are used
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
      Graphics::setClearMode(toBool(argStack[0]));
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
      Hardware::Lasers::setStateBlankingAll(toBool(argStack[0]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }
  else if (_cmdString == SET_BLANKING)
  {
    if ((_numArgs == 2)&&Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      // This is delicate: we need to stop the displaying engine, and reset it (in particular
      // the style stack, or we may run into overflows because the variable affects the program flow),
      // OR, we don't use the style stack (I decided for the later for the time being)
      Hardware::Lasers::setStateBlanking(toLaserID(argStack[0]), toBool(argStack[1]));
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
      DisplayScan::setInterPointBlankingMode(toBool(argStack[0]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // == MAKE LINE ==========================================
  else if (_cmdString == MAKE_LINE)
  {
     if (Utils::areNumbers(_numArgs, argStack)) {
    switch (_numArgs)
    {
    case 3: //origina at (0,0)
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
     } else {
       PRINTLN("> BAD PARAMETERS");
     }
  }

  // == MAKE CIRCLE ==========================================
  // a) Depending on the number of arguments, we do something different:
  //    - with one parameter (nb points), we draw a circle in (0,0) with unit radius
  //      [of course, the radius is multiplied by the current scaling factor]
  else if (_cmdString == MAKE_CIRCLE)
  {
    if (Utils::areNumbers(_numArgs, argStack))
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
    else
    {
      PRINTLN("> BAD PARAMETERS");
    }
  }

  // == MAKE RECTANGLE ==========================================
  else if (_cmdString == MAKE_RECTANGLE)
  {
    if (Utils::areNumbers(_numArgs, argStack))
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
    else
    {
      PRINTLN("> BAD PARAMETERS");
    }
  }

  // == MAKE SQUARE ==========================================
  else if (_cmdString == MAKE_SQUARE)
  {
    if (Utils::areNumbers(_numArgs, argStack))
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
    else
    {
      PRINTLN("> BAD PARAMETERS");
    }
  }

  else if (_cmdString == MAKE_ZIGZAG)
  {
    if (Utils::areNumbers(_numArgs, argStack))
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
    else
    {
      PRINTLN("> BAD PARAMETERS");
    }
  }

  else if (_cmdString == MAKE_SPIRAL)
  {
    if (Utils::areNumbers(_numArgs, argStack))
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
    else
    {
      PRINTLN("> BAD PARAMETERS");
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

      Graphics::drawLine(P2(-50.0, -30), 100, 60, 50); // first argument is the radius
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

      Graphics::drawCircle(50.0, 100); // first argument is the radius
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
  // G) ============  LOW LEVEL COMMANDS ===========================
  //==========================================================================

  else if (_cmdString == SET_DIGITAL_PIN)
  { // Param:pin, state
    if ((_numArgs == 2) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setDigitalPin(argStack[0].toInt(), toBool(argStack[1]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // Wrappers for special pins (exposed in the D25 connector):
  else if (_cmdString == SET_DIGITAL_A)
  { // Param: state
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setDigitalPinA(toBool(argStack[0]));
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_DIGITAL_B)
  { // Param: state
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setDigitalPinB(toBool(argStack[0]));
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
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
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
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
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
    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      // ...

      // * NOTE 1 : testMirrorRange() will stop the display engine, but put it again
      //            if it was working. A the LIFO stack for ISR state could come handy...
      // * NOTE 2 : power and switch of lasers is modified. Again, a stack for laser
      //            attributes would be great.

      Hardware::Lasers::pushState();

      Hardware::Scanner::testMirrorRange(argStack[0].toInt());

      Hardware::Lasers::popState();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_CIRCLE_RANGE)
  {

    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::pushState();

      Hardware::Lasers::setStatePowerAll(1000);
      Hardware::Lasers::setStateSwitchAll(true);
      Hardware::Scanner::testCircleRange(argStack[0].toInt());

      Hardware::Lasers::popState();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_CROSS_RANGE)
  {

    if ((_numArgs == 1) && Utils::isNumber(argStack[0]))
    {
      //PRINTLN("> EXECUTING... ");

      Hardware::Lasers::pushState();

      Hardware::Lasers::setStatePowerAll(1000);
      Hardware::Lasers::setStateSwitchAll(true);
      Hardware::Scanner::testCrossRange(argStack[0].toInt());

      Hardware::Lasers::popState();

      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // 8) ADVANCED COMMANDS **************************************************************************************

  else if (_cmdString == LOAD_SCRIPT)
  {
    if (_numArgs == 1)
    {
      String nameFile = argStack[0];

      //PRINTLN("  Reading file '" + nameFile + "'");
      String msgString = readScript(nameFile);
      if (msgString == "error")
      {
        PRINTLN("> LOAD ERROR ");
      }
      else
      {
        PRINTLN("------------ SCRIPT LOADED IN MEM :");
        PRINT(msgString);
        scriptStringInMemory = msgString;
        execFlag = true;
        PRINTLN("-----------------------------------");
      }
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == EXECUTE_SCRIPT)
  {
    if (_numArgs == 0)
    {
      PRINTLN("------------- CURRENT CODE IN MEM :");
      PRINT(scriptStringInMemory); // MUST end with END_CMD (which is end of line too...)
      PRINTLN("-----------------------------------");
      PRINTLN("-- EXECUTING : ");
      // NOTE: A bad parsing or command error will produce and execution error INSIDE the script, but
      // it does not mean the EXECUTE_SCRIPT command was a failure.
      // We have therefore two options: signal that there was an error in the script (this can only
      // happen if the scrupt was loaded from an SD card and created on a PC), or just through "OK". The first is
      // better - otherwise we will ALWAYS throw "OK"... But we can signal this with a proper message.
      execFlag = parseStringMessage(scriptStringInMemory);
      if (execFlag)
        PRINTLN("----------- END FAULTLESS EXECUTION");
      else
        PRINTLN("------------------- END WITH ERRORS");
    }
    else if (_numArgs == 1)
    {
      String scriptString = readScript(argStack[0]);
      if (scriptString == "error")
        PRINTLN("> LOAD ERROR ");
      else
      {
        PRINTLN("--- SCRIPT LOADED IN MEM TO EXECUTE:");
        PRINT(scriptString); // MUST end with END_CMD (which is end of line too...)
        PRINTLN("-----------------------------------");
        PRINTLN("-- EXECUTING : ");
        ;
        // NOTE: A bad parsing or command error will also produce and execution flag error.
        execFlag = parseStringMessage(scriptString);
        if (execFlag)
          PRINTLN("----------- END FAULTLESS EXECUTION");
        else
          PRINTLN("------------------- END WITH ERRORS");
      }
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //#define START_SCRIPT "BEGIN_SCRIPT"
  else if (_cmdString == START_REC_SCRIPT)
  {
    if (_numArgs == 0)
    {
      beginRecordingScript();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //#define END_SCRIPT "END_SCRIPT"
  else if (_cmdString == END_REC_SCRIPT)
  {
    if (_numArgs == 0)
    {
      endRecordingScript();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // add more commands to the recorded script
  else if (_cmdString == ADD_REC_SCRIPT)
  {
    if (_numArgs == 0)
    {
      addRecordingScript();
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //#define SAVE_SCRIPT "SAVE_SCRIPT"
  else if (_cmdString == SAVE_SCRIPT)
  {
    if (_numArgs == 1)
    {
      execFlag = saveScript(argStack[0]);
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == LIST_SD_PRM)
  {
    if (_numArgs == 0)
    {
      File root;
      root = SD.open("/");
      Hardware::SDCard::printDirectory(root, 0);
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SHOW_MEM_PRM)
  {
    if (_numArgs == 0)
    {
      PRINTLN("-- CURRENT SCRIPT CODE IN MEMORY :");
      PRINTLN("---------------------------------- ");
      PRINT(scriptStringInMemory);
      PRINTLN("---------------------------------- ");
      execFlag = true;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  //10) DEBUG COMMANDS  ****************************************************************************************
  // #define VERBOSE_MODE "VERBOSE"
  else if (_cmdString == VERBOSE_MODE)
  {
    if (_numArgs == 1)
    {
      Utils::setVerboseMode(toBool(argStack[0]));
      execFlag = false;
    }
    else
      PRINTLN("> BAD PARAMETERS");
  }

  // ***********************************************************************************************************
  // END) Catches if there is no command that matches the query ************************************************
  else
  { // unkown command or bad parameters ==> bad command (in the future, use a CmdDictionnay)
    PRINTLN("> BAD COMMAND");
    execFlag = false;
  }

  // Finally, return the execution flag (TODO: different codes, and a String explaining the error)
  return execFlag;
} // namespace Parser

} // namespace Parser
