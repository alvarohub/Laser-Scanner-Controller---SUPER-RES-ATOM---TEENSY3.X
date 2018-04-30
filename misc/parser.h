#ifndef _parser_h
#define _parser_h

#include "Arduino.h"
#include "Utils.h"
#include "hardware.h" // needed to use PRINTLN!!

/*

** Example of use (in main, where the function setShape is declared):

Command myCmdDict[] =
{
	// Generic form:
	//      Command(name , num args, function pointer),

	// List of commands with arguments:
	Command("SET_CENTER" , 2, &setCenter)
	Command("SET_RED" , 1, &setRed)

	// Commands without arguments:
	Command("DO_CIRCLE" , 0, &drawCircle)
	Command("STORE_SHAPE", 0, &storeShape) <-- this is an interesting command: it can take 0 arguments, but will execute the storing of EVERY pair (or triplet) of data in the buffer, as a trajectory object. It consumes then all the "arguments", whatever the number.
	...

	...
};

** Where setShape or setColor are declares as:

	bool setShape(const char* _valuesStack[]) { ... }

*/

// --------------------------------------------------------------------------------------
//#define MAX_LENGTH_MESSAGE 65 // (can? MUST be larger than 61, which is the max length of the payload that can be send at once by the RFM module (+1 because we want strings)

#define MAX_NUM_CMDS_DICT  25
//#define MAX_CMD_STR_LENGTH 25

#define MAX_LENGTH_STACK 10
//#define MAX_VALUE_STR_LENGTH 10

// --------------------------------------------------------------------------------------

typedef String StackArgs[MAX_LENGTH_STACK];//[MAX_VALUE_STR_LENGTH]; // arguments as strings
typedef bool (*FunctionPtr)(StackArgs _arguments); // false means that the execution could not be completed

// --------------------------------------------------------------------------------------

class Command {

public:
	Command() {};
	Command(const String _cmdName, uint8_t _numArgs, FunctionPtr _fctPtr) {
		setName(_name);
		setFctPtr(_fctPtr);
		setNumArgs(_numArgs);
	}

	// Copy constructor (do I need to do this? the problem would be the copy contructor of name, which is a char*)
	// I will use my assignement operator to be sure.
	Command(const Command& _cmd) {
		*this = _cmd;
	}

	// Assignement operator:
	Command& operator=(const Command& _other) {
		// check for self-assignment
	   if(&_other != this) {
		setName(_other.name);
		setFctPtr(_other.fctPtr);
		setNumArgs(_other.numArgs);
		}
		return *this;
	}

	bool setName(const String _cmdName) {
 		name = _cmdName;
	}

	void setFctPtr(FunctionPtr _fctPtr) {
		fctPtr = _fctPtr;
	}

	bool setNumArgs(uint8_t _numArgs) {
		if (_numArgs <= MAX_LENGTH_STACK) {
			numArgs = _numArgs;
			return true;
		}
		else
			return false;
	}

	bool checkCmd(const String _name, uint8_t _numArgs) {
        // PRINTLN(); PRINT("CHECKING COMMAND: ");
        // PRINT_CSTRING(_name);
        // PRINT("NUM ARGS: "); PRINT(_numArgs);
		return ( (_name == name) && (_numArgs == numArgs) );
	}

	bool runCmd(StackArgs _valuesStack) {
		return (*fctPtr)(_valuesStack);
	}

	bool tryCmd(const char* _name, uint8_t _numArgs, StackArgs _valuesStack) {
		if (checkCmd(_name, _numArgs)) return(runCmd(_valuesStack));
		return false;
	}

private:
	//char name[MAX_CMD_STR_LENGTH];
	String name;
	uint8_t numArgs; // actual number of arguments for this command
	FunctionPtr fctPtr; // const char* _valuesStack[]);
};

// --------------------------------------------------------------------------------------

