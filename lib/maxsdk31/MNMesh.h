/**********************************************************************
 *<
	FILE: MNMesh.h

	DESCRIPTION:  Special mesh structures useful for face and edge based mesh operations.

	CREATED BY: Steve Anderson, working for Kinetix!

	HISTORY: created March 1997 from old SDMesh and WMesh.

 *>	Copyright (c) 1996 Autodesk, Inc., All Rights Reserved.
 **********************************************************************/

// Necessary prior inclusions: max.h

// Classes:
// MNMesh
// MNVert
// MNEdge
// MNFace
// MNMeshBorder

#ifndef __MN_MESH_H_
#define __MN_MESH_H_

#define REALLOC_SIZE 10

// Boolean types: we use the same ones defined in mesh.h
//#define MESHBOOL_UNION 1
//#define MESHBOOL_INTERSECTION 2
//#define MESHBOOL_DIFFERENCE 3

// General flags for all components
// For MNVerts, MNEdges, and MNFaces, bits 0-7 are used for common characteristics
// of all components.  Bits 8-15 are used for component-specific flags.  Bits 16-23 are reserved
// for temporary use in MNMesh algorithms.  Bits 24-31 are reserved for MNMath.lib users.
#define MN_SEL (1<<0)
#define MN_DEAD (1<<1)
#define MN_TARG (1<<2)
#define MN_BACKFACING (1<<3)
#define MN_HIDDEN (1<<4)
#define MN_WHATEVER (1<<16) // Temporary flag used internally for whatever.
#define MN_USER (1<<24)	// Anything above this can be used by applications.

// Vertex flags
#define MN_VERT_DONE (1<<8)
#define MN_VERT_HIDDEN (1<<9)	// different from MN_HIDDEN - means a vert is on a face's hvtx list.

class MNMesh;

class MNVert : public FlagUser {
public:
	Point3 p;
	int orig;	// Original point this vert comes from. -- STEVE, remove, only used in connect.

	MNVert () { orig = -1; }
	DllExport MNVert & operator= (MNVert & from);
};

// Edge flags
#define MN_EDGE_INVIS (1<<8)
#define MN_EDGE_NOCROSS (1<<9)
#define MN_EDGE_MAP_SEAM (1<<10)

// Edge goes from v1 to v2
// f1 is forward-indexing face (face on "left" if surface normal above, v2 in front)
// f2 is backward-indexing face, or -1 if no such face exists.  (Face on "right")
class MNEdge : public FlagUser {
public:
	int v1, v2;
	int f1, f2;
	int track;	// Keep track of whatever.

	MNEdge() { Init(); }
	MNEdge (int vv1, int vv2, int fc) { f1=fc; f2=-1; v1=vv1; v2=vv2; track=-1; }
	void Init() { v1=v2=f1=0; f2=-1; track=-1; }
	int OtherFace (int ff) { return (ff==f1) ? f2 : f1; }
	int OtherVert (int vv) { return (vv==v1) ? v2 : v1; }
	void Invert () { int hold=v1; v1=v2; v2=hold; hold=f1; f1=f2; f2=hold; }
	DllExport void ReplaceFace (int of, int nf, int vv1=-1);
	void ReplaceVert (int ov, int nv) { if (v1 == ov) v1 = nv; else { MaxAssert (v2==ov); v2 = nv; } }
	DllExport bool Uncrossable ();
	DllExport MNEdge & operator= (const MNEdge & from);
	DllExport void MNDebugPrint ();
};

class MNMapFace {
	friend class MNMesh;
	int dalloc, halloc;
public:
	int deg, hdeg;
	int *tv, *htv;

	MNMapFace() { Init(); }
	DllExport MNMapFace (int d, int h=0);
	~MNMapFace () { Clear(); }
	DllExport void Init();
	DllExport void Clear();
	DllExport void SetAlloc (int d, int h=0);
	void SetSize (int d, int h=0) { SetAlloc(d,h); deg=d; hdeg=h; }
	DllExport void MakePoly (int fdeg, int *tt);
	DllExport void Insert (int pos, int num=1);
	DllExport void Delete (int pos, int num=1);
	DllExport void HInsert (int pos, int num=1);
	DllExport void HDelete (int pos, int num=1);
	DllExport void RotateStart (int newstart);
	DllExport void Flip ();	// Reverses order of verts.  0 remains start.

