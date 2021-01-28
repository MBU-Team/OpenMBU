/**********************************************************************
 *<
	FILE: maxtess.h

	DESCRIPTION: Tessellation Approximation class

	CREATED BY: Charles Thaeler

	HISTORY: created 12 Dec 1996
			 Updated 12-10-98 Peter Watje to support hidden interior edges

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#ifndef MAXTESS_H
#define MAXTESS_H

typedef enum {
	TESS_SET,		// This is the old MAX form for Bezier Patches
	TESS_ISO,		// This is obsolete and should not be used.
	TESS_PARAM,
	TESS_SPATIAL,
	TESS_CURVE,
	TESS_LDA,
	TESS_REGULAR 
} TessType;

typedef enum {
	ISO_ONLY,
	ISO_AND_MESH,
	MESH_ONLY
} ViewConfig;

typedef enum {
	SUBDIV_TREE,
	SUBDIV_GRID,
	SUBDIV_DELAUNAY
} TessSubdivStyle;

class TessApprox {
public:
	TessType type;
	ViewConfig vpt_cfg;
	TessSubdivStyle subdiv;
	BOOL view;
	float merge;
	int u, v;
	int u_iso, v_iso;
	float ang, dist, edge;
	int minSub, maxSub, maxTris;

//watje 12-10-98
	BOOL showInteriorFaces;

	UtilExport TessApprox();
	UtilExport TessApprox(TessType type, float distance, float edge, float angle,
                          TessSubdivStyle subdivStyle, int minSub, int maxSub,
                          float m = 0.0f);
	UtilExport TessApprox(const TessApprox &tess);
	UtilExport TessApprox & operator=(const TessApprox& tess);
	UtilExport int operator==(const TessApprox &tess) const;


	UtilExport IOResult Load(ILoad* iload);
	UtilExport IOResult Save(ISave* isave);
};

#endif
