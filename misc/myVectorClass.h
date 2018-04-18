// classes for 2d and 3d vectors

#ifndef _vectors_H
#define _vectors_H

#include "Arduino.h"

// =================  2D VECTORS =====================

template <class T>
class vector2D {

public:

    // Overloaded constructor with parameters:
    vector2D():x(0), y(0) {}
    vector2D( const T _x=0.0f, const T _y=0.0f );
    vector2D( const vector2D &vec ) {set(vec);} // note: vector2D is treated as vector2D<T> by the compiler, because
    // the declaration is inside the class declaration scope.

    // Explicit setting:
    void set( T _x, T _y );
    void set( const vector2D& vec );

    // Comparison:
    bool operator==( const vector2D& vec );
    bool operator!=( const vector2D& vec );
    bool match( const vector2D& vec, float tolerance=0.0001 );

    // Overloaded operators:
    // note: can we use the default copy constructor?
    void      operator=( const vector2D& vec );    // I cannot declare this if we want also operator chaining?
    //vector2D & operator=( const vector2D& vec ); // this is to enable operator chaining (vec1=vec2=vec3).
    vector2D  operator+( const vector2D& vec ) const; // note: "const" here means that the member function will not alter any member variable
    vector2D& operator+=( const vector2D& vec );      // why it has an output? for doing vec1=vec2+=vec3? YES!!! (operator chaining).
    vector2D  operator-( const vector2D& vec ) const;
    vector2D& operator-=( const vector2D& vec );
    vector2D  operator*( const vector2D& vec ) const;
    vector2D& operator*=( const vector2D& vec );
    vector2D  operator/( const vector2D& vec ) const;
    vector2D& operator/=( const vector2D& vec );

    //operator overloading for float:
    void      operator=( const T f);  // I cannot declare this if we want also operator chaining?
    //vector2D & operator=( const T& val ); // to allow operator chaining
    vector2D  operator+( const T f ) const;
    vector2D& operator+=( const T f );
    vector2D  operator-( const T f ) const;
    vector2D& operator-=( const T f );
    vector2D  operator-() const;

    // multiplication by a scalar:
    vector2D  operator*( const T f ) const;
    vector2D& operator*=( const T f );
    vector2D  operator/( const T f ) const;
    vector2D& operator/=( const T f );

    // Distance (between end points of two vector2Ds):
    float distance( const vector2D& pnt) const;
    float squareDistance( const vector2D& pnt ) const;

    // Length of vector2D (norm):
    float length() const;
    float squareLength() const; // faster, no sqrt

    // Scaling (to a certain length)
    vector2D  getScaled( const float length ) const;
    vector2D& scale( const float length );

    // Multiplication by a scalar - already done, but
    // we redefine it as "mult" for compatiblity:
    vector2D  getMult( const float length ) const;
    vector2D& mult( const float length );

    // Normalization:
    vector2D  getNormalized() const;
    vector2D& normalize();

    // Perpendicular normalized vector2D.
    vector2D  getPerpendicularNormed(int orientation) const;
    vector2D& perpendicular(int orientation);

    // Rotation
    vector2D  getRotatedDeg( float _angleDeg ) const;
    vector2D  getRotatedRad( float _angleRad ) const;
    vector2D& rotateDeg( float _angleDeg );
    vector2D& rotateRad( float _angleRad );

    // Translation - actually this is the same than adding using
    // the overloaded operator "+" or "-":
    void translate(const vector2D& vec);

    //vector2D product (for 3d vector2Ds - for 2d vector2Ds, something like this is just the "angle" between them):
    //vector2D getvector2DProduct(const vector2D& vec) const;
    //vector2D& vector2DProduct(const vector2D& vec) const;

    //Angle (deg) between two vector2Ds (using atan2, so between -180 and 180)
    float angleDeg( const vector2D& vec ) const;
    float angleRad( const vector2D& vec ) const;
    float angleDegHoriz( ) const; // particular case when the second vector is just (1,0)

    //Dot Product:
    float dot( const vector2D& vec ) const;

    // =================================================================
    /* Handy conversion between template types (e.g. from vector2D<float> to vector2D<uint16_t>, i.e, "ADC" units)
    NOTE: won't be necessary, as the only time this will be needed is during the final projection (homography), and I will create
    a special method for matrix product that has a V2i return type.

    // Copy constructor:
    template <typename Type2> vector2D(const vector2D<Type2> &other) { x = other.x; y = other.y; }

    // Assignement operator:
    template<typename FromT>
    Pos2<T>& operator=(const Pos2<FromT>& from) {
        x = from.x;
        y = from.y;
        return *this;
    }
    */

    // =================================================================

    // Actual variables:
    T x, y; // or make a class "point"

};

