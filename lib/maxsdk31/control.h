/**********************************************************************
 *<
	FILE:  control.h

	DESCRIPTION:  Control definitions

	CREATED BY:  Dan Silva and Rolf Berteig

	HISTORY: created 9 September 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __CONTROL__

#define __CONTROL__

#include "plugapi.h"

extern CoreExport void ApplyScaling(Matrix3 &m, const ScaleValue &v);
extern CoreExport void InitControlLists();


class ScaleValue;
class ViewExp;
class INode;
class XFormModes;
class INodeTab;
class View;

CoreExport ScaleValue operator+(const ScaleValue& s0, const ScaleValue& s1);
CoreExport ScaleValue operator-(const ScaleValue& s0, const ScaleValue& s1);
CoreExport ScaleValue operator*(const ScaleValue& s, float f);
CoreExport ScaleValue operator*(float f, const ScaleValue& s);
CoreExport ScaleValue operator+(const ScaleValue& s, float f);
CoreExport ScaleValue operator+(float f, const ScaleValue& s);

class ScaleValue {
	public:
	Point3 s;
	Quat q;
	ScaleValue() {}
	ScaleValue(const Point3& as) { s = as; q = IdentQuat(); }
	ScaleValue(const Point3& as, const Quat& aq) {s = as; q = aq;}
	ScaleValue& operator+=(const ScaleValue& s) {(*this)=(*this)+s;return (*this);}
	ScaleValue& operator*=(const float s) {(*this)=(*this)*s;return (*this);}
	ScaleValue& operator=(const ScaleValue &v) {s=v.s;q=v.q;return (*this);}
	float& operator[](int el) {return s[el];}
	};

// Types of ORTs
#define ORT_BEFORE	1
#define ORT_AFTER	2

// ORTs
#define ORT_CONSTANT			1
#define ORT_CYCLE				2
#define ORT_LOOP				3	// This is cycle with continuity.
#define ORT_OSCILLATE			4
#define ORT_LINEAR				5
#define ORT_IDENTITY			6
#define ORT_RELATIVE_REPEAT		7

/*---------------------------------------------------------------------*/

// A list of ease curves.
class EaseCurveList : public ReferenceTarget {
		friend class AddEaseRestore;
		friend class DeleteEaseRestore;

	private:
		Tab<Control*> eases;
		
	public:
	 	EaseCurveList() {OpenTreeEntry(1,ALL_TRACK_VIEWS);}
	  	CoreExport ~EaseCurveList();

		CoreExport TimeValue ApplyEase(TimeValue t,Interval &valid);
		CoreExport void AppendEaseCurve(Control *cont);
		CoreExport void DeleteEaseCurve(int i);
		CoreExport void DisableEaseCurve(int i);
		CoreExport void EnableEaseCurve(int i);
		CoreExport BOOL IsEaseEnabled(int i);
		int NumEaseCurves() {return eases.Count();}

		// Animatable
		void GetClassName(TSTR& s) { s= TSTR(_T("EaseCurve")); }  
		Class_ID ClassID() { return Class_ID(EASE_LIST_CLASS_ID,0); }
		SClass_ID SuperClassID() { return EASE_LIST_CLASS_ID; }		
		CoreExport int NumSubs();
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);
		int SubNumToRefNum(int subNum) {return subNum;}
		BOOL BypassTreeView() { return TRUE; }
		void DeleteThis() { delete this; }
		ParamDimension* GetParamDimension(int i) {return stdTimeDim;}
		CoreExport BOOL AssignController(Animatable *control,int subAnim);
		CoreExport void* GetInterface(ULONG id);

		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);
		
		// Reference
		CoreExport int NumRefs();
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);
		CoreExport RefTargetHandle Clone(RemapDir &remap = NoRemap());
		CoreExport RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	         PartID& partID,  RefMessage message);		
	};

class EaseCurveAnimProp : public AnimProperty {
	public:
		EaseCurveList *el;
		EaseCurveAnimProp() { el=NULL; }
		DWORD ID() {return PROPID_EASELIST;}
	};

#define GetEaseListInterface(anim)	((EaseCurveList*)anim->GetInterface(I_EASELIST))

/*---------------------------------------------------------------------*/
// A list of multiplier curves.
class MultCurveList : public ReferenceTarget {
		friend class AddMultRestore;
		friend class DeleteMultRestore;
	private:
		Tab<Control*> mults;
		
