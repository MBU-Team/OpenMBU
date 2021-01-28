/**********************************************************************
 *<
	FILE: Manipulator.h
				  
	DESCRIPTION:  Defines Manipulator clasess

	CREATED BY: Scott Morrison

	HISTORY: created 18 October 1999

 *>     Copyright (c) 1999, All Rights Reserved.
 **********************************************************************/

#pragma once

#ifdef MANIPSYS_IMP
#define ManipExport __declspec(dllexport)
#else
#define ManipExport __declspec(dllimport)
#endif

#include "iparamb2.h"
#include "iFnPub.h"

// Helper geometry classes

enum DisplayState { kNoRedrawNeeded, kFullRedrawNeeded, kPostRedrawNeeded };

#define MANIP_PLANE_INTERFACE  Interface_ID(0x44460ea4, 0xbf73be6)

class Plane : public FPMixinInterface {
public:
    ManipExport Plane(Point3& normal, Point3& point);
    ManipExport Plane(Point3& p1, Point3& p2, Point3& p3);
    ManipExport Plane(): mNormal(0,0,1), mPoint(0,0,0), mD(0.0f) {}

    ManipExport bool Intersect(Ray& ray, Point3& intersectionPoint);
    ManipExport Point3& GetNormal() { return mNormal; }
    ManipExport Point3& GetPoint()  { return mPoint; }
    ManipExport float   GetPlaneConstant() { return mD; }
    ManipExport Plane&  MostOrthogonal(Ray& viewDir, Plane& plane);

    ManipExport static Plane msXYPlane;
    ManipExport static Plane msXZPlane;
    ManipExport static Plane msYZPlane;
    
	// Function IDs
	enum { intersect, mostOrthogonal, getNormal, getPoint, getPlaneConstant, };
	// Function Map
	BEGIN_FUNCTION_MAP
		FN_2(intersect,		 TYPE_BOOL,		 Intersect,		   TYPE_RAY_BV, TYPE_POINT3_BR);
		FN_2(mostOrthogonal, TYPE_INTERFACE, FPMostOrthogonal, TYPE_RAY_BV, TYPE_INTERFACE);
		RO_PROP_FN(getNormal,		 GetNormal,			TYPE_POINT3_BV);
		RO_PROP_FN(getPoint,		 GetPoint,			TYPE_POINT3_BV);
		RO_PROP_FN(getPlaneConstant, GetPlaneConstant,	TYPE_FLOAT);
	END_FUNCTION_MAP

	// FP interface type-converter wrappers
	ManipExport Plane*	FPMostOrthogonal(Ray& viewRay, FPInterface* plane);

	ManipExport FPInterfaceDesc* GetDesc();

private:
    Point3  mNormal;  // Plane normal vector
    Point3  mPoint;   // Point that the plane passes through
    float   mD;       // Plane equation constant
};

#define MANIP_GIZMO_INTERFACE  Interface_ID(0x124e3169, 0xf067ad4)

class GizmoShape: public FPMixinInterface {
public:
    ManipExport GizmoShape() { mLine.Init(); }

    ManipExport void StartNewLine() {
        if (mLine.numPts > 0)
            mPolyShape.Append(mLine);
        mLine.Init();
    }
    ManipExport void AppendPoint(Point3& p) {
        mLine.Append(PolyPt(p));
    }

    ManipExport PolyShape* GetPolyShape() {
        if (mLine.numPts > 0)
            mPolyShape.Append(mLine);
        mLine.Init();
        return &mPolyShape;
    }

    ManipExport void Transform(Matrix3& tm);

	// Function IDs
	enum { startNewLine, appendPoint, transform};
	// Function Map
	BEGIN_FUNCTION_MAP
		VFN_0(startNewLine, StartNewLine);
		VFN_1(appendPoint,  AppendPoint, TYPE_POINT3_BV);
		VFN_1(transform,    Transform,   TYPE_MATRIX3_BV);
	END_FUNCTION_MAP

	ManipExport FPInterfaceDesc* GetDesc();

private:
    PolyShape mPolyShape;
    PolyLine  mLine;
};


