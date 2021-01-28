// MeshDelta.h
// By Steve Anderson of Kinetix.
// Created June 1998

#ifndef __MESHDELTA__
#define __MESHDELTA__

#include "export.h"
#ifndef __MESHADJ__
#include "meshadj.h"
#endif

// STEVE: MeshDeltas and/or MapDeltas lack the following clever features:
// - Realloc amounts: currently we realloc arrays with no extra room for growth (dv especially).

// These classes encompass the notion of a "Mesh Edit".  They are the principal means
// of keeping track of what's going on in the Edit Mesh modifier, and have many standard
// mesh edits available for use elsewhere.

// Principle is that, while these work as designed on the right mesh, they will give
// some result on the wrong mesh.

// Order of operations:
// Verts/TVerts/CVerts created or cloned.
// Faces created -- indices correspond to original vert list, then create, then clone.
// Face attributes & indices changed.
// Verts & faces deleted.

// "Channels" of data in MeshDelta -- different from PART_GEOM type channels!
#define MDELTA_VMOVE 0x0001
#define MDELTA_VCREATE 0x0002
#define MDELTA_VCLONE 0x0004
#define MDELTA_VDELETE 0x0008
#define MDELTA_VDATA 0x0010
#define MDELTA_FREMAP 0x0020
#define MDELTA_FCHANGE 0x0040
#define MDELTA_FCREATE 0x0080
#define MDELTA_FDELETE 0x0100
#define MDELTA_FDATA 0x0200
#define MDELTA_NUMBERS 0x0400
#define MDELTA_FSMOOTH 0x0800
#define MDELTA_ALL 0xffff

class VertMove {
public:
	DWORD vid;
	Point3 dv;

	VertMove () {}
	VertMove (DWORD i, Point3 p) { vid=i; dv=p; }
	~VertMove () {}
	VertMove & operator= (VertMove & from) { vid=from.vid; dv=from.dv; return (*this); }
};

class UVVertSet {
public:
	DWORD vid;
	UVVert v;

	UVVertSet () {}
	UVVertSet (DWORD i, UVVert p) { vid=i; v=p; }
	~UVVertSet () {}
	UVVertSet & operator= (UVVertSet & from) { vid=from.vid; v=from.v; return (*this); }
};

#define FR_V0	1
#define FR_V1	2
#define FR_V2	4
#define FR_ALL	7

class FaceRemap {
public:
	DWORD f, flags, v[3];	// Face being remapped
	FaceRemap () { f=flags=0; }
	FaceRemap (DWORD ff, DWORD fl, DWORD *vv) { f=ff; flags=fl; memcpy (v,vv,3*sizeof(DWORD)); }
	DllExport void Apply (Face &ff);
	DllExport void Apply (TVFace & tf);
	DllExport void Apply (FaceRemap & fr);
	Face operator* (Face &ff) { Face nf=ff; Apply(nf); return nf; }
	TVFace operator* (TVFace & ff) { TVFace nf=ff; Apply(nf); return nf; }
};

// Attribute changes available for faces:
#define ATTRIB_EDGE_A		(1<<0)
#define ATTRIB_EDGE_B		(1<<1)
#define ATTRIB_EDGE_C		(1<<2)
#define ATTRIB_EDGE_ALL	 7
#define ATTRIB_HIDE_FACE		(1<<3)
#define ATTRIB_MATID		(1<<4)

// Mat ID takes bits 5-21
#define ATTRIB_MATID_SHIFT	5
#define ATTRIB_MATID_MASK	0xffff

class FaceChange {
public:
	DWORD f, flags, val;
	FaceChange () { f=flags=0; }
	FaceChange (DWORD ff, DWORD fl, DWORD v) { f=ff; flags=fl; val=v; }
	DllExport void Apply (Face &ff);
	DllExport void Apply (FaceChange & fa);
};

class FaceSmooth {
public:
	DWORD f, mask, val;
	FaceSmooth () { f=mask=0; }
	FaceSmooth (DWORD ff, DWORD mk, DWORD vl) { f=ff; mask=mk; val=vl; }
	DllExport void Apply (Face &ff);
	DllExport void Apply (FaceSmooth & fs);
};

//STEVE: someday support applying a standard mapping to selected faces?
class MapDelta {
public:
	DWORD vnum, fnum;
	Tab<UVVertSet> vSet;
	Tab<Point3> vCreate;
	Tab<TVFace> fCreate;	// New texture vert faces -- matches master MeshDelta fCreate in size.
	Tab<FaceRemap> fRemap;	// ordered list of faces using at least one new vertex.

