/**********************************************************************
 *<
	FILE: ipoint3.h

	DESCRIPTION: Class definitions for IPoint3

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __IPOINT3__ 

#define __IPOINT3__

class ostream;

class IPoint3 {
public:
	int x,y,z;

	// Constructors
	IPoint3(){}
	IPoint3(int X, int Y, int Z)  { x = X; y = Y; z = Z;  }
	IPoint3(const IPoint3& a) { x = a.x; y = a.y; z = a.z; } 
	IPoint3(int ai[3]) { x = ai[0]; y = ai[1]; z = ai[2]; }

	// Access operators
	int& operator[](int i) { return (&x)[i]; }     
	const int& operator[](int i) const { return (&x)[i]; }  
 	
 	// Conversion function
	operator int*() { return(&x); }

	// Unary operators
	IPoint3 operator-() const { return(IPoint3(-x,-y,-z)); } 
	IPoint3 operator+() const { return *this; } 

	// Assignment operators
	DllExport IPoint3& operator-=(const IPoint3&);
	DllExport IPoint3& operator+=(const IPoint3&);

	// Binary operators
	DllExport IPoint3 operator-(const IPoint3&) const;
	DllExport IPoint3 operator+(const IPoint3&) const;
	DllExport int operator*(const IPoint3&) const;    // DOT PRODUCT
	DllExport int DotProd(const IPoint3&) const;    // DOT PRODUCT
	DllExport IPoint3 operator^(const IPoint3&) const;   // CROSS PRODUCT
	DllExport IPoint3 CrossProd(const IPoint3&) const;   // CROSS PRODUCT

	// Relational operators
	int operator==(const IPoint3& p) const { return (x == p.x && y == p.y && z == p.z); }
	int operator!=(const IPoint3& p) const { return ( (x != p.x) || (y != p.y) || (z != p.z) ); }
	};

    // friends, so you can write Length(A) instead of A.Length(), etc.
int DllExport MaxComponent(const IPoint3&);  // the component with the maximum abs value
int DllExport MinComponent(const IPoint3&);  // the component with the minimum abs value
	 
ostream DllExport &operator<<(ostream&, const IPoint3&); 

// Inlines:

inline float Length(const IPoint3& v) {	
	return (float)sqrt((double)(v.x*v.x+v.y*v.y+v.z*v.z));
	}

inline IPoint3& IPoint3::operator-=(const IPoint3& a) {	
	x -= a.x;	y -= a.y;	z -= a.z;
	return *this;
	}

inline IPoint3& IPoint3::operator+=(const IPoint3& a) {
	x += a.x;	y += a.y;	z += a.z;
	return *this;
	}

inline IPoint3 IPoint3::operator-(const IPoint3& b) const {
	return(IPoint3(x-b.x,y-b.y,z-b.z));
	}

inline IPoint3 IPoint3::operator+(const IPoint3& b) const {
	return(IPoint3(x+b.x,y+b.y,z+b.z));
	}

inline int IPoint3::operator*(const IPoint3& b) const {  
	return(x*b.x+y*b.y+z*b.z);	
	}

inline int IPoint3::DotProd(const IPoint3& b)  const { 
	return(x*b.x+y*b.y+z*b.z);	
	}

#endif

