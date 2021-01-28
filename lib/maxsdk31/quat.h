/**********************************************************************
 *<
	FILE: quat.h

	DESCRIPTION: Class definitions for Quat

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef _QUAT_H 

#define _QUAT_H 

#include "matrix3.h"

class ostream;
class Quat;

class AngAxis {
public:
	Point3 axis;
	float angle;        // This angle is left-handed!

	AngAxis() {}
	AngAxis(float x, float y, float z, float ang)
		{ axis.x = x; axis.y = y; axis.z = z; angle = ang; }
	AngAxis(const Point3& axis,float angle) {this->axis=axis;this->angle=angle;}	
	DllExport AngAxis(const Quat &q);
	AngAxis(const Matrix3& m);

	AngAxis& Set(float x, float y, float z, float ang)
		{axis.x = x; axis.y = y; axis.z = z; angle = ang; return *this; }
	AngAxis& Set(const Point3& ax, float ang)
		{axis = ax; angle = ang; return *this; }
	DllExport AngAxis& Set(const Quat& q);
	DllExport AngAxis& Set(const Matrix3& m);
    
	DllExport int GetNumRevs();
	DllExport void SetNumRevs(int num);
	};

class Quat {
public:
	float x,y,z,w;

	// Constructors
	Quat(): x(0.0f),y(0.0f),z(0.0f),w(1.0f) {}
	Quat(float X, float Y, float Z, float W)  { x = X; y = Y; z = Z; w = W; }
	Quat(double X, double Y, double Z, double W)  { 
		x = (float)X; y = (float)Y; z = (float)Z; w = (float)W; 
		}
	Quat(const Quat& a) { x = a.x; y = a.y; z = a.z; w = a.w; } 
	Quat(float af[4]) { x = af[0]; y = af[1]; z = af[2]; w = af[3]; }
	DllExport Quat(const Matrix3& mat);
	DllExport Quat(const AngAxis& aa);
	DllExport Quat(const Point3& V, float W);

	// Access operators
	float& operator[](int i) { return (&x)[i]; }     
	const float& operator[](int i) const { return (&x)[i]; }
    
	float Scalar() { return w; }
	Point3 Vector() { return Point3(x, y, z); }

	// Conversion function
	operator float*() { return(&x); }

	// Unary operators
	Quat operator-() const { return(Quat(-x,-y,-z,-w)); } 
	Quat operator+() const { return *this; }
    
    // Math functions
	DllExport Quat Inverse() const;
	DllExport Quat Conjugate() const;
	DllExport Quat LogN() const;
	DllExport Quat Exp() const;

	// Assignment operators
	DllExport Quat& operator-=(const Quat&);
	DllExport Quat& operator+=(const Quat&);
	DllExport Quat& operator*=(const Quat&);
	DllExport Quat& operator*=(float);
	DllExport Quat& operator/=(float);

	Quat& Set(float X, float Y, float Z, float W)
		{ x = X; y = Y; z = Z; w = W; return *this; }
	Quat& Set(double X, double Y, double Z, double W)
		{ x = (float)X; y = (float)Y; z = (float)Z; w = (float)W;
		return *this; }
	DllExport Quat& Set(const Matrix3& mat);
	DllExport Quat& Set(const AngAxis& aa);
	Quat& Set(const Point3& V, float W)
		{ x = V.x; y = V.y; z = V.z; w = W; return *this; } 
	DllExport Quat& SetEuler(float X, float Y, float Z);
	DllExport Quat& Invert();                 // in place

	DllExport Quat& MakeClosest(const Quat& qto);

	// Comparison
	DllExport int operator==(const Quat& a) const;
	DllExport int Equals(const Quat& a, float epsilon = 1E-6f) const;

	void Identity() { x = y = z = (float)0.0; w = (float) 1.0; }
	DllExport int IsIdentity() const;
	DllExport void Normalize();  // normalize
	DllExport void MakeMatrix(Matrix3 &mat) const;
	DllExport void GetEuler(float *X, float *Y, float *Z) const;

	// Binary operators
	DllExport Quat operator-(const Quat&) const;  //RB: Changed these to		// difference of two quaternions
	DllExport Quat operator+(const Quat&) const;  // duplicate * and /			// sum of two quaternions
	DllExport Quat operator*(const Quat&) const;  // product of two quaternions
	DllExport Quat operator/(const Quat&) const;  // ratio of two quaternions
	DllExport float operator%(const Quat&) const;   // dot product
	DllExport Quat Plus(const Quat&) const;       // what + should have done
	DllExport Quat Minus(const Quat&) const;      // what - should have done
    };

Quat DllExport operator*(float, const Quat&);	// multiply by scalar
Quat DllExport operator*(const Quat&, float);	// multiply by scalar
Quat DllExport operator/(const Quat&, float);	// divide by scalar
Quat DllExport Inverse(const Quat& q);  // Inverse of quaternion (1/q)
Quat DllExport Conjugate(const Quat& q); 
Quat DllExport LogN(const Quat& q);
Quat DllExport Exp(const Quat& q);
Quat DllExport Slerp(const Quat& p, const Quat& q, float t);
Quat DllExport LnDif(const Quat& p, const Quat& q);
Quat DllExport QCompA(const Quat& qprev,const Quat& q, const Quat& qnext);
Quat DllExport Squad(const Quat& p, const Quat& a, const Quat &b, const Quat& q, float t); 
Quat DllExport qorthog(const Quat& p, const Point3& axis);
Quat DllExport squadrev(
		float angle,	// angle of rotation 
		const Point3& axis,	// the axis of rotation 
		const Quat& p,		// start quaternion 
		const Quat& a, 		// start tangent quaternion 
		const Quat& b, 		// end tangent quaternion 
		const Quat& q,		// end quaternion 
		float t 		// parameter, in range [0.0,1.0] 
		);

void DllExport RotateMatrix(Matrix3& mat, const Quat& q);	  
void DllExport PreRotateMatrix(Matrix3& mat, const Quat& q);
Quat DllExport QFromAngAxis(float ang, const Point3& axis);
void DllExport AngAxisFromQ(const Quat& q, float *ang, Point3& axis);
float DllExport QangAxis(const Quat& p, const Quat& q, Point3& axis);
void DllExport DecomposeMatrix(const Matrix3& mat, Point3& p, Quat& q, Point3& s);
Quat DllExport TransformQuat(const Matrix3 &m, const Quat&q );
inline Quat IdentQuat() { return(Quat(0.0,0.0,0.0,1.0)); }

// Assumes Euler angles are of the form:
// RotateX(ang[0])
// RotateY(ang[1])
// RotateZ(ang[2])
//
void DllExport QuatToEuler(Quat &q, float *ang);
void DllExport EulerToQuat(float *ang, Quat &q);

ostream DllExport &operator<<(ostream&, const Quat&); 

#endif _QUAT_H 