	MapDelta () { vnum=0; fnum=0; }
	DllExport void ClearAllOps ();

	// Bookkeeping:
	DllExport int NumVSet (DWORD inVNum);
	DllExport void SetInVNum (DWORD n);
	DllExport void SetInFNum (DWORD n);
	DWORD outVNum () { return vnum + vCreate.Count(); }
	DWORD outVNum (DWORD inVNum) { return inVNum + vCreate.Count(); }
	bool IsCreate (DWORD i) { int j=i-vnum; return ((j>=0) && (j<vCreate.Count())); }
	DllExport DWORD SetID (DWORD i);

	// Topological ops:
	DllExport DWORD VCreate (UVVert *v, int num=1);
	DllExport void FCreate (TVFace *f, int num=1);
	DllExport void FCreateDefault (int num=1);
	DllExport void FCreateQuad (DWORD *t);
	DllExport void FClone (TVFace & tf, DWORD remapFlags=0, DWORD *v=NULL);
	DllExport void FRemap (DWORD f, DWORD flags, DWORD *v);	// Creates a (or modifies an existing) remap record.
	void FRemap (FaceRemap & fr) { FRemap (fr.f, fr.flags, fr.v); }
	DllExport DWORD RemapID (DWORD ff);
	DllExport DWORD IsRemapped (DWORD ff, DWORD vid);
	DllExport TVFace OutFace (TVFace *mf, DWORD ff);
	DllExport void FDelete (int offset, BitArray & fdel);

	// Geometric ops:
	DllExport void Set (DWORD i, const UVVert & p);
	DllExport void Set (BitArray & sel, const UVVert & p);

	// Uses:
	MapDelta & operator=(MapDelta & from) { CopyMDChannels (from, MDELTA_ALL); return *this; }
	DllExport MapDelta & operator*=(MapDelta & from);
	DllExport void Apply (UVVert *tv, TVFace *tf, DWORD inVNum, DWORD inFNum);

	// Handy debugging output
	DllExport void MyDebugPrint ();

	// Backup stuff:
	DllExport DWORD ChangeFlags ();
	DllExport void CopyMDChannels (MapDelta & from, DWORD channels);

	// Double-checking routines, good for after loading.
	// Return TRUE if already correct, FALSE if they had to make a correction.
	DllExport BOOL CheckOrder ();	// Checks for out of order sets or remaps
	DllExport BOOL CheckFaces ();	// Checks remaps & fCreates for out of bound map vert id's.
};

class VDataDelta {
public:
	PerData *out;

	VDataDelta () { out=NULL; }
	~VDataDelta () { if (out) delete out; }
	void SetVNum (int nv, BOOL keep=FALSE) { if (out) out->SetCount(nv,keep); }
	void Activate (int vnum, int vdID) { if (out==NULL) out = new PerData(vnum, VertexDataType(vdID)); }
	DllExport void Set (int where, void *data, int num=1);
};

class MeshDelta {
	DWORD *vlut, *flut;
	int vlutSize, flutSize;

	// Internal methods:
	// Parts of Cut:
	DWORD FindBestNextFace (Mesh & m, Tab<DWORD> *vfac, Point3 *cpv, DWORD startV, Point3 & svP);
	DWORD FindOtherFace (DWORD ff, Tab<DWORD> * vfa, Tab<DWORD> * vfb);
public:
	DWORD vnum, fnum;

	Tab<VertMove> vMove;	// Ordered list of moves to existing verts
	Tab<Point3> vCreate;	// Creation-order list of new points
	Tab<VertMove> vClone;	// Creation-order list of cloned points.
	BitArray vDelete;

	Tab<Face> fCreate;			// New faces
	Tab<FaceRemap> fRemap;	// existing faces using new verts.  (Ordered list.)
	Tab<FaceChange> fChange;	// ordered list of face flag changes
	Tab<FaceSmooth> fSmooth;	// ordered list of face smoothing group changes
	BitArray fDelete;	// Also applies to map faces.

	BitArray vsel, esel, fsel, vhide;	// Based on output mesh indexing.

	MapDelta *map;
	BitArray mapSupport;

	VDataDelta *vd;	// Based on output mesh indexing.
	BitArray vdSupport;

	DllExport MeshDelta ();
	DllExport MeshDelta (const Mesh & m);
	DllExport ~MeshDelta ();

