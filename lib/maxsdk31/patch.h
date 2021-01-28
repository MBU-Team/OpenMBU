/**********************************************************************
 *<
	FILE: patch.h

	DESCRIPTION: Main include file for bezier patches

	CREATED BY: Tom Hudson

	HISTORY: Created June 21, 1995
			 June 17, 1997 TH -- Added second texture mapping channel
			 12-10-98 Peter Watje added hide interior edge support and hidding patches
			 12-31-98 Peter Watje added hook patches, patch extrusion and bevels

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef _PATCH_H_

#define _PATCH_H_

#include "coreexp.h"
#include "meshlib.h"
#include <hitdata.h>
#include "maxtess.h"

// Uncomment the following to check for missed triangular patch 'aux' computation
//#define CHECK_TRI_PATCH_AUX

// Handy-dandy integer table class
typedef Tab<int> IntTab;

// Value for undefined patches and vertices
#define PATCH_UNDEFINED -1

// TH 5/17/99 -- Commented out MULTI_PROCESSING, it wasn't being used and was causing
// am obscure memory leak (Defect 180889)
//#define MULTI_PROCESSING	TRUE		// TRUE turns on mp vertex transformation

class HookPoint 
	{
public:
	int upperPoint, lowerPoint;
	int upperVec,lowerVec;
	int upperHookVec, lowerHookVec;
	int hookPoint;
	int upperPatch, lowerPatch, hookPatch;
	int hookEdge, upperEdge, lowerEdge;
	};

class ExtrudeData
{
public:
	int u,l,uvec,lvec;
//3-10-99 watje
	Point3 edge;
	Point3 bevelDir;
};

class ISave;
class ILoad;
class PatchMesh;

#define NEWPATCH

// PRVertex flags: contain clip flags, number of normals at the vertex
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

class PRVertex {
	public:
		PRVertex()	{ rFlags = 0; /*ern = NULL;*/ }
		CoreExport ~PRVertex();	

		DWORD		rFlags;     
		int			pos[3];	
	};					  

// Patch vector flags
#define PVEC_INTERIOR	(1<<0)

// Patch vectors

class PatchVec {
	public:
		Point3 p;			// Location
		int vert;			// Vertex which owns this vector
		int patches[2];		// Patches using this vector
		DWORD flags;
		int aux1;			// Used to track topo changes during editing (Edit Patch)
		int aux2;			// Used to track topo changes during editing (PatchMesh)
		CoreExport PatchVec();
		CoreExport PatchVec(PatchVec &from);
		void ResetData() { vert = patches[0] = patches[1] = PATCH_UNDEFINED; }
		CoreExport BOOL AddPatch(int index);
		CoreExport PatchVec& operator=(PatchVec& from);
		void Transform(Matrix3 &tm) { p = p * tm; }

		CoreExport IOResult Save(ISave* isave);
		CoreExport IOResult Load(ILoad* iload);
	};

// Patch vertex flags
#define PVERT_COPLANAR (1<<0)
#define PVERT_CORNER (0)
#define PVERT_TYPE_MASK 0xfffffffe
//watje 12-10-98
#define PVERT_HIDDEN		(1<<1)

// Patch vertex

class PatchVert {
	public:
		Point3 p;			// Location
		IntTab vectors;		// List of vectors attached to this vertex
		IntTab patches;		// List of patches using this vertex
		DWORD flags;
		int aux1;			// Used to track topo changes during editing (Edit Patch)
		int aux2;			// Used to track topo changes during editing (PatchMesh)
		CoreExport PatchVert();
		CoreExport PatchVert(PatchVert &from);
		~PatchVert() { ResetData(); }
		CoreExport PatchVert& operator=(PatchVert& from);
		void ResetData() { vectors.Delete(0,vectors.Count()); patches.Delete(0,patches.Count()); }
		CoreExport int FindVector(int index);
		CoreExport void AddVector(int index);
		CoreExport void DeleteVector(int index);
		CoreExport int FindPatch(int index);
		CoreExport void AddPatch(int index);
		CoreExport void DeletePatch(int index);
		void Transform(Matrix3 &tm) { p = p * tm; }

//watje  12-10-98
		CoreExport void SetHidden(BOOL sw = TRUE)
			{
			if(sw)
				flags |= PVERT_HIDDEN;
			else
				flags &= ~PVERT_HIDDEN;
			}
//watje  12-10-98
		BOOL IsHidden() { return (flags & PVERT_HIDDEN) ? TRUE : FALSE; }

