
#ifndef _SERIAL_COMMANDS_H_
#define _SERIAL_COMMANDS_H_

#include "Definitions.h"
#include "Utils.h"
#include "renderer2D.h"
// I include the following or low level stuff (but perhaps better to use Utils wrappers in the future):
#include "scannerDisplay.h"
#include "graphics.h"

// TODO: IT WOULD BE MUCH BETTER TO HAVE ALL THESE defines as const in the messageParser namespace to
// void conflicts!

/********************************************************************************************************
*********************************************************************************************************
                                  PARSING PROTOCOL
*******************************************************************************************************/
// NOTE: The Enter key sends a CR character (carriage return, Ctrl+M, numerical value 13 = 0x0d = 015).
#define ARG_SEPARATOR ','
#define END_CMD '\n'          // End command (CARRIAGE RETURN, ASCII 13). Without anything else,
                              // this could repeat the last GOOD command, but I won't do that.
#define LINE_FEED_IGNORE '\r' // line feed (ASCII 10) . Ignored and continue parsing.

/*******************************************************************************************************
********************************************************************************************************
                                       COMMAND LIST
********************************************************************************************************/
// TODO: a parser more like the one I used in the NEURAL project, but better: the command object should
// have a parameter struct with its name, its own parameter stack, number of args, and error strings...


// NOTATION:
// laser index or laser name:    laser_id  = [0-4], laser name = ["red", "green", "blue", "deep_blue"]
// optotuner index:              opto_id   = [0-1]
// clock index:                  clk_id    = [0-4]
// trigger processor:            trg_id   = [0-4]
// pulse shaper index:           shaper_id = [0-4]
// Ext input trigger:            trgIn_id  = [0]  (unique for the time being)
// Ext output trigger:           trgOut_id = [0]  (unique for the time being)

//************************************************************************************************************
// 0) MISC *************************************************************************************************
#define REPEAT_COMMAND "REPEAT" // repeats last command sting (good or bad)

//************************************************************************************************************
// 1) LASERS *************************************************************************************************

// a) Per-Laser:
#define SET_POWER_LASER "PWLASER"  // Param: {laser_id, 0-4095}. Changes current power state.
#define SET_SWITCH_LASER "SWLASER" // Param: {laser_id, 0/1}. Set the laser switch state.
#define SET_CARRIER "CARRIER"      // Param: {laser_id, 0/1}, where 0 means no carrier. When switch open, the laser
                                   // shines continuously at the current power (filtered PWM), otherwise it will be
                                   // a 50% PWM [chopping the analog power value]

// b) Simultaneously affecting all lasers:
#define SET_POWER_LASER_ALL "PWLASERALL"  // Param: {0-4095}
#define SET_SWITCH_LASER_ALL "SWLASERALL" // Param: {0/1}
#define SET_CARRIER_ALL "CARRIERALL"      // Param: {0/1}

// c) Lasers test:
#define TEST_LASERS "TSTLASERS" // Will test each laser with a power ramp

//************************************************************************************************************
// 2) SEQUENCER **********************************************************************************************
/* There are 6 types of modules that can be interconnected arbitrarily to make the sequencer:
   (1) One trigger input
   (2) Five independent clocks with configurable period;
   (3) Five trigger processors, each one having the ability to perform:
            - trigger detection (rise, fall, change),
            - burst+skip samples. For example, if burst is 3 and skip is 4, a series of
            trigger events such as [1 1 1 1 1 1 1 1...] becomes [1 1 1 0 0 0 0 1 1 1 0 ...]
            - delay (applied once at start). For example, if lag is 2, [1 1 1 1 1 1 1 1...]  becomes  [0 0 1 1 1 1 1 1...]
            NOTE: in the future, it may be more flexible to have separate modules for each function (trigger catcher, delay module, sequence decimation module with burst and decimation.
    (4) Five pulse shapers, that transform one (dirac) pulse into a hat: [1] --> [0 during tff, 1 during ton]
    (5) Five lasers
    (6) One external trigger

   To be part of the sequencer, we only need to activate a module and specify its unique input (the module from which
   it will receive a "bang"). Some modules don't have an input, and others cannot be used as inputs. In fact, each
   module belong to one of the following three classes:

   1) Only producing a bang (no input module to set, can be used as input for another module):
                              - clocks
                              - external trigger in

   2) Only receiving a bang (no module can set this one as input):
                             - lasers (will change their state as a function of the input)
                             - external trigger out (for the camera for instance)

   3) Receiving and producing a signal (we need to set the input; another module can set this module
      as input too):
                             - pulse shapers
                             - trigger processors

   The clocks, pulse shapers and trigger processors are configurable (and reconfigurable at any time),
   using the setting methods described below.*/