	DllExport void InitToMesh (const Mesh & m);
	DllExport void ClearAllOps ();
	DllExport void SetMapNum (int n, bool keep=TRUE);
	int GetMapNum () { return mapSupport.GetSize(); }
	DllExport void SetVDataNum (int size, bool keep=TRUE);
	int GetVDataNum () { return vdSupport.GetSize(); }
	DllExport DWORD PartsChanged ();

	// The main work of a MeshDelta.
	DllExport void Apply(Mesh& mesh);
	MeshDelta& operator=(MeshDelta& td) { CopyMDChannels (td, MDELTA_ALL); return *this; }
	MeshDelta& operator*=(MeshDelta& td) { Compose(td); return *this; }
	DllExport void Compose (MeshDelta & td, Tab<DWORD> *cloneRemapTable=NULL);

	// Following give numbers of clones or deletions, given the input numbers.
	// (We can't delete vertex 10 on an 8-vert mesh; this counts the number of valid entries.)
	DllExport DWORD NumVMove (DWORD inVNum);
	DllExport DWORD NumVClone (DWORD inVNum);
	DllExport DWORD NumVDelete (DWORD inVNum);
	DllExport DWORD NumFDelete (DWORD inFNum);

	// Sets the size of the input object -- should be used only in multiplying MeshDeltas,
	// since it destroys records of changes to out-of-range components.
	// MeshDelta may be applied to mesh without using these.
	DllExport void SetInFNum (int nface);
	DllExport void SetInVNum (int nv);

	int outVNum () { return vnum + vCreate.Count() + vClone.Count() - vDelete.NumberSet(); }
	int outVNum (int inVNum) { return inVNum + vCreate.Count() + NumVClone(inVNum) - NumVDelete(inVNum); }
	int outFNum () { return fnum + fCreate.Count() - fDelete.NumberSet(); }
	int outFNum (int inFNum) { return inFNum + fCreate.Count() - NumFDelete(inFNum); }

	DllExport void FillInFaces (Mesh & m);	// Fills in undefined mapping face verts.
	DllExport void AddVertexColors ();	// Adds vertex color mapdelta to match this meshdelta.
	DllExport void AddMap (int mapID); // Adds mapdelta on specified channel to match this meshdelta.
	DllExport void AddVertexData (int vdChan, Mesh *m=NULL);

	// Create lookup tables for fast conversion of pre- and post- vert/face indices.
	DllExport void UpdateLUTs (int extraV=0, int extraF=0);
	DllExport void ClearLUTs ();

	// Following methods turn output indices to input indices.
	DllExport DWORD VLut (DWORD i);
	DllExport DWORD FLut (DWORD i);
	// Following methods turn input indices to output indices.
	DllExport DWORD PostVIndex (DWORD i);
	DllExport DWORD PostFIndex (DWORD i);
	// Following operate on output indices
	bool IsVClone (DWORD i) { int j=VLut(i)-vnum-vCreate.Count(); return ((j>=0) && (j<vClone.Count())); }
	DWORD VCloneOf (DWORD i) { int j=VLut(i)-vnum-vCreate.Count(); return ((j>=0) && (j<vClone.Count())) ? vClone[j].vid : UNDEFINED; }
	bool IsVCreate (DWORD i) { int j=VLut(i)-vnum; return ((j>=0) && (j<vCreate.Count())); }
	bool IsFCreate (DWORD i) { int j=FLut(i)-fnum; return ((j>=0) && (j<fCreate.Count())); }
	DllExport DWORD MoveID (DWORD i);