	DllExport int VertIndex (int vv);
	DllExport void ReplaceVert (int ov, int nv);

	DllExport MNMapFace & operator= (const MNMapFace & from);
	DllExport void MNDebugPrint (bool hinfo=TRUE);
};

// MNFace flags:
#define MN_FACE_OPEN_REGION (1<<8)	// Face is not part of closed submesh.
#define MN_FACE_CHECKED (1<<9)	// for recursive face-and-neighbor-checking
#define MN_FACE_CHANGED (1<<10)

class MNFace : public FlagUser {
	friend class MNMesh;
	int dalloc, halloc, talloc;
	void Init();
	void SwapContents (MNFace & from);
public:
	int deg;	// Degree: number of vtx's and edg's that are relevant.
	int *vtx;	// Defining verts of this face.
	int *edg;
	int *tri;	// Triangulation
	int hdeg;
	int *hvtx;	// Hidden verts
	DWORD smGroup;
	MtlID material;
	int track;	// Keep track of whatever -- MNMesh internal use only.
	BitArray visedg, edgsel;

	MNFace() { Init(); }
	DllExport MNFace (int d, int h=0);
	DllExport MNFace (const MNFace *from);
	~MNFace () { Clear(); }
	DllExport void Clear();
	int TriNum() { return deg - 2 + 2*hdeg; }
	DllExport int TriVert(int k);
	DllExport void SetAlloc (int d, int h=0);
	DllExport void MakePoly (int fdeg, int *vv, bool *vis=NULL, bool *sel=NULL);
	DllExport void Insert (int pos, int num=1);
	DllExport bool Delete (int pos, int num=1, int edir=1, bool fixtri=TRUE);
	DllExport void HInsert (int pos, int num=1);
	DllExport void HDelete (int pos, int num=1);
	DllExport void RotateStart (int newstart);
	DllExport void Flip ();	// Reverses order of verts.  0 remains start.

	DllExport int VertIndex (int vv, int ee=-1);
	DllExport int EdgeIndex (int ee, int vv=-1);
	DllExport void ReplaceVert (int ov, int nv, int ee=-1);
	DllExport void ReplaceEdge (int oe, int ne, int vv=-1);

	DllExport MNFace & operator= (const MNFace & from);
	DllExport void MNDebugPrint (bool triprint=FALSE, bool hinfo=TRUE);

	DllExport IOResult Save (ISave *isave);
	DllExport IOResult Load (ILoad *iload);
};

/*
class MNFaceTriCache {
friend class MNMesh;
	int fid;
	int deg;
	int *tri;
	int hdeg;
	int *htri;
	double *hbary;

	MNFaceTriCache () { tri=NULL; htri=NULL; hbary=NULL; hdeg=0; deg=0; fid=-1; }
	~MNFaceTriCache () { if (tri) delete [] tri; if (htri) delete [] htri; if (hbary) delete [] hbary; }

	DllExport void SetDeg (int degg);
	DllExport void SetHDeg (int hdegg);
};
*/

class MNMap : public FlagUser {
	friend class MNMesh;
	int nv_alloc, nf_alloc;
public:
	MNMapFace *f;
	UVVert *v;

	int numv, numf;

	MNMap () { Init(); }
	~MNMap () { ClearAndFree (); }

	// Initialization, allocation:
	DllExport void Init ();
	DllExport void VAlloc (int num, bool keep=TRUE);
	DllExport void FAlloc (int num, bool keep=TRUE);

	// Data access:
	int VNum () const { return numv; }
	UVVert V(int i) const { return v[i]; }
	int FNum () const { return numf; }
	MNMapFace *F(int i) const { return &(f[i]); }

	// Adding new components -- all allocation should go through here!
	DllExport int NewTri (int a, int b, int c);
	DllExport int NewTri (int *vv);
	DllExport int NewQuad (int a, int b, int c, int d);
	DllExport int NewQuad (int *vv);
	DllExport int NewFace (int degg=0, int *vv=NULL);
	DllExport void setNumFaces (int nfnum);
	DllExport int NewVert (UVVert p, int uoff=0, int voff=0);
	DllExport void setNumVerts (int nvnum);

