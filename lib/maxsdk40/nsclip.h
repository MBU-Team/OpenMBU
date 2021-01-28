/**********************************************************************
 *<
	FILE: nsclip.h

	DESCRIPTION: Clip board for named selection sets

	CREATED BY: Rolf Berteig

	HISTORY: 1/9/97

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef __NSCLIP_H__
#define __NSCLIP_H__
#include "shpsels.h"

// The mesh select modifier and the editable tri object need
// to be able to share the clipboard for named selection sets,
// however they are in two separate DLLs. So I've provided a 
// place in CORE for the clipboard. Note that if 3rd parties
// run into a similar problem they could always create their
// own shared DLL, I just used CORE because it was handy.

class MeshNamedSelClip {
	public:
		TSTR name;
		Tab<BitArray*> sets;

		MeshNamedSelClip(TSTR &n) {name=n;}
		CoreExport ~MeshNamedSelClip();
	};

#define CLIP_VERT	1
#define CLIP_FACE	2
#define CLIP_EDGE	3

CoreExport void SetMeshNamedSelClip(MeshNamedSelClip *clip, int which);
CoreExport MeshNamedSelClip *GetMeshNamedSelClip(int which);

// Here is the equivalent for patches

class PatchNamedSelClip {
	public:
		TSTR name;
		Tab<BitArray*> sets;

		PatchNamedSelClip(TSTR &n) {name=n;}
		CoreExport ~PatchNamedSelClip();
	};

#define CLIP_P_VERT		1
#define CLIP_P_PATCH	2
#define CLIP_P_EDGE		3

CoreExport void SetPatchNamedSelClip(PatchNamedSelClip *clip, int which);
CoreExport PatchNamedSelClip *GetPatchNamedSelClip(int which);

// Here are the equivalents for shapes:

class ShapeNamedVertSelClip {
	public:
		TSTR name;
		Tab<ShapeVSel*> sets;

		ShapeNamedVertSelClip(TSTR &n) {name=n;}
		CoreExport ~ShapeNamedVertSelClip();
	};

CoreExport void SetShapeNamedVertSelClip(ShapeNamedVertSelClip *clip);
CoreExport ShapeNamedVertSelClip *GetShapeNamedVertSelClip();

class ShapeNamedSegSelClip {
	public:
		TSTR name;
		Tab<ShapeSSel*> sets;

		ShapeNamedSegSelClip(TSTR &n) {name=n;}
		CoreExport ~ShapeNamedSegSelClip();
	};

CoreExport void SetShapeNamedSegSelClip(ShapeNamedSegSelClip *clip);
CoreExport ShapeNamedSegSelClip *GetShapeNamedSegSelClip();

class ShapeNamedPolySelClip {
	public:
		TSTR name;
		Tab<ShapePSel*> sets;

		ShapeNamedPolySelClip(TSTR &n) {name=n;}
		CoreExport ~ShapeNamedPolySelClip();
	};

CoreExport void SetShapeNamedPolySelClip(ShapeNamedPolySelClip *clip);
CoreExport ShapeNamedPolySelClip *GetShapeNamedPolySelClip();

#endif //__NSCLIP_H__

