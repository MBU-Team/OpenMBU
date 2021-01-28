/**********************************************************************
 *<
	FILE: simpcomp.h

	DESCRIPTION:  A sample composite/leaf pair.

	CREATED BY: John Hutchinson

	HISTORY: created 8/24/98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef _COMP_OBJ
#define _COMP_OBJ

#define _CACHING

#include "compbase.h"
#include "iparamb.h"
#ifdef _CACHING
#include "shape.h"
#endif

class PickMember;
class MoveModBoxCMode;
class RotateModBoxCMode;
class UScaleModBoxCMode;
class NUScaleModBoxCMode;
class SquashModBoxCMode;
class SelectModBoxCMode;

extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

#define BEGIN_EDIT_SKIP_SO_REGISTRATION		(1<<7)

class SimpleComposite : public CompositeBase
{
	friend class SimpleCompositeIterator;
	friend class UiUpdateRestore;
	friend class OpSelectionRestore;
	friend class ToggleHiddenRestore;

	enum { kobjectarray, kcontrolarray };

	//object list (references that we make)
	Tab<Object*> m_objlist;
	//controller list
	Tab<RefTargetHandle> m_ctllist;

protected://JH 1/13/99 let derived classes get at this stuff
	NameTab m_opName;
	BitArray selbits;//bit array to store the selection state of the components
	BitArray hidebits;//array to store the hidden state
#define EXPAND_BITS 64 //the chunk size for resizing the bit array

	bool m_creating;
//	int iSelLevel; //JH 5/31/99 moving to base class
	CoreExport static SimpleComposite* editObj;

	Matrix3 m_AmbientTM;
	Interval ivalid;

#ifdef _CACHING
protected:
	Mesh meshcache;
	BezierShape bezcache;
	Interval meshvalid;
	Interval shapevalid;
#endif
	int mOpContentFlags; // a cached value; if flag is zero, it is not up-to-date
	enum
	{
		kFlagsValid = 1,
		kContainsBaseMesh = 2,
		kContainsBaseShape = 4,
		kContainsEvalMesh = 8,
		kContainsEvalShape = 16,
	};
	bool mContentUpdateInProgress;

	class CompositeDeleteKeyUser : public EventUser
	{
	private:
		Object* m_composite_obj;
	public:
		//local methods
		CompositeDeleteKeyUser(){m_composite_obj = NULL;}
		void SetObject(Object* o){m_composite_obj = o;}

		//from Eventuser
		void Notify();
	}
	m_delete_user;

public:	//Class variables
	//miscellaneous
	CoreExport static IObjParam *ip;			//Access to the interface
	CoreExport static HWND hParams1;			// the object level dialog
	CoreExport static HWND hParams2;			// the subobject dialog
	static PickMember pickCB;
	static bool BlockExtract;
	CoreExport static Class_ID m_classid;

	//subobject modes
	CoreExport static MoveModBoxCMode *moveMode;
	CoreExport static RotateModBoxCMode *rotMode;
	CoreExport static UScaleModBoxCMode *uscaleMode;
	CoreExport static NUScaleModBoxCMode *nuscaleMode;
	CoreExport static SquashModBoxCMode *squashMode;
	CoreExport static SelectModBoxCMode *selectMode;

public: //methods
	// Constructor/Destructor
	CoreExport SimpleComposite();
	CoreExport ~SimpleComposite();

	// From Animatable
	CoreExport virtual void DeleteThis(){delete this;}
	CoreExport virtual void BeginEditParams(IObjParam  *ip, ULONG flags, Animatable *prev);
	CoreExport virtual void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);
	CoreExport virtual Class_ID ClassID() { return m_classid; }
	CoreExport virtual void FreeCaches();
	CoreExport virtual void* GetInterface(ULONG id);
	CoreExport virtual void GetClassName(TSTR& s) { s = _T("SimpleComposite"); }


	// From ReferenceMaker
	CoreExport virtual RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
										PartID& partID, RefMessage message);
	CoreExport virtual IOResult Load(ILoad* iload);
	CoreExport virtual IOResult Save(ISave* isave);

	// From ReferenceTarget
	CoreExport virtual RefTargetHandle Clone(RemapDir& remap);

	// From BaseObject
	CoreExport virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CoreExport virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, 
									IPoint2 *p, ViewExp *vpt);
	CoreExport virtual void ActivateSubobjSel(int level, XFormModes& modes);
	CoreExport virtual CreateMouseCallBack* GetCreateMouseCallBack();
	CoreExport virtual TCHAR *GetObjectName();
	CoreExport virtual void SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert=FALSE);
	CoreExport virtual void ClearSelection(int selLevel);
	CoreExport virtual void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE);
	CoreExport virtual void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin=FALSE);
	CoreExport virtual void Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE);
	CoreExport virtual void TransformStart(TimeValue t);
	CoreExport virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, 
									IPoint2 *p, ViewExp *vpt, ModContext* mc);
	//4/12/99 JH
	CoreExport virtual void CloneSelSubComponents(TimeValue t); 
	CoreExport virtual void AcceptCloneSelSubComponents(TimeValue t);


	// From Object
	CoreExport virtual void InitNodeName(TSTR& s);
	CoreExport virtual Interval ObjectValidity(TimeValue t);
	CoreExport virtual int UsesWireColor(){return FALSE;}
	//CoreExport virtual int DoOwnSelectHilite(){return 0;}
	CoreExport virtual int NumPipeBranches();
	CoreExport virtual Object *GetPipeBranch(int i);
	CoreExport virtual INode *GetBranchINode(TimeValue t, INode *node, int i);
	CoreExport virtual int IsRenderable();
	CoreExport virtual BOOL IsShapeObject();

	// From IComponent
	CoreExport virtual void Add(TimeValue t, Object* obj, TSTR objname, Matrix3& oppTm, Matrix3& boolTm, Matrix3& parTM, Control* opCont = NULL);
	CoreExport virtual void Add(TimeValue t,INode *node,Matrix3& boolTm, bool delnode = true);
	CoreExport virtual void Remove(ComponentIterator& i);
	CoreExport virtual void DisposeTemporary();
	CoreExport virtual ComponentIterator * MakeIterator();
	CoreExport virtual ComponentIterator * MakeReverseIterator();
	CoreExport virtual Object* MakeLeaf(Object* o);
	CoreExport virtual bool IsLeaf() const { return false; }
	CoreExport virtual void SelectOp(int which, BOOL selected);
	CoreExport virtual void ClearSelection();
	CoreExport virtual int GetSubSelectionCount();

	// From IRefArray
	CoreExport virtual int NumRefRows() const; // the number of arrays I maintain including inner
	CoreExport virtual void EnlargeAndInitializeArrays(int newsize);
	CoreExport virtual const ReferenceArray& RefRow(int which) const;
	CoreExport virtual ReferenceArray& RefRow(int which);

	// From CompositeBase
	CoreExport virtual ObjectState EvalPipeObj(TimeValue t, Object* obj);
	CoreExport virtual void CombineComponentMeshes(TimeValue t, Mesh& m);
	CoreExport virtual void CombineComponentShapes(TimeValue t, BezierShape& Dest);
	CoreExport virtual void CombineRenderMeshes(TimeValue t, INode *inode, View& view, Mesh& m);
	CoreExport virtual bool HasOnlyShapeOperands(TimeValue t);

	//Local Methods
	CoreExport virtual void GetOpTM(TimeValue t,int which, Matrix3& tm, Interval *iv=NULL) const;
	CoreExport virtual void Invalidate();
	CoreExport virtual void HideOp(int which, BOOL hidden);
	CoreExport virtual bool IsSubObjSelected(int i);
	CoreExport virtual bool IsSubObjHidden(int i);
	CoreExport virtual void ExtractOperands(TimeValue t, INode* node, bool autodelete = true, bool justhide = false);
	CoreExport virtual void SetupUI2();
	CoreExport virtual void DoSubObjSelectionFromList();
	CoreExport virtual void CheckIfShapeIsRenderable(Object *obj){}
	CoreExport virtual void SetName(int i, TCHAR *n){ assert(m_opName.Count() > i); m_opName.SetName(i,n); }
	CoreExport virtual TSTR GetName(int i){ assert(m_opName.Count() > i  &&  m_opName[i]); return m_opName[i]; }

protected:
	CoreExport void Clone(SimpleComposite * newob, RemapDir & remap);
	CoreExport void DisableExtract(bool extract);
	CoreExport void ToggleHidden(const Tab<int>& opset);
	CoreExport void UpdateContentFlags(bool recalc = false);
	CoreExport void EnableDeleteKey(bool OnOff);
};

//FIXME JH
//Consider creating a new class GeomObjectWrapper which encapsulates
//the delegation of the geoobject methods

//SS 4/12/99: Now derives from ShapeObject; if you leaf will only
// reference mesh geometry, override SuperClassID() and IsShapeObject().
class SimpleLeaf : public ShapeObject, public IComponent
{
	friend class SimpleLeafClassDesc;

	static Class_ID m_classid;

protected:
	Object* m_obj;
	bool m_deleteMe;
private:

public:
	// Constructor/Destructor
	CoreExport SimpleLeaf() : ShapeObject(), m_obj(NULL), m_deleteMe(false)
		{}
	CoreExport SimpleLeaf(Object* o, bool dm = true);
	CoreExport ~SimpleLeaf();

	// Static methods for dynamic allocation
	CoreExport static SimpleLeaf* CreateSimpleLeaf(Object *ob, bool dm = true);
	CoreExport static void DeleteSimpleLeaf(SimpleLeaf* l);

	// From Animatable
	CoreExport void DeleteThis()
		{ delete this; }
	CoreExport SClass_ID SuperClassID();
	CoreExport void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
	CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
	CoreExport Class_ID ClassID()
		{ return m_classid; }
	CoreExport virtual void FreeCaches();
	CoreExport virtual void* GetInterface(ULONG id);
	CoreExport virtual void GetClassName(TSTR& s)
		{ s = _T("SimpleLeaf"); }

	// From ReferenceMaker
	CoreExport int NumRefs()
		{ return 1; }
	CoreExport void SetReference(int i, RefTargetHandle rtarg)
		{ m_obj = (Object*) rtarg; }
	CoreExport RefTargetHandle GetReference(int i)
		{ return m_obj; }
	CoreExport RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
										PartID& partID, RefMessage message)
		{ return REF_SUCCEED; }
	CoreExport IOResult Load(ILoad* iload)
		{ return ShapeObject::Load(iload); }
	CoreExport IOResult Save(ISave* isave)
		{ return ShapeObject::Save(isave); }

	// From ReferenceTarget
	CoreExport virtual RefTargetHandle Clone(RemapDir& remap);
	
	// From BaseObject
	CoreExport TCHAR *GetObjectName();
	CoreExport CreateMouseCallBack* GetCreateMouseCallBack()
		{ return NULL; }
	CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	CoreExport void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	CoreExport void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	CoreExport void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	// For sub-object selection
	CoreExport void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE);
	CoreExport void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin);
	CoreExport void Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin);
	CoreExport void TransformStart(TimeValue t);
	CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc);
	CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext* mc);
	CoreExport int SubObjectIndex(HitRecord *hitRec);
	CoreExport void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
	CoreExport void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);

	// From Object
	CoreExport void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm = NULL, BOOL useSel = FALSE );
	CoreExport BOOL HasUVW();
	CoreExport void SetGenUVW(BOOL sw);
	CoreExport BOOL CanConvertToType(Class_ID obtype);
	CoreExport Object* ConvertToType(TimeValue t, Class_ID obtype);
	CoreExport void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist);
	CoreExport Class_ID PreferredCollapseType();
	CoreExport int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	CoreExport ObjectState Eval(TimeValue t);
	CoreExport BOOL IsShapeObject();
	CoreExport void InitNodeName(TSTR& s);
	CoreExport Interval ObjectValidity(TimeValue t);
	CoreExport int DoOwnSelectHilite();
	CoreExport int NumPipeBranches();
	CoreExport Object *GetPipeBranch(int i);
	CoreExport INode *GetBranchINode(TimeValue t,INode *node,int i);

	// From GeomObject
	CoreExport Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);
	
	// From ShapeObject
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

	
	// From IComponent
	CoreExport virtual void Add(TimeValue t, Object*, TSTR objname, Matrix3& oppTm, Matrix3& boolTm,  Matrix3& parTM, Control* opCont = NULL);
	CoreExport virtual void Add(TimeValue t,INode *node,Matrix3& boolTm, bool delnode = true);
	CoreExport virtual void Remove(ComponentIterator& i);
	CoreExport virtual void DisposeTemporary();
	CoreExport virtual ComponentIterator * MakeIterator();
	CoreExport virtual ComponentIterator * MakeReverseIterator()
		{ return MakeIterator(); }
	CoreExport virtual Object* MakeLeaf(Object* o);
	CoreExport virtual bool IsLeaf() const
		{ return true; }
	CoreExport virtual void SelectOp(int which, BOOL selected)
		{}
	CoreExport virtual void ClearSelection()
		{}
	CoreExport virtual int GetSubSelectionCount()
		{ return 0; }
};


class SimpleCompositeIterator: public CompositeIteratorBase
{
private:
	// mutable lets us call non-const methods on SimpleComposte from
	// const iterator methods (like GetReference... ooohh).
	mutable SimpleComposite* m_pcomp;

public:
	CoreExport static SimpleCompositeIterator * CreateSimpleCompositeIterator(SimpleComposite* it);

	CoreExport SimpleCompositeIterator(/*const*/ SimpleComposite* it);
	CoreExport virtual ~SimpleCompositeIterator();

	CoreExport virtual Object* GetComponentObject() const;
	CoreExport virtual INode* GetComponentINode(INode *node) const;
	CoreExport virtual void DisposeComponentINode(INode *node) const;
	CoreExport virtual Control* GetComponentControl() const;//This need not be released
	CoreExport virtual bool Selected() const;//is the corresponding component selected?
	CoreExport virtual bool Hidden() const;//is the corresponding component hidden?
	CoreExport virtual void DeleteThis();