	DllExport void CollapseDeadVerts (MNFace *faces);	// Figures out which are dead.
	DllExport void CollapseDeadFaces (MNFace *faces);
	DllExport void Clear ();	// Deletes everything.
	DllExport void ClearAndFree ();	// Deletes everything, frees all memory

	DllExport void Transform (Matrix3 & xfm);	// o(n) -- transforms verts

	// operators and debug printing
	DllExport MNMap & operator= (const MNMap & from);
	DllExport MNMap & operator+= (const MNMap & from);
	DllExport MNMap & operator+= (const MNMesh & from);
	DllExport void ShallowCopy (const MNMap & from);
	DllExport void NewAndCopy ();
	DllExport void MNDebugPrint (MNFace *faces);
	DllExport bool CheckAllData (int mp, int nf, MNFace *faces);

	DllExport IOResult Save (ISave *isave, MNFace *faces=NULL);
	DllExport IOResult Load (ILoad *iload, MNFace *faces=NULL);
};

// Per-edge data
#define MAX_EDGEDATA 10
#define EDATA_KNOT 0

DllExport int EdgeDataType (int edID);
DllExport void *EdgeDataDefault (int edID);

#define MN_MESH_NONTRI (1<<0) // At least 2 triangles have been joined
#define MN_MESH_FILLED_IN (1<<1) // All topological links complete
#define MN_MESH_RATSNEST (1<<2) // Set if we've replicated points to avoid rats' nest meshes.
#define MN_MESH_NO_BAD_VERTS (1<<3)	// Set if we've established that each vert has exactly one connected component of faces & edges.
#define MN_MESH_VERTS_ORDERED (1<<4)	// Set if we've ordered the fac, edg tables in each vert.
#define MN_MESH_HAS_VOLUME (1<<7)	// Some subset of mesh describes closed surface of solid

#define MN_MESH_CACHE_FLAGS (MN_MESH_FILLED_IN|MN_MESH_NO_BAD_VERTS|MN_MESH_VERTS_ORDERED)

class MNMeshBorder;

typedef MNMap * MNMP;

// MNMesh selection levels:
#define MNM_SL_OBJECT 0
#define MNM_SL_VERTEX 1
#define MNM_SL_EDGE 2
#define MNM_SL_FACE 3
#define MNM_SL_TRI 4

// MNMesh display flags
#define MNDISP_VERTTICKS 0x01
#define MNDISP_SELVERTS	0x02
#define MNDISP_SELFACES	0x04
#define MNDISP_SELEDGES	0x08
#define MNDISP_NORMALS 0x10
#define MNDISP_SMOOTH_SUBSEL 0x20
#define MNDISP_BEEN_DISP 0x40

class MNMesh : public FlagUser {
private:
	int nv_alloc, ne_alloc, nf_alloc, nm_alloc;

	// Cache geometric data for quick rendering
	Box3 bdgBox;
	Point3 *fnorm;
	RVertex *rVerts;		// <<< instance specific.
	GraphicsWindow *cacheGW;  		// identifies rVerts cache
	int normalsBuilt;
	float norScale;

	//Tab<MNFaceTriCache *> tcache;

	// Internal part of SabinDoo method:
	void SDVEdgeFace (int id, int vid, int *fv, Tab<Tab<int> *> & fmv, DWORD selLevel);

	// Internal part of recursive smoothing-group search.
	DWORD FindReplacementSmGroup (int ff, DWORD os, int call);

	// Internal parts of Boolean. (MNBool.cpp)
	int BooleanZip (DWORD sortFlag, float weldThresh);
	bool BoolZipSeam (Tab<int> *lpi, Tab<int> *lpj, int & starth, int & startk, float weldThresh);
	void BoolPaintClassification (int ff, DWORD classification, int calldepth);

	// Internal data cache stuff (MNPipe.cpp)
	void buildBoundingBox ();
	void buildFaceNormals ();

public:
	MNVert *v;
	MNEdge *e;
	MNFace *f;
	MNMap *m;
	int numv, nume, numf, numm;

	// Vertex Data -- handled as in Meshes:
	PerData *vd;
	BitArray vdSupport;

