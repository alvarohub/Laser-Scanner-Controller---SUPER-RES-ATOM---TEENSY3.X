
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
#define SET_POWER_LASER     "PWLASER"  // Parameters: 0 to MAX_LASER_POWER (0-4095, 12 bit res). TODO: per laser.
#define SET_SWITCH_LASER    "SWLASER"   // Parameters: [0-1],SWLASER. Will open/close the laser ultrafast switch.
                                        // This is useful for safety, for blanking between figures, and eventually for
                                        // laser carrier modulation.
#define TEST_LASERS         "TEST LASERS" // no parameters. Will test each laser with a power ramp

// 2) Hardware::Scan commands:
#define START_DISPLAY       "START"  // start the ISR for the displaying engine
#define STOP_DISPLAY        "STOP"   // stop the displaying ISR
#define SET_INTERVAL        "DT"     // parameter: inter-point time in us (min about 20us)
#define DISPLAY_STATUS      "STATUS" // show various settings. Note that the number of points in the
                                    // current blueprint (or "figure"), and the size of the
                                    // displaying buffer may differ because of clipping.

// 3) Figures and pose:
// * NOTE : each time these commands are called, the current figure (in blueprint) is
// re-rendered with the new transforms, in this order of transformation: rotation/scale/translation
#define RESET_POSE_GLOBAL   "RESET POSE" // set angle to 0, center to (0,0) and factor to 1
#define SET_ANGLE_GLOBAL    "ANGLE"      // Parameters: angle (deg),ANGLE
#define SET_CENTER_GLOBAL   "CENTER"     // Parameters: x,y,CENTER
#define SET_FACTOR_GLOBAL   "FACTOR"     // Parameters: factor (0-...),FACTOR
#define SET_COLOR_GLOBAL    "COLOR"      // TODO

//4) Scene clearing and blanking between objects (only useful when having many figures simultanesouly)
#define CLEAR_SCENE         "CLEAR"      // clear the blueprint, and also stop the display
#define CLEAR_MODE          "CLMODE"     // [0-1],CLMODE. When set to 0, if we draw a figure it will
                                         // be ADDED to the current scene. Otherwise drawing first
                                         // clear the current scene and make a new figure.
#define SET_BLANKING      "BLANK"   // (TODO) Parameters: 0/1, OR also laser number.


// 5) Figure primitives:
#define MAKE_LINE           "LINE"     // Parametres: length,hauteur,numpoints, LINE [premier point sur (0,0)] or posX,posY,length,height,numpoint,LINE
#define MAKE_CIRCLE         "CIRCLE"   // Parametres: radius,numpoints,CIRCLE ou X,Y,radius,numpoints,CIRCLE
#define MAKE_RECTANGLE      "RECT"     // Parametres: width,height,numpointX,numPointX,RECT ou X,Y,width,height,numpointsX, numpointY,RECT
#define MAKE_SQUARE         "SQUARE"   // Parametres: size of side,numpoints side,RECT ou X,Y,size-of-side,RECT
#define MAKE_ZIGZAG         "ZIGZAG"   // Parametres: width,height,numpoints X,numpoints Y,ZIGZAG or with position first
#define MAKE_SPIRAL         "SPIRAL"   // Parametres: length-between-arms, num-tours, numpoints, SPIRAL

// 6) TEST FIGURES:
#define COMPOSITE_TEST      "TEST"     // no parameters: cross and squares (will automatically launch START)
                                       // NOTE: we can see the blanking problem here because it is a composite figure
#define CIRCLE_TEST         "CITEST"   // no parameters: makes a circle.
#define SQUARE_TEST         "SQTEST"   // no parameters: makes a square

// 8) LOW LEVEL FUNCTIONS and CHECK COMMANDS:
#define TEST_MIRRORS_RANGE  "SQRANGE"  // time of show in seconds, SQRANGE (square showing the limits of galvos)
#define TEST_CIRCLE_RANGE   "CIRANGE"  // time of show in seconds, CIRANGE (CIRCLE taille de diametre 200 centered on (0,0))
#define TEST_CROSS_RANGE    "CRRANGE"  // time of show in seconds, CRRANGE (CROSS centered on 0,0)
#define SET_DIGITAL_PIN     "SETPIN"   // pin number, state(true/false), SETPIN
#define RESET_BOARD         "RESET"    // RESET the board (note: this will disconnect the serial port)

