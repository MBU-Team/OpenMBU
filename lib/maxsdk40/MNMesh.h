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

#include "export.h"
#include "baseinterface.h"

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
#define MN_LOCAL_SEL (1<<17) // Alternate selections (not passed up the pipe).
#define MN_USER (1<<24)	// Anything above this can be used by applications.

// Vertex flags
#define MN_VERT_DONE (1<<8)

class IHardwareShader;
class TriStrip;
class MNMesh;

class MNVert : public FlagUser {
public:
	Point3 p;
	int orig;	// Original point this vert comes from

	MNVert () { orig = -1; }
	DllExport MNVert & operator= (MNVert & from);
	bool operator==(MNVert & from) { return (from.p==p)&&(from.ExportFlags()==ExportFlags()); }
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
	bool operator==(MNEdge & f) { return (f.v1==v1)&&(f.v2==v2)&&(f.f1==f1)&&(f.f2==f2)&&(f.ExportFlags()==ExportFlags()); }
	int& operator[](int i) { return i ? v2 : v1; }  
	const int& operator[](int i) const { return i ? v2 : v1; }  
	DllExport void MNDebugPrint ();
};

class MNFace;

class MNMapFace {
	friend class MNMesh;
	int dalloc;
public:
	int deg;
	int *tv;

	MNMapFace() { Init(); }
	DllExport MNMapFace (int d);
	~MNMapFace () { Clear(); }
	DllExport void Init();
	DllExport void Clear();
	DllExport void SetAlloc (int d);
	void SetSize (int d) { SetAlloc(d); deg=d; }
	DllExport void MakePoly (int fdeg, int *tt);
	DllExport void Insert (int pos, int num=1);
	DllExport void Delete (int pos, int num=1);
	DllExport void RotateStart (int newstart);
	DllExport void Flip ();	// Reverses order of verts.  0 remains start.

	DllExport int VertIndex (int vv);
	DllExport void ReplaceVert (int ov, int nv);

	DllExport MNMapFace & operator= (const MNMapFace & from);
	DllExport MNMapFace & operator= (const MNFace & from);
	DllExport bool operator== (const MNMapFace & from);
	DllExport void MNDebugPrint ();
};

// MNFace flags:
#define MN_FACE_OPEN_REGION (1<<8)	// Face is not part of closed submesh.
#define MN_FACE_CHECKED (1<<9)	// for recursive face-and-neighbor-checking
#define MN_FACE_CHANGED (1<<10)

// Diagonal sorting algorithm:
// Puts diagonals in increase-by-last-index, decrease-by-first order:
DllExport void DiagSort (int dnum, int *diag);

class MNFace : public FlagUser {
	friend class MNMesh;
	int dalloc;
	void SwapContents (MNFace & from);
public:
	int deg;	// Degree: number of vtx's and edg's that are relevant.
	int *vtx;	// Defining verts of this face.
	int *edg;	// edges on this face.
	int *diag;	// Diagonals
	DWORD smGroup;
	MtlID material;
	int track;	// Keep track of whatever -- MNMesh internal use only.
	BitArray visedg, edgsel;

	MNFace() { Init(); }
	MNFace (int d) { Init(); SetDeg (d); }
	DllExport MNFace (const MNFace *from);
	~MNFace () { Clear(); }

	DllExport void Init();
	DllExport void SetDeg (int d);
	DllExport void Clear();
	int TriNum() { return deg-2; }
	DllExport int FindTriPoint (int edge);	// finds point that makes tri with this edge.
	DllExport int FindTriPoint (int a, int b);	// does the same for nonsequential verts - result is between a,b.
	DllExport void GetTriangles (Tab<int> &tri);
	DllExport void SetAlloc (int d);
	DllExport void MakePoly (int fdeg, int *vv, bool *vis=NULL, bool *sel=NULL);
	DllExport void Insert (int pos, int num=1);
	DllExport bool Delete (int pos, int num=1, int edir=1, bool fixtri=TRUE);
	DllExport void RotateStart (int newstart);
	DllExport void Flip ();	// Reverses order of verts.  0 remains start.