		CoreExport IOResult Save(ISave* isave);
		CoreExport IOResult Load(ILoad* iload);
	};

class PatchEdge {
	public:
		int v1;		// Index of first vertex
		int vec12;	// Vector from v1 to v2
		int vec21;	// Vector from v2 to v1
		int v2;		// Index of second vertex
		int patch1;	// Index of first patch
		int patch2;	// Index of second patch
		int aux1;	// Used to track topo changes during editing (Edit Patch)
		int aux2;	// Used to track topo changes during editing (PatchMesh)
		CoreExport PatchEdge();
		CoreExport PatchEdge(PatchEdge &from);
		CoreExport PatchEdge(int v1, int vec12, int vec21, int v2, int p1, int p2, int aux1=-1, int aux2=-1);
		// Dump the patch edge structure via DebugPrints
		CoreExport void Dump();
	};

// Patch types

#define PATCH_UNDEF	0	// Undefined (watch out!)
#define PATCH_TRI	3	// Triangular patch
#define PATCH_QUAD	4	// Quadrilateral patch

// Patch Flags:
#define PATCH_AUTO			(1<<0)	// Interior verts computed automatically if set
#define PATCH_MANUAL		(0)		// Interior verts stored in 'interior' array
#define PATCH_INTERIOR_MASK 0xfffffffe
//watje 12-10-98
#define PATCH_HIDDEN		(1<<1)  //patch is hidden


// The mat ID is stored in the HIWORD of the patch flags
#define PATCH_MATID_SHIFT	16
#define PATCH_MATID_MASK	0xFFFF

class Patch {	
	public:
		int type;			// See types, above
		int	v[4];			// Can have three or four vertices
		int	vec[8];			// Can have six or eight vector points
		int	interior[4];	// Can have one or four interior vertices
		Point3 aux[9];		// Used for triangular patches only -- Degree 4 control points
		int	adjacent[4];	// Adjacent patches -- Can have three or four
		int	edge[4];		// Pointers into edge list -- Can have three or four
		DWORD	smGroup;	// Defaults to 1 -- All patches smoothed in a PatchMesh
		DWORD	flags;		// See flags, above
		int aux1;			// Used to track topo changes during editing (Edit Patch)
		int aux2;			// Used to track topo changes during editing (PatchMesh)

#ifdef CHECK_TRI_PATCH_AUX
		Point3 auxSource[9];
		CoreExport void CheckTriAux(PatchMesh *pMesh);
#endif //CHECK_TRI_PATCH_AUX

