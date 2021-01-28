/**********************************************************************
 *<
	FILE: tessint.h

	DESCRIPTION: include file for gap integration

	CREATED BY: Charles Thaeler

	HISTORY: created 10 Jan 1997

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#ifndef TESSINT_H
#define TESSINT_H

#ifdef BLD_TESS
#define TExport __declspec(dllexport)
#else
#define TExport __declspec(dllimport)
#endif

#include "max.h"
#include "maxtess.h"

class GmSurface;

// This class describes the parameters for a projective mapper.  They
// are set when a UVW mapper modifer is applied.
// SteveA 6/98: I moved the guts of this into "UVWMapper" in the mesh library.
class UVWMapperDesc : public UVWMapper {
public:
    int     channel;

    UVWMapperDesc() {channel=0;}
    UVWMapperDesc(int type, float utile, float vtile, float wtile,
                  int uflip, int vflip, int wflip, int cap,
                  const Matrix3 &tm,int channel);
    UVWMapperDesc(UVWMapperDesc& m) : UVWMapper (m)
        {
            this->channel = m.channel;
        }

    void ApplyMapper(Mesh* pMesh);
    void InvalidateMapping(Mesh* pMesh);
    IOResult Load(ILoad* iload);
    IOResult Save(ISave* isave);
};

struct SurfTabEntry {
	GmSurface *gmsurf;
    UVWMapperDesc* mpChannel1Mapper;
    UVWMapperDesc* mpChannel2Mapper;

    SurfTabEntry() {
        gmsurf = NULL;
        mpChannel1Mapper = NULL;
        mpChannel2Mapper = NULL;
    }
};

typedef Tab<SurfTabEntry> SurfTab;


typedef enum {
	BEZIER_PATCH,
	GMSURFACE,
	MAX_MESH,
	MODEL_OP
} SurfaceType;

TExport TCHAR* GapVersion(void);
TExport int GapTessellate(void *surf, SurfaceType type, Matrix3 *otm, Mesh *mesh,
							TessApprox *tess, TessApprox *disp, 
							View *view, Mtl* mtl, BOOL dumpMiFile, BOOL splitmesh);

TExport int GapInit(void);     // this should never be used by user code
TExport int GapShutdown(void); // this should never be used by user code

#endif
