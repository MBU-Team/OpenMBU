/**********************************************************************
 *<
	FILE: color.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _COLOR_H 

#define _COLOR_H

#include "point3.h"

#define FLto255(x) ((int)((x)*255.0f+.5))	

class Color;

struct RealPixel {
	unsigned char r,g,b;   // mantissas
	unsigned char e;	   // exponent
	DllExport operator Color();
	};

DllExport RealPixel MakeRealPixel(float r, float g, float b);
DllExport void ExpandRealPixel(const RealPixel &rp, float &r, float &g, float &b);


class Color {
public:
	float r,g,b;

	// Constructors
	Color()  {}
	Color(float R, float G, float B)  { r = R; g = G; b = B;  }
	Color(double R, double G, double B) { r = (float)R; g = (float)G; b = (float)B; }
	Color(int R, int G, int B) { r = (float)R; g = (float)G; b = (float)B; }
	Color(const Color& a) { r = a.r; g = a.g; b = a.b; } 
	DllExport Color(DWORD rgb);  // from Windows RGB value
	Color(Point3 p) { r = p.x; g = p.y; b = p.z; }
	Color(float af[3]) { r = af[0]; g = af[1]; b = af[2]; }
	Color(RealPixel rp) { ExpandRealPixel(rp,r,g,b); } 
	Color(const BMM_Color_24& c) { 
		r = float(c.r)/255.0f; g = float(c.g)/255.0f; b = float(c.b)/255.0f;  
		}
	Color(const BMM_Color_32& c) { 
		r = float(c.r)/255.0f; g = float(c.g)/255.0f; b = float(c.b)/255.0f;  
		}
	Color(const BMM_Color_48& c) { 
		r = float(c.r)/65535.0f; g = float(c.g)/65535.0f; b = float(c.b)/65535.0f;  
		}
	Color(const BMM_Color_64& c) { 
		r = float(c.r)/65535.0f; g = float(c.g)/65535.0f; b = float(c.b)/65535.0f;  
		}
	
		
	void Black() { r = g = b = 0.0f; }
	void White() { r = g = b = 1.0f; }

	DllExport void ClampMax();  // makes components >= 0.0
	DllExport void ClampMin();  // makes components <= 1.0
    DllExport void ClampMinMax();  // makes components in [0,1]

	// Access operators
	float& operator[](int i) { return (&r)[i]; }     
	const float& operator[](int i) const { return (&r)[i]; }  

	// Conversion function
	operator float*() { return(&r); }

	// Convert to Windows RGB
	operator DWORD() { return RGB(FLto255(r),FLto255(g), FLto255(b)); }

	// Convert to Point3
	operator Point3() { return Point3(r,g,b); }

	// Convert to RealPixel
	DllExport operator RealPixel() { return MakeRealPixel(r,g,b); }

	// Convert to Bitmap Manager types
	operator BMM_Color_24() { 
		BMM_Color_24 c; 
		c.r = int(r*255.0f); c.g = int(g*255.0f); c.b = int(b*255.0f);
		return c;
		}
	operator BMM_Color_32() { 
		BMM_Color_32 c; 
		c.r = int(r*255.0f); c.g = int(g*255.0f); c.b = int(b*255.0f);
		return c;
		}
	operator BMM_Color_48() { 
		BMM_Color_48 c; 
		c.r = int(r*65535.0f); c.g = int(g*65535.0f); c.b = int(b*65535.0f);
		return c;
		}
	operator BMM_Color_64() { 
		BMM_Color_64 c; 
		c.r = int(r*65535.0f); c.g = int(g*65535.0f); c.b = int(b*65535.0f);
		return c;
		}

	// Unary operators
	Color operator-() const { return(Color(-r,-g,-b)); } 
	Color operator+() const { return *this; } 

	// Assignment operators
	inline Color& operator-=(const Color&);
    inline Color& operator+=(const Color&);
	inline Color& operator*=(float); 
	inline Color& operator/=(float);
	inline Color& operator*=(const Color&);	// element-by-element multiplg.

	// Test for equality
	int operator==(const Color& p) const { return ((p.r==r)&&(p.g==g)&&(p.b==b)); }
	int operator!=(const Color& p) const { return ((p.r!=r)||(p.g!=g)||(p.b!=b)); }

	// Binary operators
	inline Color operator-(const Color&) const;
	inline Color operator+(const Color&) const;
	inline Color operator/(const Color&) const;
    inline Color operator*(const Color&) const;   
	inline Color operator^(const Color&) const;   // CROSS PRODUCT
	};

int DllExport MaxComponent(const Color&);  // index of the component with the maximum abs value
int DllExport MinComponent(const Color&);  // index of the component with the minimum abs value

float DllExport MaxVal(const Color&);  // value of the component with the maximum abs value
float DllExport MinVal(const Color&);  // value of the component with the minimum abs value

// Inlines:

inline float Length(const Color& v) {	
	return (float)sqrt(v.r*v.r+v.g*v.g+v.b*v.b);
	}

inline Color& Color::operator-=(const Color& a) {	
	r -= a.r;	g -= a.g;	b -= a.b;
	return *this;
	}

inline Color& Color::operator+=(const Color& a) {
	r += a.r;	g += a.g;	b += a.b;
	return *this;
	}

inline Color& Color::operator*=(float f) {
	r *= f;   g *= f;	b *= f;
	return *this;
	}

inline Color& Color::operator/=(float f) { 
	r /= f;	g /= f;	b /= f;	
	return *this; 
	}

inline Color& Color::operator*=(const Color& a) { 
	r *= a.r;	g *= a.g;	b *= a.b;	
	return *this; 
	}

inline Color Color::operator-(const Color& c) const {
	return(Color(r-c.r,g-c.g,b-c.b));
	}

inline Color Color::operator+(const Color& c) const {
	return(Color(r+c.r,g+c.g,b+c.b));
	}

inline Color Color::operator/(const Color& c) const {
	return Color(r/c.r,g/c.g,b/c.b);
	}

inline Color Color::operator*(const Color& c) const {  
	return Color(r*c.r, g*c.g, b*c.b);	
	}

inline Color operator*(float f, const Color& a) {
	return(Color(a.r*f, a.g*f, a.b*f));
	}

inline Color operator*(const Color& a, float f) {
	return(Color(a.r*f, a.g*f, a.b*f));
	}

inline Color operator/(const Color& a, float f) {
	return(Color(a.r/f, a.g/f, a.b/f));
	}

inline Color operator+(const Color& a, float f) {
	return(Color(a.r+f, a.g+f, a.b+f));
	}

inline Color operator+(float f, const Color& a) {
	return(Color(a.r+f, a.g+f, a.b+f));
	}

inline Color operator-(const Color& a, float f) {
	return(Color(a.r-f, a.g-f, a.b-f));
	}

inline Color operator-(float f, const Color& a) {
	return(Color(f-a.r, f-a.g, f-a.b));
	}


#endif