// CLOCK parameter configuration:
#define SET_CLOCK_PERIOD      "SET_PERIOD_CLK"       // Param: {clk_id, period in ms}
#define START_CLOCK	         "START_CLK"            // Param: {clk_id}
#define STOP_CLOCK	         "STOP_CLK"             // Param: {clk_id}
#define SET_CLOCK_STATE       "SET_STATE_CLK"        // Param: {clk_id, 0/1}. Switch the clock off/on
#define SET_CLOCK_STATE_ALL   "SET_STATE_CLK_ALL"    // Param: {0/1}. Switch all clocks off/on
#define RST_CLOCK             "RST_CLK"                     // Param: {clk_id}. Reset clock.
#define RST_ALL_CLOCKS        "RST_CLK_ALL"            // No parameters.

// TRIGGER PROCESSOR parameter configuration:
// Param: {trg_id, mode trigger=[0,1,2], burst=[0...], skip=[0...], offset=[0...]}
// where trigger processor mode 0 is RISE, 1 is FALL and 2 is CHANGE
// TODO: setting parameters independently
#define SET_TRIGGER_PROCESSOR "SET_TRG"

#define SET_PULSE_SHAPER "SET_PUL" // Param: {pul_id, time off (ms), time on (ms)}

// SEQUENCER activation/deactivation:
#define SET_SEQUENCER_STATE   "SET_STATE_SEQ" // Param: {0/1}. Deactivate/activate sequencer.
#define START_SEQUENCER	      "START_SEQ"
#define STOP_SEQUENCER	      "STOP_SEQ"
#define RESET_SEQUENCER       "RST_SEQ"           // Param: none. This is not the same than switching on/off the sequencer;
                                            // instead, it will call the reset() method for all the modules (basically
                                            // restarting the clock, and resetting the trigger processors so the delay
                                            // can be taken into account again)

// TODO: STATUS_CLK
// TODO: LED_CLK on/off

// TODO: a "LED" module or "digital output module" (very useful for checks!!)

// SEQUENCER BUILDING:
// This is the more tedious part given the elementary parser - we need to identify each module by its index, but also
// by its class (is it a laser, a clock...), both for identifying the module to be set, and the "input module".
// In the future I will make a better parser, but for now, the strategy will be to assign a code to each class:
// class code :
//                0 = clk (clock)
//                1 = in (ext trigger in)
//                2 = out (ext trigger out)
//                3 = las (laser)
//                4 = prc (pulse shaper)
//                5 = trg (trigger processor)
//
// followed by the index of the module (for the external triggers, it is always 0, but in the future there may be more)

// a) Adding a module to the sequencer pipeline:
#define ADD_SEQUENCER_MODULE "ADD_SEQ_MODULE" // Param: {mod_from class code [0-5] and index in the class}

// b) Interconnect two modules: mod_from (must have an output) to mod_to (must accept input)
#define SET_SEQUENCER_LINK "SET_LNK_SEQ" // Param: {mod_from class code [0-5] and index in the class [depends],
//                                                   mod_to class code [0-5] and index in the class [depends]}

// For example, to connect the clock number 1 to the trigger processor 3, and then the latter to the laser 2 we do:
//    0,SET_STATE_SEQ              <-- deactivate the sequencer to avoid weird results
//    0,1,5,3,SET_LNK_SEQ         <-- connects the clock (class 0) number 1 to the trigger processor (class 5) number 3
//    5,3,3,2,SET_LNK_SEQ         <-- connects the trigger processor 3 to the laser (class 3) number 2
//    1,SET_STATE_SEQ              <-- reactivate the sequencer

// c) Create a chain of interconnected modules at once:
// NOTE: it does NOT delecte whatever was before
#define SET_SEQUENCER_CHAIN "SET_SEQ" // Param: {module1 class and index, module two class and index, ...}
// For example, to do the same thing described above:
//    0,SET_STATE_SEQ
//    CLEAR_SEQ       <<-- very important if one does not want to add to the previous sequencer!
//    0,1,5,3,3,2,SET_CHAIN_SEQ
//    1,SET_STATE_SEQ