	// Edge Data
	PerData *ed;
	BitArray edSupport;

	DWORD selLevel;
	DWORD dispFlags;

	// Derived data:
	Tab<int> *vedg;
	Tab<int> *vfac;

	// Basic class ops
	MNMesh () { Init(); DefaultFlags (); }
	MNMesh (const Mesh & from) { Init(); SetFromTri (from); FillInMesh (); }
	MNMesh (const MNMesh & from) { Init(); DefaultFlags(); *this = from; }
	~MNMesh() { ClearAndFree (); }

	// Initialization:
	void DefaultFlags () { ClearAllFlags (); }
	DllExport void Init ();

	// Array allocation: these functions (& Init) have sole control over nvalloc, etc.
	void VAlloc (int num, bool keep=TRUE);
	void VShrink (int num=-1);	// default means "Shrink array allocation to numv"
	void freeVEdge();
	void VEdgeAlloc();
	void freeVFace();
	void VFaceAlloc();
	void EAlloc (int num, bool keep=TRUE);
	void EShrink (int num=-1);
	void FAlloc (int num, bool keep=TRUE);
	void FShrink (int num=-1);
	void MAlloc (int num, bool keep=TRUE);
	void MShrink (int num=-1);

	// Access to components
	int VNum () const { return numv; }
	MNVert *V(int i) const { return &(v[i]); }
	Point3 & P(int i) const { return v[i].p; }
	int ENum () const { return nume; }
	MNEdge *E(int i) const { return &(e[i]); }
	int FNum () const { return numf; }
	MNFace *F(int i) const { return &(f[i]); }
	int MNum () const { return numm; }
	MNMap *M(int i) const { return &(m[i]); }
	DllExport void SetMapNum (int mpnum);
	DllExport void InitMap (int mp);	// Inits to current MNMesh topology.
	void ClearMap (int mp) { if (mp<numm) { m[mp].SetFlag (MN_DEAD); m[mp].ClearAndFree (); } }
	UVVert MV (int mp, int i) const { return m[mp].v[i]; }
	MNMapFace *MF (int mp, int i) const { return m[mp].F(i); }
	DllExport int TriNum () const;
	DllExport int HVNum (bool selFaces=FALSE) const;

	// Per Vertex Data:
	DllExport void setNumVData (int ct, BOOL keep=FALSE);
	int VDNum () const { return vdSupport.GetSize(); }

	DllExport BOOL vDataSupport (int vdChan) const;
	DllExport void setVDataSupport (int vdChan, BOOL support=TRUE);
	void *vertexData (int vdChan) const { return vDataSupport(vdChan) ? vd[vdChan].data : NULL; }
	float *vertexFloat (int vdChan) const { return (float *) vertexData (vdChan); }
	DllExport void freeVData (int vdChan);
	DllExport void freeAllVData ();

	// Two specific vertex data: these VDATA constants are defined in mesh.h
	float *getVertexWeights () { return vertexFloat(VDATA_WEIGHT); }
	void SupportVertexWeights () { setVDataSupport (VDATA_WEIGHT); }
	void freeVertexWeights () { freeVData (VDATA_WEIGHT); }
	float *getVSelectionWeights () { return vertexFloat(VDATA_SELECT); }
	void SupportVSelectionWeights () { setVDataSupport (VDATA_SELECT); }
	void freeVSelectionWeights () { freeVData (VDATA_SELECT); }

	// Per Edge Data:
	DllExport void setNumEData (int ct, BOOL keep=FALSE);
	int EDNum () const { return edSupport.GetSize(); }

	DllExport BOOL eDataSupport (int edChan) const;
	DllExport void setEDataSupport (int edChan, BOOL support=TRUE);
	void *edgeData (int edChan) const { return eDataSupport(edChan) ? ed[edChan].data : NULL; }
	float *edgeFloat (int edChan) const { return (float *) edgeData (edChan); }
	DllExport void freeEData (int edChan);
	DllExport void freeAllEData ();

	// One specific edge data: this EDATA constant is defined above
	float *getEdgeKnots () { return edgeFloat(EDATA_KNOT); }
	void SupportEdgeKnots () { setEDataSupport (EDATA_KNOT); }
	void freeEdgeKnots () { freeEData (EDATA_KNOT); }

