#include "graphics.h"

namespace Graphics {

	// ======================== POSE and SIZE =============================
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

	// ======================== SCENE SETTING METHODS =====================
	extern void clearScene() {
		Renderer2D::clearBlueprint();
	}

	// ======================== BASIC SHAPES ============================
	void drawLine(const P2 &_fromPoint, const float _lenX, const float _lenY, const uint16_t _numPoints) {
		float dx = _lenX/_numPoints, dy = _lenY/_numPoints;
		P2 newPoint(_fromPoint);
		for (uint16_t i = 0; i < _numPoints; i++) {
			Renderer2D::addToBlueprint(newPoint);
			// TODO: use overloaded operators (make a good 2D vector class)!!
			newPoint.x+=dx; newPoint.y+=dy;
		}
	}
	void drawLine(const P2 &_fromPoint, const P2 &_toPoint, const uint16_t _numPoints) {
		drawLine(_fromPoint, _toPoint.x-_fromPoint.x, _toPoint.y-_fromPoint.y, _numPoints);
	}
	void drawLine(uint16_t const _numPoints) {// horizontal, "centered", unit length
		drawLine(P2(-0.5,0.0), P2(0.5, 0.0), _numPoints);
	}

	void drawRectangle(const P2 &_fromBottomLeftCornerPoint, const float _lenX, const float _lenY, const uint16_t _nx, const uint16_t _ny) {
		drawLine(_fromBottomLeftCornerPoint,  _lenX,      0,  _nx);
		drawLine(Renderer2D::getLastPoint(),      0,  _lenY,  _ny);
		drawLine(Renderer2D::getLastPoint(), -_lenX,      0,  _nx);
		drawLine(Renderer2D::getLastPoint(),      0, -_lenX,  _ny);
		}
	void drawRectangle(const P2 &_lowerLeftCorner, const P2 &_upperRightCorner, const uint16_t _nx, const uint16_t _ny) {
		P2 auxPoint(_upperRightCorner.x, _lowerLeftCorner.y);
		drawLine(_lowerLeftCorner, auxPoint, _nx);
		drawLine(auxPoint, _upperRightCorner, _ny);
		auxPoint.set(_lowerLeftCorner.x, _upperRightCorner.y);
		drawLine(_upperRightCorner, auxPoint, _nx);
		drawLine(auxPoint, _lowerLeftCorner, _ny);
	}

	void drawSquare(const P2 &_lowerLeftCorner, const float _sideLength, const uint16_t _numPointsSide) {
		float step = _sideLength/_numPointsSide;
		float x = _lowerLeftCorner.x, y = _lowerLeftCorner.y;
		for (uint16_t k = 0; k<4; k++) {
			for (uint16_t i = 0; i < _numPointsSide; i++) {
				if (k%2) x += (k<1? 1.0 : -1.0)*step*i;
				else y += (k<1? 1.0 : -1.0)*step*i;
				Renderer2D::addToBlueprint(P2(x, y));
			}
		}
	}
	void drawSquare(const P2 &_center, const uint16_t _numPointsSide) {
		drawRectangle(P2(_center.x-0.5, _center.y-0.5), P2(_center.x+0.5, _center.y+0.5), _numPointsSide, _numPointsSide);
	}
	void drawSquare(const uint16_t _numPointsSide) {
		drawRectangle(P2(-0.5, -0.5), P2(0.5, 0.5), _numPointsSide, _numPointsSide);
	}

 	void drawCircle(const P2 &_center, const float _radius, const uint16_t _numPoints) {
		for (uint16_t i = 0; i < _numPoints; i++) {
			float phi = 2.0*PI/_numPoints*i;
			P2 auxPoint(_radius*cos(phi), _radius*sin(phi));
			auxPoint.x += _center.x; auxPoint.y += _center.y;
			Renderer2D::addToBlueprint(auxPoint);
		}
	}
	void drawCircle(const float _radius, const uint16_t _numPoints) {
		drawCircle(P2(0.0,0.0), _radius, _numPoints);
	}
	void drawCircle(const uint16_t _numPoints) {
		drawCircle(1.0, _numPoints);
	}

	void drawZigZag(
		const P2 &_fromPoint,
		const float _lenX, const float _lenY,
		const uint16_t _nx, const uint16_t _ny
	) {

	}
	void drawZigZag(
		const P2 &_fromPoint, const P2 &_toPoint,
		const uint16_t _nx, const uint16_t _nuy
	) {

	}
	void drawZigZag(const uint16_t _x, const uint16_t _ny) {

	}

} // end namespace