		CoreExport Patch();	// WARNING: This does not allocate arrays -- Use SetType(type) or Patch(type)
		CoreExport Patch(int type);
		CoreExport Patch(Patch& fromPatch);
		CoreExport ~Patch();
		CoreExport void Init();
		void	setVerts(int *vrt) { memcpy(v, vrt, type * sizeof(int)); }
		void	setVerts(int a, int b, int c)  { assert(type == PATCH_TRI); v[0]=a; v[1]=b; v[2]=c; }
		void	setVerts(int a, int b, int c, int d)  { assert(type == PATCH_QUAD); v[0]=a; v[1]=b; v[2]=c; v[3]=d; }
		void	setVecs(int ab, int ba, int bc, int cb, int ca, int ac) {
			assert(type == PATCH_TRI);
			vec[0]=ab; vec[1]=ba; vec[2]=bc; vec[3]=cb; vec[4]=ca; vec[5]=ac;
			}
		void	setVecs(int ab, int ba, int bc, int cb, int cd, int dc, int da, int ad) {
			assert(type == PATCH_QUAD);
			vec[0]=ab; vec[1]=ba; vec[2]=bc; vec[3]=cb; vec[4]=cd; vec[5]=dc; vec[6]=da, vec[7]=ad;
			}
		void	setInteriors(int a, int b, int c) {
			assert(type == PATCH_TRI);
			interior[0]=a; interior[1]=b; interior[2]=c;
			}
		void	setInteriors(int a, int b, int c, int d) {
			assert(type == PATCH_QUAD);
			interior[0]=a; interior[1]=b; interior[2]=c; interior[3]=d;
			}
		int		getVert(int index)	{ return v[index]; }
		int *	getAllVerts(void)	{ return v; }
		MtlID	getMatID() {return (int)((flags>>FACE_MATID_SHIFT)&FACE_MATID_MASK);}
		void    setMatID(MtlID id) {flags &= 0xFFFF; flags |= (DWORD)(id<<FACE_MATID_SHIFT);}
		CoreExport Point3 interp(PatchMesh *pMesh, float u, float v);			// Quadrilateral
		CoreExport Point3 interp(PatchMesh *pMesh, float u, float v, float w);	// Triangle
		CoreExport void ComputeAux(PatchMesh *pMesh, int index);
		CoreExport void ComputeAux(PatchMesh *pMesh);	// Do all degree-4 points
		CoreExport void computeInteriors(PatchMesh* pMesh);
		CoreExport void SetType(int type, BOOL init = FALSE);
		CoreExport Patch& operator=(Patch& from);
		CoreExport void SetAuto(BOOL sw = TRUE);
		BOOL IsAuto() { return (flags & PATCH_AUTO) ? TRUE : FALSE; }

//watje 12-10-98
		CoreExport void SetHidden(BOOL sw = TRUE);
//watje 12-10-98
		BOOL IsHidden() { return (flags & PATCH_HIDDEN) ? TRUE : FALSE; }


		// Dump the patch mesh structure via DebugPrints

		CoreExport void Dump();

		CoreExport IOResult Save(ISave* isave);
		CoreExport IOResult Load(ILoad* iload);
	};

// Separate class for patch texture verts
class TVPatch {	
	public:
		int	tv[4];			// Texture verts (always 4 here, even for tri patches)
		CoreExport TVPatch();
		CoreExport TVPatch(TVPatch& fromPatch);
		CoreExport void Init();
		CoreExport void setTVerts(int *vrt, int count);
		CoreExport void setTVerts(int a, int b, int c, int d = 0);
		int		getTVert(int index)	{ return tv[index]; }
		int *	getAllTVerts(void)	{ return tv; }
		CoreExport TVPatch& operator=(const TVPatch& from);

		CoreExport IOResult Save(ISave* isave);
		CoreExport IOResult Load(ILoad* iload);
	};


// Flag definitions
#define COMP_TRANSFORM	0x0001	// forces recalc of model->screen transform; else will attempt to use cache
#define COMP_IGN_RECT	0x0002	// forces all polys to be rendered; else only those intersecting the box will be
#define COMP_LIGHTING	0x0004	// forces re-lighting of all verts (as when a light moves); else only relight moved verts

#define COMP_ALL		0x00ff

// If this bit is set then the node being displayed by this mesh is selected.
// Certain display flags only activate when this bit is set.
#define COMP_OBJSELECTED	(1<<8)
#define COMP_OBJFROZEN		(1<<9)

typedef int (*INTRFUNC)();

CoreExport void setPatchIntrFunc(INTRFUNC fn);

// Special types for patch vertex hits -- Allows us to distinguish what they hit on a pick
#define PATCH_HIT_PATCH		0
#define PATCH_HIT_EDGE		1
#define PATCH_HIT_VERTEX	2
#define PATCH_HIT_VECTOR	3
#define PATCH_HIT_INTERIOR	4

class PatchSubHitRec {
	private:		
		PatchSubHitRec *next;
	public:
		DWORD	dist;
		PatchMesh *patch;
		int		index;
		int		type;

		PatchSubHitRec( DWORD dist, PatchMesh *patch, int index, int type, PatchSubHitRec *next ) 
			{ this->dist = dist; this->patch = patch; this->index = index; this->type = type; this->next = next; }

		PatchSubHitRec *Next() { return next; }		
	};