	public:
	 	MultCurveList() {OpenTreeEntry(1,ALL_TRACK_VIEWS);}
	  	CoreExport ~MultCurveList();

		CoreExport float GetMultVal(TimeValue t,Interval &valid);
		CoreExport void AppendMultCurve(Control *cont);
		CoreExport void DeleteMultCurve(int i);
		CoreExport void DisableMultCurve(int i);
		CoreExport void EnableMultCurve(int i);
		CoreExport BOOL IsMultEnabled(int i);
		int NumMultCurves() {return mults.Count();}

		// Animatable
		void GetClassName(TSTR& s) { s= TSTR(_T("MultCurve")); }  
		Class_ID ClassID() { return Class_ID(MULT_LIST_CLASS_ID,0); }
		SClass_ID SuperClassID() { return MULT_LIST_CLASS_ID; }		
		CoreExport int NumSubs();
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);
		int SubNumToRefNum(int subNum) {return subNum;}
		BOOL BypassTreeView() { return TRUE; }
		void DeleteThis() { delete this; }
		ParamDimension* GetParamDimension(int i) {return stdNormalizedDim;}
		CoreExport BOOL AssignController(Animatable *control,int subAnim);
		CoreExport void* GetInterface(ULONG id);

		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);
		
		// Reference
		CoreExport int NumRefs();
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);
		CoreExport RefTargetHandle Clone(RemapDir &remap = NoRemap());
		CoreExport RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	         PartID& partID,  RefMessage message);		
	};

class MultCurveAnimProp : public AnimProperty {
	public:
		MultCurveList *ml;
		MultCurveAnimProp() { ml=NULL; }
		DWORD ID() {return PROPID_MULTLIST;}
	};

#define GetMultListInterface(anim)	((MultCurveList*)anim->GetInterface(I_MULTLIST))

/*---------------------------------------------------------------------*/


//
// For hit testing controller apparatus 
//

class CtrlHitRecord {
	friend class CtrlHitLog;
	CtrlHitRecord *next;
	public:
		INode *nodeRef;
		DWORD distance;
		ulong hitInfo;
		DWORD infoExtra;		
		CtrlHitRecord() {next=NULL; distance=0; hitInfo=0; nodeRef=NULL;}
		CtrlHitRecord(CtrlHitRecord *nxt,INode *nr, DWORD d, ulong inf, DWORD extra) {
			next=nxt;nodeRef=nr;distance=d;hitInfo=inf;infoExtra=extra;}
		CtrlHitRecord *Next() {return next;}		
	};				   	

class CtrlHitLog {
	CtrlHitRecord *first;
	int hitIndex;
	public:
		CtrlHitLog()  { first = NULL; }
		~CtrlHitLog() { Clear(); }
		CoreExport void Clear();
		CoreExport void ClearHitIndex()		{ hitIndex = 0; }
		CoreExport void IncrHitIndex()		{ hitIndex++; }
		CtrlHitRecord* First() { return first; }
		CoreExport CtrlHitRecord* ClosestHit();
		void LogHit(INode *nr,DWORD dist,ulong info,DWORD infoExtra)
			{first = new CtrlHitRecord(first,nr,dist,info,infoExtra);}
	};


// For enumerating IK paramaters
class IKEnumCallback {
	public:
		virtual void proc(Control *c, int index)=0;
	};

class IKDeriv {
	public:
		virtual int NumEndEffectors()=0;
		virtual Point3 EndEffectorPos(int index)=0;
		virtual void DP(Point3 dp,int index)=0;
		virtual void DR(Point3 dr,int index)=0;
		virtual void NextDOF()=0;
	};

// Flags passed to CompDerivs
#define POSITION_DERIV	(1<<0)
#define ROTATION_DERIV	(1<<1)


// This class is used to store IK parameters that have been
// copied to a clipboard.
class IKClipObject {
	public:
		// Identifies the creator of the clip object
		virtual SClass_ID 	SuperClassID()=0;
		virtual Class_ID	ClassID()=0;
		
		virtual void DeleteThis()=0;
	};

// Values for 'which' pasted to Copy/PasteIKParams
#define COPYPASTE_IKPOS		1
#define COPYPASTE_IKROT		2