// =============================================================================
String messageString;
#define MAX_LENGTH_STACK 20
String argStack[MAX_LENGTH_STACK];// number stack storing numeric parameters for commands (for RPN-like parser). Note that the data is saved as a String - it will be converted to int, float or whatever by the specific method associated with the correnspondant command.
String cmd; // we will parse one command at a time

enum stateParser
{
  START,
  NUMBER,
  SEPARATOR,
  CMD
} myState;
//Note: the name of an unscoped enumeration may be omitted:
// such declaration only introduces the enumerators into the enclosing scope.


// Initialization:
void initSerialCom() {
  Serial.begin(SERIAL_BAUDRATE);
  messageString.reserve(200);
  messageString = "";
}

// ====== GATHER BYTES FROM SERIAL PORT ========================================
// * NOTE: This will fill the messageString until finding a packet terminator
void serialEvent() {
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
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    messageString += inChar;
    // * If the incoming character is a packet terminator, proceed to parse the data.
    // The advantages of doing this instead of parsing for EACH command, is that we can
    // have a generic string parser [we can then use ANY other protocol to form the
    // message string, say: OSC, Lora, TCP/IP, etc]. This effectively separates the
    // serial receiver from the parser, and it simplifies debugging.
    if (inChar == END_PACKET) {
      parseStringMessage(messageString); //parse AND calls the appropiate functions
      messageString = "";
    }
  }
}


// ======== PARSE THE MESSAGE ==================================================
// NOTE: I tested this throughly and it seems to work faultlessly as for 5.April.2018
String cmdString;
uint8_t numArgs;
bool cmdExecuted;

void resetParser() { // without changing the parsing index [in the for loop]:
  numArgs = 0;
  for (uint8_t i = 0; i< MAX_LENGTH_STACK; i++) argStack[i] = "";
  cmdString = "";
  cmdExecuted = false; // = true; // we will "AND" this with every command correctly parsed in the string
  myState = START;
}