class SubPatchHitList {
	private:
		PatchSubHitRec *first;
	public:
		SubPatchHitList() { first = NULL; }
		~SubPatchHitList() {
			PatchSubHitRec *ptr = first, *fptr;
			while ( ptr ) {
				fptr = ptr;
				ptr = ptr->Next();
				delete fptr;
				}
			first = NULL;
			}	

		PatchSubHitRec *First() { return first; }
		void AddHit( DWORD dist, PatchMesh *patch, int index, int type ) {
			first = new PatchSubHitRec(dist,patch,index,type,first);
			}
	};


// Special storage class for hit records so we can know which object was hit
class PatchHitData : public HitData {
	public:
		PatchMesh *patch;
		int index;
		int type;
		PatchHitData(PatchMesh *patch, int index, int type)
			{ this->patch = patch; this->index = index; this->type = type; }
		~PatchHitData() {}
	};

// Flags for sub object hit test

// NOTE: these are the same bits used for object level.
#define SUBHIT_PATCH_SELONLY	(1<<0)
#define SUBHIT_PATCH_UNSELONLY	(1<<2)
#define SUBHIT_PATCH_ABORTONHIT	(1<<3)
#define SUBHIT_PATCH_SELSOLID	(1<<4)

#define SUBHIT_PATCH_VERTS		(1<<24)
#define SUBHIT_PATCH_VECS		(1<<25)
#define SUBHIT_PATCH_PATCHES	(1<<26)
#define SUBHIT_PATCH_EDGES		(1<<27)
#define SUBHIT_PATCH_TYPEMASK	(SUBHIT_PATCH_VERTS|SUBHIT_PATCH_VECS|SUBHIT_PATCH_EDGES|SUBHIT_PATCH_PATCHES)


// Display flags
#define DISP_VERTTICKS		(1<<0)

#define DISP_SELVERTS		(1<<10)
#define DISP_SELPATCHES		(1<<11)
#define DISP_SELEDGES		(1<<12)
#define DISP_SELPOLYS		(1<<13)

#define DISP_LATTICE		(1<<16)
#define DISP_VERTS			(1<<17)

// Selection level bits.
#define PATCH_OBJECT		(1<<0)
#define PATCH_VERTEX		(1<<1)
#define PATCH_PATCH			(1<<2)
#define PATCH_EDGE			(1<<3)

// Types for Subdivision, below:
#define SUBDIV_EDGES 0
#define SUBDIV_PATCHES 1

// PatchMesh flags

class PatchMesh {
	friend class Patch;

	private:
#if MULTI_PROCESSING
		static int		refCount;
		static HANDLE	xfmThread;
		static HANDLE	xfmMutex;
		static HANDLE	xfmStartEvent;
		static HANDLE	xfmEndEvent;
		friend DWORD WINAPI xfmFunc(LPVOID ptr);
#endif
		// derived data-- can be regenerated
		PRVertex 		*rVerts;		// <<< instance specific.
		GraphicsWindow 	*cacheGW;  		// identifies rVerts cache
		Box3			bdgBox;			// object space--depends on geom+topo
 
		// The number of interpolations this patch will use for mesh conversion
		int			meshSteps;
//3-18-99 watje to support render steps
		int			meshStepsRender;
		BOOL		showInterior;

		BOOL		adaptive;
		// GAP tessellation
		TessApprox	viewTess;	// tessellation control for the interactive renderer
		TessApprox	prodTess;	// tessellation control for the production renderer
		TessApprox	dispTess;	// displacment tessellation control for the production renderer
		BOOL		mViewTessNormals;	// use normals from the tesselator
		BOOL		mProdTessNormals;	// use normals from the tesselator
		BOOL		mViewTessWeld;	// Weld the mesh after tessellation
		BOOL		mProdTessWeld;	// Weld the mesh after tessellation

 		// Vertex and patch work arrays -- for snap code
		int			snapVCt;
		int			snapPCt;
		char		*snapV;
		char		*snapP;

		// -------------------------------------
		//
		DWORD  		flags;		  	// work flags- 

		int 		renderPatch( GraphicsWindow *gw, int index, BOOL force=FALSE);
		int 		renderEdge( GraphicsWindow *gw, int index, BOOL force=FALSE);
		void		checkRVertsAlloc(void);
		void		setCacheGW(GraphicsWindow *gw)	{ cacheGW = gw; }
		GraphicsWindow *getCacheGW(void)			{ return cacheGW; }