	DllExport int VertIndex (int vv, int ee=-1);
	DllExport int EdgeIndex (int ee, int vv=-1);
	DllExport void ReplaceVert (int ov, int nv, int ee=-1);
	DllExport void ReplaceEdge (int oe, int ne, int vv=-1);

	DllExport MNFace & operator= (const MNFace & from);
	DllExport bool operator== (const MNFace &from);
	int& operator[](int i) { return vtx[i]; }
	const int& operator[](int i) const { return vtx[i]; }  
	DllExport void MNDebugPrint (bool triprint=FALSE);

	DllExport IOResult Save (ISave *isave);
	DllExport IOResult Load (ILoad *iload);
};

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
#define EDATA_CREASE 1

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
class MNChamferData;
class MNFaceClusters;

// MNMesh selection levels:
enum PMeshSelLevel { MNM_SL_OBJECT, MNM_SL_VERTEX, MNM_SL_EDGE, MNM_SL_FACE };

// MNMesh display flags
#define MNDISP_VERTTICKS 0x01
#define MNDISP_SELVERTS	0x02
#define MNDISP_SELFACES	0x04
#define MNDISP_SELEDGES	0x08
#define MNDISP_NORMALS 0x10
#define MNDISP_SMOOTH_SUBSEL 0x20
#define MNDISP_BEEN_DISP 0x40
#define MNDISP_DIAGONALS 0x80

// Flags for sub object hit test
// NOTE: these are the same bits used for object level.
//#define SUBHIT_SELONLY		(1<<0)
//#define SUBHIT_UNSELONLY	(1<<2)
//#define SUBHIT_ABORTONHIT	(1<<3)
//#define SUBHIT_SELSOLID		(1<<4)

#define SUBHIT_MNUSECURRENTSEL (1<<22)		// When this bit is set, the sel only and unsel only tests will use the current level (edge or face) selection when doing a vertex level hit test
#define SUBHIT_OPENONLY		(1<<23)
#define SUBHIT_MNVERTS		(1<<24)
#define SUBHIT_MNFACES		(1<<25)
#define SUBHIT_MNEDGES		(1<<26)
#define SUBHIT_MNTYPEMASK	(SUBHIT_MNVERTS|SUBHIT_MNFACES|SUBHIT_MNEDGES)

// Subdivision flags:
#define MN_SUBDIV_NEWMAP 0x01

class MNMesh : public FlagUser, public BaseInterfaceServer {
	friend class HardwareMNMesh;
	friend void gfxCleanup(void *data);
private:
	static int		refCount;
	static HANDLE	workMutex;
	static HANDLE	workEndEvent;

	int nv_alloc, ne_alloc, nf_alloc, nm_alloc;

	// Cache geometric data for quick rendering
	Box3 bdgBox;
	Point3 *fnorm;
	RVertex *rVerts;		// <<< instance specific.
	GraphicsWindow *cacheGW;  		// identifies rVerts cache
	int normalsBuilt;
	float norScale;

	// Hidden map channels
	// There are always NUM_HIDDENMAPS of these, but we dynamically allocate them
	// so that M(mp) can be a const method.  (This wouldn't work if we had hmap[NUM_HIDDENMAPS].)
	MNMap *hmap;
	// Formerly public normal map channels:
	MNMap *m;

	// Vertex color arrays - for display use.
	int curVCChan;	// current map channel to use for colors (default = 0)
	UVVert *curVCVerts;	// possible external color array (default = NULL)
	MNMapFace *curVCFaces;	// possible external face array (default = NULL)

	DWTab		norInd;			// indirection array for fast normal lookup
	int			normalCount;	// total number of normals
	Point3 *	gfxNormals;		// flattened list of normals

	// Derived arrays to contain generated texture coordinates
	int			numTexCoords[GFX_MAX_TEXTURES];
	Point3 *	texCoords[GFX_MAX_TEXTURES];

	// Derived table of TriStrips, depends on topology
	Tab<TriStrip *>	*tstab;

	// Internal part of SabinDoo method:
	void SDVEdgeFace (int id, int vid, int *fv, Tab<Tab<int> *> & fmv, int selLevel);

	// Internal part of recursive smoothing-group search.
	DWORD FindReplacementSmGroup (int ff, DWORD os, int call);

