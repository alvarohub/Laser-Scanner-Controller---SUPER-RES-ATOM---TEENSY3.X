#ifndef _RENDERER_H_
#define _RENDERER_H_

// REM: in principle only used in the serialCommands translation unit, but I may use it in the main for debug;
// Therefore, I need to declare everything with external linkage (using "extern" keyword in its declaration)

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"
#include "Class_P2.h"
#include "scannerDisplay.h"
//#include "hardware.h"

// namespace DefaultParamRender {
//
//     // Renderer engine:
//     extern const P2 defaultCenter;
//     extern const float defaultAngle;
//     extern const float defaultScaleFactor;
//
//     // Display enging:
// 	extern const bool defaultBlankingFlag; // (true means off or "blankingFlag")
// 	extern const uint32_t defaultDt; // inter-point delay in usec
//
// }


namespace Renderer2D {

	// ======== STATE VARIABLES  ================
// a) Simplified OpenGL-like "state variables":
//  1)  Simplified, orthographic projection transformation:
// * Note 1: The following parameters define the "region of interest" and
// will be used to map the values from the added vertices to the
// mirror coordinates (using Arduino "map" method or a custom one).
// * NOTE 2: for the time being, these parameters are fixed;
const float minX = -100.0;
const float maxX = 100.0;
const float minY = -100.0;
const float maxY = 100.0;

// 2) the "modelview" matrix (translation, rotation and scaling):
extern P2 center;
extern float angle;
extern float scaleFactor;
extern bool colorRed; // TODO: make a proper color object/struct (not just on/off for the red)

	// b) Number of points. In the future, it would be more interesting to have a
	// "resolution" variable. The number of points should be always smaller
	// than MAX_NUM_POINTS:
	extern uint16_t sizeBlueprint; // don't forget to set it properly before
	// starting the display engine. Normally there is no pb: it is automatically
	// set while drawing figures, plus it has a default start value of 0 [extern
	// global variable definition]

	extern void clearBlueprint();
	extern uint16_t getSizeBlueprint();

	extern const P2 getLastPoint();

	extern void addToBlueprint(const P2 &_newPoint);

	extern void renderFigure(); // render with current pose transformation

	//namespace { // "private"
		//extern PointBuffer bluePrintArray;
		extern P2 bluePrintArray[MAX_NUM_POINTS];
	//}

} // end namespace

#endif