//  Manipulator system static FnPub interace
#define MANIP_MGR_INTERFACE  Interface_ID(0x2c450aa2, 0x7b9d0365)
class IManipulatorMgr : public FPStaticInterface
{
public:
	// stock gizmos
	virtual Mesh* MakeSphere(Point3& pos, float radius, int segments)=0;
	virtual Mesh* MakeTorus(Point3& pos, float radius, float radius2, int segs, int sides)=0;
	virtual Mesh* MakeBox(Point3& pos, float l, float w, float h, int lsegs, int wsegs, int hsegs)=0;

	// plane construction
	virtual Plane* MakePlane()=0;
	virtual Plane* MakePlane(Point3& p1, Point3& p2, Point3& p3)=0;
	virtual Plane* MakePlane(Point3& normal, Point3& point)=0;

	// constant planes
	virtual Plane* GetmsXYPlane()=0;
	virtual Plane* GetmsXZPlane()=0;
	virtual Plane* GetmsYZPlane()=0;

    // PolyShape gizmos
    virtual GizmoShape* MakeGizmoShape()=0;
    virtual GizmoShape* MakeCircle(Point3& center, float radius, int segments)=0;

	// Function IDs
	enum { makeSphere, makeTorus, makeBox, makePlane, makePlaneFromPts,
           makePlaneFromNormal, getmsXYPlane, getmsXZPlane, getmsYZPlane,
           makeGizmoShape, makeCircle, };
};

class ManipHitData;

class Manipulator : public HelperObject
{
public:
    
    ManipExport Manipulator(INode* pINode) { mpINode = pINode; }

    BOOL IsManipulator() { return TRUE; }

    virtual int HitTest(TimeValue t, INode* pNode, int type, int crossing,
                        int flags, IPoint2 *pScreenPoint, ViewExp *pVpt) = 0;
    virtual int Display(TimeValue t, INode* pNode, ViewExp *pVpt, int flags) = 0;

    virtual void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vp,  Box3& box ) = 0;

    // Used for manipulator set manager, which is always active.
    ManipExport virtual bool AlwaysActive() { return false; }

    virtual TSTR& GetManipName() = 0;

    // FIXME these methods should use an FP interface.

    virtual DisplayState  MouseEntersObject(TimeValue t, ViewExp* pVpt, IPoint2& m, ManipHitData* pHitData)
        {return kNoRedrawNeeded; }
    virtual DisplayState  MouseLeavesObject(TimeValue t, ViewExp* pVpt, IPoint2& m, ManipHitData* pHitData)
        {return kNoRedrawNeeded; }
    
    virtual void OnMouseMove(TimeValue t, ViewExp* pVpt, IPoint2& m, DWORD flags, ManipHitData* pHitData) {}
    virtual void OnButtonDown(TimeValue t, ViewExp* pVpt, IPoint2& m, DWORD flags, ManipHitData* pHitData) {}
    virtual void OnButtonUp(TimeValue t, ViewExp* pVpt, IPoint2& m, DWORD flags, ManipHitData* pHitData) {}

    virtual INode* GetINode() { return mpINode; }

	virtual void DeleteThis() { delete this; }

protected:

    INode*  mpINode;   // The node being manipulated

};

class ManipHitList;

class ManipulatorGizmo : public BaseInterfaceServer
{
public:
    ManipExport ManipulatorGizmo();
    ManipExport ManipulatorGizmo(PolyShape* pShape, DWORD flags,
                     Point3& unselColor,
                     Point3& selColor =  GetSubSelColor());
    ManipExport ManipulatorGizmo(Mesh* pMesh, DWORD flags,
                     Point3& unselColor,
                     Point3& selColor = GetSubSelColor());
    ManipExport ManipulatorGizmo(MarkerType markerType, Point3& position,
                     DWORD flags,
                     Point3& unselColor,
                     Point3& selColor = GetSubSelColor());
    ManipExport ManipulatorGizmo(TCHAR* pText, Point3& position,
                     DWORD flags,
                     Point3& unselColor,
                     Point3& selColor = GetSubSelColor());
    ManipExport ~ManipulatorGizmo();
                     