	// Internal parts of Boolean. (MNBool.cpp)
	int BooleanZip (DWORD sortFlag, float weldThresh);
	bool BoolZipSeam (Tab<int> *lpi, Tab<int> *lpj, int & starth, int & startk, float weldThresh);
	void BoolPaintClassification (int ff, DWORD classification);

	// Internal data cache stuff (MNPipe.cpp)
	void buildBoundingBox ();
	void buildFaceNormals ();

	GraphicsWindow *getCacheGW() { return cacheGW; }
	void setCacheGW (GraphicsWindow *gw) { cacheGW = gw; }

	// New MNMesh routines to drive HardwareShaders
	bool	CanDrawTriStrips(DWORD rndMode, int numMtls, Material *mtl);
	bool	BuildTriStrips(DWORD rndMode, int numMtls, Material *mtl);
	void	TriStripify(DWORD rndMode, int numTex, TVFace *tvf[], TriStrip *s, StripData *sd, int vtx);
	void	Draw3DTriStrips(IHardwareShader *phs, int numMat, Material *ma);
	void	Draw3DWireTriStrips(IHardwareShader *phs, int numMat, Material *ma);
	void	Draw3DVisEdgeList(IHardwareShader *phs, DWORD flags);
	int		render3DTriStrips(IHardwareShader *phs, int kmat, int kstrips);
	int		render3DWireTriStrips(IHardwareShader *phs, int kmat, int kstrips);
	int		render3DFaces(IHardwareShader *phs, DWORD index, int *custVis=NULL);

public:
	MNVert *v;
	MNEdge *e;
	MNFace *f;
	int numv, nume, numf, numm;

	// Vertex Data -- handled as in Meshes:
	PerData *vd;
	BitArray vdSupport;

	// Edge Data
	PerData *ed;
	BitArray edSupport;

	int selLevel;
	DWORD dispFlags;

	// Derived data:
	Tab<int> *vedg;
	Tab<int> *vfac;

	// Basic class ops
	MNMesh () { Init(); DefaultFlags (); }
	MNMesh (const Mesh & from) { Init(); SetFromTri (from); FillInMesh (); }
	MNMesh (const MNMesh & from) { Init(); DefaultFlags(); *this = from; }
	DllExport ~MNMesh();

	// Initialization:
	void DefaultFlags () { ClearAllFlags (); }
	DllExport void Init ();

	// Array allocation: these functions (& Init) have sole control over nvalloc, etc.
	DllExport void VAlloc (int num, bool keep=TRUE);
	DllExport void VShrink (int num=-1);	// default means "Shrink array allocation to numv"
	DllExport void freeVEdge();
	DllExport void VEdgeAlloc();
	DllExport void freeVFace();
	DllExport void VFaceAlloc();
	DllExport void EAlloc (int num, bool keep=TRUE);
	DllExport void EShrink (int num=-1);
	DllExport void FAlloc (int num, bool keep=TRUE);
	DllExport void FShrink (int num=-1);
	DllExport void MAlloc (int num, bool keep=TRUE);
	DllExport void MShrink (int num=-1);

	// Access to components
	int VNum () const { return numv; }
	MNVert *V(int i) const { return &(v[i]); }
	Point3 & P(int i) const { return v[i].p; }
	int ENum () const { return nume; }
	MNEdge *E(int i) const { return &(e[i]); }
	int FNum () const { return numf; }
	MNFace *F(int i) const { return &(f[i]); }
	int MNum () const { return numm; }
	DllExport MNMap *M(int mp) const;
	DllExport void SetMapNum (int mpnum);
	DllExport void InitMap (int mp);	// Inits to current MNMesh topology.
	DllExport void ClearMap (int mp);
	DllExport UVVert MV (int mp, int i) const;
	DllExport MNMapFace *MF (int mp, int i) const;
	DllExport int TriNum () const;

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

