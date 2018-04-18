#ifndef _P2_H
#define _P2_H

#include "Arduino.h"
//#include "Definitions.h"
//#include "Utils.h"

// Very simple 2D integer vector class:
class P2 {
    public:

    P2(): x(0), y(0) {}
    P2(const float _x, const float _y) {
      x = _x;
      y = _y;
    }
    P2(const P2& point) {
      x = point.x;
      y = point.y;
    }

    // Important: to use this as the RingBuffer type, we NEED to define the assignement operator
    inline void operator=( const P2& point ) {
      x = point.x;
      y = point.y;
    }

    inline void set(float _x, float _y) {
      x = _x;
      y = _y;
    }
    inline void set(const P2& _point) {
      x = _point.x;
      y = _point.y;
    }

    // Transfom points (do not modify points in the "moldBuffer" it to avoid approximation drifts!)
    // Note: we don't care very much about the time it takes to do this, it is only done
    // when "moving" the current figure, not during ISR-based displaying.
    inline void translate(P2& _center) {
      x += _center.x;
      y += _center.y;
    }

    inline void rotate(float _angle) {
      float auxX = x;
      x = 1.0*x * cos(DEG_TO_RAD * _angle) - 1.0*y * sin(DEG_TO_RAD * _angle);
      y = 1.0*auxX * sin(DEG_TO_RAD * _angle) + 1.0*y * cos(DEG_TO_RAD * _angle);
    }

    inline void scale(float _factor) {
      x *= _factor;
      y *= _factor;
    }

    // inline void constrainPos() { // inline? not repeated during display thought
    //   if (x > MAX_MIRRORS_ADX) x = MAX_MIRRORS_ADX;
    //   else if (x < MIN_MIRRORS_ADX) x = MIN_MIRRORS_ADX;
    //   if (y > MAX_MIRRORS_ADY) y = MAX_MIRRORS_ADY;
    //   else if (y < MIN_MIRRORS_ADY) y = MIN_MIRRORS_ADY;
    // }
    //
    // static void constrainPos(P2& point) {
    //   Utils::constrainPos(point.x, point.y);
    // }

 // private?:
    float x, y;
};

//typedef P2 PointBuffer[MAX_NUM_POINTS];

#endif