// Passed to InitIKJoints() which is called when importing
// R4 3DS files that have IK joint data.
class InitJointData {
	public:
		BOOL active[3];
		BOOL limit[3];
		BOOL ease[3];
		Point3 min, max, damping;
	};

// This structure is passed to GetDOFParams().
// Controllers that support IK can provide info about their DOFs
// so that bones can display this information.
// The first 3 DOFs are assumed to be position
// and the next 3 are assumed to be rotation
class DOFParams {
	public:
		BOOL display[6];		// Should this DOF be displayed?
		Point3 axis[6];			// DOF axis
		Point3 pos[6];			// Base of axis
		BOOL limit[6];          // is joint limited?
		float min[6];			// min limit
		float max[6];           // max limit
		float curval[6];		// Current value of the parameter
		BOOL sel[6];			// should DOF be highlighted
		BOOL endEffector;		// is there an end effector for this controller
		Matrix3 eeTM;			// world TM of the end effector if present
	};


// These two ways values can be retreived or set.
// For get:
//		RELATIVE = Apply
//		ABSOLUTE = Just get the value
// For set:
//		RELATIVE = Add the value to the existing value (i.e Move/Rotate/Scale)
//		ABSOLUTE = Just set the value
enum GetSetMethod {CTRL_RELATIVE,CTRL_ABSOLUTE};


// Control class provides default implementations for load and save which save the ORT type in these chunks:
#define CONTROLBASE_CHUNK 		0x8499
#define INORT_CHUNK				0x3000
#define OUTORT_CHUNK			0x3001
#define CONT_DISABLED_CHUNK		0x3002

// Inheritance flags.
#define INHERIT_POS_X	(1<<0)
#define INHERIT_POS_Y	(1<<1)
#define INHERIT_POS_Z	(1<<2)
#define INHERIT_ROT_X	(1<<3)
#define INHERIT_ROT_Y	(1<<4)
#define INHERIT_ROT_Z	(1<<5)
#define INHERIT_SCL_X	(1<<6)
#define INHERIT_SCL_Y	(1<<7)
#define INHERIT_SCL_Z	(1<<8)
#define INHERIT_ALL		511

class Control : public ReferenceTarget {
	public:
		Control() {SetORT(ORT_CONSTANT,ORT_BEFORE);SetORT(ORT_CONSTANT,ORT_AFTER);};
		virtual ~Control() {};

		virtual void Copy(Control *from)=0;
		virtual void CommitValue(TimeValue t) {}
		virtual void RestoreValue(TimeValue t) {}
		virtual INode* GetTarget() { return NULL; } 
		virtual RefResult SetTarget(INode *targ) {return REF_SUCCEED;}

		// Implemented by transform controllers that have position controller
		// that can be edited in the trajectory branch
		virtual Control *GetPositionController() {return NULL;}
		virtual Control *GetRotationController() {return NULL;}
		virtual Control *GetScaleController() {return NULL;}
		virtual BOOL SetPositionController(Control *c) {return FALSE;}
		virtual BOOL SetRotationController(Control *c) {return FALSE;}
		virtual BOOL SetScaleController(Control *c) {return FALSE;}

		// If a controller has an 'X', 'Y', or 'Z' controller, it can implement
		// these methods so that its sub controllers can respect track view filters
		virtual Control *GetXController() {return NULL;}
		virtual Control *GetYController() {return NULL;}
		virtual Control *GetZController() {return NULL;}

		// Implemented by look at controllers that have a float valued roll
		// controller so that the roll can be edited via the transform type-in
		virtual Control *GetRollController() {return NULL;}
		virtual BOOL SetRollController(Control *c) {return FALSE;}

		// Implemented by any Point3 controller that wishes to indicate that it is intended
		// to control floating point RGB color values
		virtual BOOL IsColorController() {return FALSE;}

		// Implemented by TM controllers that support 
		// filtering out inheritance
		virtual DWORD GetInheritanceFlags() {return INHERIT_ALL;}
		virtual BOOL SetInheritanceFlags(DWORD f,BOOL keepPos) {return FALSE;} // return TRUE if TM controller supports inheritance

		virtual BOOL IsLeaf() {return TRUE;}
		virtual int IsKeyable() {return 1;}

		// If a controller does not want to allow another controller
		// to be assigned on top of it, it can return FALSE to this method.
		virtual BOOL IsReplaceable() {return TRUE;}		

