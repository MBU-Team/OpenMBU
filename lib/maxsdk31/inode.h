/**********************************************************************
 *<
	FILE: inode.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __INODE__H
#define __INODE__H

class ObjectState;
class Object;
class Control;
class ScaleValue;
class Mtl;
class RenderData;
class View;
class IDerivedObject;

// Transform modes -- passed to Move/Rotate/Scale
#define  PIV_NONE				0
#define  PIV_PIVOT_ONLY			1
#define  PIV_OBJECT_ONLY		2
#define  PIV_HIERARCHY_ONLY		3

// Node interface													   
class INode: public ReferenceTarget {
	public:
		// If this was a temporary INode (like an INodeTransformed) this will delete it.
		virtual void DisposeTemporary() {}

		// In the case of INodeTransformed, this gets a pointer to the real node.
		virtual INode *GetActualINode() {return this;}

		virtual TCHAR* 	GetName()=0;
		virtual	void	SetName(TCHAR *s)=0; 		
		
		// Get/Set node's transform ( without object-offset or WSM affect)
		virtual Matrix3	GetNodeTM(TimeValue t, Interval* valid=NULL)=0;
		virtual void 	SetNodeTM(TimeValue t, Matrix3& tm)=0;
		
		// Invalidate node's caches
		virtual void InvalidateTreeTM()=0;
		virtual void InvalidateTM()=0;
		virtual void InvalidateWS()=0;

		// Get object's transform (including object-offset)
		// and also the WSM affect when appropriate )
		// This is used inside object Display and HitTest routines
		virtual Matrix3 GetObjectTM(TimeValue time, Interval* valid=NULL)=0;

		// Get object's transform including object-offset but not WSM affect
		virtual Matrix3 GetObjTMBeforeWSM(TimeValue time, Interval* valid=NULL)=0;

		// Get object's transform including object-offset and WSM affect
		virtual Matrix3 GetObjTMAfterWSM(TimeValue time, Interval* valid=NULL)=0;

		// evaluate the State the object after offset and WSM's applied		
		// if evalHidden is FALSE and the node is hidden the pipeline will not
		// actually be evaluated (however the TM will).
		virtual	const ObjectState& EvalWorldState(TimeValue time,BOOL evalHidden=TRUE)=0;	

		// Hierarchy manipulation
		virtual INode* 	GetParentNode()=0;
		virtual void 	AttachChild(INode* node, int keepPos=1)=0; // make node a child of this one
		virtual	void 	Detach(TimeValue t, int keepPos=1)=0;  	  // detach node
		virtual int 	NumberOfChildren()=0;
		virtual INode* 	GetChildNode(int i)=0;
		// This will delete a node, handle removing from the hierarchy, and also handle Undo.		
		virtual void Delete(TimeValue t, int keepChildPosition) {} 

		// display attributes
		virtual void	Hide(BOOL onOff)=0;			// set node's hide bit
		virtual int		IsHidden(DWORD hflags=0,BOOL forRenderer=FALSE) {return 0;}
		virtual int		IsNodeHidden(BOOL forRenderer=FALSE) {return 0;}			// is node hidden in *any* way.
		virtual void	Freeze(BOOL onOff)=0;		// stop node from being pickable
		virtual int		IsFrozen()=0;
		virtual void	XRayMtl(BOOL onOff)=0;		// use x-ray material on node
		virtual int		HasXRayMtl()=0;
		virtual void	IgnoreExtents(BOOL onOff)=0;// ignore this node during zoom extents
		virtual int		GetIgnoreExtents()=0;
		virtual void	BoxMode(BOOL onOff)=0;		// display node with a bounding box
		virtual int		GetBoxMode()=0;
		virtual void	AllEdges(BOOL onOff)=0;		// display all edges, including "hidden" ones
		virtual int		GetAllEdges()=0;
		virtual void	BackCull(BOOL onOff)=0;		// backcull display toggle
		virtual int		GetBackCull()=0;
		virtual void 	SetCastShadows(BOOL onOff)=0; 
		virtual int		CastShadows()=0;
		virtual void 	SetRcvShadows(BOOL onOff)=0;
		virtual int		RcvShadows()=0;
		virtual void 	SetGenerateCaustics(BOOL onOff)	{}
		virtual int		GenerateCaustics()				{return 0;}
		virtual void 	SetRcvCaustics(BOOL onOff)		{}
		virtual int		RcvCaustics()					{return 0;}
		virtual void 	SetGenerateGlobalIllum(BOOL onOff)	{}
		virtual int		GenerateGlobalIllum()				{return 0;}
		virtual void 	SetRcvGlobalIllum(BOOL onOff)		{}
		virtual int		RcvGlobalIllum()					{return 0;}
		virtual void 	SetMotBlur(int kind)=0;	 // kind = 0: none, 1:object blur,  2: image  blur
		virtual int		MotBlur()=0;
		virtual float   GetImageBlurMultiplier(TimeValue t) { return 1.0f;}
		virtual void    SetImageBlurMultiplier(TimeValue t, float m){};
		virtual	void  	SetImageBlurMultController(Control *cont){}
		virtual	Control *GetImageBlurMultController() {return NULL; }

		// Object motion blur enable controller. This affects both object and image motion blur
		virtual BOOL GetMotBlurOnOff(TimeValue t) { return 1;  }
		virtual void  SetMotBlurOnOff(TimeValue t, BOOL m) { }
		virtual Control *GetMotBlurOnOffController() { return NULL;}
		virtual void SetMotBlurOnOffController(Control *cont) { }

		virtual void 	SetRenderable(BOOL onOff)=0;
		virtual int		Renderable()=0;
		virtual void    SetCVertMode(int onOff)		{}
		virtual int     GetCVertMode()				{return 0;}
		virtual void    SetShadeCVerts(int onOff)	{}
		virtual int     GetShadeCVerts()			{return 0;}
		virtual int		GetTrajectoryON() {return 0;}
		virtual void    SetTrajectoryON(BOOL onOff) {}

		// bone display attributes.
		virtual void 	ShowBone(int boneVis)=0;	// 0: off, 1: show bone, 2: show bone only
		virtual void	BoneAsLine(int onOff)=0; 	// display bone as simple line
		virtual BOOL	IsBoneShowing()=0;
		virtual BOOL	IsBoneOnly() { return 0; }  	// don't display node when displaying bone

		// used for hit-testing and selecting node and target as a single unit
		virtual void	SetTargetNodePair(int onOff) {}
		virtual int		GetTargetNodePair() { return 0; }

		// Access node's wire-frame color
		virtual DWORD 	GetWireColor()=0;
		virtual void 	SetWireColor(DWORD newcol)=0;

		// Test various flags
		virtual int 	IsRootNode()=0;
		virtual int 	Selected()=0;
		virtual int  	Dependent()=0;
		virtual int 	IsTarget()=0;
		virtual	void  	SetIsTarget(BOOL b)=0;

		// Node transform locks
		virtual BOOL GetTransformLock(int type, int axis)=0;
		virtual void SetTransformLock(int type, int axis, BOOL onOff)=0;

		// Get target node if any.
		virtual	INode* 	GetTarget()=0; 	// returns NULL if node has no target.
		virtual INode* 	GetLookatNode()=0; // if this is a target, this finds the node that looks at it.

		// This is just GetParent+GetNodeTM
		virtual Matrix3 GetParentTM(TimeValue t)=0;

		// This is just GetTarget+GetNodeTM
		virtual int 	GetTargetTM(TimeValue t, Matrix3& m)=0;

		// Object reference
		virtual Object* GetObjectRef()=0;	// skips over WSM's to the object
		virtual void 	SetObjectRef(Object *)=0;  // sets the object reference directly
		virtual Object* GetObjOrWSMRef()=0; // returns the object reference directly 

		// TM Controller
		virtual Control* GetTMController()=0;
		virtual void 	SetTMController(Control *m3cont)=0;

		// Visibility controller
		virtual Control *GetVisController()=0;
		virtual void    SetVisController(Control *cont)=0;
		virtual float   GetVisibility(TimeValue t,Interval *valid=NULL)=0;  // may be inherited
		virtual float   GetVisibility(TimeValue t,View &view,Interval *valid=NULL) {return GetVisibility(t,valid);}
		virtual void	SetVisibility(TimeValue t,float vis)=0;
		virtual float   GetLocalVisibility(TimeValue t,Interval *valid=NULL)=0; // not inherited
		virtual BOOL 	GetInheritVisibility()=0;
		virtual void 	SetInheritVisibility(BOOL onOff)=0;

		// Set/Get REnderOccluded property
		virtual void  SetRenderOccluded(BOOL onOff)=0;
		virtual BOOL  GetRenderOccluded()=0;

		// Renderer Materials
		virtual Mtl *GetMtl()=0;
		virtual void SetMtl(Mtl* matl)=0;

		// GraphicsWindow Materials
		virtual Material* Mtls()=0;   // Array  of GraphicsWindow Materials 
		virtual int 	NumMtls()=0;  // number of entries in Mtls

		// Object offset from node:
		virtual void 	SetObjOffsetPos(Point3 p)=0;
		virtual	Point3 	GetObjOffsetPos()=0;
		virtual	void 	SetObjOffsetRot(Quat q)=0;
		virtual	Quat 	GetObjOffsetRot()=0;
		virtual	void 	SetObjOffsetScale(ScaleValue sv)=0;
		virtual	ScaleValue GetObjOffsetScale()=0;
		
		// Misc.
		virtual void 	FlagForeground(TimeValue t,BOOL notify=TRUE)=0;
		virtual int 	IsActiveGrid()=0;

		// A place to hang temp data. Don't expect the data to stay around after you return control
		virtual void SetNodeLong(LONG l)=0;
		virtual LONG GetNodeLong()=0;

//		virtual void GetMaterial(Material &mtl)=0;  // Why do we need this?

		// Access render data
		virtual RenderData *GetRenderData()=0;
		virtual void SetRenderData(RenderData *rd)=0;

		//
		// Access user defined property text
		//
		// The first two functions access the entire buffer
		virtual void GetUserPropBuffer(TSTR &buf)=0;
		virtual void SetUserPropBuffer(const TSTR &buf)=0;

		// These get individual properties - return FALSE if the key is not found
		virtual BOOL GetUserPropString(const TSTR &key,TSTR &string)=0;
		virtual BOOL GetUserPropInt(const TSTR &key,int &val)=0;
		virtual BOOL GetUserPropFloat(const TSTR &key,float &val)=0;
		virtual BOOL GetUserPropBool(const TSTR &key,BOOL &b)=0;
		
		// These set individual properties - create the key if it doesn't exist
		virtual void SetUserPropString(const TSTR &key,const TSTR &string)=0;
		virtual void SetUserPropInt(const TSTR &key,int val)=0;
		virtual void SetUserPropFloat(const TSTR &key,float val)=0;
		virtual void SetUserPropBool(const TSTR &key,BOOL b)=0;
		
		// Just checks to see if a key exists
		virtual BOOL UserPropExists(const TSTR &key)=0;

		// G-Buffer ID's  (user settable)
		virtual ULONG GetGBufID()=0;
		virtual void SetGBufID(ULONG id)=0;

		// G-Buffer Render ID's (set by renderer)
		virtual UWORD GetRenderID() { return 0xffff; }
		virtual void SetRenderID(UWORD id) {}

		// Transform the node about a specified axis system.
		// Either the pivot point or the object or both can be transformed.
		// Also, the children can be counter transformed so they don't move.
		virtual void Move(TimeValue t, const Matrix3& tmAxis, const Point3& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE)=0;
		virtual void Rotate(TimeValue t, const Matrix3& tmAxis, const AngAxis& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE)=0;
		virtual void Rotate(TimeValue t, const Matrix3& tmAxis, const Quat& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE)=0;
		virtual void Scale(TimeValue t, const Matrix3& tmAxis, const Point3& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE)=0;

		virtual BOOL IsGroupMember()=0;
		virtual BOOL IsGroupHead()=0;
		virtual BOOL IsOpenGroupMember() {return 0; }
		virtual BOOL IsOpenGroupHead() {return 0; }

		virtual void SetGroupMember(BOOL b) {}
		virtual void SetGroupHead(BOOL b) {}
		virtual void SetGroupMemberOpen(BOOL b) {}
		virtual void SetGroupHeadOpen(BOOL b) {}

		// Some node IK params
		virtual float GetPosTaskWeight() {return 1.0f;}
		virtual float GetRotTaskWeight() {return 1.0f;}
		virtual void SetPosTaskWeight(float w) {}
		virtual void SetRotTaskWeight(float w) {}
		virtual BOOL GetTaskAxisState(int which,int axis) {return TRUE;} // which==0:pos  which==1:rot
		virtual void SetTaskAxisState(int which,int axis,BOOL onOff) {}
		virtual DWORD GetTaskAxisStateBits() {return 127;}		

		// Access to WSM Derived object. Note that there is at most one
		// WSM derived object per node. Calling CreateWSMDerivedObject()
		// will create a WSM derived object for the node if one doesn't 
		// already exist.
		virtual void CreateWSMDerivedObject() {}
		virtual IDerivedObject *GetWSMDerivedObject() {return NULL;}

		
		// Scene XRef related methods. These methods are only implemented by root nodes.
		// Scene XRefs are stored as complete scenes with root nodes where the XRef scene root
		// node is a child of the current scene's root node.
		virtual TSTR GetXRefFileName(int i) {return TSTR();}
		virtual void SetXRefFileName(int i,TCHAR *fname,BOOL reload) {}
		virtual int GetXRefFileCount() {return 0;}
		virtual BOOL AddNewXRefFile(TSTR &name, BOOL loadNow=TRUE) {return FALSE;}
		virtual BOOL DeleteXRefFile(int i) {return FALSE;} // removes scene xref
		virtual BOOL BindXRefFile(int i) {return FALSE;} // Deletes the xref after merging it into the scene
		virtual void DeleteAllXRefs() {} // Called when loading a new file, reseting or clearing the scene
		virtual BOOL ReloadXRef(int i) {return FALSE;}
		virtual void FlagXrefChanged(int i) {}
		virtual BOOL UpdateChangedXRefs(BOOL redraw=TRUE) {return FALSE;}
		virtual INode *GetXRefTree(int i) {return NULL;}
		virtual INode *GetXRefParent(int i) {return NULL;}
		virtual void SetXRefParent(int i, INode *par) {}
		virtual BOOL FindUnresolvedXRefs(Tab<TSTR*> &fnames) {return FALSE;}  // Returns TRUE if there are still unresolved refs
		virtual void AttemptToResolveUnresolvedXRefs() {} // Try to load any refs that are currently unresolved
		virtual DWORD GetXRefFlags(int i) {return 0;}
		virtual void SetXRefFlags(int i,DWORD flag,BOOL onOff) {}
		};		

// Xref flag bits
#define XREF_UPDATE_AUTO	(1<<0)
#define XREF_BOX_DISP		(1<<1)
#define XREF_HIDDEN			(1<<2)
#define XREF_DISABLED		(1<<3)
#define XREF_IGNORE_LIGHTS	(1<<4)
#define XREF_IGNORE_CAMERAS	(1<<5)
#define XREF_IGNORE_SHAPES	(1<<6)
#define XREF_IGNORE_HELPERS	(1<<7)
#define XREF_IGNORE_ANIM	(1<<8)
#define XREF_FILE_CHANGE	(1<<10)  // This bit is set when a change notification is sent indicating that the file may have changed. We don't know for sure if the file actually changed but the ref should be reloaded.
#define XREF_LOAD_ERROR		(1<<11)  // This bit will be set when a ref can't be resolved


// Transform lock types
#define INODE_LOCKPOS		0
#define INODE_LOCKROT		1
#define INODE_LOCKSCL		2

// Transform lock axis
#define INODE_LOCK_X		0
#define INODE_LOCK_Y		1
#define INODE_LOCK_Z		2

// Derive a class from this class, implementing the callback.
class ITreeEnumProc {
	public:
		virtual int callback( INode *node )=0;
	};

// Return values for the TreeEnum callback:
#define TREE_CONTINUE			0	// Continue enumerating
#define TREE_IGNORECHILDREN		1	// Don't enumerate children, but continue
#define TREE_ABORT				2	// Stop enumerating

// Node properties:
#define PROPID_PINNODE		PROPID_USER+1  	// Returns a pointer to the node this node is pinned to
#define PROPID_PRECEDENCE	PROPID_USER+2	// Returns an integer representing this node's precedence
#define PROPID_RELPOS		PROPID_USER+3	// Returns a pointer to the relative vector between the node and its pin
#define PROPID_RELROT		PROPID_USER+4	// Returns a pointer to the relative quaternion between the node and its pin



class INodeTransformed;

// INodeTransformed can be allocated on the stack, but if you need
// to create one dynamically, use these methods.
CoreExport void DeleteINodeTransformed(INodeTransformed *n);
CoreExport INodeTransformed *CreateINodeTransformed(INode *n,Matrix3 tm,BOOL dm=TRUE);

// This class provides a layer that will add in a transformation to the
// node's objectTM.
//
// Most methods pass through to the inode, except for the objectTM methods
// which pre-multiply in the given matrix.
//
class INodeTransformed : public INode {
	public:
		INode *node;
		Matrix3 tm;
		BOOL deleteMe;

		INodeTransformed(INode *n,Matrix3 tm,BOOL dm=TRUE) {node = n;this->tm = tm;deleteMe = dm;}
		
		void DisposeTemporary() {node->DisposeTemporary(); if (deleteMe) DeleteINodeTransformed(this);}
		INode *GetActualINode() {return node->GetActualINode();}
		
		TCHAR* 	GetName() {return node->GetName();}
		void	SetName(TCHAR *s) {node->SetName(s);}
		Matrix3	GetNodeTM(TimeValue t, Interval* valid=NULL) {return node->GetNodeTM(t,valid);}
		void 	SetNodeTM(TimeValue t, Matrix3& tm) {node->SetNodeTM(t,tm);}
		void InvalidateTreeTM() {node->InvalidateTreeTM();}
		void InvalidateTM() {node->InvalidateTM();}
		void InvalidateWS() {node->InvalidateWS();}
		Matrix3 GetObjectTM(TimeValue time, Interval* valid=NULL) {return tm*node->GetObjectTM(time,valid);}
		Matrix3 GetObjTMBeforeWSM(TimeValue time, Interval* valid=NULL) {return tm*node->GetObjTMBeforeWSM(time,valid);}
		Matrix3 GetObjTMAfterWSM(TimeValue time, Interval* valid=NULL) {return tm*node->GetObjTMAfterWSM(time,valid);}
		const ObjectState& EvalWorldState(TimeValue time,BOOL evalHidden=TRUE) {return node->EvalWorldState(time,evalHidden);}
		INode* 	GetParentNode() {return node->GetParentNode();}
		void 	AttachChild(INode* node, int keepPos=1) {node->AttachChild(node,keepPos);}
		void 	Detach(TimeValue t, int keepPos=1) {node->Detach(t,keepPos);}
		int 	NumberOfChildren() {return node->NumberOfChildren();}
		INode* 	GetChildNode(int i) {return node->GetChildNode(i);}
		void    Delete(TimeValue t, int keepChildPosition) { node->Delete(t,keepChildPosition); } 
		void	Hide(BOOL onOff) {node->Hide(onOff);}
		int		IsHidden(DWORD hflags=0,BOOL forRenderer=FALSE) {return node->IsHidden(hflags,forRenderer);}
		int		IsNodeHidden(BOOL forRenderer=FALSE) { return node->IsNodeHidden(forRenderer); }
		void	Freeze(BOOL onOff) {node->Freeze(onOff);}
		int		IsFrozen() {return node->IsFrozen();}
		void	XRayMtl(BOOL onOff) {node->XRayMtl(onOff);}
		int		HasXRayMtl() {return node->HasXRayMtl();}
		void	IgnoreExtents(BOOL onOff) {node->IgnoreExtents(onOff);}
		int		GetIgnoreExtents() {return node->GetIgnoreExtents();}
		void	BoxMode(BOOL onOff) {node->BoxMode(onOff);}
		int		GetBoxMode() {return node->GetBoxMode();}
		void	AllEdges(BOOL onOff) {node->AllEdges(onOff);}
		int		GetAllEdges() {return node->GetAllEdges();}
		void	BackCull(BOOL onOff) {node->BackCull(onOff);}
		int		GetBackCull() {return node->GetBackCull();}
		void 	SetCastShadows(BOOL onOff) { node->SetCastShadows(onOff); } 
		int		CastShadows() { return node->CastShadows(); }
		void 	SetRcvShadows(BOOL onOff) { node->SetRcvShadows(onOff); }
		int		RcvShadows() { return node->RcvShadows(); }
		void 	SetGenerateCaustics(BOOL onOff) { node->SetGenerateCaustics(onOff); } 
		int		GenerateCaustics() { return node->GenerateCaustics(); }
		void 	SetRcvCaustics(BOOL onOff) { node->SetRcvCaustics(onOff); }
		int		RcvCaustics() { return node->RcvCaustics(); }
		void 	SetGenerateGlobalIllum(BOOL onOff) { node->SetGenerateGlobalIllum(onOff); } 
		int		GenerateGlobalIllum() { return node->GenerateGlobalIllum(); }
		void 	SetRcvGlobalIllum(BOOL onOff) { node->SetRcvGlobalIllum(onOff); }
		int		RcvGlobalIllum() { return node->RcvGlobalIllum(); }

		void 	SetMotBlur(BOOL onOff) { node->SetMotBlur(onOff); }
		int		MotBlur() { return node->MotBlur(); }

		float   GetImageBlurMultiplier(TimeValue t) { return node->GetImageBlurMultiplier(t);}
		void    SetImageBlurMultiplier(TimeValue t, float m) {node->SetImageBlurMultiplier(t,m); };
		void  	SetImageBlurMultController(Control *cont){ node->SetImageBlurMultController(cont); }
		Control *GetImageBlurMultController() {return node->GetImageBlurMultController(); }

		// Object motion blur enable controller. This affects only object motion blur
		BOOL GetMotBlurOnOff(TimeValue t) { return node->GetMotBlurOnOff(t); }
		void  SetMotBlurOnOff(TimeValue t, BOOL m) { node->SetMotBlurOnOff(t,m); }
		Control *GetMotBlurOnOffController() { return node->GetMotBlurOnOffController();}
		void SetMotBlurOnOffController(Control *cont) { node->SetMotBlurOnOffController(cont);}

		void 	SetRenderable(BOOL onOff) { node->SetRenderable(onOff); }
		int		Renderable() { return node->Renderable(); }
		void 	ShowBone(int boneVis) {node->ShowBone(boneVis);}
		void	BoneAsLine(int onOff) {node->BoneAsLine(onOff);}
		BOOL	IsBoneShowing() {return node->IsBoneShowing();}
		BOOL	IsBoneOnly() { return node->IsBoneOnly(); }
		DWORD 	GetWireColor() {return node->GetWireColor();}
		void 	SetWireColor(DWORD newcol) {node->SetWireColor(newcol);}
		int 	IsRootNode() {return node->IsRootNode();}
		int 	Selected() {return node->Selected();}
		int  	Dependent() {return node->Dependent();}
		int 	IsTarget() {return node->IsTarget();}
		void  	SetIsTarget(BOOL b) { node->SetIsTarget(b);}
		BOOL 	GetTransformLock(int type, int axis) {return node->GetTransformLock(type,axis);}
		void 	SetTransformLock(int type, int axis, BOOL onOff) {node->SetTransformLock(type,axis,onOff);}
		INode* 	GetTarget() {return node->GetTarget();}
		INode* 	GetLookatNode() {return node->GetLookatNode();}
		Matrix3 GetParentTM(TimeValue t) {return node->GetParentTM(t);}
		int 	GetTargetTM(TimeValue t, Matrix3& m) {return node->GetTargetTM(t,m);}
		Object* GetObjectRef() {return node->GetObjectRef();}
		void 	SetObjectRef(Object *o) {node->SetObjectRef(o);}
		Object* GetObjOrWSMRef() { return node->GetObjOrWSMRef();}  
		Control* GetTMController() {return node->GetTMController();}
		void 	SetTMController(Control *m3cont) {node->SetTMController(m3cont);}		
		Control *GetVisController() {return node->GetVisController();}
		void    SetVisController(Control *cont) {node->SetVisController(cont);}
		float   GetVisibility(TimeValue t,Interval *valid=NULL) {return node->GetVisibility(t,valid);}
		void	SetVisibility(TimeValue t,float vis) { node->SetVisibility(t,vis); }
		float   GetLocalVisibility(TimeValue t,Interval *valid) { return node->GetLocalVisibility(t,valid); }
		BOOL 	GetInheritVisibility() { return node->GetInheritVisibility(); }
		void 	SetInheritVisibility(BOOL onOff) { node->SetInheritVisibility(onOff); }

		virtual void  SetRenderOccluded(BOOL onOff) { node->SetRenderOccluded(onOff); }
		virtual BOOL  GetRenderOccluded(){ return node->GetRenderOccluded(); }
		
		Mtl *GetMtl() { return node->GetMtl(); }
		void SetMtl(Mtl* matl) { node->SetMtl(matl); }

		Material* Mtls() { return node->Mtls(); }    
		int 	NumMtls() { return node->NumMtls(); }

		RenderData *GetRenderData() {return node->GetRenderData();}
		void SetRenderData(RenderData *rd) {node->SetRenderData(rd);}

		void 	SetObjOffsetPos(Point3 p) {node->SetObjOffsetPos(p);}
		Point3 	GetObjOffsetPos() {return node->GetObjOffsetPos();}
		void 	SetObjOffsetRot(Quat q) {node->SetObjOffsetRot(q);}
		Quat 	GetObjOffsetRot() {return node->GetObjOffsetRot();}		
		void 	FlagForeground(TimeValue t,BOOL notify=TRUE) {node->FlagForeground(t,notify);}
		int 	IsActiveGrid() {return node->IsActiveGrid();}
		void SetNodeLong(LONG l) {node->SetNodeLong(l);}
		LONG GetNodeLong() {return node->GetNodeLong();}

		void GetUserPropBuffer(TSTR &buf) {node->GetUserPropBuffer(buf);}
		void SetUserPropBuffer(const TSTR &buf) {node->SetUserPropBuffer(buf);}
		BOOL GetUserPropString(const TSTR &key,TSTR &string) {return node->GetUserPropString(key,string);}
		BOOL GetUserPropInt(const TSTR &key,int &val) {return node->GetUserPropInt(key,val);}
		BOOL GetUserPropFloat(const TSTR &key,float &val) {return node->GetUserPropFloat(key,val);}
		BOOL GetUserPropBool(const TSTR &key,BOOL &b) {return node->GetUserPropBool(key,b);}
		void SetUserPropString(const TSTR &key,const TSTR &string) {node->SetUserPropString(key,string);}
		void SetUserPropInt(const TSTR &key,int val) {node->SetUserPropInt(key,val);}
		void SetUserPropFloat(const TSTR &key,float val) {node->SetUserPropFloat(key,val);}
		void SetUserPropBool(const TSTR &key,BOOL b) {node->SetUserPropBool(key,b);}
		BOOL UserPropExists(const TSTR &key) {return node->UserPropExists(key);}
		ULONG GetGBufID() { return node->GetGBufID(); }
		void SetGBufID(ULONG id) { node->SetGBufID(id); }

		UWORD GetRenderID() { return node->GetRenderID(); }
		void SetRenderID(UWORD id) { node->SetRenderID(id); }

		CoreExport void 	SetObjOffsetScale(ScaleValue sv);
		CoreExport ScaleValue GetObjOffsetScale();

		void Move(TimeValue t, const Matrix3& tmAxis, const Point3& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE) {node->Move(t,tmAxis,val,localOrigin,pivMode,ignoreLocks);}
		void Rotate(TimeValue t, const Matrix3& tmAxis, const AngAxis& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE) {node->Rotate(t,tmAxis,val,localOrigin,pivMode,ignoreLocks);}
		void Rotate(TimeValue t, const Matrix3& tmAxis, const Quat& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE) {node->Rotate(t,tmAxis,val,localOrigin,pivMode,ignoreLocks);}
		void Scale(TimeValue t, const Matrix3& tmAxis, const Point3& val, BOOL localOrigin=FALSE, BOOL affectKids=TRUE, int pivMode=PIV_NONE, BOOL ignoreLocks=FALSE) {node->Scale(t,tmAxis,val,localOrigin,pivMode,ignoreLocks);}

		BOOL IsGroupMember() {return node->IsGroupMember();}
		BOOL IsGroupHead() { return node->IsGroupHead();}
		BOOL IsOpenGroupMember(){return node->IsOpenGroupMember();}
		BOOL IsOpenGroupHead(){return node->IsOpenGroupHead();}

		void SetGroupMember(BOOL b) { node->SetGroupMember(b); }
		void SetGroupHead(BOOL b) { node->SetGroupHead(b); }
		void SetGroupMemberOpen(BOOL b) { node->SetGroupMemberOpen(b); }
		void SetGroupHeadOpen(BOOL b) { node->SetGroupHeadOpen(b); }

		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	         PartID& partID,  RefMessage message) {return REF_SUCCEED;}
	};


#endif //__INODE__H