		void 		freeVerts();
		void 		freeTVerts(int channel=0);
		void 		freeVecs();
		void  		freePatches();
		void  		freeTVPatches(int channel=0);
		void  		freeEdges();
		void  		freeRVerts();
		void		freeSnapData();
		int			buildSnapData(GraphicsWindow *gw,int verts,int edges);

	public:
		// Topology
		int			numVerts;
		int			numVecs;
		int	 		numPatches;
		int			numEdges;
		Patch *		patches;
		PatchVec *	vecs;
		PatchEdge *	edges;
		Tab<HookPoint> hooks;

//watje 4-16-99 to handle hooks and changes in topology
		Tab<Point3> hookTopoMarkers;
		Tab<Point3> hookTopoMarkersA;
		Tab<Point3> hookTopoMarkersB;
		CoreExport int HookFixTopology() ;


		// Geometry
		PatchVert *	verts;

		// Texture Coord assignment 
		Tab<int> numTVerts;
		Tab<UVVert *> tVerts;
		Tab<TVPatch *> tvPatches;  	 

		// Material assignment
		MtlID		mtlIndex;     // object material

		// Selection
		BitArray	vertSel;  		// selected vertices
		BitArray	edgeSel;  		// selected edges
		BitArray	patchSel;  		// selected patches

		// If hit bezier vector, this is its info:
		int bezVecVert;

		// Display attribute flags
		DWORD		dispFlags;

		// Selection level
		DWORD		selLevel;

		// Mesh cache
		Mesh mesh;
		int cacheSteps;		// meshSteps used for the cache
		BOOL cacheAdaptive;	// adaptive switch used for cache
		BOOL meshValid;

		CoreExport PatchMesh();
		CoreExport PatchMesh(PatchMesh& fromPatch);

		CoreExport void Init();

		CoreExport ~PatchMesh();

		CoreExport PatchMesh& 		operator=(PatchMesh& fromPatchMesh);
		CoreExport PatchMesh& 		operator=(Mesh& fromMesh);

		// The following is similar to operator=, but just takes the major components,
		// not the display flags, selection level, etc.
		CoreExport void CopyPatchDataFrom(PatchMesh &fromPatchMesh);

		CoreExport BOOL	setNumVerts(int ct, BOOL keep = FALSE);
		int				getNumVerts(void)	{ return numVerts; }
		
		CoreExport BOOL	setNumVecs(int ct, BOOL keep = FALSE);
		int				getNumVecs(void)	{ return numVecs; }
		
		CoreExport BOOL	setNumPatches(int ct, BOOL keep = FALSE);
		int				getNumPatches(void)		{ return numPatches; }

		CoreExport BOOL	setNumEdges(int ct, BOOL keep = FALSE);
		int				getNumEdges(void)		{ return numEdges; }
		
		void		setVert(int i, const Point3 &xyz)	{ verts[i].p = xyz; }
		void		setVert(int i, float x, float y, float z)	{ verts[i].p.x=x; verts[i].p.y=y; verts[i].p.z=z; }
		void		setVec(int i, const Point3 &xyz)	{ vecs[i].p = xyz; }
		void		setVec(int i, float x, float y, float z)	{ vecs[i].p.x=x; vecs[i].p.y=y; vecs[i].p.z=z; }
		
		PatchVert &	getVert(int i)					{ return verts[i];  }
		PatchVert *	getVertPtr(int i)				{ return verts+i; }
		PatchVec &	getVec(int i)					{ return vecs[i];  }
		PatchVec *	getVecPtr(int i)				{ return vecs+i; }
		PRVertex &	getRVert(int i)					{ return rVerts[i]; }
		PRVertex *	getRVertPtr(int i)				{ return rVerts+i; }
		
