/**********************************************************************
 *<
	FILE: ipoint2.h

	DESCRIPTION: Class definintion for IPoint2: Integer 2D point.

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __IPOINT2__ 

#define __IPOINT2__

class ostream;

class IPoint2 {
public:
	int x,y;

	// Constructors
	IPoint2(){}
	IPoint2(int X, int Y)  { x = X; y = Y;  }
	IPoint2(const IPoint2& a) { x = a.x; y = a.y; } 
	IPoint2(int af[2]) { x = af[0]; y = af[1]; }

	// Access operators
	int& operator[](int i) { return (&x)[i]; }     
	const int& operator[](int i) const { return (&x)[i]; }  

 	// Conversion function
	operator int*() { return(&x); }
	 
	// Unary operators
	IPoint2 operator-() const { return(IPoint2(-x,-y)); } 
	IPoint2 operator+() const { return *this; } 

	// Assignment operators
	IPoint2& operator-=(const IPoint2&);
	IPoint2& operator+=(const IPoint2&);
	DllExport IPoint2& operator*=(int);
	DllExport IPoint2& operator/=(int);

	// Binary operators
	DllExport IPoint2 operator-(const IPoint2&) const;
	DllExport IPoint2 operator+(const IPoint2&) const;
	DllExport int DotProd(const IPoint2&) const;    // DOT PRODUCT
	DllExport int operator*(const IPoint2&) const;    // DOT PRODUCT

	// Relational operators
	int operator==(const IPoint2& p) const { return (x == p.x && y == p.y); }
	int operator!=(const IPoint2& p) const { return (x != p.x || y != p.y); }
	};

int DllExport Length(const IPoint2&); 
IPoint2 DllExport Normalize(const IPoint2&); // Return a unit vector.
IPoint2 DllExport operator*(int, const IPoint2&);	// multiply by scalar
IPoint2 DllExport operator*(const IPoint2&, int);	// multiply by scalar
IPoint2 DllExport operator/(const IPoint2&, int);	// divide by scalar

ostream DllExport &operator<<(ostream&, const IPoint2&); 

// Inlines:

inline int MaxComponent(const IPoint2& p) { return(p.x>p.y?0:1); }
inline int MinComponent(const IPoint2& p) { return(p.x<p.y?0:1); }

inline int Length(const IPoint2& v) {	
	return (int)sqrt((double)(v.x*v.x+v.y*v.y));
	}

inline IPoint2& IPoint2::operator-=(const IPoint2& a) {	
	x -= a.x;	y -= a.y;  
	return *this;
	}

inline IPoint2& IPoint2::operator+=(const IPoint2& a) {
	x += a.x;	y += a.y;  
	return *this;
	}

inline IPoint2& IPoint2::operator*=(int f) {
	x *= f;   y *= f;	
	return *this;
	}

inline IPoint2& IPoint2::operator/=(int f) { 
	x /= f;	y /= f;		
	return *this; 
	}

inline IPoint2 IPoint2::operator-(const IPoint2& b) const{
	return(IPoint2(x-b.x,y-b.y));
	}

inline IPoint2 IPoint2::operator+(const IPoint2& b) const {
	return(IPoint2(x+b.x,y+b.y));
	}

inline int IPoint2::DotProd(const IPoint2& b) const{
	return(x*b.x+y*b.y);
	}

inline int IPoint2::operator*(const IPoint2& b)const {
	return(x*b.x+y*b.y);
	}

inline IPoint2 operator*(int f, const IPoint2& a) {
	return(IPoint2(a.x*f, a.y*f));
	}

inline IPoint2 operator*(const IPoint2& a, int f) {
	return(IPoint2(a.x*f, a.y*f));
	}

inline IPoint2 operator/(const IPoint2& a, int f) {
	return(IPoint2(a.x/f, a.y/f));
	}

#endif