	// Vertex face/edge list methods:
	DllExport void VClear (int vv);
	DllExport void VInit (int vv);
	DllExport int VFaceIndex (int vv, int ff, int ee=-1);
	DllExport int VEdgeIndex (int vv, int ee);
	void VDeleteEdge (int vv, int ee) { if (vedg) vedg[vv].Delete (VEdgeIndex(vv, ee), 1); }
	DllExport void VDeleteFace (int vv, int ff);
	DllExport void VReplaceEdge (int vv, int oe, int ne);
	DllExport void VReplaceFace (int vv, int of, int nf);
	DllExport void CopyVert (int nv, int ov);	// copies face & edge too if appropriate
	DllExport void MNVDebugPrint (int vv);

	// Adding new components -- all allocation should go through here!
	DllExport int NewTri (int a, int b, int c, DWORD smG=0, MtlID mt=0);
	DllExport int NewTri (int *vv, DWORD smG=0, MtlID mt=0);
	DllExport int NewQuad (int a, int b, int c, int d, DWORD smG=0, MtlID mt=0);
	DllExport int NewQuad (int *vv, DWORD smG=0, MtlID mt=0);
	DllExport int NewFace (int initFace, int degg=0, int *vv=NULL, bool *vis=NULL, bool *sel=NULL);
	DllExport int AppendNewFaces (int nfnum);
	DllExport void setNumFaces (int nfnum);
	DllExport int RegisterEdge (int v1, int v2, int f, int fpos);
	DllExport int SimpleNewEdge (int v1, int v2);
	DllExport int NewEdge (int v1, int v2, int f, int fpos);
	DllExport int AppendNewEdges (int nenum);
	DllExport void setNumEdges (int nenum);
	DllExport int NewVert (Point3 & p);
	DllExport int NewVert (Point3 & p, int vid);
	DllExport int NewVert (int vid);
	DllExport int NewVert (int v1, int v2, float prop);
	DllExport int AppendNewVerts (int nvnum);
	DllExport void setNumVerts (int nvnum);

	DllExport void KillUnusedHiddenVerts ();
	// To delete, set MN_*_DEAD flag and use following routines, which are all o(n).
	DllExport void CollapseDeadVerts ();
	DllExport void CollapseDeadEdges ();
	DllExport void CollapseDeadFaces ();
	DllExport void CollapseDeadStructs();
	DllExport void Clear ();	// Deletes everything.
	DllExport void ClearAndFree ();	// Deletes everything, frees all memory
	DllExport void freeVerts();
	DllExport void freeEdges();
	DllExport void freeFaces();
	DllExport void freeMap(int mp);
	DllExport void freeMaps();
	//DllExport void freeTCache();

	// En Masse flag-clearing and setting:
	void ClearVFlags (DWORD fl) { for (int i=0; i<numv; i++) v[i].ClearFlag (fl); }
	void ClearEFlags (DWORD fl) { for (int i=0; i<nume; i++) e[i].ClearFlag (fl); }
	void ClearFFlags (DWORD fl) { for (int i=0; i<numf; i++) f[i].ClearFlag (fl); }
	DllExport void PaintFaceFlag (int ff, DWORD fl, DWORD fenceflags=0x0);
	DllExport void VertexSelect (const BitArray & vsel);
	DllExport void EdgeSelect (const BitArray & esel);
	DllExport void FaceSelect (const BitArray & fsel);
	bool getVertexSel (BitArray & vsel) { return getVerticesByFlag (vsel, MN_SEL); }
	bool getEdgeSel (BitArray & esel) { return getEdgesByFlag (esel, MN_SEL); }
	bool getFaceSel (BitArray & fsel) { return getFacesByFlag (fsel, MN_SEL); }
	// In following 3, if fmask is 0 it's set to flags, so only those flags are compared.
	DllExport bool getVerticesByFlag (BitArray & vset, DWORD flags, DWORD fmask=0x0);
	DllExport bool getEdgesByFlag (BitArray & eset, DWORD flags, DWORD fmask=0x0);
	DllExport bool getFacesByFlag (BitArray & fset, DWORD flags, DWORD fmask=0x0);
	DllExport void ElementFromFace (int ff, BitArray & fset);
	DllExport void BorderFromEdge (int ee, BitArray & eset);