	// Internal computation routines
	// These three build on each other and should go in order:
	// FillInMesh, EliminateBadVerts, OrderVerts.
	DllExport void FillInMesh ();	// o(n*5) or so
	DllExport void FillInFaceEdges (); // o(n).
	DllExport void FillInVertEdgesFaces ();	// o(n)
	DllExport bool EliminateBadVerts (DWORD flag=0);	// o(n*8) or so
	DllExport void OrderVerts ();	// o(n*3) or so
	DllExport void OrderVert (int vid);
	DllExport void Triangulate ();	// o(n)
	DllExport void TriangulateFace (int ff);	// o(triangles)

	// Random useful stuff.
	DllExport void Transform (Matrix3 & xfm);	// o(n) -- transforms verts
	bool IsClosed() { for (int i=0; i<nume; i++) if (e[i].f2<0) return FALSE; return nume?TRUE:FALSE; } // o(n)
	DllExport void FaceBBox (int ff, Box3 & bbox);
	DllExport void BBox (Box3 & bbox, bool targonly=FALSE);
	DllExport Box3 getBoundingBox (Matrix3 *tm=NULL, bool targonly=FALSE);

	// Methods for handling MN_TARG flags.
	DllExport int TargetVertsBySelection (int selLevel);	// o(n)
	DllExport int TargetEdgesBySelection (int selLevel);	// o(n)
	DllExport int TargetFacesBySelection (int selLevel);	// o(n)
	DllExport int PropegateComponentFlags (int slTo, DWORD flTo,
		int slFrom, DWORD flFrom, bool ampersand=FALSE, bool set=TRUE);
	DllExport void DetargetVertsBySharpness (float sharpval);	// o(n*deg)

	// Face-center methods
	DllExport void ComputeCenters (Point3 *ctr, bool targonly=FALSE);	// o(n)
	DllExport void ComputeCenter (int ff, Point3 & ctr);
	DllExport void ComputeNormal (int ff, Point3 & normal, Point3 *ctr=NULL);
	DllExport void ComputeSafeCenters (Point3 *ctr, bool targonly=FALSE, bool detarg=FALSE);	// o(n)
	DllExport bool ComputeSafeCenter (int ff, Point3 & ctr);	// o(deg^2)

	// Triangulation-of-polygon methods:
	DllExport void RetriangulateFace (int ff);	// o(deg^2)
	DllExport void FindDiagonals (int ff, int *diag);
	DllExport void FindDiagonals (int deg, int *vv, int *diag);
	DllExport void BestConvexDiagonals (int ff, int *diag=NULL);
	DllExport void BestConvexDiagonals (int deg, int *vv, int *diag);
	DllExport bool SetDiagonal (int ff, int d1, int d2);

	// Normal methods
	DllExport int FindEdgeFromVertToVert (int vrt1, int vrt2);	// o(deg)
	DllExport void GetVertexSpace (int vrt, Matrix3 & tm);	// o(deg)
	DllExport Point3 GetVertexNormal (int vrt);	// o(deg)
	DllExport Point3 GetEdgeNormal (int ed);	// o(deg)
	DllExport Point3 GetFaceNormal (int fc, bool nrmlz=FALSE);	//o(deg)
	Point3 GetEdgeNormal (int vrt1, int vrt2) { return GetEdgeNormal (FindEdgeFromVertToVert(vrt1, vrt2)); }
	DllExport float EdgeAngle (int ed);
	DllExport void FlipNormal(int faceIndex);

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
	DllExport void AutoSmooth(float angle,BOOL useSel,BOOL preventIndirectSmoothing);

	// Use following to unify triangles into polygons across invisible edges.
	DllExport void MakePolyMesh (int maxdeg=0, BOOL elimCollin=TRUE);
	// NOTE: MakeConvexPolyMesh result not guaranteed for now.  Still requires MakeConvex() afterwards to be sure.
	DllExport void MakeConvexPolyMesh (int maxdeg=0);
	DllExport void RemoveEdge (int edge);
	DllExport void MakeConvex ();
	DllExport void MakeFaceConvex (int ff);
	DllExport void RestrictPolySize (int maxdeg);
	DllExport void MakePlanar (float planarThresh);
	DllExport void MakeFacePlanar (int ff, float planarThresh);
	DllExport void EliminateCollinearVerts ();
	DllExport void EliminateCoincidentVerts (float thresh=MNEPS);

