/**********************************************************************
 *<
	FILE: captypes.h

	DESCRIPTION: Capping type defintions

	CREATED BY: Tom Hudson

	HISTORY: Created 12 October 1995

 *> Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __CAPTYPES_H_

#define __CAPTYPES_H_

// Just in case...
class PolyShape;
class BezierShape;

// Capping types supported
#define CAPTYPE_MORPH 0		// AKA 3D Studio DOS capping
#define CAPTYPE_GRID 1		// Max's very own capping

// Capping information classes:
// These classes provide information on how a cap is put together, based on the following:
//
// For Mesh caps, you get a list of created triangular faces, where the vertex indices are
// either original vertices on the PolyShape, newly created vertices inside the shape, or
// newly created vertices on the edge of a shape.  New vertices are only created for GRID
// type capping.  Cap info is always stored in unflipped form -- That is, faces are oriented
// in a counterclockwise order as viewed from the shape's "front", or positive Z.
//
// New free vertices are listed in the MeshCapInfo's "newVerts" table, a simple Point3 table.
// The "newVert" member of the MeshCapVert class points to the entry in the newVerts table.
//
// New edge vertices are stored with the polygon and segment number where they reside, along
// with a position on that segment (0-1) where they reside.  This information allows the cap
// user to divide adjacent faces as needed.
//
// For Patch caps, you can only cap using MORPH type capping.  GRID capping is meant for Mesh
// caps, where distorting a non-subdivided cap would result in serious surface discontinuities.
// Patches are automatically subdivided, so GRID capping is unnecessary there.
//

// CapFace flags 
#define CF_ABLINE (1<<0)
#define CF_BCLINE (1<<1)
#define CF_CALINE (1<<2)

class CapFace {
	public:
		int va;	// Index of vertex a
		int vb;	// Index of vertex b
		int vc;	// Index of vertex c
		DWORD flags;
		CapFace() {}
		CapFace(int a, int b, int c, DWORD f) { va=a; vb=b; vc=c; flags=f; }
	};

// Mesh cap vertices:
// These can be original vertices from the PolyShape or new free vertices
// in the center of the PolyShape.

#define MCV_ORIGINAL	0
#define MCV_FREE		1

class MeshCapVert {
	public:
		int type;		// See above
		int poly;		// The polygon number
		int index;		// The index of the vertex
		int newVert;	// The index of the new vertex
		MeshCapVert() {}
		MeshCapVert(int t, int p, int i, int nv=0) { type=t; poly=p; index=i; newVert=nv; }
	};

typedef Tab<CapFace> CapFaceTab;
typedef Tab<MeshCapVert> MeshCapVertTab;
typedef Tab<Point3> Point3Tab;

// The information class for mesh capping (MORPH or GRID)

class MeshCapInfo {
	public:
		CapFaceTab faces;
		MeshCapVertTab verts;
		Point3Tab newVerts;
		MeshCapInfo &operator=(MeshCapInfo &from) { faces=from.faces; verts=from.verts; newVerts=from.newVerts; return *this; }
		CoreExport void Init(PolyShape *shape);
		CoreExport void FreeAll();
	};

// Support classes for MeshCapper

class PolyLine;

class MeshCapPoly {
	public:
		int numVerts;
		int *verts;	// List of verts in mesh corresponding to verts in the PolyLine (1 per vert)
		MeshCapPoly() { verts = NULL; }
		CoreExport void Init(PolyLine &line);
		CoreExport ~MeshCapPoly();
		CoreExport void SetVert(int index, int vertex);
	};

// This class is used to apply the MeshCapInfo data to a mesh -- It will modify the mesh as required to
// add the cap.  Simply fill in the vertices and faces bordering the cap, then call the CapMesh method.

class MeshCapper {
	public:
		int numPolys;
		MeshCapPoly *polys;
		CoreExport MeshCapper(PolyShape &shape);
		CoreExport ~MeshCapper();
		CoreExport MeshCapPoly &operator[](int index);
		CoreExport int CapMesh(Mesh &mesh, MeshCapInfo &capInfo, BOOL flip, DWORD smooth, Matrix3 *tm=NULL, int mtlID=-1);
	};

// Patch capping

class CapPatch {
	public:
		int type;		// PATCH_TRI or PATCH_QUAD
		int verts[4];
		int vecs[8];
		int interior[4];
		CapPatch() {}
		CapPatch(int va, int vab, int vba, int vb, int vbc, int vcb, int vc, int vca, int vac, int i1, int i2, int i3) {
			type=PATCH_TRI; verts[0]=va; verts[1]=vb; verts[2]=vc; vecs[0]=vab; vecs[1]=vba; vecs[2]=vbc, vecs[3]=vcb;
			vecs[4]=vca; vecs[5]=vac; interior[0]=i1; interior[1]=i2; interior[2]=i3; }
		CapPatch(int va, int vab, int vba, int vb, int vbc, int vcb, int vc, int vcd, int vdc, int vd, int vda, int vad, int i1, int i2, int i3, int i4) {
			type=PATCH_QUAD; verts[0]=va; verts[1]=vb; verts[2]=vc; verts[3]=vd; vecs[0]=vab; vecs[1]=vba; vecs[2]=vbc, vecs[3]=vcb;
			vecs[4]=vcd; vecs[5]=vdc; vecs[6]=vda, vecs[7]=vad;  interior[0]=i1; interior[1]=i2; interior[2]=i3; interior[3]=i4; }
	};

// Patch cap vertices:
// These can be original vertices from the BezierShape or new free vertices
// in the center of the BezierShape.

#define PCVERT_ORIGINAL	0
#define PCVERT_FREE		1

class PatchCapVert {
	public:
		int type;
		int poly;		// The polygon number (ORIGINAL or EDGE)
		int index;		// The index of the vertex (ORIGINAL) or the segment for the EDGE vertex
		PatchCapVert() {}
		PatchCapVert(int t, int p, int i) { type=t; poly=p; index=i; }
	};

// Patch cap vectors:
// When a patch cap is generated, new interior vectors will be generated within the patch, and patch
// edges within the cap will have new vectors.  Patch edges along the edges of the originating bezier
// shape will use existing vectors.  This class provides information on which is which.

#define PCVEC_ORIGINAL	0
#define PCVEC_NEW		1

class PatchCapVec {
	public:
		int type;		// See above
		int poly;		// Polygon number for ORIGINAL
		int index;		// Index for ORIGINAL or into newVecs table (see below)
		PatchCapVec() {}
		PatchCapVec(int t, int p, int i) { type=t; poly=p; index=i; }
	};

typedef Tab<CapPatch> CapPatchTab;
typedef Tab<PatchCapVert> PatchCapVertTab;
typedef Tab<PatchCapVec> PatchCapVecTab;

// The information class for patch capping

class PatchCapInfo {
	public:
		CapPatchTab patches;
		PatchCapVertTab verts;
		PatchCapVecTab vecs;
		Point3Tab newVerts;
		Point3Tab newVecs;
		PatchCapInfo &operator=(PatchCapInfo &from) { patches=from.patches; verts=from.verts; vecs=from.vecs; newVerts=from.newVerts; newVecs=from.newVecs; return *this; }
		CoreExport void Init(BezierShape *shape);
		CoreExport void FreeAll();
	};

// Support classes for MeshCapper

class Spline3D;

class PatchCapPoly {
	public:
		int numVerts;
		int numVecs;
		int *verts;	// List of verts in patch mesh corresponding to verts in the spline (1 per vert)
		int *vecs;	// List of vecs in patch mesh corresponding to vecs in the spline (1 per vector)
		PatchCapPoly() { verts = vecs = NULL; }
		CoreExport void Init(Spline3D &spline);
		CoreExport ~PatchCapPoly();
		CoreExport void SetVert(int index, int vertex);
		CoreExport void SetVec(int index, int vector);
	};

// This class is used to apply the PatchCapInfo data to a PatchMesh -- It will modify the mesh as required to
// add the cap.  Simply fill in the vertices, vectors and patches bordering the cap, then call the CapPatch method.

class PatchCapper {
	public:
		int numPolys;
		PatchCapPoly *polys;
		CoreExport PatchCapper(BezierShape &shape);
		CoreExport ~PatchCapper();
		CoreExport PatchCapPoly &operator[](int index);
		CoreExport int CapPatchMesh(PatchMesh &mesh, PatchCapInfo &capInfo, BOOL flip, DWORD smooth, Matrix3 *tm=NULL, int mtlID=-1);
	};


#endif // __CAPTYPES_H_
