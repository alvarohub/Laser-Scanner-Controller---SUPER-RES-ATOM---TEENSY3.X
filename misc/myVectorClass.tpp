
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of template class. This file (.tpp) must be include in the interface (.hpp)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ============================================  2D vectors ============================================

template <class T>
vector2D<T>::vector2D( T _x, T _y ) {
    x = _x; y = _y;
}

template <class T>
void vector2D<T>::set( T _x, T _y ) {
    x = _x; y = _y;
}

template <class T>
void vector2D<T>::set( const vector2D<T>& vec ) {
    x=vec.x; y=vec.y;
}

template <class T>
bool vector2D<T>::operator==( const vector2D<T>& vec ) {
    return (x == vec.x) && (y == vec.y);
}

template <class T>
bool vector2D<T>::operator!=( const vector2D<T>& vec ) {
    return (x != vec.x) || (y != vec.y);
}

template <class T>
bool vector2D<T>::match( const vector2D<T>& vec, float tolerance ) {
    return (abs(x - vec.x) < tolerance)&& (abs(y - vec.y) < tolerance);
}


/*
vector2D & operator=( const vector2D& vec ){ // returning a reference to the vector2D object for allowing operator chaining
    x = vec.x;
    y = vec.y;
    return *this;
}
 */

template <class T>
void vector2D<T>::operator=( const vector2D<T>& vec ){
    x = vec.x;  y = vec.y;
}

template <class T>
vector2D<T> vector2D<T>::operator+( const vector2D<T>& vec ) const {
    return vector2D<T>( x+vec.x, y+vec.y);
}