// d) Disconnect ALL links (aka, delete the sequencer pipeline). Not the same than reset or deactivate the sequencer!
// NOTE: clearing the pipeline and then building it again is much more simple than disconnecting the modules and then
// rearranging the connections...
#define CLEAR_SEQUENCER "CLEAR_SEQ"

// e) Display sequencer pipeline status:
#define DISPLAY_SEQUENCER_STATUS "STATUS_SEQ"

//************************************************************************************************************
// 3) OPTOTUNERS *********************************************************************************************
// Note: these outputs are not "chopped" by a fast switch, i.e., there is no "carrier-mode"):

#define SET_POWER_OPTOTUNER_ALL "PWOPTOALL" // {0-4095}. Set the power for all optotuners simultaneously
#define SET_POWER_OPTOTUNER "PWOPTO"        // {opto_id, 0-4095}. Set the power for the optotuner opto_id =[0,1]

//************************************************************************************************************
// 4) READOUT DIGITAL and ANALOG PINS ************************************************************************

#define SET_DIGITAL_A "WDIG_A"  //  Param:: {0/1}. Write digital pin A (pin 31).
#define SET_DIGITAL_B "WDIG_B"  //  Param:: {0/1}. Write digital pin B (pin 32).
#define READ_DIGITAL_A "RDIG_A" //  Read digital pin A (pin 31), output: 0-4095
#define READ_DIGITAL_B "RDIG_B" //  Read digital pin B (pin 32), output: 0-4095

#define SET_ANALOG_A "WANA_A"  // Param: {0-4095}. Write ANALOG (pwm) pin A (pin 16)
#define SET_ANALOG_B "WANA_B"  // Param: {0-4095}. Write ANALOG (pwm) pin B (pin 17)
#define READ_ANALOG_A "RANA_A" // Read analog pin A (pin 16), output: 0-4095
#define READ_ANALOG_B "RANA_B" // Read analog pin B (pin 17), output: 0-4095

//************************************************************************************************************
// 5) Display Engine (on ISR) ********************************************************************************

#define START_DISPLAY "START"   // Start the ISR for the displaying engine
#define STOP_DISPLAY "STOP"     // Stop the displaying ISR
#define SET_INTERVAL "DT"       // Param: {inter-point time in us (min about 20us)}
#define DISPLAY_STATUS "STATUS" // Echo various settings to the serial port.
                                // Note that the number of points in the current blueprint (or "figure")
                                // and the size of the displaying buffer may differ because of clipping.
#define SET_SHUTTER "SHUTTER"   // Param:: {0/1}. Set the shutter state.

//************************************************************************************************************
// 6) GRAPHIC RENDERING **************************************************************************************

// a) POSE (each time the following commands are called, the current figure (in blueprint) is
// re-rendered with the new transforms, in this order of transformation: rotation/scale/translation)
#define RESET_POSE_GLOBAL "RSTPOSE" // Set angle to 0, center to (0,0) and factor to 1
#define SET_ANGLE_GLOBAL "ANGLE"    // Param: {angle in degrees}. Rotate clockwise.
#define SET_CENTER_GLOBAL "CENTER"  // Param: {x,y}. Center the figure around (x,y). Values are from -100 to 100
#define SET_SCALE_GLOBAL "SCALE"    // Param: {scale=[0...]}. Scale the figure (note that 0 will make the figure a point)
#define SET_COLOR_GLOBAL "COLOR"    // TODO

//b) Scene clearing and blanking between objects (only useful when having many figures simultaneously)
#define CLEAR_SCENE "CLEAR" // clear the blueprint, and also stop the display
#define CLEAR_MODE "CLMODE" // Param: {0/1}. When set to 0, if we draw a figure it will be ADDED to the current scene.
                            // Otherwise a drawing first clear the canvas and make a new figure.

// The following commands affect all lasers simultaneously [TODO: per laser?]
#define SET_BLANKING_ALL "BLANKALL" //Figure-to-figure blanking. Param: [0/1]. Affects all lasers.
#define SET_BLANKING "BLANK"        // per laser fig-to-fig blanking

#define SET_INTER_POINT_BLANK "PTBLANK" // pt-to-pt blanking. ALWAYS affects all lasers for the time being.