		// This is called on TM, pos, rot, and scale controllers when their
		// input matrix is about to change. If they return FALSE, the node will
		// call SetValue() to make the necessary adjustments.
		virtual BOOL ChangeParents(TimeValue t,const Matrix3& oldP,const Matrix3& newP,const Matrix3& tm) {return FALSE;}

		// val points to an instance of a data type that corresponds with the controller
		// type. float for float controllers, etc.
		// Note that for SetValue on Rotation controllers, if the SetValue is
		// relative, val points to an AngAxis while if it is absolute it points
		// to a Quat.
		virtual void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method=CTRL_ABSOLUTE)=0;	
		virtual	void SetValue(TimeValue t, void *val, int commit=1, GetSetMethod method=CTRL_ABSOLUTE)=0;

		// Transform controllers that do not inherit their parent's  transform 
		// should override this method. Returning FALSE will cause SetValue 
		// to be called even in the case when the parent is also being transformed.
		virtual BOOL InheritsParentTransform() { return TRUE; }

		virtual int GetORT(int type) {return (aflag>>(type==ORT_BEFORE?A_ORT_BEFORESHIFT:A_ORT_AFTERSHIFT))&A_ORT_MASK;}
		CoreExport virtual void SetORT(int ort,int type);
		
		// Sets the enabled/disabled state for ORTs
		CoreExport virtual void EnableORTs(BOOL enable);

		// Default implementations of load and save handle loading and saving of out of range type.
		// Call these from derived class load and save.
		// NOTE: Must call these before any of the derived class chunks are loaded or saved.
		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);

		// For IK
		// Note: IK params must be given in the order they are applied to
		// the parent matrix. When derivatives are computed for a parameter
		// that parameter will apply itself to the parent matrix so the next
		// parameter has the appropriate reference frame. If a controller isn't
		// participating in IK then it should return FALSE and the client (usually PRS)
		// will apply the controller's value to the parent TM.
		virtual void EnumIKParams(IKEnumCallback &callback) {}
		virtual BOOL CompDeriv(TimeValue t,Matrix3& ptm,IKDeriv& derivs,DWORD flags) {return FALSE;}
		virtual float IncIKParam(TimeValue t,int index,float delta) {return 0.0f;}
		virtual void ClearIKParam(Interval iv,int index) {return;}
		virtual BOOL CanCopyIKParams(int which) {return FALSE;}
		virtual IKClipObject *CopyIKParams(int which) {return NULL;}
		virtual BOOL CanPasteIKParams(IKClipObject *co,int which) {return FALSE;}
		virtual void PasteIKParams(IKClipObject *co,int which) {}
		virtual void InitIKJoints(InitJointData *posData,InitJointData *rotData) {}
		virtual BOOL GetIKJoints(InitJointData *posData,InitJointData *rotData) {return FALSE;}
		virtual BOOL GetDOFParams(TimeValue t,Matrix3 &ptm,DOFParams &dofs,BOOL nodeSel) {return FALSE;}
		virtual BOOL CreateLockKey(TimeValue t, int which) {return FALSE;}
		virtual void MirrorIKConstraints(int axis,int which,BOOL pasteMirror=FALSE) {}
		virtual BOOL TerminateIK() {return FALSE;} // controllers can act as terminators.

		// Called on a transform controller when the a message is received from a pin node
		virtual RefResult PinNodeChanged(RefMessage message,Interval changeInt, PartID &partID) {return REF_SUCCEED;}

		// Called on a transform controller when one of the node level IK parameters has been changed
		virtual void NodeIKParamsChanged() {}

		// Called in a transform controller when a node invalidates its TM cache
		virtual void TMInvalidated() {}

		// Let's the TM controller determine if it's OK to bind (IK bind) to a particular node.
		virtual BOOL OKToBindToNode(INode *node) {return TRUE;}

		// Ease curves
		virtual BOOL CanApplyEaseMultCurves() {return TRUE;}
		CoreExport TimeValue ApplyEase(TimeValue t,Interval &valid);
		CoreExport void AppendEaseCurve(Control *cont);
		CoreExport void DeleteEaseCurve(int i);
		CoreExport int NumEaseCurves();

		// Multiplier curves		
		CoreExport float GetMultVal(TimeValue t,Interval &valid);
		CoreExport void AppendMultCurve(Control *cont);
		CoreExport void DeleteMultCurve(int i);
		CoreExport int NumMultCurves();

		// These are implemented to handle ease curves. If a controller
		// is a leaf controller, then it MUST NOT BY DEFINITION have any
		// sub controllers or references. If it is a leaf controller, then
		// these are implemented to handle the ease curve list.
		// If it is NOT a leaf controller, then these can be overridden.
		CoreExport int NumRefs();
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);
		CoreExport int NumSubs();
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);

		// Default implementations of some Animatable methods
		CoreExport void* GetInterface(ULONG id);
		CoreExport int PaintFCurves(			
			ParamDimensionBase *dim,
			HDC hdc,
			Rect& rcGraph,
			Rect& rcPaint,
			float tzoom,
			int tscroll,
			float vzoom,
			int vscroll,
			DWORD flags );
		CoreExport int GetFCurveExtents(
			ParamDimensionBase *dim,
			float &min, float &max, DWORD flags);


		// This is called on transform controller after a node is
		// cloned and the clone process has finished
		virtual void PostCloneNode() {}

		// Slave TM controllers can implement this to prevent plug-ins
		// deleting their node via the DeleteNode API.
		virtual BOOL PreventNodeDeletion() {return FALSE;}

		// New interface for visibility float controllers to allow view dependent visibility
		// The default implementation will call GetValue()
		CoreExport virtual float EvalVisibility(TimeValue t,View &view,Box3 pbox,Interval &valid);
		
		// Called on visibility controllers. Gives them the option to completely hide an object in the viewports
		virtual BOOL VisibleInViewports() {return TRUE;}

		// Called on transform controllers or visibility controllers when a node is cloned and the user has chosen to instance
		virtual BOOL CanInstanceController() {return TRUE;}

		// Should be called by any leaf controller's clone method so
		// that ease and multipier curves are cloned.
		CoreExport void CloneControl(Control *ctrl,RemapDir &remap);

		//-------------------------------------------------------
		// Controllers that wish to have an apparatus available in
		// the scene will implement these methods:
		// NOTE: Most of these methods are duplicated in BaseObject or Object
		// (see object.h for descriptions).
		virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) { return 0; };
		virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) { return 0; }
		virtual	void GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box) {}

		virtual void ActivateSubobjSel(int level, XFormModes& modes ) {}

		virtual void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert=FALSE) {}
		virtual void ClearSelection(int selLevel) {}
		virtual int SubObjectIndex(CtrlHitRecord *hitRec) {return 0;}		
		virtual void SelectAll(int selLevel) {}
		virtual void InvertSelection(int selLevel) {}

		virtual void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node) {}
		virtual void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node) {}

		// Modify sub object apparatuses
		virtual void SubMove( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE ){}
		virtual void SubRotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin=FALSE ){}
		virtual void SubScale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE ){}		
		
		// Schematic View Animatable Overides...
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *object, int id, DWORD flags);

		// Called when the user rescales time in the time configuration dialog. If FALSE
		// is returned from this method then MapKeys() will be used to perform the scaling. 
		// Controllers can override this method to handle things like rescaling tagents that 
		// MapKeys() won't affect and return TRUE if they don't want map keys to be called.
		virtual BOOL RescaleTime(Interval oseg, Interval nseg) {return FALSE;}
	};