		// Two versions of following methods, to cope with necessary change in map indexing between 2.5 and 3.
		// Old TV/VC methods are given with "TV" in the name.  For these methods, channel 0 is the original map
		// channel, 1, while any nonzero channel is vertex colors.  (No higher channels!)
		BOOL	setNumTVertsChannel(int mp, int ct, BOOL keep=FALSE) { return setNumMapVerts (mp?0:1, ct, keep); }
		BOOL setNumTVerts(int ct, BOOL keep=FALSE) { return setNumMapVerts (1, ct, keep); }
		int	getNumTVertsChannel(int mp) const { return numTVerts[mp?0:1]; }
		int getNumTVerts() const { return getNumMapVerts(1); }

		// New methods have "Map" in the name, and accept normal Object-level map indexing: 0 is VC channel, 1 or more
		// are map channels.
		CoreExport BOOL setNumMapVerts (int mp, int ct, BOOL keep = FALSE);
		int getNumMapVerts (int mp) const { return (mp<numTVerts.Count()) ? numTVerts[mp] : 0; }

		CoreExport UVVert *mapVerts (int mp) const;
		CoreExport TVPatch *mapPatches (int mp) const;

		// These are parallel to patches
		// These are called from setNumPatches() to maintain the same count.
		//
		// If they are NULL and keep = TRUE they stay NULL.
		// If they are NULL and keep = FALSE they are allocated (3D verts also init themselves from the main vert array)
		// If they are non-NULL and ct = 0 they are set to NULL (and freed)
		// Old version: nonzero = vc channel
		BOOL setNumTVPatchesChannel(int channel, int ct, BOOL keep=FALSE, int oldCt=0) { return setNumMapPatches (channel?0:1, ct, keep, oldCt); }
		BOOL setNumTVPatches(int ct, BOOL keep=FALSE, int oldCt=0) { return setNumMapPatches (1, ct, keep, oldCt); }
		// New version: 0 = vc channel
		CoreExport BOOL 	setNumMapPatches (int channel, int ct, BOOL keep=FALSE, int oldCt=0);

		void		setTVertChannel(int channel, int i, const UVVert &xyz)	{ tVerts[channel?0:1][i] = xyz; }
		void		setTVert(int i, const UVVert &xyz)	{ tVerts[1][i] = xyz; }
		void		setTVertChannel(int channel, int i, float x, float y, float z)	{ tVerts[channel?0:1][i].x=x; tVerts[channel?0:1][i].y=y; tVerts[channel?0:1][i].z=z; }
		void		setTVert(int i, float x, float y, float z)	{ tVerts[1][i].x=x; tVerts[1][i].y=y; tVerts[1][i].z=z; }
		void		setTVPatchChannel(int channel, int i, TVPatch &tvp)	{ tvPatches[channel?0:1][i] = tvp; }
		void		setTVPatch(int i, TVPatch &tvp)	{ tvPatches[1][i] = tvp; }
		UVVert &	getTVertChannel(int channel, int i)	{ return tVerts[channel?0:1][i];  }
		UVVert &	getTVert(int i)	{ return tVerts[1][i];  }
		UVVert *	getTVertPtrChannel(int channel, int i)	{ return tVerts[channel?0:1]+i; }
		UVVert *	getTVertPtr(int i)	{ return tVerts[1]+i; }
		TVPatch &	getTVPatchChannel(int channel, int i)	{ return tvPatches[channel?0:1][i];  }
		TVPatch &	getTVPatch(int i)	{ return tvPatches[1][i];  }

		// New map methods: for these, channel 0 is v.c. channel, and anything higher is a map channel.
		void setMapVert (int mp, int i, const UVVert &xyz) { tVerts[mp][i] = xyz; }
		void setMapVert (int mp, int i, float x, float y, float z) { tVerts[mp][i].x=x; tVerts[mp][i].y=y; tVerts[mp][i].z = z; }
		void setMapPatch (int mp, int i, const TVPatch &tvp) { tvPatches[mp][i] = tvp; }
		UVVert & getMapVert (int mp, int i) { return tVerts[mp][i]; }
		UVVert * getMapVertPtr (int mp, int i) { return tVerts[mp] + i; }
		TVPatch & getMapPatch (int mp, int i) { return tvPatches[mp][i];  }

		void		setMtlIndex(MtlID i)	{ mtlIndex = i; }
		MtlID		getMtlIndex(void) 		{ return mtlIndex; }
	    CoreExport MtlID		getPatchMtlIndex(int i);
		CoreExport void		setPatchMtlIndex(int i, MtlID id); 	