template <class T>
vector2D<T>& vector2D<T>::operator+=( const vector2D<T>& vec ) {
    x += vec.x;
    y += vec.y;
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::operator-( const vector2D<T>& vec ) const {
    return vector2D<T>(x-vec.x, y-vec.y);
}

template <class T>
vector2D<T>& vector2D<T>::operator-=( const vector2D<T>& vec ) {
    x -= vec.x;
    y -= vec.y;
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::operator*( const vector2D<T>& vec ) const {
    return vector2D<T>(x*vec.x, y*vec.y);
}

template <class T>
vector2D<T>& vector2D<T>::operator*=( const vector2D<T>& vec ) {
    x*=vec.x;
    y*=vec.y;
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::operator/( const vector2D<T>& vec ) const {
    return vector2D<T>( vec.x!=0 ? x/vec.x : x , vec.y!=0 ? y/vec.y : y);
}

template <class T>
vector2D<T>& vector2D<T>::operator/=( const vector2D<T>& vec ) {
    vec.x!=0 ? x/=vec.x : x;
    vec.y!=0 ? y/=vec.y : y;
    return *this;
}

//operator overloading for float:
/*
vector2D<T> & operator=( const float& val ){
    x = val;
    y = val;
    return *this;
}
 */

template <class T>
void vector2D<T>::operator=( const T f){
    x = f;  y = f;
}

template <class T>
vector2D<T> vector2D<T>::operator+( const T f ) const {
    return vector2D<T>( x+f, y+f);
}

template <class T>
vector2D<T>& vector2D<T>::operator+=( const T f ) {
    x += f; y += f;
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::operator-( const T f ) const {
    return vector2D<T>( x-f, y-f);
}

template <class T>
vector2D<T>& vector2D<T>::operator-=( const T f ) {
    x -= f; y -= f;
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::operator-() const {
    return vector2D<T>(-x, -y);
}

template <class T>
vector2D<T> vector2D<T>::operator*( const T f ) const {
    return vector2D<T>(x*f, y*f);
}

template <class T>
vector2D<T>& vector2D<T>::operator*=( const T f ) {
    x*=f; y*=f;
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::operator/( const T f ) const {
    //cout << "here" << endl;
    if(f == 0) return vector2D<T>(x, y);
    return vector2D<T>(x/f, y/f);
}

template <class T>
vector2D<T>& vector2D<T>::operator/=( const T f ) {
    if(f == 0) return *this;
    x/=f; y/=f;
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::getScaled( const float length ) const {
    float l = (float)sqrt(x*x + y*y);
    if( l > 0 )
        return vector2D<T>( (x/l)*length, (y/l)*length );
    else
        return vector2D<T>();
}

template <class T>
vector2D<T>& vector2D<T>::scale( const float length ) {
    float l = (float)sqrt(x*x + y*y);
    if (l > 0) {
        x = (x/l)*length;
        y = (y/l)*length;
    }
    return *this;
}

template <class T>
vector2D  getMult( const float _factor ) const {
    return vector2D<T>( x*_factor, y*_factor);
}
template <class T>
vector2D<T>& mult( const float _factor ) {
    x*=_factor;
    y*=_factor;
}

// Rotation
//
//

template <class T>
vector2D<T> vector2D<T>::getRotatedDeg( float angle ) const {
    float a = (float)(angle*DEG_TO_RAD);
    return vector2D<T>( x*cos(a) - y*sin(a),
                    x*sin(a) + y*cos(a) );
}

template <class T>
vector2D<T> vector2D<T>::getRotatedRad( float angle ) const {
    float a = angle;
    return vector2D<T>( x*cos(a) - y*sin(a),
                    x*sin(a) + y*cos(a) );
}

template <class T>
vector2D<T>& vector2D<T>::rotateDeg( float angle ) {
    float a = (float)(angle * DEG_TO_RAD);
    float xrot = x*cos(a) - y*sin(a);
    y = x*sin(a) + y*cos(a);
    x = xrot;
    return *this;
}

template <class T>
vector2D<T>& vector2D<T>::rotateRad( float angle ) {
    float a = angle;
    float xrot = x*cos(a) - y*sin(a);
    y = x*sin(a) + y*cos(a);
    x = xrot;
    return *this;
}

template <class T>
float vector2D<T>::distance( const vector2D<T>& pnt) const {
    float vx = x-pnt.x;
    float vy = y-pnt.y;
    return (float)sqrt(vx*vx + vy*vy);
}

template <class T>
float vector2D<T>::squareDistance( const vector2D<T>& pnt ) const {
    float vx = x-pnt.x;
    float vy = y-pnt.y;
    return vx*vx + vy*vy;
}

// Normalization:
template <class T>
vector2D<T> vector2D<T>::getNormalized() const {
    float length = (float)sqrt(x*x + y*y);
    if( length > 0 ) {
        return vector2D<T>( x/length, y/length );
    } else {
        return vector2D<T>();
    }
}

template <class T>
vector2D<T>& vector2D<T>::normalize() {
    float length = (float)sqrt(x*x + y*y);
    if( length > 0 ) {
        x /= length;
        y /= length;
    }
    return *this;
}

template <class T>
vector2D<T> vector2D<T>::getPerpendicularNormed(int orientation) const {
    float length = (float)sqrt( x*x + y*y );
    if( length > 0 )
        return vector2D<T>( -orientation*(y/length), orientation*x/length );
    else
        return vector2D<T>(0.0, 0.0); // something very small (will be used to compute a force)
}

template <class T>
vector2D<T>& vector2D<T>::perpendicular(int orientation) {
    float length = (float)sqrt( x*x + y*y );
    if( length > 0 ) {
        float _x = x;
        x = -(y/length)*orientation;
        y = _x/length*orientation;
    }
    return *this;
}

// Length (norm of vector2D<T>):
template <class T>
float vector2D<T>::length() const {
    return (float)sqrt( x*x + y*y );
}

template <class T>
float vector2D<T>::squareLength() const {
    return (float)(x*x + y*y);
}

// Angle between two vector2Ds:
template <class T>
float vector2D<T>::angleDeg( const vector2D<T>& vec ) const {
    return (float)(atan2( x*vec.y-y*vec.x, x*vec.x + y*vec.y )*RAD_TO_DEG);
}

template <class T>
float vector2D<T>::angleRad( const vector2D<T>& vec ) const {
    return atan2( x*vec.y-y*vec.x, x*vec.x + y*vec.y );
}

template <class T>
float vector2D<T>::angleDegHoriz( ) const {
    return (float)(atan2( y, x )*RAD_TO_DEG+180); // by adding 180, we get values between 0 and 360 (CCW, with 0 on the left quadrant)
}

//Dot Product:
template <class T>
float vector2D<T>::dot( const vector2D<T>& vec ) const {
    return x*vec.x + y*vec.y;
}

// ============================================  3D vectors ============================================

template <class T>
vector3D<T>::vector3D( T _x, T _y ,T _z ) {
    x = _x; y = _y; z = _z;
}

template <class T>
void vector3D<T>::set( T _x, T _y ,T _z ) {
    x = _x; y = _y; z = _z;
}

template <class T>
void vector3D<T>::set( const vector3D<T>& vec ) {
    x=vec.x; y=vec.y; z = vec.z;
}

template <class T>
bool vector3D<T>::operator==( const vector3D<T>& vec ) {
    return (x == vec.x) && (y == vec.y)&& (z == vec.z);
}

template <class T>
bool vector3D<T>::operator!=( const vector3D<T>& vec ) {
    return (x != vec.x) || (y != vec.y)|| (z != vec.z);
}

template <class T>
bool vector3D<T>::match( const vector3D<T>& vec, float tolerance ) {
    return (abs(x - vec.x) < tolerance)&& (abs(y - vec.y) < tolerance)&& (abs(z - vec.z) < tolerance);
}


/*
vector3D & operator=( const vector3D& vec ){ // returning a reference to the vector3D object for allowing operator chaining
    x = vec.x;
    y = vec.y;
    return *this;
}
 */

template <class T>
void vector3D<T>::operator=( const vector3D<T>& vec ){
    x = vec.x;  y = vec.y; z = vec.z;
}

template <class T>
vector3D<T> vector3D<T>::operator+( const vector3D<T>& vec ) const {
    return vector3D<T>( x+vec.x, y+vec.y, z+vec.z);
}

template <class T>
vector3D<T>& vector3D<T>::operator+=( const vector3D<T>& vec ) {
    x += vec.x;
    y += vec.y;
    z += vec.z;
    return *this;
}

template <class T>
vector3D<T> vector3D<T>::operator-( const vector3D<T>& vec ) const {
    return vector3D<T>(x-vec.x, y-vec.y, z-vec.z);
}

template <class T>
vector3D<T>& vector3D<T>::operator-=( const vector3D<T>& vec ) {
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;
    return *this;
}

template <class T>
vector3D<T> vector3D<T>::operator*( const vector3D<T>& vec ) const {
    return vector3D<T>(x*vec.x, y*vec.y, z*vec.z);
}

template <class T>
vector3D<T>& vector3D<T>::operator*=( const vector3D<T>& vec ) {
    x*=vec.x;
    y*=vec.y;
    z*=vec.z;
    return *this;
}

//operator overloading for float:
/*
vector3D<T> & operator=( const float& val ){
    x = val;
    y = val;
    return *this;
}
 */

template <class T>
void vector3D<T>::operator=( const T f){
    x = f;  y = f;  z = f;
}

template <class T>
vector3D<T> vector3D<T>::operator+( const T f ) const {
    return vector3D<T>( x+f, y+f, z+f);
}

template <class T>
vector3D<T>& vector3D<T>::operator+=( const T f ) {
    x += f; y += f; z+=f;
    return *this;
}

template <class T>
vector3D<T> vector3D<T>::operator-( const T f ) const {
    return vector3D<T>( x-f, y-f, z-f);
}

template <class T>
vector3D<T>& vector3D<T>::operator-=( const T f ) {
    x -= f; y -= f; z-=f;
    return *this;
}

template <class T>
vector3D<T> vector3D<T>::operator-() const {
    return vector3D<T>(-x, -y, -z);
}

template <class T>
vector3D<T> vector3D<T>::operator*( const T f ) const {
    return vector3D<T>(x*f, y*f, z*f);
}

template <class T>
vector3D<T>& vector3D<T>::operator*=( const T f ) {
    x*=f; y*=f; z*=f;
    return *this;
}

template <class T>
vector3D<T> vector3D<T>::operator/( const T f ) const {
    //cout << "here" << endl;
    if(f == 0) return vector3D<T>(x, y);
    return vector3D<T>(x/f, y/f, z/f);
}

template <class T>
vector3D<T>& vector3D<T>::operator/=( const T f ) {
    if(f == 0) return *this;
    x/=f; y/=f; z/=f;
    return *this;
}

template <class T>
vector3D<T> vector3D<T>::getScaled( const float length ) const {
    float l = (float)sqrt(x*x + y*y + z*z);
    if( l > 0 )
        return vector3D<T>( (x/l)*length, (y/l)*length , (z/l)*length);
    else
        return vector3D<T>();
}

template <class T>
vector3D<T>& vector3D<T>::scale( const float length ) {
    float l = (float)sqrt(x*x + y*y + z*z);
    if (l > 0) {
        x = (x/l)*length;
        y = (y/l)*length;
        z = (z/l)*length;
    }
    return *this;
}


template <class T>
float vector3D<T>::distance( const vector3D<T>& pnt) const {
    float vx = x-pnt.x;
    float vy = y-pnt.y;
    float vz = z-pnt.z;
    return (float)sqrt(vx*vx + vy*vy + vz*vz);
}

template <class T>
float vector3D<T>::squareDistance( const vector3D<T>& pnt ) const {
    float vx = x-pnt.x;
    float vy = y-pnt.y;
    float vz = z-pnt.z;
    return vx*vx + vy*vy+ vz*vz;
}

// Normalization:
template <class T>
vector3D<T> vector3D<T>::getNormalized() const {
    float length = (float)sqrt(x*x + y*y + z*z);
    if( length > 0 ) {
        return vector3D<T>( x/length, y/length , z/length);
    } else {
        return vector3D<T>();
    }
}

template <class T>
vector3D<T>& vector3D<T>::normalize() {
    float length = (float)sqrt(x*x + y*y + z*z);
    if( length > 0 ) {
        x /= length;
        y /= length;
        z /= length;
    }
    return *this;
}

// Length (norm of vector3D<T>):
template <class T>
float vector3D<T>::length() const {
    return (float)sqrt( x*x + y*y + z*z);
}

template <class T>
float vector3D<T>::squareLength() const {
    return (float)(x*x + y*y+ z*z);
}

// Angle between two vector3Ds:
template <class T>
float vector3D<T>::angleDeg( const vector3D<T>& vec ) const {
//atan2(norm(cross(a,b)), dot(a,b))
    return (float)(atan2( (this->cross(vec)).length(),this->dot(vec))*RAD_TO_DEG);
}

template <class T>
float vector3D<T>::angleRad( const vector3D<T>& vec ) const {
    return atan2((this->cross(vec)).length(),this->dot(vec));
}

//Dot Product:
template <class T>
float vector3D<T>::dot( const vector3D<T>& vec ) const {
    return x*vec.x + y*vec.y +  z*vec.z;
}

// Cross product:
template <class T>
vector3D<T> vector3D<T>::cross( const vector3D<T>& vec ) const {
    return vector3D<T>( y*vec.z - vec.y*z, -x*vec.z + vec.x * z, x.vec.y - vec.x* y);
}

template <class T>
vector3D<T>& vector3D<T>::cross( const vector3D<T>& vec ) {
    x = y*vec.z - vec.y*z;
    y = -x*vec.z + vec.x * z;
    z = x.vec.y - vec.x* y;
    return *this;
}
