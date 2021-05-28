/**********************************************************************
 *<
	FILE: point3.h

	DESCRIPTION: Class definitions for Point3

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _POINT3_H 

#define _POINT3_H


#include "gfloat.h"

class DllExport Point3 {
public:
	float x,y,z;

	// Constructors
	Point3(){}
	Point3(float X, float Y, float Z)  { x = X; y = Y; z = Z;  }
	Point3(double X, double Y, double Z) { x = (float)X; y = (float)Y; z = (float)Z; }
	Point3(int X, int Y, int Z) { x = (float)X; y = (float)Y; z = (float)Z; }
	Point3(const Point3& a) { x = a.x; y = a.y; z = a.z; } 
	Point3(float af[3]) { x = af[0]; y = af[1]; z = af[2]; }

    // Data members
    static const Point3 Origin;
    static const Point3 XAxis;
    static const Point3 YAxis;
    static const Point3 ZAxis;

	// Access operators
	float& operator[](int i) { return (&x)[i]; }     
	const float& operator[](int i) const { return (&x)[i]; }  

	// Conversion function
	operator float*() { return(&x); }

	// Unary operators
	Point3 operator-() const { return(Point3(-x,-y,-z)); } 
	Point3 operator+() const { return *this; }
    
    // Property functions
    float Length() const;
    float FLength() const;
    float LengthSquared() const;
    int MaxComponent() const;
    int MinComponent() const;
    Point3 Normalize() const;     // more accurate than FNormalize()
    Point3 FNormalize() const;    // faster than Normalize()

	// Assignment operators
	inline Point3& operator-=(const Point3&);
	inline Point3& operator+=(const Point3&);
	inline Point3& operator*=(float); 
	inline Point3& operator/=(float);
	inline Point3& operator*=(const Point3&);	// element-by-element multiply.

    inline Point3& Set(float X, float Y, float Z);

	// Test for equality
	int operator==(const Point3& p) const { return ((p.x==x)&&(p.y==y)&&(p.z==z)); }
	int operator!=(const Point3& p) const { return ((p.x!=x)||(p.y!=y)||(p.z!=z)); }
    int Equals(const Point3& p, float epsilon = 1E-6f) const;

    // In-place normalize
    Point3& Unify();
    float LengthUnify();              // returns old Length

	// Binary operators
	inline  Point3 operator-(const Point3&) const;
	inline  Point3 operator+(const Point3&) const;
	inline  Point3 operator/(const Point3&) const;
	inline  Point3 operator*(const Point3&) const;   

	Point3 operator^(const Point3&) const;	// CROSS PRODUCT
	inline float operator%(const Point3&) const;	    // DOT PRODUCT
	};


inline float DllExport Length(const Point3&); 
inline float DllExport FLength(const Point3&); 
inline float DllExport LengthSquared(const Point3&); 
int DllExport MaxComponent(const Point3&);  // the component with the maximum abs value
int DllExport MinComponent(const Point3&);  // the component with the minimum abs value
Point3 DllExport Normalize(const Point3&);  // Accurate normalize
Point3 DllExport FNormalize(const Point3&); // Fast normalize 


// RB: moved this here from object.h
class Ray {
	public:
		Point3 p;   // point of origin
		Point3 dir; // unit vector
	};

 
// Inlines:

inline float Point3::Length() const {	
	return (float)sqrt(x*x+y*y+z*z);
	}

inline float Point3::FLength() const {	
	return Sqrt(x*x+y*y+z*z);
	}

inline float Point3::LengthSquared() const {	
	return (x*x+y*y+z*z);
	}

inline float Length(const Point3& v) {	
	return v.Length();
	}

inline float FLength(const Point3& v) {	
	return v.FLength();
	}

inline float LengthSquared(const Point3& v) {	
	return v.LengthSquared();
	}

inline Point3& Point3::operator-=(const Point3& a) {	
	x -= a.x;	y -= a.y;	z -= a.z;
	return *this;
	}

inline Point3& Point3::operator+=(const Point3& a) {
	x += a.x;	y += a.y;	z += a.z;
	return *this;
	}

inline Point3& Point3::operator*=(float f) {
	x *= f;   y *= f;	z *= f;
	return *this;
	}

inline Point3& Point3::operator/=(float f) { 
	x /= f;	y /= f;	z /= f;	
	return *this; 
	}

inline Point3& Point3::operator*=(const Point3& a) { 
	x *= a.x;	y *= a.y;	z *= a.z;	
	return *this; 
	}

inline Point3& Point3::Set(float X, float Y, float Z) {
    x = X;
    y = Y;
    z = Z;
    return *this;
    }

inline Point3 Point3::operator-(const Point3& b) const {
	return(Point3(x-b.x,y-b.y,z-b.z));
	}

inline Point3 Point3::operator+(const Point3& b) const {
	return(Point3(x+b.x,y+b.y,z+b.z));
	}

inline Point3 Point3::operator/(const Point3& b) const {
	return Point3(x/b.x,y/b.y,z/b.z);
	}

inline Point3 Point3::operator*(const Point3& b) const {  
	return Point3(x*b.x, y*b.y, z*b.z);	
	}

inline float Point3::operator%(const Point3& b) const {
    return (x*b.x + y*b.y + z*b.z);
    }

inline Point3 operator*(float f, const Point3& a) {
	return(Point3(a.x*f, a.y*f, a.z*f));
	}

inline Point3 operator*(const Point3& a, float f) {
	return(Point3(a.x*f, a.y*f, a.z*f));
	}

inline Point3 operator/(const Point3& a, float f) {
	return(Point3(a.x/f, a.y/f, a.z/f));
	}

inline Point3 operator+(const Point3& a, float f) {
	return(Point3(a.x+f, a.y+f, a.z+f));
	}

inline float DotProd(const Point3& a, const Point3& b) { 
	return(a.x*b.x+a.y*b.y+a.z*b.z);	
	}

Point3 DllExport CrossProd(const Point3& a, const Point3& b);	// CROSS PRODUCT


#endif