// Any controller that does not evaluate itself as a function of it's
// input can subclass off this class.
// GetValueLocalTime() will never ask the controller to apply the value,
// it will always ask for it absolute.
class StdControl : public Control {
	public:		
		virtual void GetValueLocalTime(TimeValue t, void *val, Interval &valid, GetSetMethod method=CTRL_ABSOLUTE)=0;
		virtual	void SetValueLocalTime(TimeValue t, void *val, int commit=1, GetSetMethod method=CTRL_ABSOLUTE)=0;
		CoreExport void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method=CTRL_ABSOLUTE);
		CoreExport void SetValue(TimeValue t, void *val, int commit=1, GetSetMethod method=CTRL_ABSOLUTE);

		virtual void Extrapolate(Interval range,TimeValue t,void *val,Interval &valid,int type)=0;
		
		virtual void *CreateTempValue()=0;
		virtual void DeleteTempValue(void *val)=0;
		virtual void ApplyValue(void *val, void *delta)=0;
		virtual void MultiplyValue(void *val, float m)=0;
	};


// Each super class of controller may have a specific packet defined that
// the 'val' pointer will point to instead of a literal value.
// In reality, probably only the Transform controller will do this.
enum SetXFormCommand { XFORM_MOVE, XFORM_ROTATE, XFORM_SCALE, XFORM_SET };
class SetXFormPacket {
	public:
		SetXFormCommand command;
		Matrix3 tmParent;
		Matrix3 tmAxis;		// if command is XFORM_SET, this will contain the new value for the XFORM.		
		Point3 p;
		Quat q;
		AngAxis aa;
		BOOL localOrigin;		
		
