/**********************************************************************
 *<
	FILE: mesh.h

	DESCRIPTION: Main include file for triangle meshes.

	CREATED BY: Don Brittain

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _MESH_H_

#define _MESH_H_

#include "channels.h"
#include "snap.h"
#include <ioapi.h>
#include "export.h"
#include "vedge.h"  //DS
#include "utillib.h"
#include "tab.h"

#if 0	// moved to gfx.h
typedef Tab<DWORD> DWTab;
typedef unsigned short MtlID;
#endif

// This a UV coordinate. These cound be Point2s
typedef Point3 UVVert;

// We may want these to by 24bit RGBs instead of floats
typedef Point3 VertColor;


#define MESH_MULTI_PROCESSING	TRUE		// TRUE turns on mp vertex transformation

class ISave;
class ILoad;

#define NEWMESH

class RNormal {
	public:
		RNormal()	{ smGroup = mtlIndex = 0; }
		void		setNormal(const Point3 &nor) { normal = nor; }
		void		addNormal(const Point3 &nor) { normal += nor; }	
		void		normalize(void) 	{ normal = Normalize(normal); }
		Point3 &	getNormal(void) 	{ return normal; }
		void		setSmGroup(DWORD g)	{ smGroup = g; }
		void		addSmGroup(DWORD g) { smGroup |= g; }
		DWORD		getSmGroup(void)	{ return smGroup; }
		void		setMtlIndex(MtlID i){ mtlIndex = i; }
		MtlID		getMtlIndex(void)	{ return mtlIndex; }
		void		setRGB(Point3 &clr)	{ rgb = clr; };
		Point3 &	getRGB(void)		{ return rgb; }
		
	private:	
		Point3		normal;	   
		DWORD		smGroup;    
		MtlID		mtlIndex;   
		Point3		rgb;	   
	};					   



// RVertex flags: contain clip flags, number of normals at the vertex
// and the number of normals that have already been rendered.
// fine PLANE_MASK	0x00003f00UL -- now in gfx.h
#define NORCT_MASK			0x000000ffUL
#define SPECIFIED_NORMAL	0x00004000UL
#define OUT_LEFT			0x00010000UL
#define OUT_RIGHT			0x00020000UL
#define OUT_TOP				0x00040000UL
#define OUT_BOTTOM			0x00080000UL
#define RECT_MASK			0x000f0000UL
#define RND_MASK			0xfff00000UL
#define RND_NOR0			0x00100000UL
#define RND_NOR(n)  		(RND_NOR0 << (n))

class RVertex {
	public:
		RVertex()	{ rFlags = 0; ern = NULL; }
		DllExport ~RVertex();	

		DWORD		rFlags;     
		int			pos[3];	
		RNormal		rn;		   
		RNormal 	*ern;		 
	};					  



// Face Flags:
// 		3 LSBs hold the edge visibility flags
// 		Bit 3 indicates the presence of texture verticies

// if bit is 1, edge is visible
#define EDGE_VIS			1
#define EDGE_INVIS			0

// first edge-visibility bit field
#define VIS_BIT				0x0001
#define VIS_MASK			0x0007

#define EDGE_A		(1<<0)
#define EDGE_B		(1<<1)
#define EDGE_C		(1<<2)
#define EDGE_ALL	(EDGE_A|EDGE_B|EDGE_C)

#define FACE_HIDDEN	(1<<3)
#define HAS_TVERTS	(1<<4)
#define FACE_WORK	(1<<5) // used in various algorithms
#define FACE_STRIP	(1<<6)

// The mat ID is stored in the HIWORD of the face flags
#define FACE_MATID_SHIFT	16
#define FACE_MATID_MASK		0xFFFF


class Face {	
	public:
		DWORD	v[3];
		DWORD	smGroup;
		DWORD	flags;

		Face()	{ smGroup = flags = 0; }
		MtlID	getMatID() {return (int)((flags>>FACE_MATID_SHIFT)&FACE_MATID_MASK);}
		void    setMatID(MtlID id) {flags &= 0xFFFF; flags |= (DWORD)(id<<FACE_MATID_SHIFT);}
		void	setSmGroup(DWORD i) { smGroup = i; }
		DWORD	getSmGroup(void)	{ return smGroup; }
		void	setVerts(DWORD *vrt){ memcpy(v, vrt, 3*sizeof(DWORD)); }
		void	setVerts(int a, int b, int c)  { v[0]=a; v[1]=b; v[2]=c; }
		DllExport void	setEdgeVis(int edge, int visFlag);
		DllExport void    setEdgeVisFlags(int va, int vb, int vc); 
		int		getEdgeVis(int edge){ return flags & (VIS_BIT << edge); }
		DWORD	getVert(int index)	{ return v[index]; }
		DWORD *	getAllVerts(void)	{ return v; }
		BOOL	Hidden() {return flags&FACE_HIDDEN?TRUE:FALSE;}
		void	Hide() {flags|=FACE_HIDDEN;}
		void	Show() {flags&=~FACE_HIDDEN;}
		void	SetHide(BOOL hide) {if (hide) Hide(); else Show();}

		DllExport DWORD GetOtherIndex (DWORD v0, DWORD v1);
		DllExport DWORD GetEdgeIndex (DWORD v0, DWORD v1);
		DllExport int Direction (DWORD v0, DWORD v1);
		DllExport DWORD GetVertIndex (DWORD v0);
		DllExport void OrderVerts (DWORD & v0, DWORD & v1);	// switches v0,v1 if needed to put them in face-order.
	};


// This is used both for UVWs and color verts
class TVFace {
public:
	DWORD	t[3];  // indices into tVerts

	TVFace() {}
	TVFace(DWORD a, DWORD b, DWORD c) {t[0]=a; t[1]=b; t[2]=c;}
	void	setTVerts(DWORD *vrt){ memcpy(t, vrt, 3*sizeof(DWORD)); }
	void	setTVerts(int a, int b, int c)  { t[0]=a; t[1]=b; t[2]=c; }	
	DWORD	getTVert(int index)	{ return t[index]; }
	DWORD *	getAllTVerts(void)	{ return t; }

	DllExport DWORD GetVertIndex (DWORD v0);
	DllExport DWORD GetOtherIndex (DWORD v0, DWORD v1);
	DllExport int Direction (DWORD v0, DWORD v1);
	DllExport void OrderVerts (DWORD & v0, DWORD & v1);	// switches v0,v1 if needed to put them in face-order.
};


// MeshMap stuff:

#define MAX_MESHMAPS 100

#define MESHMAP_USED 0x0001
#define MESHMAP_TEXTURE 0x0002
#define MESHMAP_VERTCOLOR 0x0004
#define MESHMAP_USER 0x0100

class MeshMap {
public:
	DWORD flags;
	UVVert *tv;
	TVFace *tf;
	int vnum, fnum;

	MeshMap () { flags=0x0; tv=NULL; tf=NULL; vnum = fnum = 0; }
	~MeshMap () { if (tv) delete [] tv; if (tf) delete [] tf; }

	int getNumVerts () { return vnum; }
	DllExport void setNumVerts (int vn, BOOL keep=FALSE);
	int getNumFaces () { return fnum; }
	DllExport void setNumFaces (int fn, BOOL keep=FALSE, int oldCt=0);
	void Clear () { if (tv) delete [] tv; if (tf) delete [] tf; tv=NULL; tf=NULL; vnum = fnum = 0; }
	DllExport BitArray GetIsoVerts ();
	DllExport void DeleteVertSet (BitArray set, BitArray *delFace=NULL);
	DllExport void DeleteFaceSet (BitArray set, BitArray *isoVert=NULL);

	void SetFlag (DWORD fl) { flags |= fl; }
	void ClearFlag (DWORD fl) { flags &= ~fl; }
	BOOL GetFlag (DWORD fl) { return (flags & fl) ? TRUE : FALSE; }
	BOOL IsUsed () { return (flags & MESHMAP_USED) ? TRUE : FALSE; }

	DllExport void SwapContents (MeshMap & from);
	DllExport MeshMap & operator= (MeshMap & from);
};


// Following is used for arbitrary per-element info in meshes, such as weighted verts
// or weighted vert selections.  Methods are deliberately made to look like Tab<> methods.

// For per-vertex info: set a maximum, and reserve first ten channels
// for Discreet's use only.
#define MAX_VERTDATA 100
#define VDATA_USER 10	// Third parties should use this channel or higher.

// Indices of important per-vertex data
#define VDATA_SELECT  0   // Soft Selection
#define VDATA_WEIGHT  1   // Vertex weights (for NURMS MeshSmooth)
#define VDATA_ALPHA   2   // Vertex Alpha values

// Related constants:
#define MAX_WEIGHT ((float)1e5)
#define MIN_WEIGHT ((float)1e-5)

// Types of data
#define PERDATA_TYPE_FLOAT 0

// Vertex-specific methods:
DllExport int VertexDataType (int vdID);
DllExport void *VertexDataDefault (int vdID);

class PerData {
public:
	int dnum, type, alloc;
	void *data;

	PerData () { data=NULL; dnum=0; alloc=0; type=0; }
	PerData (int n, int tp) { data=NULL; dnum=0; alloc=0; type=tp; setAlloc (n, FALSE); }
	~PerData () { Clear (); }

	// Following only depend on type:
	DllExport void *AllocData (int num);
	DllExport void FreeData (void *addr);
	DllExport int DataSize ();
	void *Addr (void *ptr, int at) { BYTE *vd=(BYTE *)ptr; return (void *)(vd+at*DataSize()); }
	void *Addr (int at) { return Addr(data,at); }
	DllExport void CopyData (void *to, void *from, int num=1);
	void CopyData ( int to,  int from, int num=1) { CopyData (Addr(to), Addr(from), num); }
	DllExport void WeightedSum (void *to, void *fr1, float prop1, void *fr2, float prop2);
	void WeightedSum (int to, int fr1, float prop1, int fr2, float prop2) { WeightedSum (Addr(to), Addr(fr1), prop1, Addr(fr2), prop2); }

	DllExport void setAlloc (int num, BOOL keep=TRUE);
	void SetCount (int num, BOOL keep = FALSE) { setAlloc (num, keep); dnum=num; }
	void Shrink () { if (alloc>dnum) setAlloc(dnum); }
	int Count () { return dnum; }
	DllExport void Clear ();
	DllExport void DeleteSet (BitArray del);
	DllExport void Delete (int at, int num);
	DllExport void Insert (int at, int num, void *el);
	DllExport void Append (int num, void *el);
	DllExport void InsertCopies (int at, int num, void *el);
	DllExport void AppendCopies (int num, void *el);

	DllExport void SwapContents (PerData & from);
	DllExport PerData & operator= (const PerData & from);
	DllExport void MyDebugPrint ();
};

// Mesh::flags definitions
#define MESH_EDGE_LIST     (1<<1)
// Set this to prevent renderData from being deleted (except when mesh is deleted)
#define MESH_LOCK_RENDDATA (1<<2)
#define MESH_SMOOTH_BIT1   (1<<3)
#define MESH_SMOOTH_BIT2   (1<<4)
#define MESH_SMOOTH_BIT3   (1<<5)
#define MESH_SMOOTH_BIT4   (1<<6)
#define MESH_SMOOTH_MASK   0x78		// mask for SMOOTH_BIT's 1 thru 4
#define MESH_BEEN_DSP	   (1<<9)
#define MESH_SMOOTH_SUBSEL (1<<10)

#define COMP_TRANSFORM	0x0001	// forces recalc of model->screen transform; else will attempt to use cache
#define COMP_IGN_RECT	0x0002	// forces all polys to be rendered; else only those intersecting the box will be
#define COMP_LIGHTING	0x0004	// forces re-lighting of all verts (as when a light moves); else only relight moved verts

#define COMP_ALL		0x00ff

// If this bit is set then the node being displayed by this mesh is selected.
// Certain display flags only activate when this bit is set.
#define COMP_OBJSELECTED	(1<<8)

#if 0	// moved to gfx.h

// strip-related stuff

class Strip {
public:
	MtlID	mID;
	DWORD	smGrp;
	DWTab	v;
	void AddVert(DWORD vtx)	{ v.Append(1, &vtx); }
	DWTab	tv;
	void AddVert(DWORD vtx, DWORD tvtx) { v.Append(1, &vtx); tv.Append(1, &tvtx); }
};

typedef Strip *StripPtr;
typedef Tab<StripPtr> StripTab;

#endif

class StripData {
public:
	int ct;
	DWORD f[6];
	void AddFace(DWORD face)
		{ if(ct < 6) f[ct++] = face; }
};

typedef int (*INTRFUNC)();

DllExport void setMeshIntrFunc(INTRFUNC fn);


class MeshSubHitRec {
	private:		
		MeshSubHitRec *next;
	public:
		DWORD	dist;
		int		index;
		DWORD	flags;

		MeshSubHitRec(DWORD dist, int index, MeshSubHitRec *next) 
			{this->dist = dist; this->index = index; this->next = next;}
		MeshSubHitRec(DWORD dist, int index, DWORD flags, MeshSubHitRec *next) 
			{this->dist = dist; this->index = index; this->next = next;this->flags = flags;}

		MeshSubHitRec *Next() { return next; }		
	};

class SubObjHitList {
	private:
		MeshSubHitRec *first;
	public:
		SubObjHitList() { first = NULL; }
		~SubObjHitList() {
			MeshSubHitRec *ptr = first, *fptr;
			while (ptr) {
				fptr = ptr;
				ptr = ptr->Next();
				delete fptr;
				}
			first = NULL;
			}	

		MeshSubHitRec *First() { return first; }
		void AddHit( DWORD dist, int index ) {
			first = new MeshSubHitRec(dist,index,first);
			}
	};



// Flags for sub object hit test

// NOTE: these are the same bits used for object level.
#define SUBHIT_SELONLY		(1<<0)
#define SUBHIT_UNSELONLY	(1<<2)
#define SUBHIT_ABORTONHIT	(1<<3)
#define SUBHIT_SELSOLID		(1<<4)

#define SUBHIT_USEFACESEL	(1<<23)   // When this bit is set, the sel only and unsel only tests will use the faces selection when doing a vertex level hit test
#define SUBHIT_VERTS		(1<<24)
#define SUBHIT_FACES		(1<<25)
#define SUBHIT_EDGES		(1<<26)
#define SUBHIT_TYPEMASK		(SUBHIT_VERTS|SUBHIT_FACES|SUBHIT_EDGES)


// Display flags
#define DISP_VERTTICKS		(1<<0)
#define DISP_SELVERTS		(1<<10)
#define DISP_SELFACES		(1<<11)
#define DISP_SELEDGES		(1<<12)
#define DISP_SELPOLYS		(1<<13)

// Selection level bits.
#define MESH_OBJECT		(1<<0)
#define MESH_VERTEX		(1<<1)
#define MESH_FACE		(1<<2)
#define MESH_EDGE		(1<<3)


class MeshOpProgress;
class UVWMapper;

class MeshRenderData {
	public:
	virtual void DeleteThis()=0;
	};

class AdjFaceList;

class Mesh {
	friend class Face;
	friend class MeshAccess;
	friend void gfxCleanup(void *data);

	private:
#if MESH_MULTI_PROCESSING
		static int		refCount;
		static HANDLE	xfmThread;
		static HANDLE	xfmMutex;
		static HANDLE	xfmStartEvent;
		static HANDLE	xfmEndEvent;
		friend DWORD WINAPI xfmFunc(LPVOID ptr);
		static HANDLE	fNorThread;
		static HANDLE	fNorMutex;
		static HANDLE	fNorStartEvent;
		static HANDLE	fNorEndEvent;
		friend DWORD WINAPI fNorFunc(LPVOID ptr);

		static HANDLE	workThread;
		static HANDLE	workMutex;
		static HANDLE	workStartEvent;
		static HANDLE	workEndEvent;
		friend DWORD WINAPI workFunc(LPVOID ptr);
#endif
		// derived data-- can be regenerated
		RVertex 		*rVerts;		// <<< instance specific.
		GraphicsWindow 	*cacheGW;  		// identifies rVerts cache
		Point3	 		*faceNormal;	// object space--depends on geom+topo
		Box3			bdgBox;			// object space--depends on geom+topo
		int 			numVisEdges;	// depends on topo 
		int				edgeListHasAll;	// depends on topo
		VEdge 			*visEdge;		// depends on topo 	
 
 		// Vertex and face work arrays -- for snap code
		int			snapVCt;
		int			snapFCt;
		char		*snapV;
		char		*snapF;

		// -------------------------------------
		//
		long   		flags;		  	// work flags- 

		float 		norScale;	    // scale of normals -- couldn't this be done
		 							// automatically relative to bdgBox?

		// Rolf: these are instance specific and should be pulled out of here,
		// and just passed in from the Node.
		BYTE		dspNormals;    // display surface normals--- put in flags?
		BYTE		dspAllEdges;   // shows hidden edges  ---- put in flags?
		BYTE		dspVertTicks;  // shows vertex ticks

		int 		renderFace(GraphicsWindow *gw, DWORD index, int *custVis=NULL);
		int			renderEdge(GraphicsWindow *gw, DWORD face, DWORD edge);
		void		renderStrip(GraphicsWindow *gw, Strip *s);
		void 		render3DFace(GraphicsWindow *gw, DWORD index, int *custVis=NULL);
		void		render3DStrip(GraphicsWindow *gw, Strip *s);
		void		render3DWireStrip(GraphicsWindow *gw, Strip *s);
		BOOL		CanDrawStrips(DWORD rndMode, Material *mtl, int numMtls);
		BOOL		NormalsMatchVerts();
		void		checkRVertsAlloc(void);
		void  		calcNormal(int i);
		void		buildFaceNormals();		// calcs just the face normals
		void		setCacheGW(GraphicsWindow *gw)	{ cacheGW = gw; }
		GraphicsWindow *getCacheGW(void)			{ return cacheGW; }

		void 		freeVerts();
		void        freeVertCol();
		void  		freeFaces();
		void		freeFaceNormals();  
		void  		freeRVerts(BOOL forceDelete=FALSE);
		void  		freeTVerts();
		void  		freeTVFaces();
		void  		freeVCFaces();
		void		freeSnapData();
		int			buildSnapData(GraphicsWindow *gw,int verts,int edges);

	public:
		// Topology
		int			numVerts;
		int	 		numFaces;
		Face *		faces;

		// Geometry
		Point3 *	verts;		

		// Texture Coord assignment 
		int			numTVerts;
		UVVert *	tVerts;
		TVFace *	tvFace;  	 

		// Color per vertex
		int numCVerts;
		VertColor * vertCol;
		TVFace *    vcFace;

		// More maps:
		int numMaps;
		MeshMap *maps;

		// Per-vertex info (of any kind):
		BitArray vdSupport;
		PerData *vData;

		// Material assignment
		MtlID		mtlIndex;		// object material

		// Selection
		BitArray	vertSel;  		// selected vertices
		BitArray	faceSel;  		// selected faces
		BitArray	edgeSel;		// selected edges, identified as 3*faceIndex + edgeIndex
		BitArray	vertHide;		// Hide flags for vertices

		// Display attribute flags
		DWORD		dispFlags;

		// Selection level
		DWORD		selLevel;

		// true if normals have been built for the current mesh
		int			normalsBuilt;

		MeshRenderData*	 renderData;  // used by the renderer

		// derived data-- can be regenerated
		StripTab		*stab;		// depends on topo
#ifdef PUT_NORMALS_IN_STRIPS
		DWTab		norInd;			// indirection array for fast normal lookup
		int			normalCount;	// total number of normals
		Point3 *	gfxNormals;		// flattened list of normals
#endif

		DllExport Mesh();
		DllExport Mesh(const Mesh& fromMesh);
		DllExport ~Mesh(); 
		void 		Init();
		DllExport void DeleteThis();

		DllExport Mesh& operator=(const Mesh& fromMesh);
		
		DllExport BOOL 	setNumVerts(int ct, BOOL keep=FALSE, BOOL synchSel=TRUE);
		int				getNumVerts(void) const	{ return numVerts; }
		
		DllExport BOOL	setNumFaces(int ct, BOOL keep=FALSE, BOOL synchSel=TRUE);
		int				getNumFaces(void) const{ return numFaces; }
		
		// Original mapping coordinates (map channel 1)
		DllExport BOOL	setNumTVerts(int ct, BOOL keep=FALSE);
		int				getNumTVerts(void) const { return numTVerts; }
		DllExport BOOL 	setNumTVFaces(int ct, BOOL keep=FALSE, int oldCt=0);

		// Color per vertex array (map channel 0)
		DllExport BOOL 	setNumVertCol(int ct,BOOL keep=FALSE);
		int             getNumVertCol() const {return numCVerts;}
		DllExport BOOL 	setNumVCFaces(int ct, BOOL keep=FALSE, int oldCt=0);

		// For mp in following: 0=vert colors, 1=original TVerts, 2&up = new map channels
		DllExport void setNumMaps (int ct, BOOL keep=FALSE);
		int getNumMaps () const { return numMaps; }

		DllExport BOOL mapSupport (int mp) const;
		DllExport void setMapSupport (int mp, BOOL support=TRUE);
		DllExport void setNumMapVerts (int mp, int ct, BOOL keep=FALSE);
		DllExport int getNumMapVerts (int mp) const;
		DllExport void setNumMapFaces (int mp, int ct, BOOL keep=FALSE, int oldCt=0);

		DllExport UVVert *mapVerts (int mp) const;
		DllExport TVFace *mapFaces (int mp) const;
		void		setMapVert (int mp, int i, const UVVert&xyz) { if (mapVerts(mp)) mapVerts(mp)[i] = xyz; }
		DllExport void MakeMapPlanar (int mp);	// Copies mesh topology, vert locations into map.
		DllExport BitArray GetIsoMapVerts (int mp);
		DllExport void DeleteMapVertSet (int mp, BitArray set, BitArray *fdel=NULL);
		DllExport void DeleteIsoMapVerts (int mp=-1);	//	 default indicates to do all active maps
		DllExport void freeMapVerts (int mp);
		DllExport void freeMapFaces (int mp);

		DllExport void setNumVData (int ct, BOOL keep=FALSE);
		int getNumVData () const { return vdSupport.GetSize(); }

		DllExport BOOL vDataSupport (int vd) const;
		DllExport void setVDataSupport (int vd, BOOL support=TRUE);
		void *vertexData (int vd) const { return vDataSupport(vd) ? vData[vd].data : NULL; }
		float *vertexFloat (int vd) const { return (float *) vertexData (vd); }
		DllExport void freeVData (int vd);
		DllExport void freeAllVData ();

		// Two specific vertex scalars.
		float *getVertexWeights () { return vertexFloat(VDATA_WEIGHT); }
		void SupportVertexWeights () { setVDataSupport (VDATA_WEIGHT); }
		void ClearVertexWeights() { setVDataSupport (VDATA_WEIGHT, FALSE); }
		void freeVertexWeights () { freeVData (VDATA_WEIGHT); }
		float *getVSelectionWeights () { return vertexFloat(VDATA_SELECT); }
		void SupportVSelectionWeights () { setVDataSupport (VDATA_SELECT); }
		void ClearVSelectionWeights() { setVDataSupport (VDATA_SELECT, FALSE); }
		void freeVSelectionWeights () { freeVData (VDATA_SELECT); }

		// these flags are restricted to 4 bits and force the topology (strips & edges)
		// to be invalidated when they change.  Used by primitives with smoothing checkboxes
		DllExport void  setSmoothFlags(int f);
		DllExport int   getSmoothFlags();

		void		setVert(int i, const Point3 &xyz)	{ verts[i] = xyz; }
		void		setVert(int i, float x, float y, float z)	{ verts[i].x=x; verts[i].y=y; verts[i].z=z; }
		void		setTVert(int i, const UVVert &xyz)	{ tVerts[i] = xyz; }
		void		setTVert(int i, float x, float y, float z)	{ tVerts[i].x=x; tVerts[i].y=y; tVerts[i].z=z; }
		
		DllExport void		setNormal(int i, const Point3 &xyz); 
		DllExport Point3 &	getNormal(int i) const; // mjm - made const - 2.16.99

		void		setFaceNormal(int i, const Point3 &xyz) { faceNormal[i] =  xyz; }
		Point3 &	getFaceNormal(int i) { return faceNormal[i]; }

		Point3 &	getVert(int i)		{ return verts[i];  }
		Point3 *	getVertPtr(int i)	{ return verts+i; }
		UVVert &	getTVert(int i)		{ return tVerts[i];  }
		UVVert *	getTVertPtr(int i)	{ return tVerts+i; }
		RVertex &	getRVert(int i)		{ return rVerts[i]; }
		RVertex *	getRVertPtr(int i)	{ return rVerts+i; }
		
		void		setMtlIndex(MtlID	i)	{ mtlIndex = i; }
		MtlID		getMtlIndex(void) 		{ return mtlIndex; }

		// Face MtlIndex access methods;
	    DllExport MtlID		getFaceMtlIndex(int i);
		DllExport void		setFaceMtlIndex(int i, MtlID id); 	
		
		DllExport void		buildNormals();			// calcs face and vertex normals
		DllExport void 		buildRenderNormals();	// like buildNormals, but ignores mtlIndex
		// checkNormals can be used to build the normals and allocate RVert space 
		// only if necessary.  This is a very cheap call if the normals are already calculated.
		// When illum is FALSE, only the RVerts allocation is checked (since normals aren't
		// needed for non-illum rendering).  When illum is TRUE, normals will also be built, if
		// they aren't already.  So, to make sure normals are built, call this with illum=TRUE.
		DllExport void		checkNormals(BOOL illum);

		DllExport void		render(GraphicsWindow *gw, Material *ma, RECT *rp, int compFlags, int numMat=1);
		DllExport BOOL		select(GraphicsWindow *gw, Material *ma, HitRegion *hr, int abortOnHit = FALSE, int numMat=1);
		DllExport void		snap(GraphicsWindow *gw, SnapInfo *snap, IPoint2 *p, Matrix3 &tm);
		DllExport BOOL 		SubObjectHitTest(GraphicsWindow *gw, Material *ma, HitRegion *hr,
								DWORD flags, SubObjHitList& hitList, int numMat=1 );

		void		displayNormals(int b, float sc)	{ dspNormals = b; if(sc != (float)0.0) norScale = sc; }
		void		displayAllEdges(int b)	{ dspAllEdges = b; }
		DllExport void		buildBoundingBox(void);
		DllExport Box3 		getBoundingBox(Matrix3 *tm=NULL); // RB: optional TM allows the box to be calculated in any space.
		                                              // NOTE: this will be slower becuase all the points must be transformed.
		// Cache invalidation
		DllExport void 		InvalidateGeomCache();
		DllExport void 		InvalidateTopologyCache();
		DllExport void 		FreeAll(); //DS
		DllExport void      ZeroTopologyCache(); // RB set pointers to NULL but don't delete from mem.

		// edge list functions		
		DllExport void		EnableEdgeList(int e);
		DllExport void     	BuildVisEdgeList();
		DllExport void 		DrawVisEdgeList(GraphicsWindow *gw, DWORD flags);
		DllExport void 		Draw3DVisEdgeList(GraphicsWindow *gw, DWORD flags);
		DllExport void		HitTestVisEdgeList(GraphicsWindow *gw, int abortOnHit ); // RB
		DllExport void		InvalidateEdgeList(); // RB

		// strip functions				
		DllExport BOOL     	BuildStrips();
		DllExport void		Stripify(Strip *s, StripData *sd, int vtx);
		DllExport void		getStripVertColor(GraphicsWindow *gw, int cv, int flipped, MtlID mID, DWORD smGroup, Point3 &rgb);
		DllExport void		getStripNormal(int cv, MtlID mID, DWORD smGroup, Point3 &nor);
		DllExport int		getStripNormalIndex(int cv, MtlID mID, DWORD smGroup);
		DllExport BOOL		getStripTVert(GraphicsWindow *gw, int cv, int ctv, Point3 &uvw);
		DllExport void 		DrawStrips(GraphicsWindow *gw, Material *ma, int numMat);
		DllExport void 		Draw3DStrips(GraphicsWindow *gw, Material *ma, int numMat);
		DllExport void 		Draw3DWireStrips(GraphicsWindow *gw, Material *ma, int numMat);
		DllExport void		InvalidateStrips();

		DllExport void		BuildStripsAndEdges();

		// functions for use in data flow evaluation
		DllExport void 		ShallowCopy(Mesh *amesh, unsigned long channels);
		DllExport void 		DeepCopy(Mesh *amesh, unsigned long channels);
		DllExport void		NewAndCopyChannels(unsigned long channels);
		DllExport void 		FreeChannels( unsigned long channels, int zeroOthers=1);

		// Mesh flags
		void		SetFlag(DWORD f) { flags |= f; }
		DWORD		GetFlag(DWORD f) { return flags & f; }
		void		ClearFlag(DWORD f) { flags &= ~f; }

		// Display flags
		void		SetDispFlag(DWORD f) { dispFlags |= f; }
		DWORD		GetDispFlag(DWORD f) { return dispFlags & f; }
		void		ClearDispFlag(DWORD f) { dispFlags &= ~f; }

		// Selection access
		BitArray& 	VertSel() { return vertSel;  }	
		BitArray& 	FaceSel() { return faceSel;  }	

		// Constructs a vertex selection list based on the current selection level.
		DllExport BitArray 	VertexTempSel();

		DllExport IOResult Save(ISave* isave);
		DllExport IOResult Load(ILoad* iload);

		// RB: added so all objects can easily support the GeomObject method of the same name.
		DllExport int IntersectRay(Ray& ray, float& at, Point3& norm);
		DllExport int IntersectRay(Ray& ray, float& at, Point3& norm, DWORD &fi, Point3 &bary);

		// RB: I couldn't resist adding these <g>
		DllExport Mesh operator+(Mesh &mesh);  // Union
		DllExport Mesh operator-(Mesh &mesh);  // Difference
		DllExport Mesh operator*(Mesh &mesh);  // Intersection
		DllExport void MyDebugPrint ();

		DllExport void WeldCollinear(BitArray &set);

		DllExport void Optimize(
			float normThresh, float edgeThresh, 
			float bias, float maxEdge, DWORD flags, 
			MeshOpProgress *prog=NULL);

		DllExport void ApplyUVWMap(int type,
			float utile, float vtile, float wtile,
			int uflip, int vflip, int wflip, int cap,
			const Matrix3 &tm, int channel=1);
		DllExport void ApplyMapper (UVWMapper & map, int channel=1);

		DllExport void FlipNormal(int i);
		DllExport void UnifyNormals(BOOL selOnly);
		DllExport void AutoSmooth(float angle,BOOL useSel,BOOL preventIndirectSmoothing=FALSE);

		DllExport Edge *MakeEdgeList(int *edgeCount, int flagdbls=0);
		DllExport int DeleteFlaggedFaces(); // deletes all faces with FACE_WORK flag set
				
		
		// deletes all seleted elements of the current selection level
		DllExport void DeleteSelected();	
		
		// Deletes vertices as specified by the bit array
		DllExport void DeleteVertSet(BitArray set);
		
		// Deletes faces as specified by the bit array. If isoVert is non
		// null then it will be setup to flag vertices that were isolated
		// by the face deletetion. This set can then be passed to
		// DeleteVertSet to delete isolated vertices.
		DllExport void DeleteFaceSet(BitArray set, BitArray *isoVert=NULL);

		// Returns TRUE if an equivalent face already exists.
		DllExport BOOL DoesFaceExist(DWORD v0, DWORD v1, DWORD v2);

		// Removes faces that have two or more equal indices.
		// Returns TRUE if any degenerate faces were found
		DllExport BOOL RemoveDegenerateFaces();

		// Removes faces that have indices that are out of range
		// Returns TRUE if any illegal faces were found
		DllExport BOOL RemoveIllegalFaces();

		DllExport Point3 FaceNormal (DWORD fi, BOOL nrmlize=FALSE);
		DllExport Point3 FaceCenter (DWORD fi);
		DllExport float AngleBetweenFaces(DWORD f0, DWORD f1);

		// Compute the barycentric coords of a point in the plane of
		// a face relative to that face.
		DllExport Point3 BaryCoords(DWORD face, Point3 p);

		// Some edge operations
		DllExport void DivideEdge(DWORD edge, float prop=.5f, bool visDiag1=TRUE,
			bool fixNeighbors=TRUE, bool visDiag2=TRUE);
		DllExport void DivideFace(DWORD face, DWORD e1, DWORD e2, 
			float prop1=.5f, float prop2=.5f, bool fixNeighbors=TRUE, bool split=FALSE);
		DllExport void TurnEdge (DWORD edge, DWORD *otherEdge=NULL);

		// Tessellation
		DllExport void FaceCenterTessellate(BOOL ignoreSel=FALSE, MeshOpProgress *mop=NULL);
		DllExport void EdgeTessellate(float tens,BOOL ignoreSel=FALSE, MeshOpProgress *mop=NULL);

		// Extrudes selected faces. Note that this is just a topological
		// change. The new extruded faces do not change position but
		// are left on top of the original faces.
		// If doFace is FALSE then selected edges are extruded.
		DllExport void ExtrudeFaces(BOOL doFace=TRUE);

		// Indents selected faces, in a manner consistent with the outlining used in Bevel.
		// Added by SteveA for Shiva, 6/98
		DllExport void IndentSelFaces (float amount);

		// Splits verts specified in bitarray so that they are only
		// used by a single face
		DllExport void BreakVerts(BitArray set);

		// Deletes verts that aren't used by any faces
		DllExport BitArray GetIsoVerts ();
		DllExport void DeleteIsoVerts ();

		// Clone faces (and verts used by those faces)
		DllExport void CloneFaces(BitArray fset);
		DllExport void PolyFromFace (DWORD f, BitArray &set, float thresh, BOOL ignoreVisEdges, AdjFaceList *af=NULL);
		DllExport void ElementFromFace (DWORD f, BitArray &set, AdjFaceList *af=NULL);
		DllExport void FindVertsUsedOnlyByFaces (BitArray & fset, BitArray & vset);
		DllExport void FindOpenEdges (BitArray & edges);
		DllExport void FindVertexAngles (float *vang, BitArray *set=NULL);

		// used by the renderer
		void  SetRenderData(MeshRenderData *p) {renderData = p; } 
		MeshRenderData * GetRenderData() { return renderData; }
	};

class ImbMesh: public Mesh {
	public:
		~ImbMesh();
	};

// Mapping types passed to ApplyUVWMap()
#define MAP_PLANAR		0
#define MAP_CYLINDRICAL	1
#define MAP_SPHERICAL	2
#define MAP_BALL		3
#define MAP_BOX			4
#define MAP_FACE		5


// Optimize flags
#define OPTIMIZE_SAVEMATBOUNDRIES		(1<<0)
#define OPTIMIZE_SAVESMOOTHBOUNDRIES	(1<<1)
#define OPTIMIZE_AUTOEDGE				(1<<2)

void DllExport setUseVisEdge(int b);
int DllExport getUseVisEdge();

#define SMALL_VERTEX_DOTS	0
#define LARGE_VERTEX_DOTS	1

void DllExport setUseVertexDots(int b);
int DllExport getUseVertexDots();

void DllExport setVertexDotType(int t);
int DllExport getVertexDotType();

// a callback to update progress UI while doing a
// lengthy operation to a mesh
class MeshOpProgress {
	public:
		// called once with the total increments
		virtual void Init(int total)=0;

		// Called to update progress. % done = p/total
		virtual BOOL Progress(int p)=0;
	};

// Boolean operations for meshes:
#define MESHBOOL_UNION 				1
#define MESHBOOL_INTERSECTION  		2
#define MESHBOOL_DIFFERENCE 		3

//
// mesh = mesh1 op mesh2
// If tm1 or tm2 are non-NULL, the points of the corresponding
// mesh will be transformed by these tm before the bool op
// The mesh will be transformed back by either Inverse(tm1) or
// Inverse(tm2) depending whichInv (0=>tm1, 1=>tm2)
// unless whichInv is -1 in which case it will not be transformed
// back.
//
int DllExport CalcBoolOp(
	Mesh &mesh, Mesh &mesh1, Mesh &mesh2, int op,
	MeshOpProgress *prog = NULL,
	Matrix3 *tm1 = NULL,
	Matrix3 *tm2 = NULL,
	int whichInv = 0,
	int weld = TRUE);


// Combines two meshes. The matrix and whichInv parameters have
// the same meaning as they do for the CalcBoolOp above.
void DllExport CombineMeshes(
		Mesh &mesh,Mesh &mesh1,Mesh &mesh2,
		Matrix3 *tm1=NULL, Matrix3 *tm2=NULL, int whichInv=0);

// Slices a single mesh.  The Point3 N and the float offset define a
// slicing plane (by DotProd (N,X) = offset).  Default behavior is to
// split faces that cross the plane, producing 1-2 new faces on each side
// and a new vert in the middle of each edge crossing the plane.  split
// means to add 2 different but coincident points to the top and bottom
// sets of faces, splitting the mesh into two meshes.  remove means to
// delete all faces & verts below the plane.
void DllExport SliceMesh (Mesh & mesh,
						  Point3 N, float off, bool split=FALSE, bool remove=FALSE);

// Handy utilities to go with meshes:

// Translates map type into 3d location -> uvw coord mapper:
class UVWMapper {
public:
	int     type, cap;
	float   utile, vtile, wtile;
	int     uflip, vflip, wflip;
	Matrix3 tm;

	DllExport UVWMapper();
	DllExport UVWMapper(int type, const Matrix3 &tm, int cap=FALSE,
		float utile=1.0f, float vtile=1.0f, float wtile=1.0f,
		int uflip=FALSE, int vflip=FALSE, int wflip=FALSE);
	DllExport UVWMapper(UVWMapper& m);

	DllExport UVVert MapPoint(Point3 p, const Point3 & norm, int *nan=NULL);
	DllExport UVVert TileFlip (UVVert uvw);
	DllExport int MainAxis (const Point3 & n);
	bool NormalMatters () { return ((type==MAP_BOX)||((type==MAP_CYLINDRICAL)&&cap)) ? TRUE:FALSE; }
};

DllExport Mesh * CreateNewMesh();

#endif // _MESH_H_
