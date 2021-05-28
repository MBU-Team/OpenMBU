/**********************************************************************
 *<
	FILE: polyshp.h

	DESCRIPTION: Polyline shape methods

	CREATED BY: Tom Hudson

	HISTORY: Created 3 October 1995

 *> Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __POLYSHP_H_

#define __POLYSHP_H_

#include "shphier.h"
#include "captypes.h"
#include "shpsels.h"	// Shape selection classes
#include "templt.h"

// Display flags
#define DISP_VERTTICKS		(1<<0)
//#define DISP_BEZHANDLES		(1<<1)
#define DISP_SELVERTS		(1<<10)
#define DISP_SELSEGMENTS	(1<<11)
#define DISP_SELPOLYS		(1<<13)

// Snap flags
#define PSHAPE_SNAP_IGNORELAST	(1<<0)
#define PSHAPE_SNAP_NOEDGES		(1<<1)

// Selection level bits.
#define SHAPE_OBJECT		(1<<0)
#define SHAPE_SPLINE		(1<<1)
#define SHAPE_SEGMENT		(1<<2)
#define SHAPE_VERTEX		(1<<3)

// Parameter types for shape interpolation (Must match interpolation types in object.h)
#define POLYSHP_INTERP_SIMPLE 0		// Parameter space based on segments
#define POLYSHP_INTERP_NORMALIZED 1	// Parameter space normalized to curve length

// Flags for sub object hit test

// NOTE: these are the same bits used for object level.
#define SUBHIT_SHAPE_SELONLY	(1<<0)
#define SUBHIT_SHAPE_UNSELONLY	(1<<2)
#define SUBHIT_SHAPE_ABORTONHIT	(1<<3)
#define SUBHIT_SHAPE_SELSOLID	(1<<4)

#define SUBHIT_SHAPE_VERTS		(1<<24)
#define SUBHIT_SHAPE_SEGMENTS	(1<<25)
#define SUBHIT_SHAPE_POLYS		(1<<26)
#define SUBHIT_SHAPE_TYPEMASK	(SUBHIT_SHAPE_VERTS|SUBHIT_SHAPE_SEGMENTS|SUBHIT_SHAPE_POLYS)

class Spline3D;

//--------------------------------------------------------------
// Capping classes, etc.

// CapVert flags
#define CAPVERT_VISEDGE (1<<0)

class CapVert {
	public:
		int vert;
		DWORD flags;
		float ang;
		CapVert *prev,*next;
		CapVert *smaller,*bigger;
		CapVert() { vert=0; flags = 0; ang = 0.0f; prev=next=smaller=bigger = NULL; }
	};

//--------------------------------------------------------------

// Predefined PolyPt flags
// Bits 0-7 are available to the user.  Bits 8-31 are reserved for internal use

// Use these to make capping more efficient
// If your class converts to a PolyLine, use them!
#define POLYPT_KNOT 		(1<<8)	// A control point
#define POLYPT_INTERPOLATED (1<<9)	// An interpolated point

// If you convert to a PolyLine, use this bit to control smoothing of the resulting shape
// If this bit is set, it means that any mesh generated will share smoothing across the edge
#define POLYPT_SMOOTH		(1<<10) // Point is part of a smooth transition

#define POLYPT_SEG_SELECTED	(1<<11)	// The segment that starts with this point is selected

// Used internally by capping code
#define POLYPT_BRIDGE		(1<<16)	// Span between two polygons
#define POLYPT_SPLICE		(1<<17)	// Point is endpoint of a bridge
#define POLYPT_VISEDGE		(1<<18)	// Segment should be visible on mesh
#define POLYPT_NO_SPLICE	(1<<19)	// Don't allow a bridge at this point
#define POLYPT_INVIS_EDGE	(1<<20)	// Force segment to be invisible on capping
#define POLYPT_NO_SNAP		(1<<21) // Suppress snapping when set

// Flags2 field contains material IDs:
// The mat ID is stored in the HIWORD of the PolyPt flags2 field
#define POLYPT_MATID_SHIFT	16
#define POLYPT_MATID_MASK	0xFFFF

class PolyPt {
	public:
		Point3 p;
		DWORD flags;	// See above
		DWORD flags2;	// See above
		int aux;		// Auxiliary data attached to this point (usually mesh vertex number for capping)
		PolyPt() { p = Point3(0,0,0); flags = 0; flags2 = 0; aux = 0; }
		PolyPt(Point3 ip, DWORD f=0, int a=0, DWORD f2=0) { p = ip; flags = f; aux = a; flags2 = f2;}
		inline	MtlID	GetMatID() {return (int)((flags2>>POLYPT_MATID_SHIFT)&POLYPT_MATID_MASK);}
		inline	void    SetMatID(MtlID id) {flags2 &= 0xFFFF; flags2 |= (DWORD)(id<<POLYPT_MATID_SHIFT);}
	};

// PolyLine::Cap3DS / PolyShape::Make3DSCap options
#define CAP3DS_OPT_CLOSEST_BRIDGE (1<<0)	// Bridge polys at closest point

// PolyLine flags
#define POLYLINE_CLOSED			(1<<0)
#define POLYLINE_NO_SELF_INT	(1<<1)		// Ignore self-intersections (special!)

class PolyLine {
	public:
		int numPts;
		PolyPt *pts;
		DWORD flags;
		Box3 bdgBox;
		float cachedLength;
		float *lengths;		// Cached lengths for each point
		float *percents;	// Cached percentages for each point
		BOOL cacheValid;
		CoreExport PolyLine();
		CoreExport PolyLine(PolyLine& from);
		CoreExport ~PolyLine();
		CoreExport void Init();
		void Close() { flags |= POLYLINE_CLOSED; }
		CoreExport BOOL IsClosed();
		void Open() { flags &= ~POLYLINE_CLOSED; }
		CoreExport BOOL IsOpen();
		void SetNoSelfInt() { flags |= POLYLINE_NO_SELF_INT; }
		BOOL IsNoSelfInt() { return (flags & POLYLINE_NO_SELF_INT) ? TRUE : FALSE; }
		int Verts() { return numPts; }
		CoreExport int Segments();
		CoreExport BOOL SetNumPts(int count, BOOL keep = TRUE);
		CoreExport void Append(PolyPt& p);
		CoreExport void Insert(int where, PolyPt& p);
		CoreExport void Delete(int where);
		CoreExport void Reverse(BOOL keepZero=FALSE);
		CoreExport PolyLine& operator=(PolyLine& from);
		CoreExport PolyLine& operator=(Spline3D& from);
		CoreExport PolyPt& operator[](int index) { return pts[index]; }
		CoreExport void BuildBoundingBox(void);
		CoreExport void InvalidateGeomCache();
		CoreExport Box3 GetBoundingBox(Matrix3 *tm=NULL); // RB: optional TM allows the box to be calculated in any space.
		CoreExport void Render(GraphicsWindow *gw, Material *ma, RECT *rp, int compFlags, int numMat);
		// New for R4: To render a proxy, colorSegs=TRUE and numMat=-1
		CoreExport void Render(GraphicsWindow *gw, Material *ma, int numMat, BOOL colorSegs, BitArray &segsel);
		CoreExport BOOL Select(GraphicsWindow *gw, Material *ma, HitRegion *hr, int abortOnHit = FALSE);
		CoreExport void Snap(GraphicsWindow *gw, SnapInfo *snap, IPoint2 *p, Matrix3 &tm, DWORD flags);
		CoreExport void Transform(Matrix3 &tm);
		CoreExport void Dump(TCHAR *title = NULL);
		CoreExport void SpliceLine(int where, PolyLine &source, int splicePoint);
		CoreExport BOOL HitsSegment(Point2 p1, Point2 p2, BOOL findAll = FALSE, IntersectionCallback3D *cb = NULL);
		CoreExport int Cap3DS(CapVert *capverts, MeshCapInfo &capInfo, DWORD options = 0);
		CoreExport BOOL HitsPolyLine(PolyLine &line, BOOL findAll = FALSE, IntersectionCallback3D *cb = NULL);
		CoreExport BOOL SurroundsPoint(Point2 &point);
		CoreExport Point3 InterpPiece3D(int segment, float t);
		CoreExport Point3 InterpCurve3D(float u, int ptype=POLYSHP_INTERP_SIMPLE);
		CoreExport Point3 TangentPiece3D(int segment, float t);
		CoreExport Point3 TangentCurve3D(float u, int ptype=POLYSHP_INTERP_SIMPLE);
		CoreExport MtlID GetMatID(int segment);
		CoreExport float CurveLength();
		CoreExport BOOL IsClockWise();			// 2D!
		CoreExport BOOL	SelfIntersects(BOOL findAll = FALSE, IntersectionCallback3D *cb = NULL);		// 2D!
		CoreExport void GetSmoothingMap(IntTab &map);
		// IO
		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);
	};

#define CAPFACE_AB	(1<<0)
#define CAPFACE_BC	(1<<1)
#define CAPFACE_CA	(1<<2)

class ShapeObject;

// Options for steps in MakePolyShape (>=0: Use fixed steps)
// NOTE: DO NOT change these defines -- They're also used by ShapeObject (object.h)
#define PSHAPE_BUILTIN_STEPS -2		// Use the shape's built-in steps/adaptive settings (default)
#define PSHAPE_ADAPTIVE_STEPS -1	// Force adaptive steps

class PolyShape {
	public:
		int numLines;
		PolyLine *lines;
		DWORD flags;
		Box3 bdgBox;

 		// Selection
		ShapeVSel	vertSel;  		// selected vertices
		ShapeSSel	segSel;  		// selected segments
		ShapePSel	polySel;  		// selected polygons

		// Selection level
		DWORD		selLevel;

		// Display attribute flags
		DWORD		dispFlags;

		// Capping caches
		MeshCapInfo morphCap;
		BOOL morphCapCacheValid;
		MeshCapInfo gridCap;
		BOOL gridCapCacheValid;
		PatchCapInfo patchCap;
		BOOL patchCapCacheValid;

		// Hierarchy cache
		ShapeHierarchy cachedHier;
		BOOL hierCacheValid;

		CoreExport PolyShape();
		CoreExport PolyShape(PolyShape& from);
		CoreExport ~PolyShape();
		CoreExport void Init();			// Used by constructors
		CoreExport void NewShape();		// Deletes all lines
		CoreExport BOOL SetNumLines(int count, BOOL keep = TRUE);
		CoreExport PolyLine* NewLine();
		CoreExport void Append(PolyLine &l);
		CoreExport void Insert(int where, PolyLine& l);
		CoreExport void Delete(int where);
		CoreExport PolyShape& operator=(PolyShape& from);
		CoreExport PolyShape& operator=(BezierShape& from);
		CoreExport void BuildBoundingBox(void);
		CoreExport void InvalidateGeomCache(BOOL unused);		// Also invalidates capping caches
		CoreExport void InvalidateCapCache();
		CoreExport Box3 GetBoundingBox(Matrix3 *tm=NULL); // RB: optional TM allows the box to be calculated in any space.
		CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );
		CoreExport void Render(GraphicsWindow *gw, Material *ma, RECT *rp, int compFlags, int numMat);
		CoreExport BOOL Select(GraphicsWindow *gw, Material *ma, HitRegion *hr, int abortOnHit = FALSE);
		CoreExport void Snap(GraphicsWindow *gw, SnapInfo *snap, IPoint2 *p, Matrix3 &tm);
		CoreExport void Snap(GraphicsWindow *gw, SnapInfo *snap, IPoint2 *p, Matrix3 &tm, DWORD flags);
		CoreExport void Transform(Matrix3 &tm);
		CoreExport int MakeCap(TimeValue t, MeshCapInfo &capInfo, int capType);
		CoreExport int MakeCap(TimeValue t, PatchCapInfo &capInfo);
		CoreExport int Make3DSCap(MeshCapInfo &capInfo, DWORD options = 0);
		CoreExport int MakeGridCap(MeshCapInfo &capInfo);
		CoreExport void Dump(TCHAR *title = NULL);
		CoreExport void UpdateCachedHierarchy();
		CoreExport ShapeHierarchy &OrganizeCurves(TimeValue t, ShapeHierarchy *hier = NULL);
		CoreExport void UpdateSels();
		CoreExport void Reverse(int poly, BOOL keepZero=FALSE);
		CoreExport void Reverse(BitArray &reverse, BOOL keepZero=FALSE);
		CoreExport MtlID GetMatID(int poly, int piece);
		// Constructs a vertex selection list based on the current selection level.
		CoreExport BitArray	VertexTempSel(int poly);
		// functions for use in data flow evaluation
		CoreExport void ShallowCopy(PolyShape *ashape, ULONG_PTR channels);
		CoreExport void DeepCopy(PolyShape *ashape, ULONG_PTR channels);
		CoreExport void	NewAndCopyChannels(ULONG_PTR channels);
		CoreExport void FreeChannels( ULONG_PTR channels, int zeroOthers=1);
		// IO
		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);

	};

#endif // __POLYSHP_H_