	// Following set NOCROSS flags, delete INVIS flags to make "fences" for Sabin-Doo
	DllExport void FenceMaterials ();
	DllExport void FenceSmGroups ();
	DllExport void FenceFaceSel ();
	DllExport void FenceOneSidedEdges();
	DllExport void FenceNonPlanarEdges (float thresh=.9999f, bool makevis=FALSE);
	DllExport void SetMapSeamFlags ();
	DllExport void SetMapSeamFlags (int mp);

	// Find get detail about a point on a face.
	DllExport int FindFacePointTri (int ff, Point3 & pt, double *bary, int *tri);
	DllExport UVVert FindFacePointMapValue (int ff, Point3 & pt, int mp);

	// Extrapolate map information about a point near a face.
	DllExport UVVert ExtrapolateMapValue (int face, int edge, Point3 & pt, int mp);

	// Useful for tessellation algorithms
	DllExport void Relax (float relaxval, bool targonly=TRUE);

	// Returns map verts for both ends of each edge (from f1's perspective)
	// (Very useful for creating new faces at borders.) mv[j*2] corresponds to edge j's v1.
	DllExport void FindEdgeListMapVerts (const Tab<int> & lp, Tab<int> & mv, int mp);

	// Following functions can be used to find & fix holes in a mesh, if any.
	DllExport void GetBorder (MNMeshBorder & brd, int selLevel=MNM_SL_OBJECT, DWORD targetFlag=MN_SEL);
	DllExport void FillInBorders (MNMeshBorder *b=NULL);
	DllExport void FindOpenRegions ();

	// Doubled mapping verts are individual map vertices that are used to correspond
	// to different regular vertices.  For instance, a box could have a single (1,1,0)
	// vertex that it uses in the upper-right corner of all quads.  This design is
	// harmful to some of our algorithms, such as the various Tessellators.  So the
	// following two methods detect and fix such vertices.

	// This method is not really appropriate for release, it's more of a debugging
	// tool.  All doubled mapping verts will be DebugPrinted.
	DllExport BOOL CheckForDoubledMappingVerts();

	// This one's ok and encouraged for release.  (Linear-time algorithm.)
	DllExport void EliminateDoubledMappingVerts();
	DllExport void EliminateIsoMapVerts();
	DllExport void EliminateIsoMapVerts(int mp);

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
	DllExport int SplitEdge (int ee, float prop, Tab<int> *newTVerts);
	DllExport int SplitEdge (int ff, int ed, float prop, bool right, int *nf=NULL,
		int *ne=NULL, bool neVis=FALSE, bool neSel=FALSE, bool allconvex=FALSE);
	DllExport int IndentFace (int ff, int ei, int nv, int *ne=NULL, bool nevis=TRUE, bool nesel=FALSE);
	DllExport void SeparateFace (int ff, int a, int b, int & nf, int & ne, bool neVis=FALSE, bool neSel=FALSE);
	DllExport bool Slice (Point3 & N, float off, float thresh, bool split, bool remove, bool flaggedFacesOnly=false, DWORD faceFlags=MN_SEL);
	DllExport void DeleteFlaggedFaces (DWORD deathflags, DWORD nvCopyFlags=0x0);
	DllExport bool WeldVerts (int a, int b);
	DllExport bool WeldEdge (int ee);

	// Tessellation methods: (MNTess.cpp)
	DllExport void TessellateByEdges (float bulge, MeshOpProgress *mop=NULL);
	DllExport bool AndersonDo (float interp, int selLevel, MeshOpProgress *mop=NULL, DWORD subdivFlags=0);
	DllExport void TessellateByCenters (MeshOpProgress *mop=NULL);

	// Sabin-Doo tessellation: (MNSabDoo.cpp)
	DllExport void SabinDoo (float interp, int selLevel, MeshOpProgress *mop=NULL, Tab<Point3> *offsets=NULL);
	DllExport void SabinDooVert (int vid, float interp, int selLevel, Point3 *ctr, Tab<Point3> *offsets=NULL);

	// Non-uniform Rational Mesh Smooth (NURMS.cpp)
	DllExport void CubicNURMS (MeshOpProgress *mop=NULL,
		Tab<Point3> *offsets=NULL, DWORD subdivFlags=0);

