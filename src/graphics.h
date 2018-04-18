
/*
=============================================================================
========================= FIGURE PRIMITIVES =================================
=============================================================================
** NOTE1: the graphic renderer is very simple here: only one "object" at a time or
"primitive" (but it can be arbitrarily complex up to MAX_NUM_POINTS).
A graphic primitive is built by setting its "scaffold" or "mold" before applying
geometric transformation [in OpenGL this buffer is called the "vertex array"].
Here the array is formed by "P2" points, and resides in a namespace:

Renderer2D::blueprintVertexArray[MAX_NUM_POINTS])

** NOTE2: The graphic primitive is in general "normalized" [that is: "non resized,
rotated or translated"]. In OpenGL, applying the modelview transformation [i.e.
the first "rendering" transformation before projection] is done at the end of
the vertex array setting [that is, at the end of the begin()...end() code].
Here rendering is done by a specific call to the method:

Renderer2D::renderFigure()).

* NOTE3: The object pose is set separatedly BEFORE calling to the renderer using:

Renderer2D::setCenter(...),
Renderer2D::setAngle(...)
Renderer2D::setScaleFactor(...)

The order of the calls is not important, as we are not really applying transformations
to the "current modelview matrix": in fact, these calls sets ONE modelview matrix
formed by FIRST rotating/resizing, and THEN translating.

* NOTE4: it is important to properly set the number of figure points, so that
the scanning display engine (check Scanner namespace methods) can actually go
through all these points - no less or more! This is done by a call to:

Renderer2D::setNumberFigurePoints(...);

This can be done at any time [before or after the vertex array filling, since then
buffers should be always larger than the number of points in the figure]. However,
it may be a good idea to call it at the end of the task and use a vertex counter
so we know exactly how many vertices we are adding. This can be easily done by passing the output of the last render [which is the index+1 of the last rendered point]

* NOTE 5: the output of each drawing method is the LAST point of the drawing. It is
useful for starting the next piece of figure. We could have output a struct with the
index and the point too... but in the future we will use a more reasonable, more
openGL like renderer.
*/

#ifndef _H_WRAPPER_GRAPHICS_
#define _H_WRAPPER_GRAPHICS_

#include "Arduino.h"
#include "Definitions.h"
#include "Utils.h"
#include "Class_P2.h"
#include "renderer2D.h"

namespace Graphics {

	// (1) Setter for OPENGL-like pose ("modelview matrix") ==========
	// * NOTE: figure is recomputed from its (unchanged, stored) blueprint to
	// avoid approximation drift.
	extern void setCenter(const P2 &_center);
	extern void setCenter(const float &_x, const float &_y);
	extern void setAngle(const float _angle);
	extern void setScaleFactor(const float _scaleFactor);

	// (2) "Scene" setting methods and rendering wrappers:
	extern void clearScene(); // force clear scene
	extern void updateScene();
	extern void setClearMode(bool _clearModeFlag);
	extern bool getClearMode() ;
	//extern void renderFigure(); // unnecessary wrapper, and less clear than
	//calling the Renderer2D (we could then change the rendering engine easily)

	extern void addVertex(const P2& _newPoint);

	//(3) Basic shapes. We need to pass at least the number of points - we
	// * NOTE1: could have a default "opengl-like" state variable, but it's
	// not very meaninful.
	// * NOTE2: if no clearScene() call is made, the figure will be ADDED to
	// the former one [which may be of course a nice thing to compose
	// "scenes"]. However, since we are using a very simple renderer, there is
	// no blanking in between them nor special pose... beware!
	extern void drawLine(
		const P2 &_fromPoint,
		const float _lenX, const float _lenY,
		const uint16_t _numPoints
	);
	extern void drawLine(
		const P2 &_fromPoint, const P2 &_toPoint,
		const uint16_t _numPoints
	);
	extern void drawLine(uint16_t const _numPoints);// center on (0,0),
	// unitary length

	extern void drawCircle(const P2 &_center, const float _radius, const uint16_t _numPoints);
	extern void drawCircle(const float _radius, const uint16_t _numPoints);
	extern void drawCircle(const uint16_t _numPoints); // center on (0,0), unitary radius.

	extern void drawRectangle(
		const P2 &_fromBottomLeftCornerPoint,
		const float _lenX, const float _lenY,
		const uint16_t _nx, const uint16_t _ny
	);
	extern void drawRectangle(
		const P2 &_lowerLeftCorner, const P2 &_upperRightCorner,
		const uint16_t _nx, const uint16_t _ny
	);

	extern void drawSquare(
		const P2 &_fromBottomLeftCornerPoint,
		const float sideLength,
		const uint16_t _numPointsSide
	);
	extern void drawSquare(const P2 &_center, const uint16_t _numPointsSide);
	extern void drawSquare(const uint16_t _numPointsSide); // center on (0,0), unitary sides

	extern void drawZigZag(
		const P2 &_fromPoint,
		const float _lenX, const float _lenY,
		const uint16_t _nx, const uint16_t _ny
	);
	extern void drawZigZag(
		const P2 &_fromPoint, const P2 &_toPoint,
		const uint16_t _nx, const uint16_t _nuy
	);
	extern void drawZigZag(const uint16_t _x, const uint16_t _ny);

	extern bool clearModeFlag;

} // end namespace

#endif
