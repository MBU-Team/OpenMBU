/**********************************************************************
 *<
	FILE: gutil.h

	DESCRIPTION: Geometric utility functions

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _GUTIL_H 

#define _GUTIL_H


//- BaryCoords-----------------------------------------------------
// Given three points in space forming a triangle (p0,p1,p2), 
// and a fourth point in the plane of that triangle, returns the
// barycentric coords of that point relative to the triangle.

DllExport Point3 BaryCoords(Point3 p0, Point3 p1, Point3 p2, Point3 p);

// Same thing in 2D
DllExport Point3 BaryCoords(Point2 p0, Point2 p1, Point2 p2, Point2 p);



//--RayHitsBox--------------------------------------------------------
// Returns true of the ray pierces the box

DllExport BOOL RayHitsBox(Ray &ray, Box3 &b);


// DistPtToLine ---------------------------------------------------------
// find distance of q from line p0->p1 

DllExport float  DistPtToLine(Point2 *p0, Point2 *p1, Point2 *q );


// Dist3DPtToLine ---------------------------------------------------------
// find distance of q from line p0->p1 

DllExport float  Dist3DPtToLine(Point3* p0, Point3* p1, Point3*  q );


//----------------------------------------------------------------------
// USE CompBumpVec2D INSTEAD! This is here for compatibility only.
//
// Computing the 3 Bump basis vectors from the UVW's at the triangle.
//
//  input:
//    tv: texture coordinates at 3 triangle vertices
//    v: coordinates of triangle vertices (usually in camera space)
//
// output:
//    bvec: the 3 bump basis vectors (normalized) corresponding to the U,V,and W axes.
//
DllExport void ComputeBumpVectors(const Point3 tv[3], const Point3 v[3], Point3 bvec[3]);


//----------------------------------------------------------------------
// Computing the 2 Bump basis vectors from the UV, VW, or WU  at a triangle.
//
//  input:
//    axis: 0=>UV,  1=>VW,  2=>WU ( use defined constants AXIS_UV, AXIS_VW, AXIS_WU in imtl.h
//    tv: texture coordinates at 3 triangle vertices
//    v: coordinates of triangle vertices (usually in camera space)
//
// output:
//    bvec: the 2 bump basis vectors (normalized) corresponding to the specified axes.
//
// THIS IS THE RECOMMENDED WAY TO COMPUTE BUMP VECTORS, NOT ComputeBumpVectors, which 
// can give erroneous results
DllExport void ComputeBumpVec2D(int axis, const Point3 tv[3], const Point3 v[3], Point3 bvec[2]);

//-------------------------------------------------------------------------------
// Quick and dirty unit vector compression. Only accurate to 1 part in 512
//
ULONG DllExport CompressNormal(Point3 p);

Point3 DllExport DeCompressNormal(ULONG n);

//-------------------------------------------------------------------------------
// JH 10/05/98
// Method to come up with an arbitray axis system given an "up" direction
// Conforms to the AutoCAD algorithm
void DllExport ArbAxis(const Point3& zAxis, Matrix3& matrix);




#endif