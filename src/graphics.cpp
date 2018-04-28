#include "graphics.h"

namespace Graphics {

	bool clearModeFlag = true;

	// ======================== POSE and SIZE =============================
	// REM: in the future make a more OpenGL matrix stack
	void setCenter(const P2 &_center) {
		Renderer2D::center.set(_center);
	}

	void setCenter(const float &_x, const float &_y) {
		Renderer2D::center.set(_x, _y);
	}

	void setAngle(const float _angle) {
		Renderer2D::angle = _angle;
	}

	void setScaleFactor(const float _scaleFactor) {
		Renderer2D::scaleFactor = _scaleFactor;
	}

	void resetGlobalPose() { // this can be done without rendering [~openGL set identity modelview]
		setCenter(0.0,0.0);
		setAngle(0.0);
		setScaleFactor(1.0);
	}

 	void setColorRed(bool _state) {
	 	Renderer2D::colorRed = _state;
 	}

	// ======================== SCENE SETTING METHODS =====================
	void clearScene() {
		// NOTE: for now we just clear the "whole" blueprint, meaning
		// that clearScene and clearBlueprint do the same, but
		// in the future we will have separate blueprints for each object.
		Renderer2D::clearBlueprint(); // clear blueprint is just like drawing an
		// object with... zero points (and not added to the others).

		// NOTE: this does not STOPS the display engine if it was working!

		// Goes to center position (not by loading the blueprint):
		Hardware::Scanner::recenterPosRaw();
	}

	void setClearMode(bool _clearModeFlag) {
		clearModeFlag = _clearModeFlag;
	}

	bool getClearMode() {
		return(clearModeFlag);
	}

	void updateScene() {
		// internally called ~ "private"
		if (clearModeFlag) clearScene();
	}

	void addVertex(const P2& _newPoint) {
		// NOTE: as with the clearScene, for now this is just equal to
		// Renderer2D::addToBlueprint(const P2 _newPoint), but we will
		// have different objects in the future - with an ID, and defined
		// with begin/end if we want to "copy" OpenGL style.
		Renderer2D::addToBlueprint(_newPoint);
	}

	// ======================== BASIC SHAPES ============================
	void drawLine(
		const P2 &_fromPoint,
		const float _lenX, const float _lenY,
		const uint16_t _numPoints
	) {
		float dx = _lenX/_numPoints, dy = _lenY/_numPoints;
		P2 newPoint(_fromPoint);
		// Note the <= because of the line endpoint
		for (uint16_t i = 0; i <= _numPoints; i++) {
			addVertex(newPoint);
			// TODO: use overloaded operators (make a good 2D vector class)!!
			newPoint.x+=dx; newPoint.y+=dy;
		}

	}
	//Centered:
	void drawLine(
		const float _lenX, const float _lenY,
		const uint16_t _numPoints
	) {
		drawLine(
			P2(-_lenX/2,-_lenY/2),
			_lenX, _lenY,
			_numPoints
		);
	}

	void drawCircle(
		const P2 &_center,
		const float _radius,
		const uint16_t _numPoints
	) {
		for (uint16_t i = 0; i < _numPoints+1; i++) {
			float phi = 2.0*PI/_numPoints*i;
			P2 auxPoint(_radius*cos(phi), _radius*sin(phi));
			auxPoint.x += _center.x; auxPoint.y += _center.y;
			addVertex(auxPoint);
		}
	}
	// Centered:
	void drawCircle(
		const float _radius,
		const uint16_t _numPoints
	) {
		drawCircle(
			P2(0.0,0.0),
			_radius,
			_numPoints
		);
	}

	void drawRectangle(
		const P2 &_lowerLeftCorner,
		const float _lenX, const float _lenY,
		const uint16_t _nx, const uint16_t _ny
	) {
		drawLine(_lowerLeftCorner,  			  _lenX,      0,  _nx);
		drawLine(P2(Renderer2D::getLastPoint()),      0,  _lenY,  _ny);
		drawLine(P2(Renderer2D::getLastPoint()), -_lenX,      0,  _nx);
		drawLine(P2(Renderer2D::getLastPoint()),      0, -_lenY,  _ny);
	}
	// Centered:
	void drawRectangle(
		const float _lenX, const float _lenY,
		const uint16_t _nx, const uint16_t _ny
	) {
		drawRectangle(
			P2(-_lenX/2,-_lenY/2),
			_lenX, _lenY,
			_nx, _ny
		);
	}

	void drawSquare(
		const P2 &_lowerLeftCorner,
		const float _sideLength,
		const uint16_t _numPointsSide
	) {
		drawRectangle(
			_lowerLeftCorner,
			_sideLength, _sideLength,
			_numPointsSide, _numPointsSide
		);
	}
	// Centered:
	void drawSquare(
		const float _sideLength,
		const uint16_t _numPointsSide
	) {
		drawSquare(
			P2(-_sideLength/2,-_sideLength/2),
			_sideLength,
			_numPointsSide
		);
	}

	void drawZigZag(
		const P2 &_fromPoint,
		const float _lenX, const float _lenY,
		const uint16_t _nx, const uint16_t _ny
	) {
		P2 point(_fromPoint);
		float stepY = _lenY / _ny;
		for (uint16_t i = 0; i < _ny/2; i++) {
			drawLine(point, _lenX, 0, _nx);
			point = Renderer2D::getLastPoint();
			point.y+=stepY;
			drawLine(point, -_lenX, 0, _nx);
			point = Renderer2D::getLastPoint();
			point.y+=stepY;
		}
	}
	// Centered:
	void drawZigZag(
		const float _lenX, const float _lenY,
		const uint16_t _nx, const uint16_t _ny
	) {
		drawZigZag(
			P2(-_lenX/2, -_lenY/2),
			_lenX, _lenY,
			_nx, _ny
		);
	}


// Draw a spiral (equal steph length, not constant angle step!)
	void drawSpiral(const P2 &_center,
		const float _radiusArm, // r = _radiusArm * theta
		const float _numTours,
		const uint16_t _numPoints
	)	{
		float phi = 2.0*PI*_numTours;
		float length = _radiusArm/(4.0*PI)*( phi*sqrt(1+phi*phi) + log(phi+sqrt(1+phi*phi)) );
		float stepLength = 1.0*length/_numPoints;

		Serial.println(length);

		float theta = 0, stepTheta;
		while (theta<phi) {
			float r = _radiusArm * theta;
			P2 point( _center.x + r*cos(theta), _center.y + r*sin(theta) );
			addVertex(point);

			// Use dicotomy to find the stephTheta such that the length increase is equal to stepLength:
			// float stepTheta = 1.0*phi/_numPoints; // the step should be smaller than that
			// ... OR, for large number of points, we have the approximation:
			stepTheta = stepLength/(_radiusArm*sqrt(1+theta*theta));

			theta+=stepTheta;
		}
	}

	// centered:
	void drawSpiral(
		const float _radiusArm, // r = _radiusArm * theta
		const float _numTours,
		const uint16_t _numPoints
	) {
		drawSpiral(P2(0,0),_radiusArm, _numTours, _numPoints);
	}

} // end namespace
