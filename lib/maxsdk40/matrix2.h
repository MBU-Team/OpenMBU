/**********************************************************************
 *<
	FILE: matrix2.h

	DESCRIPTION: Class definitions for Matrix2

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __MATRIX2__ 

#define __MATRIX2_

#include "ioapi.h"
#include "point2.h"
#include "point3.h"

class Matrix2 {
	Point2& operator[](int i) { return((Point2&)(*m[i]));  }
	Point2& operator[](int i) const { return((Point2&)(*m[i])); }
public:
	float m[3][2];

	// Constructors
	Matrix2(){}	 // NO INITIALIZATION done in this constructor!! (can use Zero or IdentityMatrix)
	Matrix2(BOOL init) { IdentityMatrix(); } // An option to initialize

	DllExport Matrix2(float (*fp)[2]); 
	
    // Data member
    static const Matrix2 Identity;
    
    // Assignment operators
	DllExport Matrix2& operator-=( const Matrix2& M);
	DllExport Matrix2& operator+=( const Matrix2& M); 
	DllExport Matrix2& operator*=( const Matrix2& M);  	// Matrix multiplication

 	// Conversion function
	operator float*() { return(&m[0][0]); }

	// Initialize matrix
	DllExport void IdentityMatrix(); 	// Set to the Identity Matrix
	DllExport void Zero();		// Set all elements to 0

	Point2 GetRow(int i) const { return (*this)[i]; }	
	DllExport void SetRow(int i, Point2 p) { (*this)[i] = p; }

	DllExport Point3 GetColumn(int i);
	DllExport void SetColumn(int i,  Point3 col);
	DllExport Point2 GetColumn2(int i);

	// Access the translation row
	void SetTrans(const Point2 p) { (*this)[2] = p;  }
	void SetTrans(int i, float v) { (*this)[2][i] = v;  }
	Point2 GetTrans() { return (*this)[2]; }
   
	// Apply Incremental transformations to this matrix
	DllExport void Translate(const Point2& p);
	DllExport void Rotate(float angle);  
	// if trans = FALSE the translation component is unaffected:
	DllExport void Scale(const Point2& s, BOOL trans=FALSE);

	// Apply Incremental transformations to this matrix
	// Equivalent to multiplying on the LEFT by transform 
	DllExport void PreTranslate(const Point2& p);
	DllExport void PreRotate(float angle);  
	DllExport void PreScale(const Point2& s, BOOL trans = FALSE);

    // Set matrix as described
    DllExport void SetTranslate(const Point2& s); // makes translation matrix
    DllExport void SetRotate(float angle);        // makes rotation matrix
    DllExport void Invert();
		
	// Binary operators		
	DllExport Matrix2 operator*(const Matrix2& B) const;	
	DllExport Matrix2 operator+(const Matrix2& B) const;
	DllExport Matrix2 operator-(const Matrix2& B) const;

	DllExport IOResult Save(ISave* isave);
	DllExport IOResult Load(ILoad* iload);

	};

// Build new matrices for transformations
Matrix2 DllExport RotateMatrix(float angle);   
Matrix2 DllExport TransMatrix(const Point2& p);
Matrix2 DllExport ScaleMatrix(const Point2& s);
 
Matrix2 DllExport Inverse(const Matrix2& M);

// Transform point with matrix:
Point2 DllExport operator*(const Matrix2& A, const Point2& V);
Point2 DllExport operator*( const Point2& V, const Matrix2& A);
Point2 DllExport VectorTransform(const Matrix2& M, const Point2& V);

// Printout
ostream DllExport &operator<< (ostream& s, const Matrix2& A); 

#endif