		// XFORM_SET
		SetXFormPacket(const Matrix3& mat,const Matrix3& par=Matrix3(1))
			{command=XFORM_SET,tmParent=par,tmAxis=mat;}

		// XFORM_MOVE
		SetXFormPacket(Point3 pt, const Matrix3& par=Matrix3(1), 
						const Matrix3& a=Matrix3(1))
			{command=XFORM_MOVE;tmParent=par;tmAxis=a;p=pt;localOrigin=FALSE;}

		// XFORM_ROTATE
		SetXFormPacket(Quat qt, BOOL l, const Matrix3& par=Matrix3(1),
						const Matrix3& a=Matrix3(1))
			{command=XFORM_ROTATE;tmParent=par;tmAxis=a;q=qt;aa=AngAxis(q);localOrigin=l;}
		SetXFormPacket(AngAxis aA, BOOL l, const Matrix3& par=Matrix3(1),
						const Matrix3& a=Matrix3(1))
			{command=XFORM_ROTATE;tmParent=par;tmAxis=a;q=Quat(aA);aa=aA;localOrigin=l;}

		// XFORM_SCALE
		SetXFormPacket(Point3 pt, BOOL l, const Matrix3& par=Matrix3(1),
						const Matrix3& a=Matrix3(1))
			{command=XFORM_SCALE;tmParent=par;tmAxis=a;p=pt;localOrigin=l;}

		// Just in case you want to do it by hand...
		SetXFormPacket() {};
	};



// This is a special control base class for controllers that control
// morphing of geomoetry.
//
// The 'val' pointer used with GetValue will point to an object state.
// This would be the result of evaluating a combination of targets and
// producing a new object that is some combination of the targets.
//
// The 'val' pointer used with SetValue will point to a 
// SetMorphTargetPacket data structure. This has a pointer to
// an object (entire pipeline) and the name of the target.

// A pointer to one of these is passed to SetValue
class SetMorphTargetPacket {
	public:
		Matrix3 tm;
		Object *obj;
		TSTR name;
		BOOL forceCreate; // Make sure the key is created even if it is at frame 0
		SetMorphTargetPacket(Object *o,TSTR n,Matrix3 &m,BOOL fc=FALSE) {obj = o;name = n;tm = m;forceCreate=fc;}
		SetMorphTargetPacket(Object *o,TSTR n,BOOL fc=FALSE) {obj = o;name = n;tm = Matrix3(1);forceCreate=fc;}
	};

class MorphControl : public Control {
	public:
		
		// Access the object pipelines of the controller's targets. Note
		// that these are pointers to the pipelines, not the result of
		// evaluating the pipelines.
		virtual int NumMorphTargs() {return 0;}
		virtual Object *GetMorphTarg(int i) {return NULL;}
		virtual void DeleteMorphTarg(int i) {}
		virtual void GetMorphTargName(int i,TSTR &name) {name.printf(_T("Target #%d"),i);}
		virtual void SetMorphTargName(int i,TSTR name) {}
		virtual Matrix3 GetMorphTargTM(int i) {return Matrix3(1);}

		// Checks an object to see if it is an acceptable target.
		virtual BOOL ValidTarget(TimeValue t,Object *obj) {return FALSE;}

		// When a REFMSG_SELECT_BRANCH message is received the morph controller should
		// mark the target indicated and be prepared to return its ID from this method.
		virtual int GetFlaggedTarget() {return -1;}

		// Should call these methods on targets
		virtual BOOL HasUVW() { return 1; }
		virtual void SetGenUVW(BOOL sw) {  }
	};

//-------------------------------------------------------------
// Control base class for Master Controllers
//

