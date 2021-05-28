/**********************************************************************
 *<
	FILE: box3.h

	DESCRIPTION: 3D Box class

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef _BOX3_H 

#define _BOX3_H 

#include "point3.h"
#include "matrix3.h"

class Box3 {
	public:
		Point3 pmin,pmax;
		DllExport Box3();
		Box3(const Point3& p, const Point3& q) { pmin = p; pmax = q;}
		DllExport void Init();

		DllExport void MakeCube(const Point3& p, float side);

		// Access
		Point3 Min() const { return pmin; }
		Point3 Max() const { return pmax; }
		Point3 Center() const { return(pmin+pmax)/(float)2.0; }
		Point3 Width() const { return(pmax-pmin); }

		/* operator[] returns ith corner point: (i == (0..7) )
			Mapping:
			        X   Y   Z
			[0] : (min,min,min)
			[1] : (max,min,min)
			[2] : (min,max,min)
			[3] : (max,max,min)
			[4] : (min,min,max)
			[5] : (max,min,max)
			[6] : (min,max,max)
			[7] : (max,max,max)
			*/
		DllExport Point3 operator[](int i) const;	  

		// Modifiers
		DllExport Box3& operator+=(const Point3& p);	// expand this box to include Point3
		DllExport Box3& operator+=(const Box3& b);   // expand this box to include  Box3

		DllExport void Scale(float s); // scale box about center
		DllExport void Translate(const Point3 &p); // translate box
		DllExport void EnlargeBy(float s); // enlarge by this amount on all sides

		// include an array of points, optionally transformed by tm
		DllExport void IncludePoints(Point3 *pts, int numpoints, Matrix3 *tm=NULL); 

		// Returns a box that bounds the 8 transformed corners of the input box.
		DllExport Box3 operator*(const Matrix3& tm) const;

		// Tests
		DllExport int IsEmpty() const;   // is this box empty?
		DllExport int Contains(const Point3& p) const;  // is point in this box?
		DllExport int Contains(const Box3& b) const;  // is box b totally in this box?
		DllExport int Intersects(const Box3& b) const;  // does  box b intersect this box at all?
		
		

	};


#endif
