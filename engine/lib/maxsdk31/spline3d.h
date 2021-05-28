/**********************************************************************
 *<
	FILE: spline3d.cpp

	DESCRIPTION: General-purpose 3D spline class

	CREATED BY: Tom Hudson & Dan Silva

	HISTORY: created 2/23/95
		4/16/97 TH: Converted to use relative values for bezier vectors

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#ifndef __SPLINE3D_H__

#define __SPLINE3D_H__

#include "polyshp.h"	// Need this for PolyLine class

// Parameter types for shape interpolation (Must match types in object.h)
#define PARAM_SIMPLE 0		// Parameter space based on segments
#define PARAM_NORMALIZED 1	// Parameter space normalized to curve length

// Point flags for PolyShape representation
#define BEZ_SHAPE_KNOT			(1<<0)	// It's a knot point
#define BEZ_SHAPE_INTERPOLATED	(1<<1)	// It's an interpolated point between two knots

// Line types:
#define LTYPE_CURVE 0
#define LTYPE_LINE 1

// Compound line types
#define CURVE_CURVE (LTYPE_CURVE | (LTYPE_CURVE<<2))
#define LINE_CURVE (LTYPE_LINE | (LTYPE_CURVE<<2))
#define CURVE_LINE (LTYPE_CURVE | (LTYPE_LINE<<2))
#define LINE_LINE (LTYPE_LINE | (LTYPE_LINE<<2))

// Knot types
#define KTYPE_AUTO 0
#define KTYPE_CORNER 1
#define KTYPE_BEZIER 2
#define KTYPE_BEZIER_CORNER (KTYPE_BEZIER | KTYPE_CORNER)

// Parameter types
#define PARM_UNIFORM		0
#define PARM_ARCLENGTH		1
#define PARM_CENTRIPETAL	2
#define PARM_CUSTOM			3

// The drawPhase values
#define DRAW_IDLE 0
#define DRAW_INITIAL_MOUSE_DOWN 1
#define DRAW_FREEMOVE_POINT 2
#define DRAW_FREEMOVE_POINT_MOUSE_DOWN 3	// Inserting's initial click inside spline
#define DRAW_INITIAL_BEZ_ADJ 11
#define DRAW_DRAGGING_VECTOR 22

// Parameter types for shape interpolation (Must match interpolation types in object.h)
#define SPLINE_INTERP_SIMPLE 0		// Parameter space based on segments
#define SPLINE_INTERP_NORMALIZED 1	// Parameter space normalized to curve length

class Spline3D;
class SplineKnotAssy;
class SplineKnot;			  

//
// 'aux' fields in spline are available in 3 channels:
// 0: Used in capping process
// 1: Used to track topology changes in spline editing
// 2: Available to user
//

//watje
#define SEGMENT_VISIBLE	(1<<0)


// This class has the vertex-level point information for the Spline3D class
class SplinePoint {
	friend class Spline3D;
	friend class SplineKnotAssy;
	friend class SplineKnot;

	private:
		Point3 point;
		int aux[3];
	public:
		CoreExport SplinePoint();
		CoreExport SplinePoint(Point3 &p, int a1 = -1, int a2 = -1, int a3 = -1);
		CoreExport SplinePoint& operator=(SplinePoint &fromSP);
		inline Point3& GetPoint() { return point; }
		inline int GetAux(int index) { return aux[index]; }
		inline void SetPoint(const Point3 &p) { point = p; }
		inline void SetAux(int index, int a) { aux[index] = a; }
	};

// The mat ID is stored in the HIWORD of the knot flags
#define SPLINE_MATID_SHIFT	16
#define SPLINE_MATID_MASK	0xFFFF

// This class is used for the internal storage of spline knot assemblies
// in the Spline3D class
class SplineKnotAssy {
	friend class Spline3D;
	friend class SplineKnot;

	private:
		int ktype;			// Knot type
		int ltype;			// Line type
		float du;			// Parameter value
		SplinePoint inVec;	// The in vector
		SplinePoint knot;	// The knot
		SplinePoint outVec;	// The out vector
		DWORD flags;
	public:
		CoreExport SplineKnotAssy();
		CoreExport SplineKnotAssy(int k, int l, Point3 p, Point3 in, Point3 out, int a1= -1, int a2= -1, int a3= -1, int Ia1= -1, int Ia2= -1, int Ia3= -1, int Oa1= -1, int Oa2= -1, int Oa3= -1, DWORD f=0);
		CoreExport SplineKnotAssy(int k, int l, SplinePoint p, SplinePoint in, SplinePoint out, DWORD f=0);
		CoreExport SplineKnotAssy(SplineKnot &k);
		inline	int		Ktype() { return ktype; }
		inline	void	SetKtype(int t) { ktype=t; }
		inline	int		Ltype() { return ltype; }
		inline	void	SetLtype(int t) { ltype=t; }
		inline	Point3	Knot() { return knot.point; }
		inline	void	SetKnot(const Point3 &p) { knot.point=p; }
		inline	Point3	InVec() { return inVec.point; }
		inline	void	SetInVec(const Point3 &p) { inVec.point=p; }
		inline	Point3	OutVec() { return outVec.point; }
		inline	void	SetOutVec(const Point3 &p) { outVec.point=p; }
		inline	float	GetParm() { return du; }
		inline	void	SetParm(float p) { du = p; }
		inline	MtlID	GetMatID() {return (int)((flags>>SPLINE_MATID_SHIFT)&SPLINE_MATID_MASK);}
		inline	void    SetMatID(MtlID id) {flags &= 0xFFFF; flags |= (DWORD)(id<<SPLINE_MATID_SHIFT);}
		
		// The following methods allow access as if the in/knot/out components
		// are contained vertices.  index:0=inVec 1=knot 2=outVec  which: 0=aux1 1=aux2 2=aux3
		CoreExport int	GetAux(int index, int which);
		CoreExport void	SetAux(int index, int which, int value);
		CoreExport Point3 GetVert(int index);
		CoreExport void SetVert(int index, const Point3 &p);

		inline	SplinePoint GetKnot() { return knot; }
		inline	SplinePoint GetInVec() { return inVec; }
		inline	SplinePoint GetOutVec() { return outVec; }
		inline	void SetKnot(SplinePoint &sp) { knot = sp; }
		inline	void SetInVec(SplinePoint &sp) { inVec = sp; }
		inline	void SetOutVec(SplinePoint &sp) { outVec = sp; }


//watje		
		inline	BOOL	IsHidden() {return (flags&SEGMENT_VISIBLE);}
		inline	void    Hide() { flags |= (DWORD)(SEGMENT_VISIBLE);}
		inline	void    Unhide() { flags &= (DWORD)(~SEGMENT_VISIBLE);}

	};

// This class is used by plugins to get and set knot information in the Spline3D class.
// This is primarily here for backward-compatibility with versions prior to MAXr3

class SplineKnot {
	friend class Spline3D;
	friend class SplineKnotAssy;

	int ktype;
	int ltype;
	Point3 point;
	Point3 inVec;
	Point3 outVec;
	int aux;		// Used for capping
	int aux2;		// Used to track topo changes in spline editing
	int aux3;		// User aux field
	int inAux;
	int inAux2;
	int inAux3;
	int outAux;
	int outAux2;
	int outAux3;
	DWORD flags;
public:
	CoreExport SplineKnot();
	CoreExport SplineKnot(int k, int l, Point3 p, Point3 in, Point3 out, int a1= -1, int a2= -1, int a3= -1, int Ia1= -1, int Ia2= -1, int Ia3= -1, int Oa1= -1, int Oa2= -1, int Oa3= -1, DWORD f=0);
	CoreExport SplineKnot(SplineKnotAssy &k);
	inline	int		Ktype() { return ktype; }
	inline	void	SetKtype(int t) { ktype=t; }
	inline	int		Ltype() { return ltype; }
	inline	void	SetLtype(int t) { ltype=t; }
	inline	int		Aux() { return aux; }
	inline	void	SetAux(int a) { aux=a; }
	inline	int		Aux2() { return aux2; }
	inline	void	SetAux2(int a) { aux2=a; }
	inline	int		Aux3() { return aux3; }
	inline	void	SetAux3(int a) { aux3=a; }
	inline	int		InAux() { return inAux; }
	inline	void	SetInAux(int a) { inAux=a; }
	inline	int		InAux2() { return inAux2; }
	inline	void	SetInAux2(int a) { inAux2=a; }
	inline	int		InAux3() { return inAux3; }
	inline	void	SetInAux3(int a) { inAux3=a; }
	inline	int		OutAux() { return outAux; }
	inline	void	SetOutAux(int a) { outAux=a; }
	inline	int		OutAux2() { return outAux2; }
	inline	void	SetOutAux2(int a) { outAux2=a; }
	inline	int		OutAux3() { return outAux3; }
	inline	void	SetOutAux3(int a) { outAux3=a; }
	inline	Point3	Knot() { return point; }
	inline	void	SetKnot(Point3 p) { point=p; }
	inline	Point3	InVec() { return inVec; }
	inline	void	SetInVec(Point3 p) { inVec=p; }
	inline	Point3	OutVec() { return outVec; }
	inline	void	SetOutVec(Point3 p) { outVec=p; }
	inline	MtlID	GetMatID() {return (int)((flags>>SPLINE_MATID_SHIFT)&SPLINE_MATID_MASK);}
	inline	void    SetMatID(MtlID id) {flags &= 0xFFFF; flags |= (DWORD)(id<<SPLINE_MATID_SHIFT);}

//watje
	inline	BOOL	IsHidden() {return (flags&SEGMENT_VISIBLE);}
	inline	void    Hide() { flags |= (DWORD)(SEGMENT_VISIBLE);}
	inline	void    Unhide() { flags &= (DWORD)(~SEGMENT_VISIBLE);}

	};

// Private spline flags
#define SPLINE_CLOSED	(1<<0)
#define SPLINE_ORTHOG   (1<<1)

class Spline3D {
    friend class BezierShape;
    friend class SplineShape;
private:
	static	int			splineCount;	// Number of splines in the system
			int			parmType;		// Interpolation parameter type	(needed?)
			int			knotCount;		// Number of points in spline

			int			flags;			// Private flags
			int			iCur;			// Current editing point
			int			Interact(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3* mat, IObjParam *ip=NULL );	// Handle mouse interaction
			float		cachedLength;
			BOOL		cacheValid;
			SplineKnotAssy*	knots;		// Knot assembly array
			float *		lengths;	// Cached lengths
#ifdef DESIGN_VER
public:
#endif // break encapsulation to avoid changing the API
       // used by railings - which must be modified if use of percents is modified!
      float *		percents;	// Cached length percentages
#ifdef DESIGN_VER
private:
#endif
			int			drawPhase;		// Drawing phase
			int			editMode;		// 1 if editing, 0 otherwise

			// Creation settings
			int			initialType;	// Knot type at initial click
			int			dragType;		// Knot type at drag
			
			BOOL		selfInt;		// Cached self-intersection flag
			BOOL		siCacheValid;	// Self-intersection cache valid?
			Box3		bbox;			// Cached bounding box
			BOOL		bboxValid;		// Bounding box valid?
			BOOL		paramsValid;	// Knot parameter values valid?
			BOOL		bezPointsValid;	// Bezier points valid?
			BOOL		clockwise;		// Clockwise cache
			BOOL		cwCacheValid;	// Clockwise cache valid?
			PolyLine	polyLine;		// Polyline cache
			BOOL		plineOpt;
			int			plineSteps;
			BOOL		plineCacheValid;	// Polyline cache valid?
protected:
	CoreExport		void		Allocate(int count);
	CoreExport		void		ChordParams();							// Compute chord length params
	CoreExport		void		UniformParams();						// Compute uniform params
	CoreExport		void		CentripetalParams();					// Compute centripetal params
	CoreExport		void		LinearFwd(int i);
	CoreExport		void		LinearBack(int i);
	CoreExport		void		ContinFwd(int i);
	CoreExport		void		ContinBack(int i);
	CoreExport		void		HybridPoint(int i);
	CoreExport		void		CompCornerBezPoints(int n);
	CoreExport		void		CompAdjBesselBezPoints(int i);
	CoreExport		void		BesselStart(int i);
	CoreExport		void		BesselEnd(int i);
	CoreExport		void		NaturalFwd(int i);
	CoreExport		void		NaturalBack(int i);
public:

	CoreExport		Spline3D(int itype = KTYPE_CORNER,int dtype = KTYPE_BEZIER,int ptype = PARM_UNIFORM);		// Constructor	
	CoreExport		Spline3D(Spline3D& fromSpline);
	CoreExport		~Spline3D();	// Destructor
	CoreExport		Spline3D& 	operator=(Spline3D& fromSpline);
	CoreExport		Spline3D& 	operator=(PolyLine& fromLine);
	CoreExport		void		NewSpline();
	inline			int			ParmType() { return parmType; };
	inline			int			KnotCount() { return knotCount; }						// Point (knot) count
	inline			int			Flags() { return flags; }
	CoreExport		int			Segments();												// Segment count
	inline			int			Closed() { return (flags & SPLINE_CLOSED) ? 1:0; }		// Returns closed status
	CoreExport		int			ShiftKnot(int where,int direction);						// Shove array left or right 1,
																						// starting at given point
	CoreExport		int			AddKnot(SplineKnot &k,int where = -1);					// Add a knot to the spline
	CoreExport		void		SetKnot(int i, SplineKnot &k);
	CoreExport		SplineKnot	GetKnot(int i);

	CoreExport		int			DeleteKnot(int where);									// Delete the specified knot
	CoreExport		int			Create(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3* mat, IObjParam *ip=NULL);	// Create the spline
	CoreExport		int			StartInsert(ViewExp *vpt,int msg, int point, int flags, IPoint2 theP, Matrix3* mat, int where );	// Start an insertion operation on the spline
	CoreExport		int			SetParam(int index,float param);		// Set custom param value
	CoreExport		float		GetParam(int index);					// Get param value
	inline			int			GetKnotType(int index) { return knots[index].ktype; }
	CoreExport		int			SetKnotType(int index,int type);		// Set the knot type
	inline			int			GetLineType(int index) { return knots[index].ltype; }
	CoreExport		int			SetLineType(int index,int type);		// Set the line type
	virtual			void		CustomParams() { UniformParams(); }		// Replace this as needed
	CoreExport		void		CompParams();							// Compute param values
	CoreExport		void		ComputeBezPoints();
	CoreExport		void		FindSegAndParam(float u, int ptype, int &seg, float &param);	// Find segment and parameter for whole-curve parameter
	CoreExport		void		RefineCurve(float u, int ptype=SPLINE_INTERP_SIMPLE);
	CoreExport		void		RefineSegment(int segment, float t, int ptype=SPLINE_INTERP_SIMPLE);
	CoreExport		Point2		InterpBezier(IPoint2 *bez, float t);
	CoreExport		Point3		InterpBezier3D(int segment, float t, int ptype=SPLINE_INTERP_SIMPLE);
	CoreExport		Point3		InterpCurve3D(float u, int ptype=SPLINE_INTERP_SIMPLE);
	CoreExport		Point3		TangentBezier3D(int segment, float t, int ptype=SPLINE_INTERP_SIMPLE);
	CoreExport		Point3		TangentCurve3D(float u, int ptype=SPLINE_INTERP_SIMPLE);
	CoreExport		Point3		AverageTangent(int i);
	CoreExport		void		MakeBezCont(int i);
	CoreExport		void		RedistTangents(int i, Point3 d);
	CoreExport		void		FixAdjBezTangents(int i);
	CoreExport		void		DrawCurve(GraphicsWindow *gw, Material *mtl);
	inline			void		SetEditMode(int mode) { editMode = mode ? 1:0; }
	CoreExport		int			IsAuto(int i);
	CoreExport		int			IsBezierPt(int i);
	CoreExport		int			IsCorner(int i);
	CoreExport		Point3		GetDragVector(ViewExp *vpt,IPoint2 p,int i,Matrix3* mat);
	CoreExport		int			AppendPoint(ViewExp *vpt,const Point3& p, int where = -1);
	CoreExport		int			DrawPhase() { return drawPhase; }
	CoreExport		int			GetiCur() { return iCur; }
	CoreExport		void		GetBBox(TimeValue t,  Matrix3& tm, Box3& box);
	CoreExport		IPoint2		ProjectPoint(ViewExp *vpt, Point3 fp, Matrix3 *mat);
	CoreExport		Point3		UnProjectPoint(ViewExp *vpt, IPoint2 p, Matrix3 *mat);
	CoreExport		void		Snap(GraphicsWindow *gw, SnapInfo *snap, IPoint2 *p, Matrix3 &tm);
	CoreExport		IOResult 	Save(ISave *isave);
	CoreExport		IOResult	Load(ILoad *iload);
	CoreExport		int			SetClosed(int flag = 1);
	CoreExport		int			SetOpen();
	CoreExport		void		Dump(int where);
	CoreExport 		Point3		GetInVec(int i);
	CoreExport		void		SetInVec(int i, const Point3 &p);
	CoreExport		Point3		GetRelInVec(int i);
	CoreExport		void		SetRelInVec(int i, const Point3 &p);
	CoreExport		Point3		GetKnotPoint(int i);
	CoreExport		void		SetKnotPoint(int i, const Point3 &p);
	CoreExport		Point3		GetOutVec(int i);
	CoreExport		void		SetOutVec(int i, const Point3 &p);
	CoreExport		Point3		GetRelOutVec(int i);
	CoreExport		void		SetRelOutVec(int i, const Point3 &p);
	// The following methods return absolute coords for the bezier vertices
	CoreExport		Point3		GetVert(int i);
	CoreExport		void		SetVert(int i, const Point3& p);
	inline			int			Verts() { return knotCount*3; }
	// The following methods get/set the knot aux fields based on knot index
	// These are here for backward compatibility with MAXr2
	CoreExport		int			GetAux(int knot);
	CoreExport		void		SetAux(int knot, int value);
	CoreExport		int			GetAux2(int knot);
	CoreExport		void		SetAux2(int knot, int value);
	CoreExport		int			GetAux3(int knot);
	CoreExport		void		SetAux3(int knot, int value);
	// The following methods are new to MAXr3 and get/set aux fields for in/knot/out
	// knot: knot index
	// which: 0=aux1 1=aux2 2=aux3
	CoreExport		int			GetKnotAux(int knot, int which);
	CoreExport		void		SetKnotAux(int knot, int which, int value);
	CoreExport		int			GetInAux(int knot, int which);
	CoreExport		void		SetInAux(int knot, int which, int value);
	CoreExport		int			GetOutAux(int knot, int which);
	CoreExport		void		SetOutAux(int knot, int which, int value);
	// The following methods get/set the aux fields	based on bezier vertex index
	// i: bezier vertex index
	// which: 0=aux1 1=aux2 2=aux3
	CoreExport		int			GetVertAux(int i, int which);
	CoreExport		void		SetVertAux(int i, int which, int value);
	// The following methods get/set the material ID for a spline segment
	CoreExport		MtlID		GetMatID(int seg);
	CoreExport		void		SetMatID(int seg, MtlID id);

	CoreExport		float		SplineLength();
	CoreExport		float		SegmentLength(int seg);
	CoreExport		void		Transform(Matrix3 *tm);
	CoreExport		void		Reverse(BOOL keepZero = FALSE);
	CoreExport		BOOL		Append(Spline3D *spline, BOOL weldCoincidentFirstVertex=TRUE);	// Returns TRUE if first point auto-welded
	CoreExport		BOOL		Prepend(Spline3D *spline, BOOL weldCoincidentLastVertex=TRUE);	// Returns TRUE if last point auto-welded
	CoreExport		BOOL		IsClockWise();			// 2D!
	CoreExport		BOOL		SelfIntersects();		// 2D!
	CoreExport		BOOL		IntersectsSpline(Spline3D *spline);		// 2D!
	CoreExport		BOOL		SurroundsPoint(Point2 p);	// 2D!
	CoreExport		void		MakePolyLine(PolyLine &line, int steps = -1, BOOL optimize = FALSE);
	CoreExport		void		InvalidateGeomCache();
	CoreExport		void		GetSmoothingMap(IntTab &map);
	};

#endif // __SPLINE3D_H__