class MasterPointControl : public Control {
	public:
		// Set the number of sub-controllers
		virtual	void SetNumSubControllers(int num, BOOL keep=FALSE) {}
		// Return the number of sub-controllers
		virtual	int	 GetNumSubControllers() { return 0; }
		// Delete all the sub-controllers that are set to TRUE in the BitArray
		virtual void DeleteControlSet (BitArray set) {}
		// Add a new sub-controller
		virtual int	 AddSubController(Control* ctrl) { return 0; }
		// Return i'th of sub-controller
		virtual Control* GetSubController(int i) { return NULL; }
		// Set the i'th sub-controller
		virtual	void SetSubController(int i, Control* ctrl) {}
};


//----------------------------------------------------------------//
//
// Some stuff to help with ORTs - these could actually be Interval methods

inline TimeValue CycleTime(Interval i,TimeValue t)
	{
	int res, dur = i.Duration()-1;
	if (dur<=0) return t;		
	res	= (t-i.Start())%dur;
	if (t<i.Start()) {
		return i.End()+res;
	} else {
		return i.Start()+res;
		}
	}

inline int NumCycles(Interval i,TimeValue t)
	{
	int dur = i.Duration()-1;
	if (dur<=0) return 1;
	if (t<i.Start()) {
		return (abs(t-i.Start())/dur)+1;
	} else 
	if (t>i.End()) {
		return (abs(t-i.End())/dur)+1;
	} else {
		return 0;
		}
	}



// Types that use this template must support:
//  T + T, T - T, T * float, T + float 

template <class T> T 
LinearExtrapolate(TimeValue t0, TimeValue t1, T &val0, T &val1, T &endVal)
	{
	return (T)(endVal + (val1-val0) * float(t1-t0));
	}

template <class T> T 
RepeatExtrapolate(Interval range, TimeValue t, 
		T &startVal, T &endVal, T &cycleVal)
	{
	int cycles = NumCycles(range,t);
	T delta;
	if (t<range.Start()) {
		delta = startVal - endVal;
	} else {
		delta = endVal - startVal;
		}
	return (T)(cycleVal + delta * float(cycles));
	}

template <class T> T 
IdentityExtrapolate(TimeValue endPoint, TimeValue t, T &endVal )
	{
	return (T)(endVal + float(t-endPoint));
	}

CoreExport Quat LinearExtrapolate(TimeValue t0, TimeValue t1, Quat &val0, Quat &val1, Quat &endVal);
CoreExport Quat RepeatExtrapolate(Interval range, TimeValue t, 
					Quat &startVal, Quat &endVal, Quat &cycleVal);
CoreExport Quat IdentityExtrapolate(TimeValue endPoint, TimeValue t, Quat &endVal );

CoreExport ScaleValue LinearExtrapolate(TimeValue t0, TimeValue t1, ScaleValue &val0, ScaleValue &val1, ScaleValue &endVal);
CoreExport ScaleValue RepeatExtrapolate(Interval range, TimeValue t, ScaleValue &startVal, ScaleValue &endVal, ScaleValue &cycleVal);
CoreExport ScaleValue IdentityExtrapolate(TimeValue endPoint, TimeValue t, ScaleValue &endVal);


template <class T> T
LinearInterpolate(const T &v0,const T &v1,float u)
	{
	return (T)((1.0f-u)*v0 + u*v1);
	}

inline Quat 
LinearInterpolate(const Quat &v0,const Quat &v1,float u)
	{
	return Slerp(v0,v1,u);
	}

inline ScaleValue 
LinearInterpolate(const ScaleValue &v0,const ScaleValue &v1,float u)
	{
	ScaleValue res;
	res.s = ((float)1.0-u)*v0.s + u*v1.s;
	res.q = Slerp(v0.q,v1.q,u);
	return res;
	}


inline Interval TestInterval(Interval iv, DWORD flags)
	{
	TimeValue start = iv.Start();
	TimeValue end = iv.End();
	if (!(flags&TIME_INCLEFT)) {
		start++;
		}	
	if (!(flags&TIME_INCRIGHT)) {
		end--;
		}
	if (end<start) {
		iv.SetEmpty();
	} else {
		iv.Set(start,end);
		}
	return iv;	
	}

inline Quat ScaleQuat(Quat q, float s)
	{
	float angle;
	Point3 axis;
	AngAxisFromQ(q,&angle,axis);
	return QFromAngAxis(angle*s,axis);
	}

