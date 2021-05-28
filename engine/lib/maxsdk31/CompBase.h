/**********************************************************************
 *<
	FILE: compbase.h

	DESCRIPTION:  A base class for components implemented as RefArrays

	CREATED BY: John Hutchinson

	HISTORY: created 9/1/98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef _COMPBASE
#define _COMPBASE

#include "RefArrayBase.h"
#include "Icomponent.h"

class CompositeBase : public RefArrayBase, public IComponent
{
protected:
	int iSelLevel;
	enum authority_context {eOpColor};

public:
	CompositeBase():iSelLevel(0){};
	// From Animatable
	CoreExport void DeleteThis();
	// CompositeBase now returns SHAPE_CLASS_ID for its SuperClassID when
	// it only contains shapes. This allows an instance to behave more
	// like a true shape if it only contains shapes. If you want to support
	// this behavior, you'll need two ClassDesc's, one returning GEOM and
	// one returning SHAPE for the super class id. They should both return
	// the same class id. If you want to disable this behavior, override
	// this method.
	CoreExport SClass_ID SuperClassID();

	// This is a flag that *should* be exported elsewhere in the SDK.
	#define DISP_NODEFAULTCOLOR (1<<12)
	// This is an additional flag that can be used by Display() and HitTest().
	#define EDITOBJ_THIS (1 << (HITFLAG_STARTUSERBIT+1))

	//This flag is used by hostcomposite when using special materilas in the viewport
	//It needs to indicate that the material has been usurped and that the mesh
	//should not be passed inode::NumMtls(), rather it should be passed 1.
	#define DISP_NUMMATS_INVALID (1 << (HITFLAG_STARTUSERBIT+2))

	// From BaseObject
	CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	CoreExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	CoreExport void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	CoreExport void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	// For sub-object selection
	CoreExport void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE);
	CoreExport void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin=FALSE);
	CoreExport void Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE);
	CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc);
	CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext* mc);
	CoreExport int SubObjectIndex(HitRecord *hitRec);
	CoreExport void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
	CoreExport void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);

	// From Object
	CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm = NULL, BOOL useSel = FALSE );
	CoreExport BOOL HasUVW();
	CoreExport void SetGenUVW(BOOL sw);
	CoreExport int CanConvertToType(Class_ID obtype);
	CoreExport Object* ConvertToType(TimeValue t, Class_ID obtype);
	CoreExport void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);
	CoreExport Class_ID PreferredCollapseType();
	CoreExport int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	CoreExport ObjectState Eval(TimeValue t);

	// From GeomObject
	CoreExport Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);

	// From ShapeObject
	CoreExport void SetRenderable(BOOL sw);
	CoreExport void SetThickness(float t);
	CoreExport void SetGenUVs(BOOL sw);
	CoreExport int NumberOfVertices(TimeValue t, int curve = -1);
	CoreExport int NumberOfCurves();
	CoreExport BOOL CurveClosed(TimeValue t, int curve);
	CoreExport Point3 InterpCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE);
	CoreExport Point3 TangentCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE);
	CoreExport float LengthOfCurve(TimeValue t, int curve);
	CoreExport int NumberOfPieces(TimeValue t, int curve);
	CoreExport Point3 InterpPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE);
	CoreExport Point3 TangentPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE);
	CoreExport MtlID GetMatID(TimeValue t, int curve, int piece);
	CoreExport BOOL CanMakeBezier();
	CoreExport void MakeBezier(TimeValue t, BezierShape &shape);
	CoreExport void MakePolyShape(TimeValue t, PolyShape &shape, int steps = PSHAPE_BUILTIN_STEPS, BOOL optimize = FALSE);
	CoreExport ShapeHierarchy &OrganizeCurves(TimeValue t, ShapeHierarchy *hier=NULL);
	CoreExport int MakeCap(TimeValue t, MeshCapInfo &capInfo, int capType);
	CoreExport int MakeCap(TimeValue t, PatchCapInfo &capInfo);
	CoreExport BOOL AttachShape(TimeValue t, INode *thisNode, INode *attachNode, BOOL weldEnds=FALSE, float weldThreshold=0.0f);

	// From RefArrayBase
	CoreExport class IRefArray *Containee(void){return NULL;}

	// Local methods
	CoreExport virtual ObjectState EvalPipeObj(TimeValue t, Object* obj) = 0;
	CoreExport virtual void CombineComponentMeshes(TimeValue t, Mesh& m) = 0;
	CoreExport virtual void CombineComponentShapes(TimeValue t, BezierShape& Dest) = 0;
	CoreExport virtual void CombineRenderMeshes(TimeValue t, INode *inode, View& view, Mesh& m) = 0;
	CoreExport virtual bool HasOnlyShapeOperands(TimeValue t) = 0;
	CoreExport virtual bool isCompositeAuthority(authority_context context, int contextflags);

	CoreExport virtual bool IsAShape(Object *obj);
	CoreExport void LockComponent();
	CoreExport void UnlockComponent();

protected:
	// Used by most ShapeObject methods.
	CoreExport bool GetNextShape(ComponentIterator* iter, ShapeObject*& shape);
	CoreExport bool CurveFound(int& curve, ComponentIterator* iter, ShapeObject*& shape);
};

//If you implement a component using the compbase class, then you can implement 
//the iterator for it by deriving from the following class. The assumptions include:
// 1) The composite isa IRefArray

class CompositeIteratorBase: public ComponentIterator
{
protected:
	long m_current;
public:
	CoreExport CompositeIteratorBase() : m_current(0) {}
	CoreExport virtual ~CompositeIteratorBase();
	CoreExport virtual void First();
	CoreExport virtual void Next();
	CoreExport virtual bool IsDone() /*const*/;
	CoreExport virtual int SubObjIndex() { return m_current; }
	CoreExport virtual IComponent* GetComponent(void** ipp = NULL, ULONG iid = I_BASEOBJECT) const;
	//Note: this object may be dynamically allocated. Calls must 
	//be matched with calls to DisposeTemporary()

	CoreExport virtual void DeleteThis();

	CoreExport virtual bool WantsDisplay(DWORD flags){return !Hidden();}
	CoreExport virtual Point3 DisplayColor(DWORD flags, INode* n = NULL, GraphicsWindow* gw = NULL);
protected:
	CoreExport virtual Object* GetCompositeObject() = 0;
	CoreExport virtual const Object* GetCompositeObject() const = 0;
};


#endif //_COMPBASE