	// Basic topological operations:
	// Those that accept DWORD indices require post-operation indices.
	DllExport DWORD VCreate (Point3 *p, int num=1, BitArray *sel=NULL, BitArray *hide=NULL);
	DllExport DWORD VClone (DWORD *v, int num=1);
	DllExport DWORD VClone (DWORD *v, Point3 *off, int num=1);
	DllExport DWORD VClone (VertMove *vm, int num=1);
	DWORD VClone (DWORD v) { return VClone (&v, 1); }
	DWORD VClone (DWORD v, Point3 off) { return VClone (&v, &off, 1); }
	DllExport Point3 OutVert (Mesh & m, DWORD v);
	DllExport void VDelete (DWORD *v, int num=1);
	DllExport void VDelete (BitArray & vdel);
	DllExport DWORD FCreate (Face *f, int num=1);
	DllExport DWORD FCreateQuad (DWORD *v, DWORD smG=0, MtlID matID=0);
	DllExport DWORD FClone (Face & f, DWORD ff, DWORD remapFlags=0, DWORD *v=NULL);
	DllExport void CreateDefaultMapFaces (int num=1);
	DllExport void FRemap (FaceRemap *f, int num=1);
	DllExport void FRemap (DWORD f, DWORD flags, DWORD *v);
	DllExport DWORD RemapID (DWORD ff);
	DllExport DWORD IsRemapped (DWORD ff, DWORD vid);
	DllExport Face OutFace (Mesh & m, DWORD ff);
	DllExport void FChange (FaceChange *f, int num=1);
	DllExport void FChange (DWORD f, DWORD flags, DWORD dat);
	DllExport void FSmooth (FaceSmooth *f, int num=1);
	DllExport void FSmooth (DWORD f, DWORD mask, DWORD val);
	void SetMatID (DWORD f, MtlID mt) { FChange (f, ATTRIB_MATID, mt<<ATTRIB_MATID_SHIFT); }
	void SetSmGroup (DWORD f, DWORD smG) { FSmooth (f, ~0x0, smG); }
	void SetEdgeVis (DWORD f, DWORD ed, BOOL vis=TRUE) { FChange (f, (1<<ed), vis?(1<<ed):0); }
	DllExport void FDelete (DWORD *f, int num=1);
	DllExport void FDelete (BitArray & fdel);

	// Following method will turn vertex clones-of-creates into separate creates, fixing rest of MeshDelta to match.
	DllExport void DigestClones (Tab<DWORD> *cloneRemapTable=NULL);

	// Geometric ops:
	DllExport void Move (int i, const Point3 & p);
	DllExport void Move (BitArray & sel, const Point3 & p);
	DllExport void Move (VertMove *vm, int num);

	// Gotta be able to save and load this complex thing...
	DllExport IOResult Save (ISave *isave);
	DllExport IOResult Load (ILoad *iload);

	// Handy debugging output
	DllExport void MyDebugPrint (bool lut=FALSE, bool mp=FALSE);

	// Backup-relevant characteristics:
	DllExport DWORD ChangeFlags (Tab<DWORD> *mChannels=NULL);
	DllExport void CopyMDChannels (MeshDelta & from, DWORD channels, Tab<DWORD> *mChannels=NULL);

	// Double-checking routines, good for after loading.
	// Returns TRUE if order was already correct, FALSE if it had to make a correction.
	DllExport BOOL CheckOrder ();
	DllExport BOOL CheckMapFaces ();

	// More complex operations, built on the list above.
	// Mesh given is expected to be result of the current MeshDelta.
	// Found in MDAppOps.cpp
	DllExport void AutoSmooth(Mesh &m, BitArray sel, float angle, AdjFaceList *af=NULL, AdjEdgeList *ae=NULL);
	DllExport void Bevel (Mesh & m, BitArray vset, float outline, Tab<Point3> *odir,
		float height, Tab<Point3> *hdir);
	DllExport DWORD CreatePolygon (Mesh & m, int deg, int *v, DWORD smG=0, MtlID matID=0);
	DllExport void DeleteVertSet (Mesh & m, BitArray sel);	// does delete faces
	DllExport void DeleteEdgeSet (Mesh & m, BitArray sel);	// doesn't delete verts
	DllExport void DeleteFaceSet (Mesh & m, BitArray sel);	// doesn't delete verts.
	DllExport void DeleteSelected (Mesh & m);
	DllExport void DeleteIsoVerts (Mesh & m);
	DllExport void FlipNormal (Mesh & m, DWORD face);
	DllExport void MakeSelFacesPlanar (Mesh &m, BitArray sel);
	DllExport void MakeSelVertsPlanar (Mesh &m, BitArray sel);
	DllExport void MoveVertsToPlane (Mesh & m, BitArray sel, Point3 & N, float offset);
	DllExport void RestrictMatIDs (Mesh & m, int numMats);	// like "FitMeshIDsToMaterial".
	DllExport void SelectFacesByFlags (Mesh & m, BOOL onoff, DWORD flagmask, DWORD flags);
	// if adj is non-NULL, it uses it to set the "other side" visible too.
	DllExport void SetSingleEdgeVis (Mesh & m, DWORD ed, BOOL vis, AdjFaceList *adj=NULL);