//-------------------------------------------------------------------
// A place to store values during Hold/Restore periods
//
//********************************************************
// TempStore:  This is a temporary implementation:
//  It uses a linear search-
//  A hash-coded dictionary would be faster.
//  (if there are ever a lot of entries)
//********************************************************

struct Slot {
	void *key;
	void *pdata;
	int nbytes;	
	Slot *next;
	public:
		Slot() { pdata = NULL; }
		~Slot() {
			 if (pdata) free(pdata); 
			 pdata = NULL;
			 }

	};

class TempStore {
	Slot *slotList;				
	Slot* Find(int n, void *data, void *ptr);
	public:
		TempStore() { 	slotList = NULL;	}
		~TempStore() {	ClearAll();	}
		CoreExport void ClearAll();   // empty out the store 
		CoreExport void PutBytes(int n, void *data, void *ptr);
		CoreExport void GetBytes(int n, void *data, void *ptr);
		CoreExport void Clear(void *ptr);  // Remove single entry
		void PutFloat(float  f, void *ptr) {
			 PutBytes(sizeof(float),(void *)&f,ptr);
			 }
		CoreExport void PutInt(int i, void *ptr) {
			 PutBytes(sizeof(int),(void *)&i,ptr);
			 }
		CoreExport void GetFloat(float *f, void *ptr) { 
			GetBytes(sizeof(float),(void *)f,ptr);
			}
		CoreExport void GetInt(int *i, void *ptr) { 
			GetBytes(sizeof(int),(void *)i,ptr);
			}
		CoreExport void PutPoint3(Point3  f, void *ptr) {
			 PutBytes(sizeof(Point3),(void *)&f,ptr);
			 }
		CoreExport void GetPoint3(Point3 *f, void *ptr) { 
			GetBytes(sizeof(Point3),(void *)f,ptr);
			}
		CoreExport void PutQuat( Quat  f, void *ptr) {
			 PutBytes(sizeof(Quat),(void *)&f,ptr);
			 }
		CoreExport void GetQuat( Quat *f, void *ptr) { 
			GetBytes(sizeof(Quat),(void *)f,ptr);
			}
		CoreExport void PutScaleValue( ScaleValue  f, void *ptr) {
			 PutBytes(sizeof(ScaleValue),(void *)&f,ptr);
			 }
		CoreExport void GetScaleValue( ScaleValue *f, void *ptr) { 
			GetBytes(sizeof(ScaleValue),(void *)f,ptr);
			}
	};


extern CoreExport TempStore tmpStore;   // this should be in the scene data struct.


CoreExport int Animating();	 // is the animate switch on??
CoreExport void AnimateOn();  // turn animate on
CoreExport void AnimateOff();  // turn animate off
CoreExport void SuspendAnimate(); // suspend animation (uses stack)
CoreExport void ResumeAnimate();   // resume animation ( " )

CoreExport TimeValue GetAnimStart();
CoreExport TimeValue GetAnimEnd();
CoreExport void SetAnimStart(TimeValue s);
CoreExport void SetAnimEnd(TimeValue e);

CoreExport Control *NewDefaultFloatController();
CoreExport Control *NewDefaultPoint3Controller();
CoreExport Control *NewDefaultMatrix3Controller();
CoreExport Control *NewDefaultPositionController();
CoreExport Control *NewDefaultRotationController();
CoreExport Control *NewDefaultScaleController();
CoreExport Control *NewDefaultBoolController();
CoreExport Control *NewDefaultColorController();
CoreExport Control *NewDefaultMasterPointController();

CoreExport Control* CreateInterpFloat();
CoreExport Control* CreateInterpPosition();
CoreExport Control* CreateInterpPoint3();
CoreExport Control* CreateInterpRotation();
CoreExport Control* CreateInterpScale();
CoreExport Control* CreatePRSControl();
CoreExport Control* CreateLookatControl();
CoreExport Control* CreateMasterPointControl();

CoreExport void SetDefaultController(SClass_ID sid, ClassDesc *desc);
CoreExport ClassDesc *GetDefaultController(SClass_ID sid);

CoreExport void SetDefaultColorController(ClassDesc *desc);
CoreExport void SetDefaultBoolController(ClassDesc *desc);



#endif //__CONTROL__