	// Following also set visedg, edgsel bits on faces:
	DllExport void SetEdgeVis (int ee, BOOL vis=TRUE);
	DllExport void SetEdgeSel (int ee, BOOL sel=TRUE);

	// I/O with regular Meshes.
	void SetFromTri (const Mesh & from) { Clear (); AddTri (from); }
	DllExport void AddTri (const Mesh & from);	// o(n) -- Add another mesh -- simple union
	DllExport void OutToTri (Mesh & tmesh);	// o(n)

	// Internal computation: each of the following 3 requires the previous.
	DllExport void FillInMesh ();	// o(n*5) or so
	DllExport void EliminateBadVerts ();	// o(n*8) or so
	DllExport void OrderVerts ();	// o(n*3) or so
	DllExport void Triangulate ();	// o(n)
	DllExport void TriangulateFace (int ff);	// o(triangles)

	// Random useful stuff.
	DllExport void Transform (Matrix3 & xfm);	// o(n) -- transforms verts
	bool IsClosed() { for (int i=0; i<nume; i++) if (e[i].f2<0) return FALSE; return nume?TRUE:FALSE; } // o(n)
	DllExport void FaceBBox (int ff, Box3 & bbox);
	DllExport void BBox (Box3 & bbox, bool targonly=FALSE);

	// Methods for handling MN_TARG flags.
	DllExport int TargetVertsBySelection (DWORD selLevel);	// o(n)
	DllExport int TargetEdgesBySelection (DWORD selLevel);	// o(n)
	DllExport int TargetFacesBySelection (DWORD selLevel);	// o(n)
	DllExport int PropegateComponentFlags (DWORD slTo, DWORD flTo,
		DWORD slFrom, DWORD flFrom, bool ampersand=FALSE, bool set=TRUE);
	DllExport void DetargetVertsBySharpness (float sharpval);	// o(n*deg)

	// Face-center methods
	DllExport void ComputeCenters (Point3 *ctr, bool targonly=FALSE);	// o(n)
	DllExport void ComputeCenter (int ff, Point3 & ctr);
	DllExport void ComputeSafeCenters (Point3 *ctr, bool targonly=FALSE, bool detarg=FALSE);	// o(n)
	DllExport bool ComputeSafeCenter (int ff, Point3 & ctr);	// o(deg^2)

	// Triangulation-of-polygon methods:
	DllExport void RetriangulateFace (int ff);	// o(deg^2)
	DllExport void FindExternalTriangulation (int ff, int *tri);
	DllExport void BestConvexTriangulation (int ff, int *tri=NULL);

	// Normal methods
	DllExport int FindEdgeFromVertToVert (int vrt1, int vrt2);	// o(deg)
	DllExport Point3 GetVertexNormal (int vrt);	// o(deg)
	DllExport Point3 GetEdgeNormal (int ed);	// o(deg)
	DllExport Point3 GetFaceNormal (int fc, bool nrmlz=FALSE);	//o(deg)
	Point3 GetEdgeNormal (int vrt1, int vrt2) { return GetEdgeNormal (FindEdgeFromVertToVert(vrt1, vrt2)); }

	// Smoothing-group handling
	DllExport void Resmooth (bool smooth=TRUE, bool targonly=FALSE, DWORD targmask=~0x0);	// o(n)
	DllExport DWORD CommonSmoothing (bool targonly=FALSE);	// o(n)
	DllExport DWORD GetNewSmGroup (bool targonly=FALSE);	// o(n)
	DllExport MtlID GetNewMtlID (bool targonly = FALSE); // o(n)
	DllExport DWORD GetOldSmGroup (bool targonly=FALSE);	// up to o(n).
	DllExport DWORD GetAllSmGroups (bool targonly=FALSE);	// up to o(n)
	DllExport DWORD FindReplacementSmGroup (int ff, DWORD os);
	DllExport void PaintNewSmGroup (int ff, DWORD os, DWORD ns);
	DllExport bool SeparateSmGroups (int v1, int v2);

