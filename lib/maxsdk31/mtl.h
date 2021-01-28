/**********************************************************************
 *<
	FILE: mtl.h

	DESCRIPTION: Material and texture class definitions

	CREATED BY: Don Brittain

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#if !defined(_MTL_H_)


#define UVSOURCE_MESH  0      // use UVW coords from a standard map channel
#define UVSOURCE_XYZ   1      // compute UVW from object XYZ
#define UVSOURCE_MESH2 2      // use UVW2 (vertexCol) coords
#define UVSOURCE_WORLDXYZ 3   // use world XYZ as UVW
#ifdef DESIGN_VER
#define UVSOURCE_GEOXYZ 4
#endif

#define _MTL_H_

// main material class definition
class  Material {
public:
	DllExport Material();
	DllExport ~Material();
	
    Point3		Ka;
    Point3		Kd;
    Point3		Ks;
    float		shininess;
    float		shinStrength;
    float		opacity;
	float		selfIllum;
	int			dblSided;
	int			shadeLimit;
	int			useTex;
	int			faceMap;
	DWORD 		textHandle;  // texture handle
	int 		uvwSource;  
	int         mapChannel;
	Matrix3 	textTM;  // texture transform
	UBYTE 		tiling[3]; // u,v,w tiling:  GW_TEX_REPEAT, GW_TEX_MIRROR, or GW_TEX_NO_TILING
};

#endif // _MTL_H_