bool parseStringMessage(const String &_messageString) {
  static String oldMessageString =""; // oldMessageString is for repeating last GOOD string-command

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
      if ((myState == START)||(myState == SEPARATOR)) myState = NUMBER;
      if (myState == NUMBER) { // it could be in CMD state...
        argStack[numArgs] += val;
        //  PRINTLN(" (data)");
      } else { // actually this just means myState == CMD
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
    else if ( ( (val >= 'A') && (val <= 'Z') ) || (val == '_') || (val == ' ')) // the last is for composing commands with underscore (ex: MAKE_CIRCLE)
    {

      if ((myState == START)||(myState == SEPARATOR)) myState = CMD;
      if (myState == CMD) { // Could be in NUMBER state
        cmdString += val;
        //   PRINTLN(" (command)");
      }
      else
      {
        PRINTLN("> BAD FORMED PACKET");
        #ifdef CONTINUE_PARSING
        resetParser(); // reset the parser, but continue from next string character.
        #else
        break; // abort parsing - we could also restart it from here with resetParser()
        #endif
      }
    }

    // Store numeric argument when receiving a NUMBER_SEPARATOR
    else if (val == NUMBER_SEPARATOR) {
      // NOTE: do not permit a number separator AFTER a command, another separator,
      // or nothing: only a separator after a number is legit
      if (myState == NUMBER) { // no need to test: }&&(argStack[numArgs].length() > 0)) {
        //PRINTLN(" (separator)");
        //PRINT("> ARG n."); PRINT(numArgs); PRINT(" : "); PRINTLN(argStack[numArgs]);
        numArgs++;
        myState = SEPARATOR;
      }
      else { // number separator without previous data, another number separator or command
        PRINTLN("> BAD FORMED PACKET");
        //PRINT_LCD("> BAD FORMED ARG LIST");
        #ifdef CONTINUE_PARSING
        resetParser(); // reset parsing data, and continue from next character.
        #else
        break; // abort parsing (we could also restart it from here with resetParser())
        #endif
      }
    }

    else if ((val == END_PACKET)||(val == COMMAND_SEPARATOR)) { // or another CMD terminator code...
      if (myState == START) { // implies cmdString.length() equal to zero
        // END of packet received WITHOUT a command:
        // repeat command IF there was a previous good command:
        // NOTE: for the time being, this discards the rest of the message
        // (in case it came from something else than a terminal of course)
        if (val == END_PACKET) {
          if (oldMessageString != "") {
            PRINTLN(oldMessageString);
            parseStringMessage(oldMessageString); // <<== ATTN: not ideal perhaps to use recurrent
            //  call here.. but when using in command line input, the END_PACKET is the
            // last character, so there is no risk of deep nested calls (on return the parser
            // will end in the next loop iteration)
          }
          else {
            PRINTLN("> NO PREVIOUS COMMAND TO REPEAT");
            #ifdef CONTINUE_PARSING
            resetParser(); // reset parsing data, and continue from next character.
            #else
            break; // abort parsing (we could also restart it from here with resetParser())
            #endif
          }
        } else { // concatenator without previous CMD
          PRINTLN("> BAD FORMED PACKET");
          #ifdef CONTINUE_PARSING
          resetParser(); // reset parsing data, and continue from next character.
          #else
          break; // abort parsing (we could also restart it from here with resetParser())
          #endif
        }
      }
      else if (myState == CMD) { // anything else before means bad formed!
        // * Note 1 : state == CMD here implies cmdString.length() > 0
        // * Note 2 : we don't check argument number, can be anything including nothing.

        PRINT("> EXEC: ");
        for (uint8_t k=0; k<numArgs; k++) {
          PRINT(argStack[k]); PRINT(",");
        }
        PRINTLN(cmdString);

        //PRINT("> trying execution... ");
        // *********************************************************
        cmdExecuted = interpretCommand(cmdString, numArgs, argStack);
        // *********************************************************

        if (cmdExecuted) { // save well executed command string:
          oldMessageString = messageString;
          PRINTLN("> OK");
          //PRINT("> ");
          //Hardware::blinkLedMessage(2);
        } else {
          PRINTLN("> FAIL");
          //PRINT("> ");
        }

        // NOTE: commands can be concatenated AFTER and END_PACKET in case of input from something else than a terminal,
        // so we need to restart parsing from here (will only happen when input string is stored in memory or sent from OSC, etc)
        resetParser();
      }
      else {
        PRINTLN("> BAD FORMED PACKET");
        #ifdef CONTINUE_PARSING
        resetParser(); // reset parsing data, and continue from next character.
        #else
        break; // abort parsing (we could also restart it from here with resetParser())
        #endif
      }
    }
    else if (val == LINE_FEED_IGNORE) {
      // do nothing with this, continue parsing
    }

    else {  // this means we received something else (not a number, not a letter from A-Z,
      // not a number, packet terminator, not a concatenator)
      PRINTLN("> BAD FORMED PACKET");
      //PRINT("> ");
      #ifdef CONTINUE_PARSING
      resetParser(); // reset parsing data, and continue from next character.
      #else
      break; // break the for-loop of parsing
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
bool interpretCommand(String _cmdString, uint8_t _numArgs, String argStack[]) {
  bool execFlag = false;

  //==========================================================================
  // A) ====== LASER COMMANDS ================================================
  //==========================================================================
  if (_cmdString == SET_POWER_LASER) {     // Parameters: 0 to 4096 (12 bit res).
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setPowerRed(constrain(argStack[0].toInt(), 0, MAX_LASER_POWER));
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_SWITCH_LASER) {     // Parameters: 0 to 4096 (12 bit res).
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::setSwitchRed( (argStack[0].toInt() > 0)? 1 : 0);
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_LASERS) {
  if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      Hardware::Lasers::test();
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  // B) ====== SCANNER COMMANDS  =============================================
  //==========================================================================
  else if (_cmdString == START_DISPLAY) {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::startDisplay();
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == STOP_DISPLAY) {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::stopDisplay();
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_INTERVAL) {
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::setInterPointTime((uint16_t)atol(argStack[0].c_str())); // convert c-string to long, then cast to unsigned int
      // the method strtoul needs a c-string, so we need to convert the String to that:
      //DisplayScan::setInterPointTime(strtoul(argStack[0].c_str(),NULL,10); // base 10
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == DISPLAY_STATUS)    {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");

      PRINT(" 1-CLEAR MODE: ");
      if (Graphics:: getClearMode())
      PRINTLN("ON");
      else
      PRINTLN("OFF");

      PRINT(" 2-SCENE PTS: ");
      PRINTLN(Renderer2D::getSizeBlueprint());

      PRINT(" 3-DISPLAY ISR: ");
      if (DisplayScan::getRunningState())
      PRINTLN("ON");
      else
      PRINTLN("OFF");

      PRINT(" 5-DISPLAY PTS: ");
      PRINTLN(DisplayScan::getBufferSize());

      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }



  //==========================================================================
  // C) ======= POSE PARAMETERS ("OpenGL"-like state machine) ================
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
  else if (_cmdString == RESET_POSE_GLOBAL)       {     // Parameters: none
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      Graphics::resetGlobalPose();
      // As explained above, we need to RE-RENDER the display buffer:
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_ANGLE_GLOBAL)  {     // Parameters: angle in DEG (float)
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      Graphics::setAngle(argStack[0].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_CENTER_GLOBAL)    {  // Parameters: x,y
    if (_numArgs == 2) {
      //PRINTLN("> EXECUTING... ");
      Graphics::setCenter(argStack[0].toFloat(), argStack[1].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_FACTOR_GLOBAL)    {  // Parameters: scale
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      Graphics::setScaleFactor(argStack[0].toFloat());
      Renderer2D::renderFigure();
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_COLOR_GLOBAL)    {  // Parameters: color bool [TODO: real colors]
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      Graphics::setColorRed(argStack[0].toInt());
      Renderer2D::renderFigure(); // <<== this will really be useful soon, as we will have a per-point color
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  //==========================================================================
  // D) ============ FIGURES (check Graphics namespace) ======================
  // * NOTE 1 : after all the figure composition, it is
  // imperative to call to the method Renderer2D::renderFigure().
  // * NOTE 2 : The pose parameters are COMPOSED with the global ones.
  // * NOTE 3 : Depending on the number of arguments, different pre-sets are used
  //==========================================================================

  // == CLEAR SCENE and CLEAR MODE ==========================================
  else if (_cmdString == CLEAR_SCENE)     {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");

      // The sequence order and items is arbitrary:

      Graphics::clearScene();
      // clear also the pose parameters - otherwise there is a lot of confusion:
      Graphics::resetGlobalPose();

      // Switch Off laser (power state is not affected):
      DisplayScan::stopDisplay(); // necessary before switching off the laser, because
      // blanking may be set to true (so the pin is toggled at eatch ISR call)

      Hardware::Lasers::setSwitchRed(true); //... not good to do this [will be ok when per-point color]
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == CLEAR_MODE)     {
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      Graphics::setClearMode((argStack[0].toInt()>0? 1 : 0));
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == SET_BLANKING) {
    // for the time being, this is a "DisplayScan"
    // method [in the future, a per-laser method?]
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      DisplayScan::setBlankingRed((argStack[0].toInt()>0? 1 : 0));
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  // == MAKE LINE ==========================================
  else if (_cmdString == MAKE_LINE) {
    switch(_numArgs) {
      case 3:  //centered
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawLine(
        argStack[0].toFloat(), argStack[2].toFloat(),
        argStack[4].toInt()
      );
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
          argStack[4].toInt()
        );
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
  else if (_cmdString == MAKE_CIRCLE)     {

    switch(_numArgs) {
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
  else if (_cmdString == MAKE_RECTANGLE)     {
    switch(_numArgs) {
      case 4: // [centered]
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawRectangle(
        argStack[0].toFloat(), argStack[1].toFloat(),
        argStack[2].toInt(), argStack[3].toInt()
      );
      Renderer2D::renderFigure();
      execFlag = true;
      break;
      case 6:
      {   // From lower left corner:
        //PRINTLN("> EXECUTING... ");
        Graphics::updateScene();
        P2 fromP2(argStack[0].toFloat(), argStack[1].toFloat());
        Graphics::drawRectangle(
          fromP2,
          argStack[2].toFloat(), argStack[3].toFloat(),
          argStack[4].toInt(), argStack[5].toInt()
        );
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
  else if (_cmdString == MAKE_SQUARE) {
    switch(_numArgs) {
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

  else if (_cmdString == MAKE_ZIGZAG)     {
    switch(_numArgs) {
      case 4: // Centered on [0,0]
      //PRINTLN("> EXECUTING... ");
      Graphics::updateScene();
      Graphics::drawZigZag(
        argStack[0].toFloat(), argStack[1].toFloat(),
        argStack[2].toInt(), argStack[3].toInt()
      );
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
          argStack[4].toInt(), argStack[5].toInt()
        );
        Renderer2D::renderFigure();
        execFlag = true;
      }
      break;
      default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  else if (_cmdString == MAKE_SPIRAL)     {
    switch(_numArgs) {
      case 5 : {
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
        argStack[2].toInt()
      );
      Renderer2D::renderFigure();
      execFlag = true;
      break;
      default:
      PRINTLN("> BAD PARAMETERS");
      break;
    }
  }

  // ....

  // 7) TEST FIGURES [this is a test: scene is CLEARED whatever the clear state,
  // and displaying is STARTED whatever the previous state]
  // a) CIRCLE, NO PARAMETERS [500 ADC units radius circle, centered, 100 points]
  else if (_cmdString == CIRCLE_TEST)     {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      Graphics::clearScene();
      Graphics::drawCircle(50.0, 100); // fisrt argument is the radius
      // REM: equal to: Graphics::setScaleFactor(500); Graphics::drawCircle(100);
      Renderer2D::renderFigure();

      // THIS IS A TEST: Force display whatever the previous state, and force
      // also the laser power and blanking state:
      // TODO: make a LIFO stack (push/pop) for the display engine attributes?
      Hardware::Lasers::setPowerRed(1000);
      Hardware::Lasers::setSwitchRed(true);
      DisplayScan::startDisplay(); // start engine, whatever the previous state

      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  // b) : SQUARE, NO PARAMETERS [500 ADC units side, centered, 10 points/side]
  else if (_cmdString == SQUARE_TEST)  {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      Graphics::clearScene();
      Graphics::drawSquare(100, 50.0); //length of side, num points per side
      Renderer2D::renderFigure();

      Hardware::Lasers::setPowerRed(1000);
      Hardware::Lasers::setSwitchRed(true);
      DisplayScan::startDisplay();

      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  // c) : SQUARE + CIRCLE TEST
  else if (_cmdString == COMPOSITE_TEST)  {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      float radius = 75;
      Graphics::clearScene();
      Graphics::drawSquare(2*radius, 50.0);
      Graphics::drawCircle(radius, 100.0);
      Graphics::drawSquare(1.414*radius, 50.0);
      Graphics::drawLine(P2(-90, 0), 180, 0, 50.0);
      Graphics::drawLine(P2(0, -90), 0, 180, 50.0);
      Renderer2D::renderFigure();

      Hardware::Lasers::setPowerRed(1000);
      Hardware::Lasers::setSwitchRed(true);
      DisplayScan::startDisplay();

      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  // .........................................................................
  // ... BUILD HERE WHAT YOU NEED
  // .........................................................................


  //==========================================================================
  // E) ============  LOW LEVEL COMMANDS ===========================
  //==========================================================================

  else if (_cmdString == SET_DIGITAL_PIN)   {     // Parameters:pin, state
    if (_numArgs == 2) {
      //PRINTLN("> EXECUTING... ");
      Hardware::Gpio::setDigitalPin(argStack[0].toInt(), argStack[1].toInt());
      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == RESET_BOARD)    {
    if (_numArgs == 0) {
      //PRINTLN("> EXECUTING... ");
      delay(500);
      Hardware::resetBoard();
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_MIRRORS_RANGE)   {
    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");
      // ...

      // * NOTE 1 : testMirroRange() will stop the display engine, but put it again
      //            if it was working. A the LIFO stack for ISR state could come handy...
      // * NOTE 2 : power and switch of lasers is modified. Again, a stack for laser
      //            attributes would be great.
      Hardware::Lasers::setSwitchRed(true);
      Hardware::Lasers::setPowerRed(1000);
      Hardware::Scanner::testMirrorRange(argStack[0].toInt());

      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_CIRCLE_RANGE)   {

    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");

      Hardware::Lasers::setSwitchRed(true);
      Hardware::Lasers::setPowerRed(1000);
      Hardware::Scanner::testCircleRange(argStack[0].toInt());

      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }

  else if (_cmdString == TEST_CROSS_RANGE)   {

    if (_numArgs == 1) {
      //PRINTLN("> EXECUTING... ");

      Hardware::Lasers::setSwitchRed(true); // TODO: do a push/pop for laser color and power attributes!
      Hardware::Lasers::setPowerRed(1000);
      Hardware::Scanner::testCrossRange(argStack[0].toInt());

      execFlag = true;
    }
    else PRINTLN("> BAD PARAMETERS");
  }


  else { // unkown command or bad parameters ==> bad command (in the future, use a CmdDictionnay)
    PRINTLN("> BAD COMMAND");
    execFlag = false;
  }

  return execFlag;
}
