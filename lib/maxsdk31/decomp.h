/**********************************************************************
 *<
	FILE: decomp.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _H_Decompose
#define _H_Decompose

typedef struct {
	Point3 t;	/* Translation components */
	Quat q;	/* Essential rotation	  */
	Quat u;	/* Stretch rotation	  */
	Point3 k;	/* Stretch factors	  */
	float f;	/* Sign of determinant	  */
	} AffineParts;

CoreExport void SpectralDecomp(Matrix3 m, Point3 &s, Quat& q);
CoreExport void decomp_affine(Matrix3 A, AffineParts *parts);
CoreExport void invert_affine(AffineParts *parts, AffineParts *inverse);
#endif

