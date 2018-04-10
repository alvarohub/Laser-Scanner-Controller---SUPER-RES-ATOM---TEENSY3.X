
#include "renderer2D.h"
#include "Utils.h"
// I include the following or low level stuff (but perhaps better to use Utils wrappers in the future):
#include "scannerDisplay.h"
#include "graphics.h"

// ==================== SPEED COMMUNICATION:
#define SERIAL_BAUDRATE 38400

// ==================== PACKET ENCAPSULATION:
#define NUMBER_SEPARATOR    ','
#define END_PACKET          '\n' // newline (ASCII value 10)

// ==================== COMMANDS:
// 1) Laser commands:
#define SET_BLANKING_RED    "BLANK"  // Parameters: 0/1. It sets the boolean blankingFlag
#define SET_POWER_RED       "POWER"  // Parameters: 0 to 2047 (11 bit res).

// 2) Hardware::Scan commands:
#define SCAN_STATE           "SCAN" // Parameters: 0/1 (0: stop, 1: resume scan)
#define SCAN_PAUSE           "PAUSE"

// 3) Figures and pose:
#define RESET_POSE_GLOBAL   "RESET POSE"
#define SET_ANGLE_GLOBAL    "ANGLE"
#define SET_CENTER_GLOBAL   "CENTER"
#define SET_FACTOR_GLOBAL   "FACTOR"

//  b) Figure primitives:
#define CLEAR_SCENE         "CLEAR"
#define CIRCLE_TEST         "CIRCLE TEST"

#define MAKE_LINE           "LINE"
#define ADD_LINE            "ADD LINE"
#define MAKE_CIRCLE         "CIRCLE"
#define ADD_CIRCLE          "ADD CIRCLE"

#define MAKE_RECTANGLE      "RECTANGLE"

#define MAKE_DISK_SQUARES   "MANY SQUARES"
// TODO: other figures...

// 4) LOW LEVEL FUNCTIONS and CHECK COMMANDS:
#define DISPLAY_STATUS      "DISPLAY STATUS"
#define TEST_MIRRORS_RANGE  "TEST A"
#define TEST_CIRCLE_RANGE   "TEST B"
#define SET_DIGITAL_PIN     "PIN"   // Up to the user to check range and initialization
#define RESET_BOARD         "RESET" // (no parameters)

// =============================================================================
String messageString;
#define MAX_LENGTH_STACK 5
String argStack[MAX_LENGTH_STACK];// number stack storing numeric parameters for commands (for RPN-like parser). Note that the data is saved as a String - it will be converted to int, float or whatever by the specific method associated with the correnspondant command.
String cmd; // we will parse one command at a time