// c) Figure primitives:
#define MAKE_LINE "LINE"      // Param: width,height,numpoints,LINE [from (0,0)] or posX,posY,length,height,numpoint,LINE
#define MAKE_CIRCLE "CIRCLE"  // Param: radius,numpoints,CIRCLE ou X,Y,radius,numpoints,CIRCLE
#define MAKE_RECTANGLE "RECT" // Param: width,height,numpointX,numPointX,RECT ou X,Y,width,height,numpointsX, numpointY,RECT
#define MAKE_SQUARE "SQUARE"  // Param: size of side,numpoints side,RECT ou X,Y,size-of-side,RECT
#define MAKE_ZIGZAG "ZIGZAG"  // Param: width,height,numpoints X,numpoints Y,ZIGZAG or with position first
#define MAKE_SPIRAL "SPIRAL"  // Param: length-between-arms, num-tours, numpoints, SPIRAL

// d) TEST FIGURES:
#define LINE_TEST "LITEST"
#define CIRCLE_TEST "CITEST"  // no parameters: makes a circle.
#define SQUARE_TEST "SQTEST"  // no parameters: makes a square
#define COMPOSITE_TEST "MIRE" // no parameters: cross and squares (will automatically launch START)
                              // NOTE: we may see the blanking problem here because it is a composite figure

//************************************************************************************************************
// 7) LOW LEVEL FUNCTIONS and CHECKING COMMANDS **************************************************************

#define TEST_MIRRORS_RANGE "SQRANGE" // {Time of show in seconds}. Displays a square showing the limits of galvos.
#define TEST_CIRCLE_RANGE "CIRANGE"  // {Time of show in seconds}. Displays a circle with diameter 200 centered at (0,0)
#define TEST_CROSS_RANGE "CRRANGE"   // {Time of show in seconds}. Displays a cross centered at (0,0)
#define SET_DIGITAL_PIN "SETPIN"     // {pin number, state(true/false)}. Set the value of a digital pin.
#define RESET_BOARD "RESET"          // RESET the board (note: this will disconnect the serial port!)

//************************************************************************************************************
// 8) ADVANCED COMMANDS **************************************************************************************

#define EXECUTE_SCRIPT "EXE_PRM" // Param: if none, it will execute the current recorded script; otherwise
                                    //it takes an argument name (number or non-capital letters ok) and
                                    // will attempt to exectue the script "name".txt" in the file system (micro SD)

// RECORD SCRIPT being input from serial port:
#define START_REC_SCRIPT "BEGIN_PRM"
#define END_REC_SCRIPT "END_PRM"
#define ADD_REC_SCRIPT "ADD_PRM" // does not reset the previous recorded script

// LOAD or SAVE script in memory
#define LOAD_SCRIPT "LOAD_PRM"
#define SAVE_SCRIPT "SAVE_PRM"

// LIST SCRIPTS in SD card
#define LIST_SD_PRM  "LIST_SD_PRM" //TODO: discriminate scripts from other files?
#define SHOW_MEM_PRM "SHOW_PRM"

//10) DEBUG COMMANDS  ****************************************************************************************
#define VERBOSE_MODE "VERBOSE"

/********************************************************************************************************
*********************************************************************************************************
                                        COMMAND PARSER  NAMESPACE
       TODO: use a better parser with a command dictionnary and an "addCommand(...) method.
       Perhaps using a serial parser command library? such as one of these:
                        - https://github.com/geekfactory/Shell
                        - https://github.com/pvizeli/CmdParser/
       TODO: ... and for more advanced stages, use a JSON parser (or an object serializer)
********************************************************************************************************/
namespace Parser
{

const uint8_t SIZE_CMD_STACK = 50; // Maximum size of the *command* stack (TODO: vector... )

// messageParser.h is (for now) only included in main.cpp and SerialCommands.cpp, so we don't need to declare
// the functions "extern".
bool parseStringMessage(const String &_messageString);
bool interpretCommand(String _cmdString, uint8_t _numArgs, String argStack[]);

void resetParser();
void beginRecordingScript();
void endRecordingScript();

// Kind of STL map... sadly, no implemenation of maps in STL arduino
// These methods affect the red laser if no match (TODO: return -1 and check error)
int8_t toBool(String _str);
int8_t toClassID(String _str);
int8_t toLaserID(String _str);
int8_t toTrgMode(String _str);

} // namespace Parser

#endif