	// Boolean functions: (MNBool.cpp)
	DllExport void PrepForBoolean ();
	DllExport bool BooleanCut (MNMesh & m2, int cutType, int fstart=0, MeshOpProgress *mop=NULL);
	DllExport bool MakeBoolean (MNMesh & m1, MNMesh & m2, int type, MeshOpProgress *mop=NULL);
	DllExport void Connect (MNMeshBorder & borderList, int segs, float tension,
		bool sm_bridge, bool sm_ends, Tab<int> *vsep=NULL);
	DllExport void ConnectLoops (Tab<int> & loop1, Tab<int> & loop2,
		int segs, float tension, DWORD smGroup, MtlID mat, bool sm_ends);

	// Small-scale Operations - in MNOps.cpp
	DllExport void FacePointBary (int ff, Point3 & p, Tab<float> & bary);
	DllExport void CloneVerts (DWORD cloneFlag = MN_SEL, bool clear_orig=TRUE);
	DllExport void CloneFaces (DWORD cloneFlag = MN_SEL, bool clear_orig=TRUE);
	DllExport int DivideFace (int ff, Tab<float> & bary);
	DllExport int CreateFace (int degg, int *vv);
	DllExport bool MakeFlaggedPlanar (int selLev, DWORD flag=MN_SEL, Point3 *delta=NULL);
	DllExport bool MoveVertsToPlane (Point3 & norm, float offset, DWORD flag=MN_SEL, Point3 *delta=NULL);
	DllExport bool SplitFlaggedVertices (DWORD flag=MN_SEL);
	DllExport bool SplitFlaggedEdges (DWORD flag=MN_SEL);
	DllExport bool DetachFaces (DWORD flag=MN_SEL);
	DllExport bool DetachElementToObject (MNMesh & nmesh, DWORD fflags=MN_SEL, bool delDetached=true);
	DllExport bool ExtrudeFaceClusters (MNFaceClusters & fclust);
	DllExport bool ExtrudeFaceCluster (MNFaceClusters & fclust, int cl);
	DllExport bool ExtrudeFaces (DWORD flag=MN_SEL);	// Does each face separately
	// GetExtrudeDirection is based on faces with MN_SEL flag if face clusters are NULL.
	DllExport void GetExtrudeDirection (MNChamferData *mcd,
		MNFaceClusters *fclust=NULL, Point3 *clustNormals=NULL);
	DllExport bool SetVertColor (UVVert clr, int mp, DWORD flag=MN_SEL);
	DllExport bool SetFaceColor (UVVert clr, int mp, DWORD flag=MN_SEL);
	DllExport bool ChamferVertices (DWORD flag=MN_SEL, MNChamferData *mcd=NULL);
	DllExport bool ChamferEdges (DWORD flag=MN_SEL, MNChamferData *mcd=NULL);
	DllExport bool FlipElementNormals (DWORD flag=MN_SEL);
	DllExport void SmoothByCreases (DWORD creaseFlag);
	DllExport int CutFace (int f1, Point3 & p1, Point3 & p2, Point3 & Z, bool split);
	DllExport int CutEdge (int e1, float prop1, int e2, float prop2, Point3 & Z, bool split);
	DllExport int Cut (int startv, Point3 & end, Point3 & Z, bool split);
	DllExport bool WeldBorderVerts (int v1, int v2, Point3 *destination);
	DllExport bool WeldBorderEdges (int e1, int e2);
	DllExport bool WeldBorderVerts (float thresh, DWORD flag=MN_SEL);

	// Pipeline object requirements.  (MNPipe.cpp)
	DllExport void ApplyMapper (UVWMapper & mp, int channel=0, BOOL useSel=FALSE);
	DllExport void InvalidateGeomCache ();
	DllExport void InvalidateTopoCache ();
	DllExport void UpdateDisplayVertexColors ();
	DllExport void SetDisplayVertexColors (int chan);
	DllExport void SetDisplayVertexColors (UVVert *mv, MNMapFace *mf);
	DllExport void PrepForPipeline ();
	DllExport void allocRVerts ();
	DllExport void updateRVerts (GraphicsWindow *gw);
	DllExport void freeRVerts ();

