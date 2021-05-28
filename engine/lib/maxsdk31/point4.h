/**********************************************************************
 *<
	FILE: point4.h

	DESCRIPTION: Class definitions for Point4

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _POINT4_H 

#define _POINT4_H

#include "point3.h"

class DllExport Point4 {
public:
	float x,y,z,w;

	// Constructors
	Point4(){}
	Point4(float X, float Y, float Z, float W)  { x = X; y = Y; z = Z; w = W; }
	Point4(double X, double Y, double Z, double W) { x = (float)X; y = (float)Y; z = (float)Z; w = (float)W; }
	Point4(int X, int Y, int Z, int W) { x = (float)X; y = (float)Y; z = (float)Z; w = (float)W; }
	Point4(const Point3& a, float W=0.0f) { x = a.x; y = a.y; z = a.z; w = W; } 
	Point4(const Point4& a) { x = a.x; y = a.y; z = a.z; w = a.w; } 
	Point4(float af[4]) { x = af[0]; y = af[1]; z = af[2]; w = af[3]; }

    // Data members
    static const Point4 Origin;
    static const Point4 XAxis;
    static const Point4 YAxis;
    static const Point4 ZAxis;
    static const Point4 WAxis;

	// Access operators
	float& operator[](int i) { return (&x)[i]; }     
	const float& operator[](int i) const { return (&x)[i]; }  

	// Conversion function
	operator float*() { return(&x); }

	// Unary operators
	Point4 operator-() const { return(Point4(-x,-y,-z, -w)); } 
	Point4 operator+() const { return *this; } 

	// Assignment operators
	inline Point4& operator-=(const Point4&);
	inline Point4& operator+=(const Point4&);
	inline Point4& operator*=(float); 
	inline Point4& operator/=(float);
	inline Point4& operator*=(const Point4&);	// element-by-element multiply.
    inline Point4& Set(float X, float Y, float Z, float W);

	// Test for equality
	int operator==(const Point4& p) const { return ((p.x==x)&&(p.y==y)&&(p.z==z)&&(p.w==w)); }
	int operator!=(const Point4& p) const { return ((p.x!=x)||(p.y!=y)||(p.z!=z)||(p.w!=w)); }
    int Equals(const Point4& p, float epsilon = 1E-6f);

	// Binary operators
	inline  Point4 operator-(const Point4&) const;
	inline  Point4 operator+(const Point4&) const;
	inline  Point4 operator/(const Point4&) const;
	inline  Point4 operator*(const Point4&) const;   

	};

// Inlines:

inline Point4& Point4::operator-=(const Point4& a) {	
	x -= a.x;	y -= a.y;	z -= a.z; w -= a.w;
	return *this;
	}

inline Point4& Point4::operator+=(const Point4& a) {
	x += a.x;	y += a.y;	z += a.z; w += a.w;
	return *this;
	}

inline Point4& Point4::operator*=(float f) {
	x *= f;   y *= f;	z *= f; w *= f;
	return *this;
	}

inline Point4& Point4::operator/=(float f) { 
	if (f==0.0f) f = .000001f;
	x /= f;	y /= f;	z /= f;	 w /= f;
	return *this; 
	}

inline Point4& Point4::operator*=(const Point4& a) { 
	x *= a.x;	y *= a.y;	z *= a.z;	w *= a.w;
	return *this; 
	}

inline Point4& Point4::Set(float X, float Y, float Z, float W) {
    x = X;
    y = Y;
    z = Z;
    w = W;
    return *this;
    }

inline Point4 Point4::operator-(const Point4& b) const {
	return(Point4(x-b.x,y-b.y,z-b.z, w-b.w));
	}

inline Point4 Point4::operator+(const Point4& b) const {
	return(Point4(x+b.x,y+b.y,z+b.z, w+b.w));
	}

inline Point4 Point4::operator/(const Point4& b) const {
	return Point4(x/b.x,y/b.y,z/b.z,w/b.w);
	}

inline Point4 Point4::operator*(const Point4& b) const {  
	return Point4(x*b.x, y*b.y, z*b.z,w*b.w);	
	}

inline Point4 operator*(float f, const Point4& a) {
	return(Point4(a.x*f, a.y*f, a.z*f, a.w*f));
	}

inline Point4 operator*(const Point4& a, float f) {
	return(Point4(a.x*f, a.y*f, a.z*f, a.w*f));
	}

inline Point4 operator/(const Point4& a, float f) {
	return(Point4(a.x/f, a.y/f, a.z/f, a.w/f));
	}

inline Point4 operator+(const Point4& a, float f) {
	return(Point4(a.x+f, a.y+f, a.z+f, a.w+f));
	}

inline int Point4::Equals(const Point4& p, float epsilon) {
    return (fabs(p.x - x) <= epsilon && fabs(p.y - y) <= epsilon
            && fabs(p.z - z) <= epsilon && fabs(p.w - w) <= epsilon);
    }

#endif

