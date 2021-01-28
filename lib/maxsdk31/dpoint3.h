/**********************************************************************
 *<
	FILE: dpoint3.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __DPOINT3__ 

#define __DPOINT3__

#include "point3.h"

class ostream;

class DPoint3 {
public:
	double x,y,z;

	// Constructors
	DPoint3(){}
	DPoint3(double X, double Y, double Z)  { x = X; y = Y; z = Z;  }
	DPoint3(const DPoint3& a) { x = a.x; y = a.y; z = a.z; } 
	DPoint3(const Point3& a) { x = a.x; y = a.y; z = a.z; } 
	DPoint3(double af[3]) { x = af[0]; y = af[1]; z = af[2]; }

	// Access operators
	double& operator[](int i) { return (&x)[i]; }     
	const double& operator[](int i) const { return (&x)[i]; }  

 	// Conversion function
	operator double*() { return(&x); }

 	// Unary operators
	DPoint3 operator-() const { return(DPoint3(-x,-y,-z)); } 
	DPoint3 operator+() const { return *this; } 

	// Assignment operators
	DllExport DPoint3& operator=(const Point3& a) {	x = a.x; y = a.y; z = a.z;	return *this; }
	DllExport DPoint3& operator-=(const DPoint3&);
	DllExport DPoint3& operator+=(const DPoint3&);
	DllExport DPoint3& operator*=(double);
	DllExport DPoint3& operator/=(double);

	// Binary operators
	DllExport DPoint3 operator-(const DPoint3&) const;
	DllExport DPoint3 operator+(const DPoint3&) const;
	DllExport double operator*(const DPoint3&) const;		// DOT PRODUCT
	DllExport DPoint3 operator^(const DPoint3&) const;	// CROSS PRODUCT

	};

double DllExport Length(const DPoint3&); 
int DllExport MaxComponent(const DPoint3&);  // the component with the maximum abs value
int DllExport MinComponent(const DPoint3&);  // the component with the minimum abs value
DPoint3 DllExport Normalize(const DPoint3&); // Return a unit vector.

DPoint3 DllExport operator*(double, const DPoint3&);	// multiply by scalar
DPoint3 DllExport operator*(const DPoint3&, double);	// multiply by scalar
DPoint3 DllExport operator/(const DPoint3&, double);	// divide by scalar

ostream DllExport &operator<<(ostream&, const DPoint3&); 
	 
// Inlines:

inline double Length(const DPoint3& v) {	
	return (double)sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
	}

inline DPoint3& DPoint3::operator-=(const DPoint3& a) {	
	x -= a.x;	y -= a.y;	z -= a.z;
	return *this;
	}

inline DPoint3& DPoint3::operator+=(const DPoint3& a) {
	x += a.x;	y += a.y;	z += a.z;
	return *this;
	}

inline DPoint3& DPoint3::operator*=(double f) {
	x *= f;   y *= f;	z *= f;
	return *this;
	}

inline DPoint3& DPoint3::operator/=(double f) { 
	x /= f;	y /= f;	z /= f;	
	return *this; 
	}

inline DPoint3 DPoint3::operator-(const DPoint3& b) const {
	return(DPoint3(x-b.x,y-b.y,z-b.z));
	}

inline DPoint3 DPoint3::operator+(const DPoint3& b) const {
	return(DPoint3(x+b.x,y+b.y,z+b.z));
	}

inline double DPoint3::operator*(const DPoint3& b) const {  
	return(x*b.x+y*b.y+z*b.z);	
	}

inline DPoint3 operator*(double f, const DPoint3& a) {
	return(DPoint3(a.x*f, a.y*f, a.z*f));
	}

inline DPoint3 operator*(const DPoint3& a, double f) {
	return(DPoint3(a.x*f, a.y*f, a.z*f));
	}

inline DPoint3 operator/(const DPoint3& a, double f) {
	return(DPoint3(a.x/f, a.y/f, a.z/f));
	}

DPoint3 DllExport CrossProd(const DPoint3& a, const DPoint3& b);	// CROSS PRODUCT
	
double DllExport DotProd(const DPoint3& a, const DPoint3& b) ;		// DOT PRODUCT

#endif

