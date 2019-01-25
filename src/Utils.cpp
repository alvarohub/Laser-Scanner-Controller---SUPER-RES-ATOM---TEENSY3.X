
#include "Utils.h"

namespace Utils
{

bool verboseMode = DEFAULT_VERBOSE_MODE;

void setVerboseMode(bool _active)
{
	verboseMode = _active;
}

bool isDigit(char _val)
{
	//  Check if characters is a number (base 10) plus '-' and '.' to form floats and negative numbers.
	// ('/' must be excluded in the range)
	return ((_val >= '-') && (_val <= '9') && (_val != '/'));
}

// check if ALL the characters are small cap (then an acceptable command argument)
bool isSmallCaps(String _arg)
{
	bool ismall = true;
	for (uint8_t k = 0; k < _arg.length(); k++)
	{
		char val = _arg[k];
		if ( (val < 'a') || (val > 'z') )
			ismall = false;
		break;
	}
}

bool isNumber(String _str)
{
	// the check is very simple here: it will just verify on the first character!
	return (isDigit(_str[0]));
}

int8_t getIndexLaserFromName(String _str)
{
	// uint8_t classID = classMap.find(_className);
	int8_t index = 0;
	for (uint8_t k = 0; k < NUM_LASERS; k++)
	{
		if (_str == Definitions::laserNames[k])
		{
			index = k;
			break;
		}
	}
	return (index);
}

int8_t getIndexClassFromName(String _str)
{
	// uint8_t classID = classMap.find(_className);
	int8_t index = 0;
	for (uint8_t k = 0; k < NUM_MODULE_CLASSES; k++)
	{
		if (_str == Definitions::classNames[k])
		{
			index = k;
			break;
		}
	}
	return (index);
}

} // namespace Utils