    ManipExport BOOL HitTest(GraphicsWindow* pGW, HitRegion* pHR, ManipHitList* pHitList,
                 Matrix3* tm, IPoint2 pScreenPoint, int gizmoIndex);

    ManipExport void Render(ViewExp* pVpt, TimeValue t, INode* pNode, int flags, bool selected, BOOL isStandAlone);

    ManipExport Box3 GetBoundingBox(INode* pNode, ViewExp* pVpt);

    // Gizmo flags

    // Don't display this gizmo.  It is still hit-tested.
    ManipExport static const DWORD kGizmoDontDisplay;

    // Don't hit test this gizmo.  It is still displayed.
    ManipExport static const DWORD kGizmoDontHitTest;

    // Scale this gizmo to viewport size, using mGizmoSize as the size in pixels
    // Only for mesh and shape gizmos.
    ManipExport static const DWORD kGizmoScaleToViewport;

    // The coordinates are in normalized screen space.  the X and Y values are
    // in the range 0.0 to 1.0, and interpreted as percentages of screen space.
    // This is only supported for PolyShape, Marker and Text gizmos.
    ManipExport static const DWORD kGizmoUseRelativeScreenSpace;  

    // The coordinates are in screen space.  
    // This is only supported for PolyShape, Marker and Text gizmos.
    ManipExport static const DWORD kGizmoUseScreenSpace;  

    // Only display the gizmo in the active viewport.
    ManipExport static const DWORD kGizmoActiveViewportOnly;

private:
    void RenderMesh(ViewExp* pVpt, TimeValue t, INode* pNode, int flags, bool selected, BOOL isStandAlone);
    void RenderShape(ViewExp* pVpt, TimeValue t, INode* pNode, int flags, bool selected, BOOL isStandAlone);
    void RenderMarker(ViewExp* pVpt, TimeValue t, INode* pNode, int flags, bool selected, BOOL isStandAlone);
    void RenderText(ViewExp* pVpt, TimeValue t, INode* pNode, int flags, bool selected, BOOL isStandAlone);

    BOOL HitTestShape(GraphicsWindow* pGW, HitRegion* pHR, ManipHitList* pHitList,
                      Matrix3* tm, IPoint2 pScreenPoint, int gizmoIndex);
    BOOL HitTestMesh(GraphicsWindow* pGW, HitRegion* pHR, ManipHitList* pHitList,
                     Matrix3* tm, IPoint2 pScreenPoint, int gizmoIndex);
    BOOL HitTestMarker(GraphicsWindow* pGW, HitRegion* pHR, ManipHitList* pHitList,
                      Matrix3* tm, IPoint2 pScreenPoint, int gizmoIndex);
    BOOL HitTestText(GraphicsWindow* pGW, HitRegion* pHR, ManipHitList* pHitList,
                     Matrix3* tm, IPoint2 pScreenPoint, int gizmoIndex);

    Box3 GetShapeBoundingBox(INode* pNode, ViewExp* pVpt);
    Box3 GetMeshBoundingBox(INode* pNode, ViewExp* pVpt);
    Point3 GetMarkerBoundingBox(INode* pNode, ViewExp* pVpt);
    Box3 GetTextBoundingBox(INode* pNode, ViewExp* pVpt);

    void GetScaleFactor(GraphicsWindow* pGW, Point3& scale, Point3& center);

    void GetScreenCoords(GraphicsWindow* pGW, Point3& input, int& x, int& y);

    BOOL UseScreenSpace() { return mFlags & kGizmoUseRelativeScreenSpace ||
                                   mFlags & kGizmoUseScreenSpace; }

    PolyShape*  mpShape;      // Polyshape gizmo
    Mesh*       mpMesh;       // Mesh gizmo

    Point3      mPosition;    // Used for markers and text
    MarkerType* mpMarkerType; // Used for marker gizmos
    TSTR*       mpText;       // Used for text gizmos 

    Point3      mUnselColor;  // Color of gizmo
    Point3      mSelColor;    // Color of gizmo when selected
    DWORD       mFlags;       // Display and hit testing flags