		// Automatically update all the adjacency info, etc.
		// Returns TRUE if patch mesh is valid, FALSE if it's not!
		CoreExport BOOL		buildLinkages();
		
		// Compute the interior bezier points for each patch in the mesh
		CoreExport void		computeInteriors();

		// Compute the degree-4 bezier points for each triangular patch in the mesh
		CoreExport void		computeAux();

		CoreExport void		render(GraphicsWindow *gw, Material *ma, RECT *rp, int compFlags, int numMat=1);
		CoreExport BOOL		select(GraphicsWindow *gw, Material *ma, HitRegion *hr, int abortOnHit=FALSE, int numMat=1);
		CoreExport void		snap(GraphicsWindow *gw, SnapInfo *snap, IPoint2 *p, Matrix3 &tm);
		CoreExport BOOL 	SubObjectHitTest(GraphicsWindow *gw, Material *ma, HitRegion *hr,
								DWORD flags, SubPatchHitList& hitList, int numMat=1 );

		CoreExport void		buildBoundingBox(void);
		CoreExport Box3		getBoundingBox(Matrix3 *tm=NULL); // RB: optional TM allows the box to be calculated in any space.
		                                              // NOTE: this will be slower becuase all the points must be transformed.
		CoreExport void		GetDeformBBox(Box3& box, Matrix3 *tm=NULL, BOOL useSel=FALSE);
		
		CoreExport void 	InvalidateGeomCache();
		CoreExport void		InvalidateMesh();
		CoreExport void 	FreeAll(); //DS
				
		// functions for use in data flow evaluation
		CoreExport void 	ShallowCopy(PatchMesh *amesh, unsigned long channels);
		CoreExport void 	DeepCopy(PatchMesh *amesh, unsigned long channels);
		CoreExport void		NewAndCopyChannels(unsigned long channels);
		CoreExport void 	FreeChannels( unsigned long channels, int zeroOthers=1);

		// Display flags
		void		SetDispFlag(DWORD f) { dispFlags |= f; }
		DWORD		GetDispFlag(DWORD f) { return dispFlags & f; }
		void		ClearDispFlag(DWORD f) { dispFlags &= ~f; }

		// Selection access
		BitArray& 	VertSel() { return vertSel;  }	
		BitArray& 	PatchSel() { return patchSel;  }	
		BitArray& 	EdgeSel() { return edgeSel;  }	

		// Constructs a vertex selection list based on the current selection level.
		CoreExport BitArray 	VertexTempSel();

		// Apply the coplanar constraints to the patch mesh
		// (Optionally only apply it to selected vertices)
		CoreExport void ApplyConstraints(BOOL selOnly = FALSE);

		// Create triangular or quadrilateral patch
		CoreExport BOOL MakeQuadPatch(int index, int va, int vab, int vba, int vb, int vbc, int vcb, int vc, int vcd, int vdc, int vd, int vda, int vad, int i1, int i2, int i3, int i4, DWORD sm);
		CoreExport BOOL MakeTriPatch(int index, int va, int vab, int vba, int vb, int vbc, int vcb, int vc, int vca, int vac, int i1, int i2, int i3, DWORD sm);

		// Get/Set mesh steps, adaptive switch
		CoreExport void SetMeshSteps(int steps);
		CoreExport int GetMeshSteps();
//3-18-99 watje to support render steps
		CoreExport void SetMeshStepsRender(int steps);
		CoreExport int GetMeshStepsRender();
		CoreExport void SetShowInterior(BOOL si);
		CoreExport BOOL GetShowInterior();


		CoreExport void SetAdaptive(BOOL sw);
		CoreExport BOOL GetAdaptive();