// 3D VECTORS (maybe not used)
template <class T>
class vector3D {

public:

    // Overloaded constructor with parameters:
    vector3D( T _x=0.0f, T _y=0.0f ,  T _z=0.0f );
    vector3D( const vector3D& vec) {set(vec);};

    // Explicit setting:
    void set( T _x, T _y, T _z );
    void set( const vector3D& vec );

    // Comparison:
    bool operator==( const vector3D& vec );
    bool operator!=( const vector3D& vec );
    bool match( const vector3D& vec, float tollerance=0.0001 );

    // Overloaded operators:
    //
    void    operator=( const vector3D& vec );    // I cannot declare this if we want also operator chaining?
    //vector3D & operator=( const vector2D& vec ); // this is to enable operator chaining (vec1=vec2=vec3).
    vector3D  operator+( const vector3D& vec ) const;
    vector3D& operator+=( const vector3D& vec );      // why it has an output? for doing vec1=vec2+=vec3? YES!!! (operator chaining).
    vector3D  operator-( const vector3D& vec ) const;
    vector3D& operator-=( const vector3D& vec );
    vector3D  operator*( const vector3D& vec ) const;
    vector3D& operator*=( const vector3D& vec );

    // division "dimension by dimension" (like the matlab "./"):
    vector3D  operator/( const vector3D& vec ) const;
    vector3D& operator/=( const vector3D& vec );
    // note: division by a vector is properly defined for 2d vectors: norm is divided and argument is substracted (complex division).

    //operator overloading for float:
    void       operator=( const T f);  // I cannot declare this if we want also operator chaining?
    //vector3D & operator=( const float& val ); // to allow operator chaining
    vector3D  operator+( const T f ) const;
    vector3D& operator+=( const T f );
    vector3D  operator-( const T f ) const;
    vector3D& operator-=( const T f );
    vector3D  operator-() const;


    // multiplication by a scalar:
    vector3D  operator*( const T f ) const;
    vector3D& operator*=( const T f );
    vector3D  operator/( const T f ) const;
    vector3D& operator/=( const T f );

    // Distance (between end points of two vector3Ds):
    float distance( const vector3D& pnt) const;
    float squareDistance( const vector3D& pnt ) const;

    // Length of vector3D (norm):
    float length() const;
    float squareLength() const; // faster, no sqrt

    // Scaling:
    vector3D  getScaled( const float length ) const;
    vector3D& scale( const float length );

    // Normalization:
    vector3D  getNormalized() const;
    vector3D& normalize();


    //Angle (deg) between two vector2Ds (using atan2, so between -180 and 180)
    float angleDeg( const vector3D& vec ) const;
    float angleRad( const vector3D& vec ) const;

    //Dot Product:
    float dot( const vector3D& vec ) const;

    //Cross product:
    vector3D  cross( const vector3D& vec ) const;
    vector3D& cross( const vector3D& vec );

    // =================================================================

    // Actual variables:
    T x, y, z; // or make a class "point"

};

// ======================================================================================

template <class T>
class Rectangle {
public:
    Rectangle() {};
    Rectangle(T _x0, T _y0, T _x1, T _y1): x0(_x0), y0(_y0), x1(_x1), y1(_y1) {};
    // can we use a default constructor for a templated class? should be ok,
    // compiler calls the default constructor of the template type for the members.

    //template <class T2>
    //void set(const Rectangle<T2>& rec ) { x0=rec.x0; y0=rec.y0; x1=rec.x1; y1=rec.y1; }; // implicit conversion of types in the x0=rec.x0...

    void set(const Rectangle &rec ) {
        x0=rec.x0; y0=rec.y0;
        x1=rec.x1; y1=rec.y1;
    }
    void set(T _x0, T _y0, T _x1, T _y1) {
        x0=_x0; y0=_y0;
        x1=_x1; y1=_y1;
    }

    // can we use the default assignement operator for a template struct? should be ok too! but problem...
    // check: http://www.cplusplus.com/forum/general/8580/
    //template <class T2>
    //void operator=( const Rectangle<T2>& rec ){ x0=rec.x0; y0=rec.y0; x1=rec.x1; y1=rec.y1; };

    void operator=( const Rectangle& rec ) { x0=rec.x0; y0=rec.y0; x1=rec.x1; y1=rec.y1; };

    T x0, y0, x1, y1;
};

// ======================================================================================

//Handy typedefs:
typedef vector2D<uint16_t> V2i; // i.e. "ADC" coordinates
typedef vector2D<float> V2f;

typedef vector3D<float> V3f;

typedef Rectangle<uint16_t> Recti;
typedef Rectangle<float> Rectf;

// ======================================================================================

#include "myVectorClass.tpp"

#endif