    // The size of the gizmo in pixels for kGizmoScaleToViewport gizmos.
    int         mGizmoSize;
};

enum MouseState {
    kMouseIdle,
    kMouseDragging,
    kMouseOverManip,
};

// Manipulator with a built-in ParamBlock2 and many methods implemented
// by default.
// SimpleManipulator also provides support for a table of meshes ,
// poly shapes, markers and text for use as gizmos.

// FnPub interface to SimpleManipulators (for scripted manipulators)
#define SIMPLE_MANIP_INTERFACE  Interface_ID(0x617c41d4, 0x6af06a5f)

class ISimpleManipulator : public FPMixinInterface
{
public:
	// the published API
    virtual void	ClearPolyShapes()=0;

    virtual void	AppendPolyShape(PolyShape* pPolyShape, DWORD flags,
                                    Point3& unselColor,
                                    Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected))=0;
    
    virtual void	AppendGizmo(GizmoShape* pGizmoShape, DWORD flags,
                                Point3& unselColor,
                                Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected))=0;
    
    virtual void	AppendMesh(Mesh* pMesh, DWORD flags,
                               Point3& unselColor,
                               Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected))=0;
    
    virtual void	AppendMarker(MarkerType markerType, Point3& position,
                                 DWORD flags,
                                 Point3& unselColor,
                                 Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected))=0;
    
    virtual void	AppendText(TCHAR* pText, Point3& position,
                               DWORD flags,
                               Point3& unselColor,
                               Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected))=0;
    
    virtual MouseState GetMouseState()=0;
    virtual void	GetLocalViewRay(ViewExp* pVpt, IPoint2& m, Ray& viewRay)=0;;
    virtual void	UpdateShapes(TimeValue t, TSTR& toolTip)=0;
    virtual void	OnMouseMove(TimeValue t, ViewExp* pVpt, IPoint2& m, DWORD flags, ManipHitData* pHitData)=0;

	// Function IDs
	enum { clearPolyShapes, appendPolyShape, appendMesh, getMouseState, getLocalViewRay, 
			updateShapes, onMouseMove, appendGizmo, appendMarker, appendText};
	// enumeration IDs
	enum { mouseState, markerType, };
	// Function Map
	BEGIN_FUNCTION_MAP
		VFN_0(clearPolyShapes,				  ClearPolyShapes	 );
		VFN_4(appendMesh,					  FPAppendMesh,		 TYPE_MESH, TYPE_INT, TYPE_POINT3_BV, TYPE_POINT3_BV);
		VFN_4(appendGizmo,					  FPAppendGizmo,	 TYPE_INTERFACE, TYPE_INT, TYPE_POINT3_BV, TYPE_POINT3_BV);
		VFN_5(appendMarker,					  FPAppendMarker,    TYPE_ENUM, TYPE_POINT3_BV, TYPE_INT, TYPE_POINT3_BV, TYPE_POINT3_BV);
		VFN_5(appendText,					  AppendText,	     TYPE_STRING, TYPE_POINT3_BV, TYPE_INT, TYPE_POINT3_BV, TYPE_POINT3_BV);
		VFN_2(updateShapes,					  UpdateShapes,		 TYPE_TIMEVALUE, TYPE_TSTR_BR);
//		VFN_3(onMouseMove,					  FPOnMouseMove,	 TYPE_TIMEVALUE, TYPE_POINT2_BV, TYPE_INT);
		FN_1(getLocalViewRay,    TYPE_RAY_BV, FPGetLocalViewRay, TYPE_POINT2_BV);
		RO_PROP_FN(getMouseState,			  GetMouseState,	 TYPE_ENUM);
	END_FUNCTION_MAP

	// FP interface type-converter wrappers
	ManipExport Ray		FPGetLocalViewRay(Point2& m);
	ManipExport void	FPAppendMesh(Mesh* pMesh, DWORD flags, Point3& unselColor, Point3& selColor);
	ManipExport void	FPAppendGizmo(FPInterface* pGizmo, DWORD flags, Point3& unselColor, Point3& selColor);