		CoreExport void SetViewTess(TessApprox tess);
		CoreExport TessApprox GetViewTess();
		CoreExport void SetProdTess(TessApprox tess);
		CoreExport TessApprox GetProdTess();
		CoreExport void SetDispTess(TessApprox tess);
		CoreExport TessApprox GetDispTess();
		CoreExport BOOL GetViewTessNormals();
		CoreExport void SetViewTessNormals(BOOL use);
		CoreExport BOOL GetProdTessNormals();
		CoreExport void SetProdTessNormals(BOOL use);
		CoreExport BOOL GetViewTessWeld();
		CoreExport void SetViewTessWeld(BOOL weld);
		CoreExport BOOL GetProdTessWeld();
		CoreExport void SetProdTessWeld(BOOL weld);

		// Find the edge index for a given vertex-vector-vector-vertex sequence
		int GetEdge(int v1, int v12, int v21, int v2, int p);

		// Apply mapping to the patch mesh
		CoreExport void setNumMaps (int ct, BOOL keep=TRUE);
		int getNumMaps () { return numTVerts.Count(); }
		BOOL getMapSupport (int mp) { return ((mp<tvPatches.Count()) && tvPatches[mp]) ? TRUE : FALSE; }
		int NumMapChannels () { return MAX_MESHMAPS; }
		CoreExport void ApplyUVWMap(int type,
			float utile, float vtile, float wtile,
			int uflip, int vflip, int wflip, int cap,
			const Matrix3 &tm,int channel=1);

		// Tag the points in the patch components to record our topology (This stores
		// identifying values in the various aux2 fields in the Patch)
		// This info can be used after topology-changing operations to remap information
		// tied to vertices, edges and patches.
		// Returns TRUE if tagged successfully
		CoreExport BOOL RecordTopologyTags();

		CoreExport void Transform(Matrix3 &tm);

		// Weld the vertices
		CoreExport BOOL Weld(float thresh, BOOL weldIdentical=FALSE, int startVert=0);

		// General-purpose deletion
		CoreExport void DeletePatchParts(BitArray &delVerts, BitArray &delPatches);

		// Subdivision
		CoreExport void Subdivide(int type, BOOL propagate);

		// Add patch to selected single-patch edges
		CoreExport void AddPatch(int type);

		// Hooks a vertex to a patch edge
		CoreExport int AddHook();
		//tries to add hook patch at the specified vert
		CoreExport int AddHook(int index);
		CoreExport int AddHook(int vertIndex, int segIndex) ;

		CoreExport int RemoveHook();
		//goes through and looks for invalid hooks and tries to fix them used when topology changes
		CoreExport int UpdateHooks();

		Tab<Point3> extrudeDeltas;
		Tab<ExtrudeData> extrudeData;
//creates initial extrude faces and temporary data used in move normal
		CoreExport void CreateExtrusion();
//computes the average normals of the selected faces
		CoreExport Point3 AverageNormals();
		CoreExport Point3 PatchNormal(int index);
		CoreExport void MoveNormal(float amount, BOOL useLocalNorms);

//creates temporary data used in Bevel
		BitArray bevelEdges;
		Tab<float> edgeDistances;
		CoreExport void CreateBevel();
		CoreExport void Bevel(float amount, int smoothStart, int smoothEnd);
//computes the bevel direction of patch based on which edges are open
		Point3 GetBevelDir(int patchVertID);

		// Attach a second PatchMesh, adjusting materials
		CoreExport void Attach(PatchMesh *attPatch, int mtlOffset);

		// Change the interior type of a patch or selected patches (index < 0)
		CoreExport void ChangePatchInterior(int index, int type);

		// Change the type of a vertex or selected vertices (index < 0)
		CoreExport void ChangeVertType(int index, int type);

		CoreExport BOOL SelVertsSameType();	// Are all selected vertices the same type?
		CoreExport BOOL SelPatchesSameType();	// Are all selected patches the same type?

		// Dump the patch mesh structure via DebugPrints
		CoreExport void Dump();
#ifdef CHECK_TRI_PATCH_AUX
		CoreExport void CheckTriAux();
#endif //CHECK_TRI_PATCH_AUX

		// Ready the mesh cache
		CoreExport void PrepareMesh();

		// Get the Mesh version
		CoreExport Mesh& GetMesh();

		CoreExport int IntersectRay(Ray& ray, float& at, Point3& norm);

		CoreExport IOResult Save(ISave* isave);
		CoreExport IOResult Load(ILoad* iload);

	};


#endif // _PATCH_H_