	// Use following to unify triangles into polygons across invisible edges.
	DllExport void MakePolyMesh (int maxdeg=0, BOOL elimCollin=TRUE);
	// NOTE: MakeConvexPolyMesh result not guaranteed for now.  Still requires MakeConvex() afterwards to be sure.
	DllExport void MakeConvexPolyMesh (int maxdeg=0);
	DllExport void RemoveEdge (int edge);
	DllExport void MakeConvex ();
	DllExport void MakeFaceConvex (int ff);
	DllExport void EliminateCollinearVerts ();
	DllExport void EliminateCoincidentVerts (float thresh=MNEPS);

	// Following set NOCROSS flags, delete INVIS flags to make "fences" for Sabin-Doo
	DllExport void FenceMaterials ();
	DllExport void FenceSmGroups ();
	DllExport void FenceFaceSel ();
	DllExport void FenceOneSidedEdges();
	DllExport void FenceNonPlanarEdges (float thresh=.9999f);
	DllExport void SetMapSeamFlags (int mapID=-1);

	// Find get detail about a point on a face.
	DllExport int FindFacePointTri (int ff, Point3 & pt, double *bary=NULL, int *tri=NULL, int tnum=0);
	DllExport UVVert FindFacePointMapValue (int ff, Point3 & pt, int mp);

	// Extrapolate map information about a point near a face.
	DllExport UVVert ExtrapolateMapValue (int face, int edge, Point3 & pt, int mp);

	// Useful for tessellation algorithms
	DllExport void Relax (float relaxval, bool targonly=TRUE);

	// Returns map verts for both ends of each edge (from f1's perspective)
	// (Very useful for creating new faces at borders.) mv[j*2] corresponds to edge j's v1.
	DllExport void FindEdgeListMapVerts (const Tab<int> & lp, Tab<int> & mv, int mp);

	// Following functions can be used to find & fix holes in a mesh, if any.
	DllExport void GetBorder (MNMeshBorder & brd, DWORD selLevel=MESH_OBJECT);
	DllExport void FillInBorders (MNMeshBorder *b=NULL);
	DllExport void FindOpenRegions ();

	// To make hidden verts behave when moving real ones, use following:
	DllExport void PrepForGeomChange ();
	DllExport void CompleteGeomChange ();

	// operators and debug printing (MNFace.cpp)
	DllExport MNMesh & operator= (const MNMesh & from);
	DllExport MNMesh & operator+= (const MNMesh & from);
	DllExport void MNDebugPrint (bool triprint=FALSE);
	DllExport void MNDebugPrintVertexNeighborhood (int vv, bool triprint=FALSE);
	DllExport bool CheckAllData ();

	// Split functions maintain topological info.  (MNSplit.cpp)
	DllExport int SplitTriEdge (int ee, float prop=.5f, float thresh=MNEPS,
		bool neVis=TRUE, bool neSel=FALSE);
	DllExport int SplitTriFace (int ff, double *bary=NULL, float thresh=MNEPS,
		bool neVis=TRUE, bool neSel=FALSE);
	DllExport void SplitTri6 (int ff, double *bary=NULL, int *nv=NULL);
	DllExport int SplitEdge (int ee, float prop=.5f);
	DllExport int SplitEdge (int ee, float prop, int *ntv);
	DllExport int SplitEdge (int ff, int ed, float prop, bool right, int *nf=NULL,
		int *ne=NULL, bool neVis=FALSE, bool neSel=FALSE, bool allconvex=FALSE);
	DllExport int IndentFace (int ff, int ei, int nv, int *ne=NULL, bool nevis=TRUE, bool nesel=FALSE);
	DllExport void SeparateFace (int ff, int a, int b, int & nf, int & ne, bool neVis=FALSE, bool neSel=FALSE);
	DllExport void Slice (Point3 & N, float off, float thresh, bool split, bool remove, DWORD selLevel);
	DllExport void DeleteFlaggedFaces (DWORD deathflags, DWORD nvCopyFlags=0x0);
	DllExport bool WeldVerts (int a, int b);
	DllExport bool WeldEdge (int ee);

	// Tessellation methods: (MNTess.cpp)
	DllExport void TessellateByEdges (float bulge, MeshOpProgress *mop=NULL);
	DllExport bool AndersonDo (float interp, DWORD selLevel, MeshOpProgress *mop=NULL);
	DllExport void TessellateByCenters (MeshOpProgress *mop=NULL);