//	ManipExport void	FPOnMouseMove(TimeValue t, Point2& m, DWORD flags);
    ManipExport void	FPAppendMarker(int markerType, Point3& position,
                                 DWORD flags, Point3& unselColor, Point3& selColor);

	ManipExport FPInterfaceDesc* GetDesc();
};

class SimpleManipulator: public Manipulator, public ISimpleManipulator
{
public:

    ManipExport SimpleManipulator();
    ManipExport SimpleManipulator(INode* pNode);
    ManipExport ~SimpleManipulator();

    // ReferenceMaker functions
    ManipExport int NumRefs();
    ManipExport RefTargetHandle GetReference(int i);
    ManipExport void SetReference(int i, RefTargetHandle rtarg);
    ManipExport RefResult NotifyRefChanged(Interval changeInt,RefTargetHandle hTarget,
                               PartID& partID, RefMessage message);
    
    // From Object
    ManipExport ObjectState Eval(TimeValue time);
    void InitNodeName(TSTR& s) {s = GetObjectName();}
    ManipExport Interval ObjectValidity(TimeValue t);
    
    // From GeomObject
    ManipExport void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
    ManipExport void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
    ManipExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel );
    ManipExport void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
    ManipExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
    
    // Animatable methods
    ManipExport void GetClassName(TSTR& s) {s = GetObjectName();}		
    ManipExport int NumSubs() { return 1; }  
    ManipExport Animatable* SubAnim(int i) { return mpPblock; }
    ManipExport TSTR SubAnimName(int i);
	BaseInterface* GetInterface(Interface_ID id) { if (id == SIMPLE_MANIP_INTERFACE) return (ISimpleManipulator*)this; else return FPMixinInterface::GetInterface(id); }
    
    // Implement the basic manipulator operations
    ManipExport int HitTest(TimeValue t, INode* pNode, int type, int crossing,
                int flags, IPoint2 *pScreenPoint, ViewExp *pVpt);
    ManipExport int Display(TimeValue t, INode* pNode, ViewExp *pVpt, int flags);
    ManipExport static const int kNoneSelected;

    ManipExport void ClearPolyShapes();
    ManipExport void AppendPolyShape(PolyShape* pPolyShape, DWORD flags,
                                     Point3& unselColor,
                                     Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected));
    ManipExport void AppendGizmo(GizmoShape* pGizmoShape, DWORD flags,
                                 Point3& unselColor,
                                 Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected));
    ManipExport void AppendMesh(Mesh* pMesh, DWORD flags,
                                Point3& unselColor,
                                Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected));
    ManipExport void AppendMarker(MarkerType markerType, Point3& position,
                                  DWORD flags,
                                  Point3& unselColor,
                                  Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected));
    ManipExport void AppendText(TCHAR* pText, Point3& position,
                                DWORD flags,
                                Point3& unselColor,
                                Point3& selColor =  ColorMan()->GetColorAsPoint3(kManipulatorsSelected));

    ManipExport TSTR& GetManipName() {return mToolTip; }

    ManipExport void SetGizmoScale(float gizmoScale) { mGizmoScale = gizmoScale; }
    ManipExport TSTR& GetToolTip() { return mToolTip; }
    ManipExport void SetToolTipWnd(HWND hWnd) { mToolTipWnd = hWnd; }
    ManipExport void SetToolTipTimer(UINT timer) { mToolTipTimer = timer; }
    ManipExport UINT GetToolTipTimer() { return mToolTipTimer; }
    ManipExport HWND GetToolTipWnd() { return mToolTipWnd; }

    ManipExport IParamBlock2* GetPBlock() { return mpPblock; }

    // These must be implemented in the sub-class of SimpleManipulator

    // Called when the sub-class needs to update it's poly shapes
    // The toolTip string is used to signal
    virtual void UpdateShapes(TimeValue t, TSTR& toolTip) = 0;

    ManipExport virtual void ManipulatorSelected() {};

    ManipExport void SetManipTarget(RefTargetHandle hTarg);
    ManipExport RefTargetHandle GetManipTarget() { return mhTarget; }

    ManipExport void SetMouseState(MouseState state) { mState = state; }
    ManipExport MouseState GetMouseState() { return mState; }

    ManipExport void OnButtonDown(TimeValue t, ViewExp* pVpt, IPoint2& m, DWORD flags, ManipHitData* pHitData);
    ManipExport void OnMouseMove(TimeValue t, ViewExp* pVpt, IPoint2& m, DWORD flags, ManipHitData* pHitData);
    ManipExport void OnButtonUp(TimeValue t, ViewExp* pVpt, IPoint2& m, DWORD flags, ManipHitData* pHitData);

    ManipExport DisplayState  MouseEntersObject(TimeValue t, ViewExp* pVpt, IPoint2& m, ManipHitData* pHitData);
    ManipExport DisplayState  MouseLeavesObject(TimeValue t, ViewExp* pVpt, IPoint2& m, ManipHitData* pHitData);

    ManipExport IPoint2& GetTipPos() { return mToolTipPos; }

    // Get the view ray going through the given screen coordinate.
    // result is in local coordinates of the owning INode.
    ManipExport void GetLocalViewRay(ViewExp* pVpt, IPoint2& m, Ray& viewRay);

    ManipExport Invalidate() { mValid = NEVER; }

    // From Object
    BOOL UseSelectionBrackets() { return FALSE; }

    ManipExport void UnRegisterViewChange(BOOL fromDelete = FALSE);
    void RegisterViewChange();
    void SetResettingFlag(BOOL val) { mResetting = val; }
    BOOL GetResettingFlag() { return mResetting; }
    ManipExport void KillToolTip();

    ManipExport Point3 GetUnselectedColor();
    ManipExport BOOL ActiveViewOnly() { return mActiveViewOnly; }