	// Following will initialize to the mesh given: they can't be used to "add" ops to an existing MeshDelta.
	// (To add these ops, make a new MeshDelta, call one of the following, and append it to your previous one with Compose.)
	// Found in MDOps.cpp
	DllExport void AttachMesh (Mesh & m, Mesh &attachment, Matrix3 & relativeTransform,
		int matOffset);
	DllExport void BreakVerts (Mesh & m, BitArray vset);
	DllExport void ChamferEdges (Mesh & m, BitArray eset, MeshChamferData &mcd, AdjEdgeList *ae=NULL);
	DllExport void ChamferMove (Mesh & m, MeshChamferData &mcd, float amount, AdjEdgeList *ae=NULL);
	DllExport void ChamferVertices (Mesh & m, BitArray vset, MeshChamferData &mcd, AdjEdgeList *ae=NULL);
	DllExport void CloneFaces (Mesh & m, BitArray fset);
	DllExport void CloneVerts (Mesh & m, BitArray vset);
	DllExport void CollapseEdges(Mesh &m, BitArray ecol, AdjEdgeList *ae=NULL);
	DllExport DWORD Cut (Mesh & m, DWORD ed1, float prop1, DWORD ed2, float prop2,
		Point3 & norm, bool fixNeighbors=TRUE, bool split=FALSE);
	DllExport void Detach (Mesh & m, Mesh *out, BitArray fset, BOOL faces, BOOL del, BOOL elem);
	DllExport void DivideEdge (Mesh & m, DWORD ed, float prop=.5f, AdjEdgeList *el=NULL,
		bool visDiag1=FALSE, bool fixNeighbors=TRUE, bool visDiag2=FALSE, bool split=FALSE);
	DllExport void DivideEdges (Mesh & m, BitArray eset, AdjEdgeList *el=NULL);
	DllExport void DivideFace (Mesh & m, DWORD f, float *bary=NULL);
	DllExport void DivideFaces (Mesh & m, BitArray fset, MeshOpProgress *mop=NULL);
	DllExport void EdgeTessellate(Mesh &m, BitArray fset, float tens,
		AdjEdgeList *ae=NULL, AdjFaceList *af=NULL, MeshOpProgress *mop=NULL);
	DllExport void ExplodeFaces(Mesh &m, float thresh, bool useFaceSel=FALSE, AdjFaceList *af=NULL);
	DllExport void ExtrudeEdges (Mesh & m, BitArray eset, Tab<Point3> *edir=NULL);
	DllExport void ExtrudeFaces (Mesh & m, BitArray fset, AdjEdgeList *el=NULL);
	DllExport void ResetVertWeights (Mesh & m);
	DllExport void SetFaceColors (Mesh &m, BitArray fset, VertColor vc);
	DllExport void SetVertColors (Mesh &m, BitArray vset, VertColor vc);
	DllExport void SetVertWeights (Mesh &m, BitArray vset, float weight);
	DllExport DWORD TurnEdge (Mesh & m, DWORD ed, AdjEdgeList *el=NULL);
	DllExport BOOL WeldByThreshold (Mesh & m, BitArray vset, float thresh);
	DllExport void WeldVertSet (Mesh & m, BitArray vset, Point3 *weldPoint=NULL);
	DllExport void PropagateFacing (Mesh & m, BitArray & fset, int face,
		AdjFaceList &af, BitArray &done,BOOL bias=1);
	DllExport void UnifyNormals (Mesh & m, BitArray fset, AdjFaceList *af=NULL);

	// In slicer.cpp:
	DllExport void Slice (Mesh & m, Point3 N, float off, bool sep=FALSE, bool remove=FALSE, BitArray *fslice=NULL, AdjEdgeList *ae=NULL);
};

// Following classes provide standard interface for modifiers and objects that
// use mesh deltas -- specifically Edit Mesh and Editable Mesh for now.

enum meshCommandMode { McmCreate, McmAttach, McmExtrude, McmBevel, McmChamfer,
		McmSlicePlane, McmCut, McmWeldTarget, McmFlipNormalMode, McmDivide, McmTurnEdge };
enum meshButtonOp    { MopHide, MopUnhideAll, MopDelete, MopDetach, MopBreak, MopViewAlign,
		MopGridAlign, MopMakePlanar, MopCollapse, MopTessellate, MopExplode, MopSlice, MopWeld,
		MopRemoveIsolatedVerts, MopSelectOpenEdges, MopCreateShapeFromEdges, MopShowNormal,
		MopFlipNormal, MopUnifyNormal, MopAutoSmooth, MopVisibleEdge, MopInvisibleEdge, MopAutoEdge };
enum meshUIParam { MuiSelByVert, MuiIgBack, MuiIgnoreVis, MuiPolyThresh, MuiSoftSel,
		MuiSSUseEDist, MuiSSEDist, MuiSSBack, MuiFalloff, MuiPinch, MuiBubble,
		MuiWeldBoxSize, MuiWeldDist, MuiExtrudeType };