	// Sabin-Doo tessellation: (MNSabDoo.cpp)
	DllExport void SabinDoo (float interp, DWORD selLevel, MeshOpProgress *mop=NULL, Tab<Point3> *offsets=NULL);
	DllExport void SabinDooVert (int vid, float interp, DWORD selLevel, Point3 *ctr, Tab<Point3> *offsets=NULL);

	// Non-uniform Rational Mesh Smooth (NURMS.cpp)
	DllExport void CubicNURMS (MeshOpProgress *mop=NULL, Tab<Point3> *offsets=NULL);
	DllExport void QuadraticNURMS (MeshOpProgress *mop=NULL, Tab<Point3> *offsets=NULL);

	// Boolean functions: (MNBool.cpp)
	DllExport void PrepForBoolean ();
	DllExport bool BooleanCut (MNMesh & m2, int cutType, int fstart=0, MeshOpProgress *mop=NULL);
	DllExport bool MakeBoolean (MNMesh & m1, MNMesh & m2, int type, MeshOpProgress *mop=NULL);

	// New functionality: mostly pipeline object requirements.  (MNPipe.cpp)
	DllExport void ApplyMapper (UVWMapper & mp, BOOL channel=0, BOOL useSel=FALSE);
	DllExport void InvalidateGeomCache ();
	DllExport void InvalidateTopoCache ();
	DllExport void allocRVerts ();
	DllExport void updateRVerts (GraphicsWindow *gw);
	DllExport void freeRVerts ();

	GraphicsWindow *getCacheGW() { return cacheGW; }
	void setCacheGW(GraphicsWindow *gw) { cacheGW = gw; }

	DllExport void checkNormals (BOOL illum);
	DllExport void buildNormals ();
	DllExport void buildRenderNormals ();
	DllExport void UpdateBackfacing (GraphicsWindow *gw, bool force);

	// Display flags
	void		SetDispFlag(DWORD f) { dispFlags |= f; }
	DWORD		GetDispFlag(DWORD f) { return dispFlags & f; }
	void		ClearDispFlag(DWORD f) { dispFlags &= ~f; }

	DllExport void render(GraphicsWindow *gw, Material *ma, RECT *rp, int compFlags, int numMat=1);
	DllExport void renderFace (GraphicsWindow *gw, int ff);
	DllExport void renderEdge (GraphicsWindow *gw, int ee);
	DllExport BOOL select (GraphicsWindow *gw, Material *ma, HitRegion *hr, int abortOnHit=FALSE, int numMat=1);
	DllExport void		snap(GraphicsWindow *gw, SnapInfo *snap, IPoint2 *p, Matrix3 &tm);
	DllExport BOOL 		SubObjectHitTest(GraphicsWindow *gw, Material *ma, HitRegion *hr,
							DWORD flags, SubObjHitList& hitList, int numMat=1 );
	DllExport int IntersectRay (Ray& ray, float& at, Point3& norm);
	DllExport int IntersectRay (Ray& ray, float& at, Point3& norm, int &fi, int &ti, Point3 &bary);
	DllExport BitArray VertexTempSel (DWORD fmask=MN_DEAD, DWORD fset=0);

	DllExport void 		ShallowCopy(MNMesh *amesh, DWORD channels);
	DllExport void		NewAndCopyChannels(DWORD channels);
	DllExport void 		FreeChannels (DWORD channels, BOOL zeroOthers=1);

	DllExport IOResult Save (ISave *isave);
	DllExport IOResult Load (ILoad *iload);
};

class MNMeshBorder {
	friend class MNMesh;
	Tab<Tab<int> *> bdr;
	BitArray btarg;
public:
	~MNMeshBorder () { Clear(); }
	void Clear () { for (int i=0; i<bdr.Count(); i++) if (bdr[i]) delete bdr[i]; bdr.ZeroCount(); }
	int Num () { return bdr.Count(); }
	Tab<int> *Loop (int i) { return bdr[i]; }
	bool LoopTarg (int i) { return ((i>=0) && (i<bdr.Count()) && (btarg[i])); }
	DllExport void MNDebugPrint (MNMesh *m);
};

#endif
