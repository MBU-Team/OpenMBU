/**********************************************************************
 *<
	FILE: simpmod.h

	DESCRIPTION:  Simple modifier base class

	CREATED BY: Dan Silva & Rolf Berteig

	HISTORY: created 30 Jauary, 1995

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __SIMPMOD__
#define __SIMPMOD__


//--- SimpleMod -----------------------------------------------------------

class SimpleMod: public Modifier {
	public:	
		Control *tmControl;
		Control *posControl;
		IParamBlock *pblock;
				
		CoreExport static IObjParam *ip;
		static MoveModBoxCMode *moveMode;
		static RotateModBoxCMode *rotMode;
		static UScaleModBoxCMode *uscaleMode;
		static NUScaleModBoxCMode *nuscaleMode;
		static SquashModBoxCMode *squashMode;		
		static SimpleMod *editMod;
			
		CoreExport SimpleMod();
		CoreExport virtual ~SimpleMod();

		ChannelMask ChannelsUsed()  { return PART_GEOM|PART_TOPO|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL; }
		ChannelMask ChannelsChanged() { return PART_GEOM; }
		CoreExport void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
		Class_ID InputType() {return defObjectClassID;}
		CoreExport Interval LocalValidity(TimeValue t);
		CoreExport Matrix3 CompMatrix(TimeValue t, ModContext& mc, Matrix3& ntm, 
			Interval& valid, BOOL needOffset);
		CoreExport void CompOffset( TimeValue t, Matrix3& offset, Matrix3& invoffset);

		// From BaseObject
		CoreExport int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc);
		CoreExport int Display(TimeValue t, INode* inode, ViewExp *vpt, int flagst, ModContext *mc);
		CoreExport void GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box, ModContext *mc);
		
		CoreExport void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
		CoreExport void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
		BOOL ChangeTopology() {return FALSE;}

		CoreExport IParamArray *GetParamBlock();
		CoreExport int GetParamBlockIndex(int id);

		// Affine transform methods
		CoreExport void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE );
		CoreExport void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin=FALSE );
		CoreExport void Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE );
		
		int NumRefs() {return 3;}
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);

		int NumSubs() {return 3;}
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);
		CoreExport int SubNumToRefNum(int subNum);
		CoreExport BOOL AssignController(Animatable *control,int subAnim);

		CoreExport RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);

		CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 
		CoreExport void ActivateSubobjSel(int level, XFormModes& modes);

		// When clients are cloning themselves, they should call this 
		// method on the clone to copy SimpleMod's data.
		CoreExport void SimpleModClone(SimpleMod *smodSource);

		// Clients of simpmod probably want to override these. If they do
		// the should call these from within thier methods.
		CoreExport void BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams(IObjParam *ip, ULONG flags,Animatable *next);

		// Clients of simpmod need to implement this method
		virtual Deformer& GetDeformer(TimeValue t,ModContext &mc,Matrix3& mat,Matrix3& invmat)=0;
		virtual void InvalidateUI() {}
		virtual Interval GetValidity(TimeValue t) {return FOREVER;}
		virtual	ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}
		virtual BOOL GetModLimits(TimeValue t,float &zmin, float &zmax, int &axis) {return FALSE;}
	};


// This is the ref ID of the parameter block
#define SIMPMOD_PBLOCKREF	2

// ParamBlock2 specialization added JBW 2/9/99 (replaces a IParamBlock with IParamBlock2 block pointer)
class IParamBlock2;
class SimpleMod2 : public SimpleMod {
	public:
		IParamBlock2* pblock2;

		SimpleMod2() { pblock2 = NULL; }
		// From ref
		RefTargetHandle GetReference(int i);
		void SetReference(int i, RefTargetHandle rtarg);		
		Animatable* SubAnim(int i);
	};

//--- SimpleWSMMod -----------------------------------------------------------

class SimpleWSMMod: public Modifier {	
	public:
		WSMObject  	*obRef;
		INode       *nodeRef;
		IParamBlock *pblock;
				
		CoreExport static IObjParam *ip;
		static SimpleWSMMod *editMod;
	
		CoreExport SimpleWSMMod();
		CoreExport virtual ~SimpleWSMMod();

		ChannelMask ChannelsUsed()  { return PART_GEOM|PART_TOPO; }
		ChannelMask ChannelsChanged() { return PART_GEOM; }
		CoreExport void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
		Class_ID InputType() {return defObjectClassID;}
		CoreExport Interval LocalValidity(TimeValue t);		
		BOOL ChangeTopology() {return FALSE;}
		CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;}

		int NumRefs() {return 3;}
		CoreExport RefTargetHandle GetReference(int i);
		CoreExport void SetReference(int i, RefTargetHandle rtarg);

		int NumSubs() {return 1;}
		CoreExport Animatable* SubAnim(int i);
		CoreExport TSTR SubAnimName(int i);

		CoreExport RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message);
		
		CoreExport IParamArray *GetParamBlock();
		CoreExport int GetParamBlockIndex(int id);

		// Evaluates the node reference and returns the WSM object.
		CoreExport WSMObject *GetWSMObject(TimeValue t);
				
		// When clients are cloning themselves, they should call this 
		// method on the clone to copy SimpleMod's data.
		CoreExport void SimpleWSMModClone(SimpleWSMMod *smodSource);

		// Clients of simpmod probably want to override these. If they do
		// the should call these from within thier methods.
		CoreExport void BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams(IObjParam *ip, ULONG flags,Animatable *next);

		// Clients of simpmod need to implement this method
		virtual Deformer& GetDeformer(TimeValue t,ModContext &mc,Matrix3& mat,Matrix3& invmat)=0;
		virtual void InvalidateUI() {}
		virtual Interval GetValidity(TimeValue t) {return FOREVER;}
		virtual	ParamDimension *GetParameterDim(int pbIndex) {return defaultDim;}
		virtual TSTR GetParameterName(int pbIndex) {return TSTR(_T("Parameter"));}
		virtual void InvalidateParamMap() {}

		// Schematic view Animatable overides...
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
	};


#define SIMPWSMMOD_OBREF		0
#define SIMPWSMMOD_NODEREF		1
#define SIMPWSMMOD_PBLOCKREF	2

#define SIMPLEOSMTOWSM_CLASSID	Class_ID(0x3fa72be3,0xa5ee1bf9)

// Used by SimpleOSMToWSMObject to create WSMs out of OSMs
class SimpleOSMToWSMMod : public SimpleWSMMod {
	public:
		CoreExport SimpleOSMToWSMMod();
		CoreExport SimpleOSMToWSMMod(INode *node);

		void GetClassName(TSTR& s) {s=GetObjectName();}
		SClass_ID SuperClassID() {return WSM_CLASS_ID;}
		Class_ID ClassID() {return SIMPLEOSMTOWSM_CLASSID;} 
		void DeleteThis() {delete this;}
		CoreExport RefTargetHandle Clone(RemapDir& remap = NoRemap());
		CoreExport TCHAR *GetObjectName();

		CoreExport Deformer& GetDeformer(TimeValue t,ModContext &mc,Matrix3& mat,Matrix3& invmat);		
		CoreExport Interval GetValidity(TimeValue t);		
	};

CoreExport ClassDesc* GetSimpleOSMToWSMModDesc();

#endif

