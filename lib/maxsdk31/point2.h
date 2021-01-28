/**********************************************************************
 *<
	FILE: point2.h

	DESCRIPTION: Class definition for Point2

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __POINT2__ 

#define __POINT2__

class ostream;

class DllExport Point2 {
public:
	float x,y;

	// Constructors
	Point2(){}
	Point2(float X, float Y)  { x = X; y = Y;  }
	Point2(double X, double Y)  { x = (float)X; y = (float)Y;  }
	Point2(const Point2& a) { x = a.x; y = a.y; } 
	Point2(float af[2]) { x = af[0]; y = af[1]; }

    // Data members
    static const Point2 Origin;
    static const Point2 XAxis;
    static const Point2 YAxis;

	// Access operators
	float& operator[](int i) { return (&x)[i]; }     
	const float& operator[](int i) const { return (&x)[i]; }  
	
	// Conversion function
	operator float*() { return(&x); }

	// Unary operators
	Point2 operator-() const { return(Point2(-x,-y)); } 
	Point2 operator+() const { return *this; }
    
    // Property functions
    float Length() const;
    int MaxComponent() const;
    int MinComponent() const;
    Point2 Normalize() const; // more accurate than *this/Length();

	// Assignment operators
	Point2& operator-=(const Point2&);
	Point2& operator+=(const Point2&);
	Point2& operator*=(float);
	Point2& operator/=(float);

    Point2& Set(float X, float Y);

	// Binary operators
	Point2 operator-(const Point2&) const;
	Point2 operator+(const Point2&) const;
	float DotProd(const Point2&) const;		// DOT PRODUCT
	float operator*(const Point2&) const;	// DOT PRODUCT

	// Relational operators
	int operator==(const Point2& p) const { return (x == p.x && y == p.y); }
	int operator!=(const Point2& p) const { return ( (x != p.x) || (y != p.y) ); }
    int Equals(const Point2& p, float epsilon = 1E-6f);
    
    // In-place normalize
    Point2& Unify();
    float LengthUnify();          // returns old Length
    };


inline float DllExport Length(const Point2&); 
int DllExport MaxComponent(const Point2&);  // the component with the maximum abs value
int DllExport MinComponent(const Point2&);  // the component with the minimum abs value
Point2 DllExport Normalize(const Point2&);  // more accurate than v/Length(v)
	 
Point2 DllExport operator*(float, const Point2&);	// multiply by scalar
Point2 DllExport operator*(const Point2&, float);	// multiply by scalar
Point2 DllExport operator/(const Point2&, float);	// divide by scalar

ostream DllExport &operator<<(ostream&, const Point2&);
	 
// Inlines:

inline float Length(const Point2& v) {	
	return (float)sqrt(v.x*v.x+v.y*v.y);
	}

inline float Point2::Length() const {	
	return (float)sqrt(x*x+y*y);
	}

inline Point2& Point2::operator-=(const Point2& a) {	
	x -= a.x;	y -= a.y;  
	return *this;
	}

inline Point2& Point2::operator+=(const Point2& a) {
	x += a.x;	y += a.y;  
	return *this;
	}

inline Point2& Point2::operator*=(float f) {
	x *= f;   y *= f;	
	return *this;
	}

inline Point2& Point2::operator/=(float f) { 
	x /= f;	y /= f;		
	return *this; 
	}

inline Point2& Point2::Set(float X, float Y) {
    x = X; y = Y;
    return *this;
    }

inline Point2 Point2::operator-(const Point2& b) const{
	return(Point2(x-b.x,y-b.y));
	}

inline Point2 Point2::operator+(const Point2& b) const {
	return(Point2(x+b.x,y+b.y));
	}

inline float Point2::DotProd(const Point2& b) const{
	return(x*b.x+y*b.y);
	}

inline float Point2::operator*(const Point2& b)const {
	return(x*b.x+y*b.y);
	}

inline Point2 operator*(float f, const Point2& a) {
	return(Point2(a.x*f, a.y*f));
	}

inline Point2 operator*(const Point2& a, float f) {
	return(Point2(a.x*f, a.y*f));
	}

inline Point2 operator/(const Point2& a, float f) {
	return(Point2(a.x/f, a.y/f));
	}

#endif

