#include "graphics.h"

namespace Graphics
{

bool clearModeFlag = true;

// ======================== POSE and SIZE =============================
// REM: in the future make a more OpenGL matrix stack
void setCenter(const P2 &_center)
{
	Renderer2D::center.set(_center);
}

void setCenter(const float &_x, const float &_y)
{
	Renderer2D::center.set(_x, _y);
}

void setAngle(const float _angle)
{
	Renderer2D::angle = _angle;
}

void setScaleFactor(const float _scaleFactor)
{
	Renderer2D::scaleFactor = _scaleFactor;
}

void resetGlobalPose()
{ // this can be done without rendering [~openGL set identity modelview]
	setCenter(0.0, 0.0);
	setAngle(0.0);
	setScaleFactor(1.0);
}

void setColorRed(bool _state)
{
	Renderer2D::colorRed = _state;
}

// ======================== SCENE SETTING METHODS =====================
void clearScene()
{
	// NOTE: for now we just clear the "whole" blueprint, meaning
	// that clearScene and clearBlueprint do the same, but
	// in the future we will have separate blueprints for each object.
	Renderer2D::clearBlueprint(); // clear blueprint is just like drawing an
	// object with... zero points (and not added to the others).

	// Will clearing the scene stop the display engine? If yes, then we may
	// need to START the engine again. Should this be done automatically when
	// rendering a figure? Sounds nice. Let's do that.
	DisplayScan::stopDisplay();

	// Goes to center position or stay in the last point? Second option for the time being.
	// Repositioning of the mirrors can be done after clearing the scene anyway (in the
	// interpret command method)
	// Hardware::Scanner::recenterPosRaw();
}

void setClearMode(bool _clearModeFlag)
{
	clearModeFlag = _clearModeFlag;
}

bool getClearMode()
{
	return (clearModeFlag);
}

void updateScene()
{
	// internally called ~ "private"
	if (clearModeFlag)
		clearScene();
}

void addVertex(const P2 &_newPoint)
{
	// NOTE: as with the clearScene, for now this is just equal to
	// Renderer2D::addToBlueprint(const P2 _newPoint), but we will
	// have different objects in the future - with an ID, and defined
	// with begin/end if we want to "copy" OpenGL style.
	Renderer2D::addToBlueprint(_newPoint);
}

void addVertex(const P2 &_newPoint, uint16_t _manyTimes)
{
	for (uint8_t k = 0; k < _manyTimes; k++) // repeated left start-line point to avoid deformation
		addVertex(_newPoint);
}

// ======================== BASIC SHAPES ============================
void drawLine(
	const P2 &_fromPoint,
	const float _lenX, const float _lenY,
	const uint16_t _numPoints)
{
	float dx = _lenX / (_numPoints - 1); // Note the -1 because of the line endpoint
	float dy = _lenY / (_numPoints - 1);
	P2 newPoint(_fromPoint);

	for (uint16_t i = 0; i < _numPoints; i++)
	{
		addVertex(newPoint);
		// TODO: use overloaded operators (make a good 2D vector class)!!
		newPoint.x += dx;
		newPoint.y += dy;
	}
}
//Origin at (0,0):
void drawLine(
	const float _lenX, const float _lenY,
	const uint16_t _numPoints)
{
	drawLine(
		P2(0, 0),
		_lenX, _lenY,
		_numPoints);
}

void drawCircle(
	const P2 &_center,
	const float _radius,
	const uint16_t _numPoints)
{
	for (uint16_t i = 0; i <= _numPoints; i++)
	{
		float phi = 2.0 * PI / (_numPoints - 1) * i;
		P2 auxPoint(_radius * cos(phi), _radius * sin(phi));
		auxPoint.x += _center.x;
		auxPoint.y += _center.y;
		addVertex(auxPoint);
	}
}
// Centered:
void drawCircle(
	const float _radius,
	const uint16_t _numPoints)
{
	drawCircle(
		P2(0.0, 0.0),
		_radius,
		_numPoints);
}

void drawRectangle(
	const P2 &_lowerLeftCorner,
	const float _lenX, const float _lenY,
	const uint16_t _nx, const uint16_t _ny)
{
	drawLine(_lowerLeftCorner, _lenX, 0, _nx);
	drawLine(P2(Renderer2D::getLastPoint()), 0, _lenY, _ny);
	drawLine(P2(Renderer2D::getLastPoint()), -_lenX, 0, _nx);
	drawLine(P2(Renderer2D::getLastPoint()), 0, -_lenY, _ny);
}
// Centered:
void drawRectangle(
	const float _lenX, const float _lenY,
	const uint16_t _nx, const uint16_t _ny)
{
	drawRectangle(
		P2(-_lenX / 2, -_lenY / 2),
		_lenX, _lenY,
		_nx, _ny);
}

void drawSquare(
	const P2 &_lowerLeftCorner,
	const float _sideLength,
	const uint16_t _numPointsSide)
{
	drawRectangle(
		_lowerLeftCorner,
		_sideLength, _sideLength,
		_numPointsSide, _numPointsSide);
}
// Centered:
void drawSquare(
	const float _sideLength,
	const uint16_t _numPointsSide)
{
	drawSquare(
		P2(-_sideLength / 2, -_sideLength / 2),
		_sideLength,
		_numPointsSide);
}

void drawZigZag(
	const P2 &_fromPoint,
	const float _lenX, const float _lenY,
	const uint16_t _nx, const uint16_t _ny,
	bool _mode) // mode 0: without return / mode 1: interlaced (with return)
{
	uint8_t numPointsStepLine = 3;
	uint8_t numRepeatedStartLine = 0;
	if (_mode)
	{
		numPointsStepLine = 5;
		numRepeatedStartLine = 5;
	}

	P2 point(_fromPoint);
	float stepY = _lenY / _ny * (1 + _mode);
	uint8_t lines = _ny / 2 / (1 + _mode);

	for (uint16_t i = 0; i < lines; i++)
	{
		addVertex(point, numRepeatedStartLine); // repeated left start-line point to avoid deformation
		drawLine(point, _lenX, 0, _nx);
		point = Renderer2D::getLastPoint();

		addVertex(point, numRepeatedStartLine);
		drawLine(point, 0, stepY, numPointsStepLine);
		point = Renderer2D::getLastPoint();

		addVertex(point, numRepeatedStartLine);
		drawLine(point, -_lenX, 0, _nx);
		point = Renderer2D::getLastPoint();

		addVertex(point, numRepeatedStartLine);
		drawLine(point, 0, stepY, numPointsStepLine);
		point = Renderer2D::getLastPoint();
	}
	if (_mode)
	{ // do the interlaced return:
		point.y -= stepY / 2;
		for (uint16_t i = 0; i < lines; i++)
		{
			addVertex(point, numRepeatedStartLine); // repeated left start-line point to avoid deformation
			drawLine(point, _lenX, 0, _nx);
			point = Renderer2D::getLastPoint();

			addVertex(point, numRepeatedStartLine);
			drawLine(point, 0, -stepY, numPointsStepLine);
			point = Renderer2D::getLastPoint();

			addVertex(point, numRepeatedStartLine);
			drawLine(point, -_lenX, 0, _nx);
			point = Renderer2D::getLastPoint();

			addVertex(point, numRepeatedStartLine);
			drawLine(point, 0, -stepY, numPointsStepLine);
			point = Renderer2D::getLastPoint();
		}
	}
}
// Centered:
void drawZigZag(
	const float _lenX, const float _lenY,
	const uint16_t _nx, const uint16_t _ny,
	bool _mode) // mode 0: without return / mode 1: interlaced (with return)
{
	drawZigZag(
		P2(-_lenX / 2, -_lenY / 2),
		_lenX, _lenY,
		_nx, _ny,
		_mode);
}

// Draw a spiral (equal steph length, not constant angle step!)
void drawSpiral(const P2 &_center,
				const float _radiusArm, // r = _radiusArm * theta
				const float _numTours,
				const uint16_t _numPoints,
				bool _mode) // mode 0: without return / mode 1: interlaced (with return)
{
	float radiusArm = _radiusArm * (1 + _mode);
	float numTours = _numTours / (1 + _mode);
	uint16_t numPoints = _numPoints / (1 + _mode);
	float phi = 2.0 * PI * numTours;
	float length = radiusArm / (4.0 * PI) * (phi * sqrt(1 + phi * phi) + log(phi + sqrt(1 + phi * phi)));

	float stepLength = 1.0 * length / (numPoints - 1);
	float theta = 0, stepTheta = 0;

	// 1) Go outwards:
	while (theta <= phi)
	{
		float r = radiusArm * theta / 2 / PI;
		P2 point(_center.x + r * cos(theta), _center.y + r * sin(theta));
		addVertex(point);

		// Use dicotomy to find the stephTheta such that the length increase is equal to stepLength:
		// float stepTheta = 1.0*phi/_numPoints; // the step should be smaller than that
		// ... OR, for large number of points, we have the approximation:
		stepTheta = stepLength / (radiusArm * sqrt(1 + theta * theta));

		theta += stepTheta;
	}

	if (_mode)
	{
		// 2) Go inwards:
		// a) do a phase offset:
		theta = theta - PI - stepTheta;
		while (theta > PI)
		{
			float r = radiusArm * theta / 2 / PI;
			// b) ... and rotate by PI:
			P2 point(_center.x + r * cos(theta + PI), _center.y + r * sin(theta + PI));
			addVertex(point);

			// Use dicotomy to find the stephTheta such that the length increase is equal to stepLength:
			// float stepTheta = 1.0*phi/_numPoints; // the step should be smaller than that
			// ... OR, for large number of points, we have the approximation:
			stepTheta = stepLength / (radiusArm * sqrt(1 + theta * theta));

			theta -= stepTheta;
		}
	}
}

// centered:
void drawSpiral(
	const float _radiusArm, // r = _radiusArm * theta
	const float _numTours,
	const uint16_t _numPoints,
	bool _mode) // mode 0: without return / mode 1: interlaced (with return)
{
	drawSpiral(P2(0, 0), _radiusArm, _numTours, _numPoints, _mode);
}

} // namespace Graphics