#define EM_SL_OBJECT 0
#define EM_SL_VERTEX 1
#define EM_SL_EDGE 2
#define EM_SL_FACE 3
#define EM_SL_POLYGON 4
#define EM_SL_ELEMENT 5

class MeshDeltaUser {
public:
	virtual void LocalDataChanged (DWORD parts)=0;
	// start or stop interactive command mode, uses mode enum above
	virtual void ToggleCommandMode (meshCommandMode mode)=0;
	// perform button op, uses op enum above
	virtual void ButtonOp (meshButtonOp opcode)=0;

	// UI controls access
	virtual void GetUIParam (meshUIParam uiCode, int & ret) { }
	virtual void SetUIParam (meshUIParam uiCode, int val) { }
	virtual void GetUIParam (meshUIParam uiCode, float & ret) { }
	virtual void SetUIParam (meshUIParam uiCode, float val) { }
	virtual void UpdateApproxUI () { }

	// Should work on any local command mode.
	virtual void ExitCommandModes ()=0;

	virtual bool Editing () { return FALSE; }	// returns TRUE iff between BeginEditParams, EndEditParams
	virtual DWORD GetEMeshSelLevel () { return EM_SL_OBJECT; }
	virtual void SetEMeshSelLevel (DWORD sl) { }
};

class MeshDeltaUserData {
public:
	virtual void ApplyMeshDelta (MeshDelta & md, MeshDeltaUser *mdu, TimeValue t)=0;
	virtual MeshDelta *GetCurrentMDState () { return NULL; }	// only non-null in Edit Mesh
	// functional interface to mesh ops
	virtual void MoveSelection(int level, TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)=0;
	virtual void RotateSelection(int level, TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin)=0;
	virtual void ScaleSelection(int level, TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)=0;
	virtual void ExtrudeSelection(int level, BitArray* sel, float amount, float bevel, BOOL groupNormal, Point3* direction)=0;
};

// Constants used in Edit(able) Mesh's shortcut table - THESE MUST BE MATCHED to values in Editable Mesh's resources.
#define EM_SHORTCUT_ID 0x38ba1366

#define MDUID_EM_SELTYPE                   40001
#define MDUID_EM_SELTYPE_BACK              40002
#define MDUID_EM_SELTYPE_VERTEX            40003
#define MDUID_EM_SELTYPE_EDGE              40004
#define MDUID_EM_SELTYPE_FACE              40005
#define MDUID_EM_SELTYPE_POLYGON           40006
#define MDUID_EM_SELTYPE_ELEMENT           40007
#define MDUID_EM_SELTYPE_OBJ               40008
#define MDUID_EM_AUTOSMOOTH                40009
#define MDUID_EM_ATTACH                    40010
#define MDUID_EM_BREAK                     40011
#define MDUID_EM_IGBACK                    40012
#define MDUID_EM_BEVEL                     40013
#define MDUID_EM_CREATE                    40014
#define MDUID_EM_CUT                       40015
#define MDUID_EM_DIVIDE                    40016
#define MDUID_EM_EXTRUDE                   40017
#define MDUID_EM_FLIPNORM                  40018
#define MDUID_EM_SS_BACKFACE               40019
#define MDUID_EM_UNIFY_NORMALS             40020
#define MDUID_EM_HIDE                      40021
#define MDUID_EM_EDGE_INVIS                40022
#define MDUID_EM_IGNORE_INVIS                 40023
#define MDUID_EM_IGNORE_INVIS              40023
#define MDUID_EM_COLLAPSE                  40024
#define MDUID_EM_SHOWNORMAL                40025
#define MDUID_EM_SELOPEN                   40026
#define MDUID_EM_REMOVE_ISO                40027
#define MDUID_EM_SLICEPLANE                40028
#define MDUID_EM_SOFTSEL                   40029
#define MDUID_EM_SLICE                     40030
#define MDUID_EM_DETACH                    40031
#define MDUID_EM_TURNEDGE                  40032
#define MDUID_EM_UNHIDE                    40033
#define MDUID_EM_EDGE_VIS                  40034
#define MDUID_EM_SELBYVERT                 40035
#define MDUID_EM_AUTOEDGE                  40036
#define MDUID_EM_WELD                      40038
#define MDUID_EM_EXPLODE                   40039

DllExport void FindTriangulation (Mesh & m, int deg, int *vv, int *tri);


#endif