protected:
	// CompositeIteratorBase methods.
	CoreExport virtual Object* GetCompositeObject();
	CoreExport virtual const Object* GetCompositeObject() const;

	CoreExport virtual bool ValidIndex(int which) const;
};

class SimpleCompositeReverseIterator: public SimpleCompositeIterator
{
public:
	CoreExport SimpleCompositeReverseIterator(SimpleComposite* it);
	CoreExport virtual ~SimpleCompositeReverseIterator() {}

	CoreExport virtual void First();
	CoreExport virtual void Next();
	CoreExport virtual bool IsDone();
};

class SimpleCompositeClassDesc : public ClassDesc
{
public:
	int 			IsPublic() {return 0;}
	void *			Create(BOOL loading = FALSE) {return new SimpleComposite();}
	const TCHAR *	ClassName();
	//SS 4/12/99: RefArrayBase now derives from ShapeObject
	SClass_ID		SuperClassID() {return SHAPE_CLASS_ID;}
	Class_ID		ClassID() {return SimpleComposite::m_classid;}
	const TCHAR* 	Category();
	void			ResetClassParams (BOOL fileReset);
	BOOL			OkToCreate(Interface *i);
	int				BeginCreate(Interface *i);
	int				EndCreate(Interface *i);

};

class SimpleLeafClassDesc : public ClassDesc
{
public:
	int 			IsPublic() {return 0;}
	void *			Create(BOOL loading = FALSE) {return new SimpleLeaf();}
	const TCHAR *	ClassName() { return _T("Simple Leaf"); }
	//SS 4/12/99: RefArrayBase now derives from ShapeObject
	SClass_ID		SuperClassID() {return SHAPE_CLASS_ID;}
	Class_ID		ClassID() {return SimpleLeaf::m_classid;}
	const TCHAR* 	Category() { return _T(""); }
	void			ResetClassParams (BOOL fileReset) {}
};