// Initialization:
void initSerialCom() {
    Serial.begin(SERIAL_BAUDRATE);
    messageString.reserve(100);
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
bool parseStringMessage(const String & _messageString) {
    //Set some special led to signal a received complete message, or send back
    // some notification? (handshake)
    digitalWrite(PIN_LED_MESSAGE, HIGH);
    //PRINTLN("* STRING RECEIVED: "); PRINTLN(_messageString);

    String cmdString = "";
    bool cmdExecuted = false; // = true; // we will "AND" this with every command correctly parsed in the string

    // clean stack:
    for (uint8_t i = 0; i< MAX_LENGTH_STACK; i++) argStack[i] = "";
    uint8_t numArgs = 0;

    for (uint8_t i = 0; i < _messageString.length(); i++)
    {
        char val = _messageString[i];

        // PRINT(">"); PRINT((char)val);

        // Put numeric ASCII characters in numString:
        if ((val >= '-') && (val <= '9') && (val != '/')) //this is '0' to '9' plus '-' and '.' to form floats and negative numbers
        {
            argStack[numArgs] += val;
            //  PRINTLN(" (data)");
        }
        // Put letter ('A' to 'Z') in cmdString:
        else if ( ( (val >= 'A') && (val <= 'Z') ) || (val == '_') || (val == ' ')) // the last is for composing commands with underscore (ex: MAKE_CIRCLE)
        {
            cmdString += val;
            //  PRINTLN(" (command)");
        }

        // Store argument:
        else if (val == NUMBER_SEPARATOR) {
            if (argStack[numArgs].length() > 0) {
                //PRINTLN(" (separator)");
                PRINT("* ARGUMENT n."); PRINT(numArgs); PRINT(" equal to "); PRINTLN(argStack[numArgs]);
                numArgs++;
            }
            else { // ignore the separator
                // ...signal the mistake?
            }
        }

        else if (val == END_PACKET) {
            if (cmdString.length() > 0) {
                //PRINTLN(" (terminator)");
                PRINT(">> COMMAND: "); PRINT(cmdString);
                PRINT("  / NUM ARG: "); PRINTLN(numArgs);

                //PRINT(">> trying execution... ");
                // *********************************************************
                cmdExecuted = interpretCommand(cmdString, numArgs, argStack);
                // *********************************************************

                if (cmdExecuted) PRINTLN(">> END SUCCESSFUL EXECUTION");
                //else PRINTLN(">>FAIL");

                // Even if "cmdExecuted" is false, reset the number of stack arguments
                numArgs = 0;
            }
            else { // an end of packet was received WITHOUT a command: ignore everything

            }
        }

        else {
            // this means we received anything else (not a number, not a letter from A-Z, not a number or packet terminator):
            // ignore and signal the problem?
        }

    }
    return (cmdExecuted);
}

// =============================================================================
// ========== COMMAND INTERPRETER ==============================================
// NOTE: Here you can add commands at will; but I would wait until I can do a
// proper command dictionnary so as to simplify the way you write the new ones.
// =============================================================================
bool interpretCommand(String _cmdString, uint8_t _numArgs, String argStack[]) {
    bool parseOk = true;

    //==========================================================================
    // A) ====== LASER COMMANDS ================================================
    //==========================================================================
    if ((_cmdString == SET_POWER_RED)&&(_numArgs == 1)) {     // Parameters: 0 to 4096 (12 bit res).
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Hardware::Lasers::setPowerRed(constrain(argStack[0].toInt(), 0, 4095));
    }

    //==========================================================================
    // B) ====== SCANNER COMMANDS  =============================================
    //==========================================================================
    else if ((_cmdString == SCAN_STATE)&&(_numArgs == 1)) {         // Parameters: 0/1 (0: stop, 1: resume scan)
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        if (argStack[0].toInt() > 0)
        DisplayScan::startDisplay();
        else
        DisplayScan::stopDisplay();
    }
    else if ((_cmdString == SCAN_PAUSE)&&(_numArgs == 1)) {         // Parameters: 0/1 (0: stop, 1: resume scan)
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        if (argStack[0].toInt() > 0) DisplayScan::pauseDisplay();
        else
        DisplayScan::resumeDisplay();
    }
    else if ((_cmdString == SET_BLANKING_RED)&&(_numArgs == 1))   {     // Parameters: 0/1. It sets blankinFlag state
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        DisplayScan::setBlankingRed( (argStack[0].toInt() > 0 ? 1 : 0 ) );
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
    else if ((_cmdString == RESET_POSE_GLOBAL)&&(_numArgs == 0))       {     // Parameters: none
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::setCenter(0.0,0.0);
        Graphics::setAngle(0.0);
        Graphics::setScaleFactor(1.0);
        // As explained above, we need to RE-RENDER the display buffer:
        Renderer2D::renderFigure();
    }
    else if ((_cmdString == SET_ANGLE_GLOBAL)&&(_numArgs == 1))  {     // Parameters: angle in deg
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::setAngle(argStack[0].toFloat());
        Renderer2D::renderFigure();
    }
    else if ((_cmdString == SET_CENTER_GLOBAL)&&(_numArgs == 2))     {  // Parameters: x,y
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::setCenter(argStack[0].toFloat(), argStack[1].toFloat());
        Renderer2D::renderFigure();
    }
    else if ((_cmdString == SET_FACTOR_GLOBAL)&&(_numArgs == 1))     {  // Parameters: scale
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::setScaleFactor(argStack[0].toFloat());
        Renderer2D::renderFigure();
    }

    //==========================================================================
    // D) ============ FIGURES (check Graphics namespace) ======================
    // * NOTE 1 : after all the figure composition, it is imperative to call to
    //  the method Renderer2D::renderFigure().
    //==========================================================================
    else if ((_cmdString == CLEAR_SCENE)&&(_numArgs == 0))     {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::clearScene();
        Renderer2D::renderFigure();
    }
    // EXAMPLE 1 : NO PARAMETERS (unit radius circle, centered, 100 points)
    else if ((_cmdString == CIRCLE_TEST) &&(_numArgs == 0))     {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::clearScene();    // <-- CLEAR previous figures
        Graphics::drawCircle(100); // also: drawCircle(P2(0,0), 1.0, 100);
        Renderer2D::renderFigure();

        // Force display whatever the previous state:
        DisplayScan::startDisplay(); // start engine (whatever the previous state)
        DisplayScan::resumeDisplay();// and resume in case it was in pause
    }
    // EXAMPLE 2 : MORE POSE PARAMETERS
    // * The pose parameters are COMPOSED with the global ones.
    // * We will draw a circle in (0,0) with unit radius.
    // Parameter is nb points: drawCircle(number points)
    else if ((_cmdString == MAKE_CIRCLE)&&(_numArgs == 1))     {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::clearScene();    // <-- CLEAR previous figures
        Graphics::drawCircle(argStack[0].toInt());
        Renderer2D::renderFigure();
    }
    // EXAMPLE 3 : ... and the same without clearing the scene:
    else if ((_cmdString == ADD_CIRCLE)&&(_numArgs == 1))     {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::drawCircle(argStack[0].toInt());
        Renderer2D::renderFigure();
    }
    // EXAMPLE 4 : MORE POSE PARAMETERS for the SAME COMMAND STRING
    // * The pose parameters are COMPOSED with the global ones.
    // * We will draw a circle: drawCircle(center, radius, number points);
    else if ((_cmdString == MAKE_CIRCLE)&&(_numArgs == 4))     {
        PRINTLN(">> COMMAND AVAILABLE: EXECUTING...");
        Graphics::clearScene();    // <-- CLEAR previous figures
        P2 centerP2(argStack[0].toFloat(), argStack[1].toFloat());
        Graphics::drawCircle(centerP2, argStack[2].toFloat(), argStack[3].toInt());
        Renderer2D::renderFigure();
    }
    // EXAMPLE 5 : CONSERVE PREVIOUS FIGURES:
    // * We will draw a line: drawLine(start point, end point, number points);
    else if ((_cmdString == ADD_LINE)&&(_numArgs == 5))  {
        //Graphics::clearScene(); // <-- Previous figures remain unaltered
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        P2 startP2(argStack[0].toFloat(), argStack[1].toFloat());
        P2 endP2(argStack[2].toFloat(), argStack[3].toFloat());
        Graphics::drawLine(startP2, endP2, argStack[2].toInt());
        Renderer2D::renderFigure();
    }
    // EXAMPLE 6: RECTANGLE, no parameters:
    else if ((_cmdString == MAKE_RECTANGLE)&&(_numArgs == 0))  {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::clearScene();
        Graphics::drawRectangle(P2(-100,0), P2(200, 100), 20.0, 30.0);
        Renderer2D::renderFigure();
    }
    // EXAMPLE 7: COMPOSITE FIGURES:
    // * Example here is SEVEN UNIT SQUARES in a circle (parameters: radius)
    // * NOTE: of course this is centered, rotated and scaled using current global
    //   pose; meaning that if we want to rotate this or scale, we just need to
    //   use SET_ANGLE_GLOBAL, SET_SCALE_GLOBAL, SET_CENTER_GLOBAL
    else if ((_cmdString == MAKE_DISK_SQUARES)&&(_numArgs == 1))  {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Graphics::clearScene();
        float radius = argStack[0].toFloat();
        for (uint8_t k=0; k<7; k++) {
            float phi = 2.0*PI/7*k;
            P2 centerP2(radius*cos(phi), radius*sin(phi));
            Graphics::drawSquare(centerP2, 20); // square with 20 points/side
        }
        // And finally, do the render [see? only at the END of all the
        // figure drawing and settings]
        Renderer2D::renderFigure();
    }
    // .........................................................................
    // ... BUILD HERE WHAT YOU NEED
    // .........................................................................


    //==========================================================================
    // E) ============ MISCELANEA LOW LEVEL COMMANDS ===========================
    //==========================================================================

    else if ((_cmdString == DISPLAY_STATUS)&&(_numArgs == 0))    {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");

        if (DisplayScan::getRunningState())
        PRINTLN(">> ISR STATE: ON");
        else
        PRINTLN(">> ISR STATE: OFF");

        if (DisplayScan::getPauseState())
        PRINTLN(">> DISPLAY STATUS: ON");
        else
        PRINTLN(">> DISPLAY STATUS: OFF");

        PRINT(">> NUM BUFFER POINTS:"); PRINTLN(DisplayScan::getBufferSize());
    }

    else if ((_cmdString == TEST_MIRRORS_RANGE)&&(_numArgs == 1))    {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Hardware::Scanner::testMirrorRange(argStack[0].toInt());
    }
    else if ((_cmdString == TEST_CIRCLE_RANGE)&&(_numArgs == 1))    {
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Hardware::Scanner::testCircleRange(argStack[0].toInt());
    }


    else if ((_cmdString == SET_DIGITAL_PIN)&&(_numArgs == 2))    {     // Parameters:pin, sate
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        Hardware::Gpio::setDigitalPin(argStack[0].toInt(), argStack[1].toInt());
    }

    else if ((_cmdString == RESET_BOARD)&&(_numArgs == 0))     {     // Parameters: 0/1 (0: stop, 1: resume scan)
        PRINTLN(">> COMMAND AVAILABLE - EXECUTING...");
        delay(1000);
        Hardware::resetBoard();
    }

    else { // unkown command!
        PRINTLN(">> UNKOWN COMMAND.");
        parseOk = false;
    }

    if (parseOk) { // signal that everything went fine:
        digitalWrite(PIN_LED_MESSAGE, LOW);
        digitalWrite(PIN_LED_MESSAGE, HIGH);
    }

    return parseOk;
}