	DllExport void checkNormals (BOOL illum);
	DllExport void buildNormals ();
	DllExport void buildRenderNormals ();
	DllExport void UpdateBackfacing (GraphicsWindow *gw, bool force);

	// Display flags
	void		SetDispFlag(DWORD f) { dispFlags |= f; }
	DWORD		GetDispFlag(DWORD f) { return dispFlags & f; }
	void		ClearDispFlag(DWORD f) { dispFlags &= ~f; }

	DllExport void render(GraphicsWindow *gw, Material *ma, RECT *rp, int compFlags, int numMat=1, InterfaceServer *pi=NULL);
	DllExport void renderFace (GraphicsWindow *gw, int ff);
	DllExport void render3DFace (GraphicsWindow *gw, int ff);
	DllExport void render3DDiagonals (GraphicsWindow *gw, DWORD compFlags);
	DllExport void renderDiagonals (GraphicsWindow *gw, DWORD compFlags);
	DllExport void renderDiagonal (GraphicsWindow *gw, int ff, bool useSegments=false, bool *lastColorSubSel=NULL);
	DllExport void render3DEdges (GraphicsWindow *gw, DWORD compFlags);
	DllExport void renderEdges (GraphicsWindow *gw, DWORD compFlags);
	DllExport void renderEdge (GraphicsWindow *gw, int ee, bool useSegments=false, bool *lastColorSubSel=NULL);
	DllExport BOOL select (GraphicsWindow *gw, Material *ma, HitRegion *hr, int abortOnHit=FALSE, int numMat=1);
	DllExport BOOL 		SubObjectHitTest(GraphicsWindow *gw, Material *ma, HitRegion *hr,
							DWORD flags, SubObjHitList& hitList, int numMat=1 );
	DllExport int IntersectRay (Ray& ray, float& at, Point3& norm);
	DllExport int IntersectRay (Ray& ray, float& at, Point3& norm, int &fi, Tab<float> & bary);
	DllExport BitArray VertexTempSel (DWORD fmask=MN_DEAD|MN_SEL, DWORD fset=MN_SEL);

	DllExport void 		ShallowCopy(MNMesh *amesh, ULONG_PTR channels);
		// WIN64 Cleanup: Shuler
	DllExport void		NewAndCopyChannels(ULONG_PTR channels);
		// WIN64 Cleanup: Shuler
	DllExport void 		FreeChannels (ULONG_PTR channels, BOOL zeroOthers=1);
		// WIN64 Cleanup: Shuler

	DllExport IOResult Save (ISave *isave);
	DllExport IOResult Load (ILoad *iload);
	DllExport void ClearFlag (DWORD fl);

	// --- from InterfaceServer
	DllExport BaseInterface* GetInterface(Interface_ID id);
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

class MNFaceElement {
public:
	// For each face, which element is it in
	Tab<int> elem;
	int count;

	DllExport MNFaceElement (MNMesh &mesh);
	int operator[](int i) {return elem[i];}
};

class MNFaceClusters {
public:
	// Cluster #, one for each face - non-selected faces have -1 for their id.
	Tab<int> clust;
	int count;

	// Makes clusters from distinct selected components.
	DllExport MNFaceClusters (MNMesh & mesh, DWORD clusterFlags);
	// This version separates cluster also using a minimum angle and optionally by flags.
	DllExport MNFaceClusters (MNMesh & mesh, float angle, DWORD clusterFlags);
	int operator[](int i) { return clust[i]; }
	DllExport void MakeVertCluster(MNMesh &mesh, Tab<int> & vclust);
	DllExport void GetNormalsCenters (MNMesh &mesh, Tab<Point3> & norm, Tab<Point3> & ctr);
	DllExport void GetBorder (MNMesh &mesh, int clustID, Tab<int> & cbord);
	DllExport void GetOutlineVectors (MNMesh & m, Tab<Point3> & cnorms, Tab<Point3> & odir);
};

class MNEdgeClusters {
public:
	Tab<int> clust;
	int count;