protected:
    // Index of manip that mouse is over, for display
    int     mDispSelectedIndex; 
    TSTR    mToolTip;       // text and location for tooltip
    float   mGizmoScale;
    IParamBlock2 *mpPblock;
    Interval   mValid; // Validity of reference
    RefTargetHandle mhTarget;  // The object/modifier/controller being manipulated

    MouseState mState;

    BOOL mActiveViewOnly;
    BOOL mResetting;

private:

    void StartToolTipTimer(HWND hWnd, IPoint2& m);


    Tab<ManipulatorGizmo*>   mGizmos;

    // Tooltip management
    HWND     mToolTipWnd;
    HWND     mToolTipParent;
    UINT     mToolTipTimer;
    IPoint2  mToolTipPos;

    bool mNotificationsRegistered;
};

// Stock gizmo objects
ManipExport Mesh* MakeSphere(Point3& pos, float radius, int segments);
ManipExport Mesh* MakeTorus(Point3& pos, float radius, float radius2, int segs, int sides);
ManipExport Mesh* MakeBox(Point3& pos, float l, float w, float h, int lsegs, int wsegs, int hsegs);
ManipExport void AddCubeShape(PolyShape& shape, Point3& pos, float size);


// Special storage class for hit records so we can know which manipulator was hit
class ManipHitData : public HitData 
{
public:
    Manipulator* mpManip;
    int mShapeIndex;

    ManipExport ManipHitData(Manipulator* pManip) {
        mpManip = pManip;
        mShapeIndex = -1;
    }

    ManipExport ManipHitData() {
        mpManip = NULL;
    }

    virtual ManipHitData* Copy() { return new ManipHitData(mpManip); }

    ManipExport ~ManipHitData() {}
};

// Special storage class for hit records so we can know which manipulator was hit
class SimpleManipHitData : public ManipHitData 
{
public:

    ManipExport SimpleManipHitData(int shapeIndex, Manipulator* pManip) {
        mpManip = pManip;
        mShapeIndex = shapeIndex;
    }

    ManipExport SimpleManipHitData() {
        mShapeIndex = -1;
        mpManip = NULL;
    }
    ManipExport ~SimpleManipHitData() {}

    virtual ManipHitData* Copy() { return new SimpleManipHitData(mShapeIndex, mpManip); }
};