/*
Strategy for solving the ambient tm problem:
1. Enhance the DependentEnumProc to accept a return value of SKIP.
	This is pretty much required for this to work.
	4a. Define 3 constants and enhance ReferenceTarget::EnumDependents() to 
		support skipping (see my email).
	DONE
2. When an operand is deselected, all its children's selection bits are 
	cleared. There is no sense keeping these bits around, and they will
	confuse the enumerator.
	2a. This clearing must be done recursively, because the user can jump to
		the top of the mod stack in one step.
3. We implement the AmbientTmEnumerator to collect all the tms in the selected
	branch, then calculate the world space transform after enumeration is
	complete.
4. If looking at the operands of a block definition, extraction is disabled
	(should already be that way now).

// Here's how this is used: make this call from the link composite or leaf that
// wants to know its world space tm.
AmbientTmEnumerator getTM(this);
EnumDependents(getTM);
Matrix3 worldSpaceTMOfMe(1);
assert(getTM.TmIsValid());
if (getTM.TmIsValid())
	worldSpaceTMOfMe = getTM.AmbientTm();
*/


class AmbientTmEnumerator : public DependentEnumProc
{
private:
	// A stack of ptrs.
	Tab<ReferenceMaker*> mSelectedBranch;
	// A stack of tms.
	Tab<Matrix3> mTmStack;
	// initiating reference target
	ReferenceTarget * mInitiator;
	// A status so we know we ended where we should.
	bool reachedTheNode;

public:
	CoreExport AmbientTmEnumerator(ReferenceTarget * rtarg) : reachedTheNode(false) { mInitiator = rtarg; }

	CoreExport virtual int proc(ReferenceMaker* rmaker);
	CoreExport bool TmIsValid() { return reachedTheNode; }
	CoreExport Matrix3 AmbientTm()
	{
		Matrix3 tm(1);
		for (int i = 0; i < mTmStack.Count(); i++)
			tm = mTmStack[i] * tm;
		//FIXME: calculate the tm using the stack of ptrs here; Pete?
		return tm;
	}
};

class NodeEnum : public DependentEnumProc
{
private:
	// initiating reference target
	ReferenceTarget * mInitiator;

	INode * node;

	// A status so we know we ended where we should.
	bool reachedTheNode;

public:
	CoreExport NodeEnum(ReferenceTarget * rtarg) : reachedTheNode(false) { mInitiator = rtarg; }

	CoreExport virtual int proc(ReferenceMaker* rmaker);
	CoreExport bool NodeIsValid() { return reachedTheNode; }
	CoreExport INode * Node() { return node; }
};


#endif //_COMP_OBJ
