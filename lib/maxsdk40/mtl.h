/**********************************************************************
 *<
	FILE: mtl.h

	DESCRIPTION: Material and texture class definitions

	CREATED BY: Don Brittain

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#if !defined(_MTL_H_)

#define _MTL_H_

#include "tab.h"

#include "BaseInterface.h"
class MtlBase;

#define UVSOURCE_MESH  0      // use UVW coords from a standard map channel
#define UVSOURCE_XYZ   1      // compute UVW from object XYZ
#define UVSOURCE_MESH2 2      // use UVW2 (vertexCol) coords
#define UVSOURCE_WORLDXYZ 3   // use world XYZ as UVW
#ifdef DESIGN_VER
#define UVSOURCE_GEOXYZ 4
#endif

// texture class definition
class  TextureInfo : public BaseInterfaceServer {
public:
	DllExport TextureInfo();
	virtual ~TextureInfo() {};
	DllExport TextureInfo& operator=(const TextureInfo &from);
	
	int			useTex;
	int			faceMap;
	DWORD_PTR	textHandle;  // texture handle
	int 		uvwSource;  
	int         mapChannel;
	Matrix3 	textTM;  // texture transform
	UBYTE 		tiling[3]; // u,v,w tiling:  GW_TEX_REPEAT, GW_TEX_MIRROR, or GW_TEX_NO_TILING
	UBYTE		colorOp;	// color texture operation
	UBYTE		colorAlphaSource;	// color blend alpha source
	UBYTE		colorScale;	// color scale factor
	UBYTE		alphaOp;	// alpha texture operation
	UBYTE		alphaAlphaSource;	// alpha blend alpha source
	UBYTE		alphaScale;	// alpha scale factor
};

// main material class definition
class  Material : public BaseInterfaceServer {
protected:

public:
	DllExport Material();
	virtual ~Material() {};
	DllExport Material& operator=(const Material &from);

    Point3		Ka;
    Point3		Kd;
    Point3		Ks;
    float		shininess;
    float		shinStrength;
    float		opacity;
	float		selfIllum;
	int			dblSided;
	int			shadeLimit;
	Tab<TextureInfo> texture;
	DllExport BaseInterface *GetInterface(Interface_ID id);
};

#endif // _MTL_H_
