#include "parser.hpp"

bool CmdDictionnary::interpretCommand(const char* _cmd, uint8_t _numArgs, StackArgs _cmdArgs) {
    // PRINTLN(); PRINT("INTERPRET COMMAND: ");
    // PRINT_CSTRING(_cmd);
    // PRINT("NUM ARGS: "); PRINTLN(_numArgs);
	for (uint8_t indexCmd = 0; indexCmd < numCmds ; indexCmd++) {
		if (cmdArray[indexCmd].tryCmd(_cmd, _numArgs, _cmdArgs)) return true;
	}
	return false;
}

// rem: this could return the command or null, or the index of the command in the dictionnary...
bool CmdDictionnary::isInDictionnary(const char* _cmd, uint8_t _numArgs) {
	for (uint8_t indexCmd = 0; indexCmd < numCmds ; indexCmd++) {
		if (cmdArray[indexCmd].checkCmd(_cmd, _numArgs)) return true;
	}
	return false;
}

// ========================================================

// Pre-instantiated cross-file global variable
Parser myParser;

bool Parser::parseStringMessage(const char* _rawString) {
	// NOTE: _rawString is supposed to be a well formed C string
    // (i.e, it ends with a zero byte = '\0')
    //strcpy(copyRawString, _rawString);
		copyRawString = (char*) _rawString;


	bool result = true; // we will "AND" this with every command correctly parsed in the string

	// Static variables (so we can re-entry on the parser if the string is too long and cannot be accomodated in
	// only one RFM packet)
	static uint8_t numArgs = 0;
	static uint8_t  indexArgString=0;
	static uint8_t  indexCmdString=0;

	enum stateParser {START, NUMBER, CMD} myState; //Note: the name of an unscoped enumeration may be omitted: such declaration only introduces the enumerators into the enclosing scope.
	myState= START;

	for (readingHead=0; readingHead <= strlen(copyRawString); readingHead++) { // note: we include the c_string terminator!
		uint8_t val=copyRawString[readingHead];

         PRINT(val);  PRINT(" --> "); PRINT((char)val);

		// Save only ASCII numeric characters on numString:
	    if ((val >= '-') && (val <= '9') && (val != '/'))  { // this is 45 to 57 (including '-', '/' and '.')
		  argStack[numArgs][indexArgString++] = val;
		  myState = NUMBER;
           PRINTLN(F(" (is a digit)"));
	    }
		// Save any other ASCII characters BUT not including '/', NL or 0 in cmdString:
		else if ((val!=0)&&(val!='/')&&(val!='\n')&&(val!='\r')) {
			cmdString[indexCmdString++] = val;
			myState = CMD;
            PRINTLN(F(" (is a command letter)"));
		}

		else { // this means that we received '/' or 0, which can terminate either a number ('/') or a command ('/' or 0)
			if (myState == NUMBER) {
				 //numString[indexArgString] = '\0'; // must form a good C-string
				 // argStack[numArgs] = atoi(numString); // if using floats: atof(numString); // double atof (const char* str);
				 argStack[numArgs][indexArgString] = '\0'; // must form a good C-string
				 indexArgString=0;

                PRINT(F(" END ARG: "));PRINT_CSTRING(argStack[numArgs]);
                numArgs++;

			}
			else if (myState == CMD) { // this means we completed the reception of a command: process right away
				cmdString[indexCmdString] = '\0';
				indexCmdString=0;

        PRINTLN(); PRINT(F("  END COMMAND: "));PRINT_CSTRING(cmdString);
				PRINT(F("  NUM ARGS: "));PRINTLN(numArgs);

				if (!myCmdDic->interpretCommand(cmdString, numArgs, argStack)) result = false;

				// And in ANY case - command ok or not - reset the stack size since we "consummed" the values:
				numArgs = 0;
			}
			else {
				//... this means that we received '/' but nothing before. Let's ignore this and proceed.
			}
		}
	}
	return(result);
}