	DllExport MNEdgeClusters (MNMesh &mesh, DWORD clusterFlags);
	int operator[](int i) {return clust[i];}
	DllExport void MakeVertCluster (MNMesh &mesh, Tab<int> & vclust);
	DllExport void GetNormalsCenters (MNMesh &mesh, Tab<Point3> & norm, Tab<Point3> & ctr);
};

class MNChamferData {
	Tab<UVVert> hmdir[NUM_HIDDENMAPS];
public:
	Tab<Point3> vdir;
	Tab<float> vmax;
	Tab<UVVert> *mdir;

	MNChamferData () { mdir=NULL; }
	MNChamferData (const MNMesh & m) { mdir=NULL; InitToMesh(m); }
	~MNChamferData () { if (mdir) delete [] mdir; }

	DllExport void InitToMesh (const MNMesh & m);
	DllExport void setNumVerts (int nv, bool keep=TRUE, int resizer=0);
	DllExport void ClearLimits ();
	DllExport void GetDelta (float amount, Tab<Point3> & delta);
	DllExport bool GetMapDelta (MNMesh & mm, int mapChannel, float amount, Tab<UVVert> & delta);
	Tab<UVVert> & MDir (int mp) { return (mp<0) ? hmdir[-1-mp] : mdir[mp]; }
};

class MNTempData : public BaseInterfaceServer {
private:
	MNEdgeClusters *edgeCluster;
	MNFaceClusters *faceCluster;
	Tab<int> *vertCluster;
	Tab<Point3> *normals;
	Tab<Point3> *centers;
	Tab<Point3> *vnormals;
	Tab<Tab<float> *> *clustDist;
	Tab<float> *selDist;
	Tab<float> *vsWeight;
	MNChamferData *chamData;

	//Tab<Point3> *extDir;
	Tab<Point3> *outlineDir;

	MNMesh *mesh;

public:
	DllExport MNTempData ();
	DllExport MNTempData (MNMesh *m);
	DllExport ~MNTempData ();

	void SetMesh (MNMesh *m) { mesh = m; }

	DllExport MNFaceClusters *FaceClusters (DWORD clusterFlags=MN_SEL);
	DllExport MNEdgeClusters *EdgeClusters (DWORD clusterFlags=MN_SEL);
	DllExport Tab<int> *VertexClusters (int sl, DWORD clusterFlags=MN_SEL);
	DllExport Tab<Point3> *ClusterNormals (int sl, DWORD clusterFlags=MN_SEL);
	DllExport Tab<Point3> *ClusterCenters (int sl, DWORD clusterFlags=MN_SEL);
	DllExport Matrix3 ClusterTM (int clust);
	DllExport Tab<Point3> *VertexNormals ();
	DllExport Tab<float> *VSWeight (BOOL useEdgeDist, int edgeIts,
									BOOL ignoreBack, float falloff, float pinch, float bubble,
									DWORD selFlags=MN_SEL);
	DllExport Tab<float> *SelectionDist (BOOL useEdgeDist, int edgeIts, DWORD selFlags=MN_SEL);
	DllExport Tab<float> *ClusterDist (int sl, DWORD clusterFlags, int clustId, BOOL useEdgeDist, int edgeIts);
	//DllExport Tab<Point3> *FaceExtDir (int extrusionType);
	DllExport Tab<Point3> *OutlineDir (int extrusionType, DWORD clusterFlags=MN_SEL);

	DllExport MNChamferData *ChamferData();

	DllExport void Invalidate (DWORD part);
	DllExport void InvalidateDistances ();
	DllExport void InvalidateSoftSelection ();
	DllExport void freeClusterDist ();
	DllExport void freeBevelInfo ();
	DllExport void freeChamferData();
	DllExport void freeAll ();
};

DllExport void SelectionDistance (MNMesh & mesh, float *selDist, DWORD selFlags);
DllExport void SelectionDistance (MNMesh & mesh, float *selDist, int iters, DWORD selFlags);
DllExport void ClustDistances (MNMesh & mesh, int numClusts, int *vclust,
							   Tab<float> **clustDist);
DllExport void ClustDistances (MNMesh & mesh, int numClusts, int *vclust,
							   Tab<float> **clustDist, int iters);

#endif