class CmdDictionnary {
public:
	CmdDictionnary() : numCmds(0) {};
	CmdDictionnary(const CmdDictionnary& _cmdDict) {// User defined copy contructor (I need a deep copy)
		*this = _cmdDict; // user defined assignement operator
	}

	// User defined assignement operator:
	CmdDictionnary& operator=(const CmdDictionnary& _other) {
		numCmds = _other.numCmds;
		for (uint8_t indexCmd = 0; indexCmd < numCmds ; indexCmd++) cmdArray[indexCmd] = _other.cmdArray[indexCmd];
		return(*this);
	}

	// From a "naked" Command list: +++++ THIS DOES NOT WORK! ARRAY_SIZE is WRONG ++++
	CmdDictionnary(const Command _cmdArray[]) {
        // PRINT("NUMBER COMMANDS IN DICTIONNARY: ");
        // PRINT("Size using arraysize: "); PRINTLN(arraySize(_cmdArray));
		numCmds = ARRAY_SIZE(_cmdArray);
        // PRINT("Size using ARRAY_SIZE: "); PRINTLN(numCmds);
		// cmdArray = _cmdDict.cmdArray;
		for (uint8_t indexCmd = 0; indexCmd < numCmds ; indexCmd++) cmdArray[indexCmd] = _cmdArray[indexCmd];
	}

	bool addCommand(const Command& _newCmd) { // return false if we exceeded the max number of commands
		if (numCmds<MAX_NUM_CMDS_DICT) {cmdArray[numCmds++] = _newCmd; return true;}
		return false;
	}

	bool interpretCommand(const char* _cmd, uint8_t _numArgs, StackArgs _cmdArgs);
	bool isInDictionnary(const char* _cmd, uint8_t _numArgs);

private:
	uint8_t numCmds; // actual number of commands in this dictionnary
	Command cmdArray [MAX_NUM_CMDS_DICT];

};

// --------------------------------------------------------------------------------------



class Parser {
public:
		Parser() {}
		Parser(CmdDictionnary* _cmdDic) : myCmdDic(_cmdDic) {} // myCmdDic(_cmdDic) uses the copy constructor
		// (I could just have a pointer to the command dictionnary, but we may want to set the dictionnary directly from a
		// volatile const object in the arguments)

		void setCmdDict(CmdDictionnary* _cmdDic) {
			myCmdDic = _cmdDic; // assignement operator
		}

    // From a "naked" Command list ++++ DOES NOT WORK!!! +++
    void setCmdDict(const Command _cmdArray[]) {
			myCmdDic = new CmdDictionnary(_cmdArray); // CmdDictionnary constructor and assignement operator
		}

		bool parseStringMessage(const char* _c_string);  // the string message should be a complete set of commands-value

        uint8_t getReadingPosition() {return readingHead;}
        char* getRestMessage() {return (&copyRawString[readingHead+1]);}
		char* getRestMessageNL() { // ADD new line to the message:
			uint8_t len = strlen(copyRawString); // not including null terminator!
			// extend it by 1, adding carriage return + new line, just like println
			// (ASCII 13, or '\r') + newline character (ASCII 10, or '\n')
			copyRawString[len] = '\r';
			copyRawString[len+1] = '\n';
			copyRawString[len+2] = '\0'; // necessary to properly calculate strlen in the RF sendTo method.
			return (&copyRawString[readingHead+1]);
		}
        void abortParsing() {readingHead = strlen(copyRawString);}

private:
	CmdDictionnary* myCmdDic;

    //char copyRawString[MAX_LENGTH_MESSAGE];
		char* copyRawString;
    uint8_t readingHead; // I need this "global" in Parser to extract the rest of the message untouched.

	// The following could be local to parseStringMessage:
	char cmdString[MAX_CMD_STR_LENGTH];
	StackArgs argStack; //[MAX_LENGTH_STACK][MAX_VALUE_STR_LENGTH];
	//uint8_t numArgs = 0;
};

extern Parser myParser;

#endif
