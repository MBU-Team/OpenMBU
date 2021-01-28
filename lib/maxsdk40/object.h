/**********************************************************************
 *<
	FILE: object.h
				  
	DESCRIPTION:  Defines Object Classes

	CREATED BY: Dan Silva

	HISTORY: created 9 September 1994

 *>     Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _OBJECT_

#define _OBJECT_

#include "inode.h"
#include "maxapi.h"
#include "plugapi.h"
#include "snap.h"
#include "genshape.h"
#include <hitdata.h>
#include "imtl.h"

typedef short MtlIndex; 
typedef short TextMapIndex;

CoreExport void setHitType(int t);
CoreExport int  getHitType(void);
CoreExport BOOL doingXORDraw(void);

// Hit test types:
#define HITTYPE_POINT   1
#define HITTYPE_BOX             2
#define HITTYPE_CIRCLE  3
#define HITTYPE_SOLID   4
#define HITTYPE_FENCE   5

// Flags for hit test.
#define HIT_SELONLY				(1<<0)
#define HIT_UNSELONLY			(1<<2)
#define HIT_ABORTONHIT			(1<<3)
#define HIT_SELSOLID			(1<<4)
#define HIT_ANYSOLID			(1<<5)
#define HIT_TRANSFORMGIZMO		(1<<6)	// CCJ - 06/29/98 - Hittest transform gizmo
#define HIT_SWITCH_GIZMO		(1<<7)	// CCJ - 06/29/98 - Switch axis when hit
#define HIT_MANIP_SUBHIT        (1<<8)  // SCM - 5/31/00 - hit test sub-manipulators

// These are filters for hit testing. They also
// are combined into the flags parameter.
#define HITFLTR_ALL                     (1<<10)
#define HITFLTR_OBJECTS         (1<<11)
#define HITFLTR_CAMERAS         (1<<12)
#define HITFLTR_LIGHTS          (1<<13)
#define HITFLTR_HELPERS         (1<<14)
#define HITFLTR_WSMOBJECTS      (1<<15)
#define HITFLTR_SPLINES         (1<<16)

// Starting at this bit through the 31st bit can be used
// by plug-ins for sub-object hit testing
#define HITFLAG_STARTUSERBIT    24


#define VALID(x) (x)

class Modifier;
class Object;
class NameTab; 
class ExclList; 
class Texmap;
class ISubObjType;
class MaxIcon;
  
typedef Object* ObjectHandle;

MakeTab(TextMapIndex)
typedef TextMapIndexTab TextTab;

#define BASEOBJAPPDATACID Class_ID(0x48a057d2, 0x44f70d8a)
#define BASEOBJAPPDATALASTSELCID Class_ID(0x1cef158c, 0x1da8486f)
#define BASEOBJAPPDATACURSELCID Class_ID(0x5b3b25fc, 0x35af6260)
#define BASEOBJAPPDATASCID USERDATATYPE_CLASS_ID

//---------------------------------------------------------------  
class IdentityTM: public Matrix3 {
	public:
		IdentityTM() { IdentityMatrix(); }              
	};

CoreExport extern IdentityTM idTM;


//-------------------------------------------------------------
// This is passed in to GetRenderMesh to allow objects to do
// view dependent rendering.
//

// flag defines for View::flags
#define RENDER_MESH_DISPLACEMENT_MAP  1   // enable displacement mapping

class View : public InterfaceServer{
	public: 
		float screenW, screenH;  // screen dimensions
		Matrix3 worldToView;
		virtual Point2 ViewToScreen(Point3 p)=0;
		// the following added for GAP
		int projType;
		float fov, pixelSize;
		Matrix3 affineTM;         // worldToCam
		DWORD flags;

		// Call during render to check if user has cancelled render.  
		// Returns TRUE iff user has cancelled.
		virtual BOOL CheckForRenderAbort() { return FALSE; }

		// Generic expansion function
		virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 

		View() { projType = -1; flags = RENDER_MESH_DISPLACEMENT_MAP; } // projType not set, this is to deal with older renderers.
	};
//-------------------------------------------------------------

// Class ID of general deformable object.
extern CoreExport Class_ID defObjectClassID;

//-------------------------------------------------------------

// Class ID of general texture-mappable object.
extern CoreExport Class_ID mapObjectClassID;

//-------------------------------------------------------------
// ChannelMask: bits specific channels in the OSM dataflow. 

typedef ULONG_PTR ChannelMask;
	// WIN64 Cleanup: Shuler

// an array of channel masks for all the channels *within*
// the Object.
CoreExport extern ChannelMask chMask[];

class Object;

//-- ObjectState ------------------------------------------------------------
// This is what is passed down the pipeline, and ultimately used by the Node
//  to Display, Hittest, render:

// flags bits

class ObjectState {
		ulong flags;
		Matrix3 *tm;
		Interval tmvi;   
		int mtl;
		Interval mtlvi;                         
		void AllocTM();
	public: 
		Object *obj;  // object: provides interval with obj->ObjectValidity()
		CoreExport ObjectState();
		CoreExport ObjectState(Object *ob);
		CoreExport ObjectState(const ObjectState& os); 
		CoreExport ~ObjectState();
		void OSSetFlag(ulong f) { flags |= f; }
		void OSClearFlag(ulong f) { flags &= ~f; }
		ulong OSTestFlag(ulong f) const { return flags&f; }
		CoreExport void OSCopyFlag(ulong f, const ObjectState& fromos);
		CoreExport ObjectState& operator=(const ObjectState& os);
		Interval tmValid() const { return tmvi; }
		Interval mtlValid() const  { return mtlvi; }
		CoreExport Interval Validity(TimeValue t) const;
		CoreExport int TMIsIdentity() const;
		CoreExport void SetTM(Matrix3* mat, Interval iv);
		CoreExport Matrix3* GetTM() const;
		CoreExport void SetIdentityTM();
		CoreExport void ApplyTM(Matrix3* mat, Interval iv);
		CoreExport void CopyTM(const ObjectState &fromos);
		CoreExport void CopyMtl(const ObjectState &fromos);
		CoreExport void Invalidate(ChannelMask channels, BOOL checkLock=FALSE);
		CoreExport ChannelMask DeleteObj(BOOL checkLock=FALSE);
	};

class INodeTab : public Tab<INode*> {
	public:         
		void DisposeTemporary() {
			for (int i=0; i<Count(); i++) (*this)[i]->DisposeTemporary();
			}
	};

//---------------------------------------------------------------  
// A reference to a pointer to an instance of this class is passed in
// to ModifyObject(). The value of the pointer starts out as NULL, but
// the modifier can set it to point at an actual instance of a derived
// class. When the mod app is deleted, if the pointer is not NULL, the
// LocalModData will be deleted - the virtual destructor alows this to work.

class LocalModData : public InterfaceServer {
	public:
		virtual ~LocalModData() {}
		virtual LocalModData *Clone()=0;
		virtual void* GetInterface(ULONG id) { return NULL; }  // to access sub-obj selection interfaces, JBW 2/5/99
	}; 

class ModContext : public BaseInterfaceServer {
	public:
	Matrix3                 *tm;
	Box3                    *box;
	LocalModData    *localData;
	
	CoreExport ~ModContext();
	CoreExport ModContext();
	CoreExport ModContext(const ModContext& mc);
	CoreExport ModContext(Matrix3 *tm, Box3 *box, LocalModData *localData);
	};

class ModContextList : public Tab<ModContext*> {};


class HitRecord;


// Values passed to NewSetByOperator()
#define NEWSET_MERGE			1
#define NEWSET_INTERSECTION		2
#define NEWSET_SUBTRACT			3


// Flags passed to Display()
#define USE_DAMAGE_RECT                 (1<<0)  
#define DISP_SHOWSUBOBJECT              (1<<1)

// The base class of Geometric objects, Lights, Cameras, Modifiers, 
//  Deformation objects--
// --anything with a 3D representation in the UI scene. 

class IParamArray;

class BaseObject: public ReferenceTarget {
		
		friend class ModifyTaskImp;
		int subObjLevel;
	public:
		CoreExport void* GetInterface(ULONG id);
		virtual BaseInterface* GetInterface(Interface_ID id) { return ReferenceTarget::GetInterface(id); }

		CoreExport BaseObject();
		virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt){return 0;};
		virtual void SetExtendedDisplay(int flags)      {}      // for setting mode-dependent display attributes
		virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) { return 0; };   // quick render in viewport, using current TM.         
		virtual void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) {}       // Check for snap, updating SnapInfo
		virtual void GetWorldBoundBox(TimeValue t, INode * inode, ViewExp* vp, Box3& box ){};  // Box in world coords.
		virtual void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vp,  Box3& box ){};  // box in objects local coords
		virtual CreateMouseCallBack* GetCreateMouseCallBack()=0;
		
		// This is the name that will appear in the history browser.
		virtual TCHAR *GetObjectName() { return _T("Object"); }

		// Sends the REFMSG_IS_OK_TO_CHANGE_TOPOLOGY off to see if any
		// modifiers or objects down the pipeline depend on topology.
		// modName will be set to the dependent modifier's name if there is one.
		CoreExport virtual BOOL OKToChangeTopology(TSTR &modName);

		// Return true if this object(or modifier) is cabable of changing 
		//topology when it's parameters are being edited.
		virtual BOOL ChangeTopology() {return TRUE;}

		virtual void ForceNotify(Interval& i)
			{NotifyDependents(i, PART_ALL,REFMSG_CHANGE);}
				
		// If an object or modifier wishes it can make its parameter block
		// available for other plug-ins to access. The system itself doesn't
		// actually call this method -- this method is optional.
		virtual IParamArray *GetParamBlock() {return NULL;}
		
		// If a plug-in make its parameter block available then it will
		// need to provide #defines for indices into the parameter block.
		// These defines should probably not be directly used with the
		// parameter block but instead converted by this function that the
		// plug-in implements. This way if a parameter moves around in a 
		// future version of the plug-in the #define can be remapped.
		// -1 indicates an invalid parameter id
		virtual int GetParamBlockIndex(int id) {return -1;}


		///////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////
		//
		// The following methods are for sub-object selection. If the
		// derived class is NOT a modifier, the modContext pointer passed
		// to some of the methods will be NULL.
		//

		// Affine transform methods
		virtual void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE ){}
		virtual void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin=FALSE ){}
		virtual void Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin=FALSE ){}

		// The following is called before the first Move(), Rotate() or Scale() call and
		// before a hold is in effect
		virtual void TransformStart(TimeValue t) {}

		// The following is called before the first Move(), Rotate() or Scale() call and
		// after a hold is in effect
		virtual void TransformHoldingStart(TimeValue t) {}

		// The following is called after the user has completed the Move, Rotate or Scale operation and
		// before the undo object has been accepted.
		virtual void TransformHoldingFinish(TimeValue t) {}             

		// The following is called after the user has completed the Move, Rotate or Scale operation and
		// after the undo object has been accepted.
		virtual void TransformFinish(TimeValue t) {}            

		// The following is called when the transform operation is cancelled by a right-click and
		// the undo has been cancelled.
		virtual void TransformCancel(TimeValue t) {}            

		virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc) { return 0; }
		virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext* mc) { return 0; };   // quick render in viewport, using current TM.         
		virtual void GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box, ModContext *mc) {}
		
		virtual void CloneSelSubComponents(TimeValue t) {}
		virtual void AcceptCloneSelSubComponents(TimeValue t) {}

		// Changes the selection state of the component identified by the
		// hit record.
		virtual void SelectSubComponent(
			HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert=FALSE) {}
		
		// Clears the selection for the given sub-object type.
		virtual void ClearSelection(int selLevel) {}
		virtual void SelectAll(int selLevel) {}
		virtual void InvertSelection(int selLevel) {}

		// Returns the index of the subobject entity identified by hitRec.
		virtual int SubObjectIndex(HitRecord *hitRec) {return 0;}               
		
		// This notifies an object being edited that the current sub object
		// selection level has changed. level==0 indicates object level selection.
		// level==1 or greater refer to the types registered by the object in the
		// order they appeared in the list when registered.
		// If level >= 1, the object should specify sub-object xform modes in the
		// modes structure (defined in cmdmode.h).
		virtual void ActivateSubobjSel(int level, XFormModes& modes ) {}

		// An object that supports sub-object selection can choose to
		// support named sub object selection sets. Methods in the the
		// interface passed to objects allow them to add items to the
		// sub-object selection set drop down.
		// The following methods are called when the user picks items
		// from the list.
		virtual BOOL SupportsNamedSubSels() {return FALSE;}
		virtual void ActivateSubSelSet(TSTR &setName) {}
		virtual void NewSetFromCurSel(TSTR &setName) {}
		virtual void RemoveSubSelSet(TSTR &setName) {}

		// New for version 2. To support the new edit named selections dialog,
		// plug-ins must implemented the following methods:
		virtual void SetupNamedSelDropDown() {}
		virtual int NumNamedSelSets() {return 0;}
		virtual TSTR GetNamedSelSetName(int i) {return _T("");}
		virtual void SetNamedSelSetName(int i,TSTR &newName) {}
		virtual void NewSetByOperator(TSTR &newName,Tab<int> &sets,int op) {}


		// New way of dealing with sub object coordinate systems.
		// Plug-in enumerates its centers or TMs and calls the callback once for each.
		// NOTE:cb->Center() should be called the same number of times and in the
		// same order as cb->TM()
		// NOTE: The SubObjAxisCallback class is defined in animatable and used in both the
		// controller version and this version of GetSubObjectCenters() and GetSubObjectTMs()
		virtual void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc) {}
		virtual void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc) {}                          


		// Find out if the Object or Modifer is is generating UVW's
		// on map channel 1.
		virtual BOOL HasUVW () { return 0; }
		// or on any map channel:
		virtual BOOL HasUVW (int mapChannel) { return (mapChannel==1) ? HasUVW() : FALSE; }

		// Change the state of the object's Generate UVW boolean.
		// IFF the state changes, the object should send a REFMSG_CHANGED down the pipe.
		virtual void SetGenUVW(BOOL sw) {  }	// applies to mapChannel 1
		virtual void SetGenUVW (int mapChannel, BOOL sw) { if (mapChannel==1) SetGenUVW (sw); }

		// Notify the BaseObject that the end result display has been switched.
		// (Sometimes this is needed for display changes.)
		virtual void ShowEndResultChanged (BOOL showEndResult) { }

		//
		//
		///////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////

		// This method is called before a modifier or object is collapsed. In case it
		// is a modifier, the derObj contains the DerivedObject and the index the 
		// index of the modifier in the DerivedObject. In case it is a Object, derObj
		// is NULL and index is 0.
		virtual void NotifyPreCollapse(INode *node, IDerivedObject *derObj, int index){};

		// This method is called before a modifier or object is collapsed. In case it
		// is a modifier, the derObj contains the DerivedObject and the index the 
		// index of the modifier in the DerivedObject. In case it is a Object, derObj
		// is NULL and index is 0.
		virtual void NotifyPostCollapse(INode *node, Object *obj, IDerivedObject *derObj, int index){};
		
		// New in R4. objects and modifiers, that support subobjects have to overwrite 
		// these 2 methods and return a class derived from ISubObjType in GetSubObjType(). 
		// Developers can use the GenSubObjType for convenience.If the parameter passed 
		// into GetSubObjType is -1, the system requests a ISubObjType, for the current
		// SubObjectLevel that flows up the modifier stack. If the subobject selection 
		// of the modifier or base object does not affect the subobj selection that 
		// flows up the stack, the method must return NULL. See meshsel.cpp for a 
		// sample implementation

		virtual int NumSubObjTypes(){ return 0;}
		virtual ISubObjType *GetSubObjType(int i) { return NULL; }
		
		// This method returns the subobject level, that the modifier or base 
		// object is in. The subObjLevel is set by the system. 0 is the object level 
		// and 1 - n are the subobject levels in the same order as they are
		// returned by GetSubObjType(int i) (with an offset of 1 obviously).

		CoreExport virtual int GetSubObjectLevel();
	};

//-------------------------------------------------------------
// Callback object used by Modifiers to deform "Deformable" objects
class Deformer {
	public:
		virtual Point3 Map(int i, Point3 p) = 0; 
		void ApplyToTM(Matrix3* tm);
	};

// Mapping types passed to ApplyUVWMap()
#define MAP_PLANAR              0
#define MAP_CYLINDRICAL 1
#define MAP_SPHERICAL   2
#define MAP_BALL                3
#define MAP_BOX                 4

/*------------------------------------------------------------------- 
   Object is the class of all objects that can be pointed to by a node:
   It INcludes Lights,Cameras, Geometric objects, derived objects,
   and deformation Objects (e.g. FFD lattices)
   It EXcludes Modifiers
---------------------------------------------------------------------*/
#ifdef _WIN64
#define OBJECT_LOCKED 0x0800000000000000
#else // _WIN32 only
// 32 bit pointers
#define OBJECT_LOCKED 0x08000000
#endif
	// WIN64 Cleanup: Shuler

class ShapeObject;
class XTCObject;

class XTCContainer {

public:	
	XTCObject *obj;
	int prio;
	int branchID;
	XTCContainer(){obj = NULL; prio = 0; branchID = -1;}
	
};

#define IXTCACCESS_INTERFACE_ID Interface_ID(0x60b033d7, 0x3e1d4d0d)

class IXTCAccess : public BaseInterface
{
public:
	virtual Interface_ID	GetID() { return IXTCACCESS_INTERFACE_ID; }
	virtual LifetimeType	LifetimeControl() { return noRelease; }
	virtual void AddXTCObject(XTCObject *pObj, int priority = 0, int branchID = -1)=0;
	virtual int NumXTCObjects()=0;
	virtual XTCObject *GetXTCObject(int index)=0;
	virtual void RemoveXTCObject(int index)=0;
	virtual void SetXTCObjectPriority(int index,int priority)=0;
	virtual int GetXTCObjectPriority(int index)=0;
	virtual void SetXTCObjectBranchID(int index,int branchID)=0;
	virtual int GetXTCObjectBranchID(int index)=0;
	virtual void MergeAdditionalChannels(Object *from, int branchID)=0;
	virtual void BranchDeleted(int branchID, bool reorderChannels)=0;	
	virtual void CopyAdditionalChannels(Object *from, bool deleteOld = true, bool bShallowCopy = false)=0;
	virtual void DeleteAllAdditionalChannels()=0;
};

class XTCAccessImp;

class Object: public BaseObject {
		ChannelMask locked;   // lock flags for each channel + object locked flag
			// WIN64 Cleanup: Shuler
		Interval noEvalInterval;  // used in ReducingCaches
		Interval xtcValid;

		Tab<XTCContainer *> xObjs;
		XTCAccessImp *pXTCAccess;
	public:
		CoreExport Object();
		CoreExport ~Object();
		virtual int IsRenderable()=0;  // is this a renderable object?
		virtual void InitNodeName(TSTR& s)=0;
		virtual int UsesWireColor() { return TRUE; }    // TRUE if the object color is used for display
		virtual int DoOwnSelectHilite() { return 0; }
		
		// This used to be in GeomObject but I realized that other types of objects may
		// want this (mainly to participate in normal align) such as grid helper objects.
		virtual int IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm) {return FALSE;}

		// Objects that don't support IntersectRay() (like helpers) can implement this
		// method to provide a default vector for normal align.
		virtual BOOL NormalAlignVector(TimeValue t,Point3 &pt, Point3 &norm) {return FALSE;}

		// locking of object as whole. defaults to NOT modifiable.
		void LockObject() { locked |= OBJECT_LOCKED; }
		void UnlockObject() { locked &= ~OBJECT_LOCKED; }
		int  IsObjectLocked() { return (locked&OBJECT_LOCKED ? 1 : 0); }
			// WIN64 Cleanup: Shuler

		// the validity intervals are now in the object.
		virtual ObjectState Eval(TimeValue t)=0;

		// Access the lock flags for th specified channels
		void LockChannels(ChannelMask channels) { locked |= channels; } 
		void UnlockChannels(ChannelMask channels) { locked &= ~channels; }
		ChannelMask     GetChannelLocks() { return locked; }    
		void SetChannelLocks(ChannelMask channels) { locked = channels; }       
		ChannelMask GetChannelLocks(ChannelMask m) { return locked; }
		
		// Can this object have channels cached?
		// Particle objects flow up the pipline without making shallow copies of themselves and therefore cannot be cached
		virtual BOOL CanCacheObject() {return TRUE;}

		// This is called by a node when the node's world space state has
		// become invalid. Normally an object does not (and should not) be
		// concerned with this, but in certain cases (particle systems) an
		// object is effectively a world space object an needs to be notified.
		virtual void WSStateInvalidate() {}

		// Identifies the object as a world space object. World space
		// objects (particles for example) can not be instanced because
		// they exist in world space not object space.
		virtual BOOL IsWorldSpaceObject() {return FALSE;}
		
		// This is only valid for world-space objects (they must return TRUE for
		// the IsWorldSpaceObject method).  It locates the node which contains the
		// object.  Non-world-space objects will return NULL for this!
		CoreExport INode *GetWorldSpaceObjectNode();

		// Is the derived class derived from ParticleObject?
		virtual BOOL IsParticleSystem() {return FALSE;}

		// copy specified flags from obj
		CoreExport void CopyChannelLocks(Object *obj, ChannelMask needChannels);

		// topology has been changed by a modifier -- update mesh strip/edge lists
		virtual void TopologyChanged() { }

		//
		// does this object implement the generic Deformable Object procs?
		//
		virtual int IsDeformable() { return 0; } 

		// DeformableObject procs: only need be implemented  
		// IsDeformable() returns TRUE.
		virtual int NumPoints(){ return 0;}
		virtual Point3 GetPoint(int i) { return Point3(0,0,0); }
		virtual void SetPoint(int i, const Point3& p) {}             
			
		// Completes the deformable object access with two methods to
		// query point selection. 
		// IsPointSelected returns a TRUE/FALSE value
		// PointSelection returns the weighted point selection, if supported.
		// Harry D, 11/98
		virtual BOOL IsPointSelected (int i) { return FALSE; }
		virtual float PointSelection (int i) {
			return IsPointSelected(i) ? 1.0f : 0.0f;
		}

		// These allow the NURBS Relational weights to be modified
		virtual BOOL HasWeights() { return FALSE; }
		virtual double GetWeight(int i) { return 1.0; }
		virtual void SetWeight(int i, const double w) {}

        // Get the count of faces and vertices for the polyginal mesh
        // of an object.  If it return FALSE, then this function
        // isn't supported.  Plug-ins should use GetPolygonCount(Object*, int&, int&)
        // to count the polys in an arbitrary object
        virtual BOOL PolygonCount(TimeValue t, int& numFaces, int& numVerts) { return FALSE; }

		// informs the object that its points have been deformed,
		// so it can invalidate its cache.
		virtual void PointsWereChanged(){}

		// deform the object with a deformer.
		CoreExport virtual void Deform(Deformer *defProc, int useSel=0);

		// box in objects local coords or optional space defined by tm
		// If useSel is true, the bounding box of selected sub-elements will be taken.
		CoreExport virtual void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm=NULL, BOOL useSel=FALSE );

		//
		// does this object implement the generic Mappable Object procs?
		//
		virtual int IsMappable() { return 0; }
		virtual int NumMapChannels () { return IsMappable(); }	// returns number possible.
		virtual int NumMapsUsed () { return NumMapChannels(); }	// at least 1+(highest channel in use).

		// This does the texture map application -- Only need to implement if
		// IsMappable returns TRUE
		virtual void ApplyUVWMap(int type,
			float utile, float vtile, float wtile,
			int uflip, int vflip, int wflip, int cap,
			const Matrix3 &tm,int channel=1) {}

		// Objects need to be able convert themselves 
		// to TriObjects. Most modifiers will ask for
		// Deformable Objects, and triobjects will suffice.

		CoreExport virtual int CanConvertToType(Class_ID obtype);

		// Developers have to make sure, that the channels, that the BaseObject implements 
		// (e.g. ExtensionChannels) are copied over to the new object as well. They can do this by simply
		// calling CopyXTCObjects(this,false); The validity will be automatically copied with it..
		CoreExport virtual Object* ConvertToType(TimeValue t, Class_ID obtype);
		
		// Indicate the types this object can collapse to
		virtual Class_ID PreferredCollapseType() {return Class_ID(0,0);}
		CoreExport virtual void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);
		
		virtual Object *CollapseObject() { return this;}
		
		// return the current sub-selection state
		virtual DWORD GetSubselState() {return 0;} 
		virtual void SetSubSelState(DWORD s) {}

		// If the requested channels are locked, replace their data
		// with a copy/ and unlock them, otherwise leave them alone
		CoreExport void ReadyChannelsForMod(ChannelMask channels);

		// Virtual methods to be implemented by plug-in object:-----
		
		// NS: 12-14-99 Classes that derive from Object *have* to implement certain
		// things to support the new Extension Channel (XTCObject)
		// See examples in Triobj.cpp [START]

		// access the current validity interval for the nth channel
		// For this method, the derived class has to check if the channel is the 
		// Extension channel and return the base classes ChannelValidity :
		// case EXTENSION_CHAN_NUM: return Object::ChannelValidity(t,nchan); break;
		CoreExport virtual Interval ChannelValidity(TimeValue t, int nchan);

		// The derived class has to simply call the implementation of the baseclass.
		CoreExport virtual void SetChannelValidity(int nchan, Interval v);

		// invalidate the specified channels
		// The derived class has to simply call the implementation of the baseclass.
		CoreExport virtual void InvalidateChannels(ChannelMask channels);

		// validity interval of Object as a whole at current time
		// The derived class has incorporate the BaseClasses validity into the returned validity
		CoreExport virtual Interval ObjectValidity(TimeValue t);

		// Makes a copy of its "shell" and shallow copies only the
		// specified channels.  Also copies the validity intervals of
		// the copied channels, and sets Invalidates the other intervals.
		// The derived class has to call ShallowCopy on the BaseClass, so it can copy all its channels
		virtual Object *MakeShallowCopy(ChannelMask channels) { return NULL; }

		// Shallow-copies the specified channels from the fromOb to this.
		// Also copies the validity intervals. 
		// The derived class has to simply call the implementation of the baseclass.
		CoreExport virtual void ShallowCopy(Object* fromOb, ChannelMask channels);

		// Free the specified channels
		// The derived class has to simply call the implementation of the baseclass.
		CoreExport virtual void FreeChannels(ChannelMask channels);                                  

		// This replaces locked channels with newly allocated copies.
		// It will only be called if the channel is locked.
		// The derived class has to simply call the implementation of the baseclass.
		CoreExport virtual void NewAndCopyChannels(ChannelMask channels);                
		
		// Allow the object to enlarge its viewport rectangle, if it wants to.
		// The derived class has to simply call the implementation of the baseclass.
		CoreExport virtual void MaybeEnlargeViewportRect(GraphicsWindow *gw, Rect &rect);



		// quick render in viewport, using current TM.
		//CoreExport virtual Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) { return 0; };   
		
		// NS: 12-14-99 Classes that derive from Object *have* o call the following methods
		// See examples in Triobj.cpp [END]
		
		CoreExport bool IsBaseClassOwnedChannel(int nchan) { return (nchan == EXTENSION_CHAN_NUM) ? true : false;}
		CoreExport void UpdateValidity(int nchan, Interval v);  // AND in interval v to channel validity

		Interval GetNoEvalInterval() { return noEvalInterval; }
		void SetNoEvalInterval(Interval iv) {noEvalInterval = iv; }

		// Give the object chance to reduce its caches, 
		// depending on the noEvalInterval.
		CoreExport virtual void ReduceCaches(TimeValue t);

		// Is this object a construction object:
		virtual int IsConstObject() { return 0; }               

		// Retreives sub-object branches from an object that supports branching.
		// Certain objects combine a series of input objects (pipelines) into
		// a single object. These objects act as a multiplexor allowing the
		// user to decide which branch(s) they want to see the history for.
		//
		// It is up to the object how they want to let the user choose. The object
		// may use sub object selection to allow the user to pick a set of
		// objects for which the common history will be displayed.
		// 
		// When the history changes for any reason, the object should send
		// a notification (REFMSG_BRANCHED_HISTORY_CHANGED) via NotifyDependents.
		
		// The selected parameter is new in Rel. 4 and must be supported by all
		// compound objects.
		// In case the selected parameter is true the obejct should only return
		// the number of pipebranches, that are currently selected in the UI (this
		// is the way it worked in R3 and before.
		// In case this parameter is false, the object has to return the number of 
		// *all* branches, no matter if they are selected or not

		virtual int NumPipeBranches(bool selected = true) {return 0;}

		// The selected parameter is new in Rel. 4 and must be supported by all
		// compound objects.
		// In case the selected parameter is true the obejct should only consider
		// the branches, that are currently selected in the UI (this
		// is the way it worked in R3 and before.
		// In case this parameter is false, the object has to consider 
		// *all* branches, no matter if they are selected or not

		virtual Object *GetPipeBranch(int i, bool selected = true) {return NULL;}
		
		// When an object has sub-object branches, it is likely that the
		// sub-objects are transformed relative to the object. This method
		// gives the object a chance to modify the node's transformation so
		// that operations (like edit modifiers) will work correctly when 
		// editing the history of the sub object branch.

		// The selected parameter is new in Rel. 4 and must be supported by all
		// compound objects.
		// In case the selected parameter is true the obejct should only consider
		// the branches, that are currently selected in the UI (this
		// is the way it worked in R3 and before.
		// In case this parameter is false, the object has to consider 
		// *all* branches, no matter if they are selected or not
		
		virtual INode *GetBranchINode(TimeValue t,INode *node,int i, bool selected = true) {return node;}

		// Shape viewports can reference shapes contained within objects, so we
		// need to be able to access shapes within an object.  The following methods
		// provide this access
		virtual int NumberOfContainedShapes() { return -1; }    // NOT a container!
		virtual ShapeObject *GetContainedShape(TimeValue t, int index) { return NULL; }
		virtual void GetContainedShapeMatrix(TimeValue t, int index, Matrix3 &mat) {}
		virtual BitArray ContainedShapeSelectionArray() { return BitArray(); }

        // Return TRUE for ShapeObject class or GeomObjects that are Shapes too
        virtual BOOL IsShapeObject() { return FALSE; }
    
		// For debugging only. TriObject inplements this method by making sure
		// its face's vert indices are all valid.
		virtual BOOL CheckObjectIntegrity() {return TRUE;}              

		// Find out if the Object is generating UVW's
		virtual BOOL HasUVW() { return 0; }
		// or on any map channel:
		virtual BOOL HasUVW (int mapChannel) { return (mapChannel==1) ? HasUVW() : FALSE; }

		// This is overridden by DerivedObjects to search up the pipe for the base object
		virtual Object *FindBaseObject() { return this;	}

		// Access a parametric position on the surface of the object
		virtual BOOL IsParamSurface() {return FALSE;}
		virtual int NumSurfaces(TimeValue t) {return 1;}
		// Single-surface version (surface 0)
		virtual Point3 GetSurfacePoint(TimeValue t, float u, float v,Interval &iv) {return Point3(0,0,0);}
		// Multiple-surface version (Implement if you override NumSurfaces)
		virtual Point3 GetSurfacePoint(TimeValue t, int surface, float u, float v,Interval &iv) {return Point3(0,0,0);}
		// Get information on whether a surface is closed (default is closed both ways)
		virtual void SurfaceClosed(TimeValue t, int surface, BOOL &uClosed, BOOL &vClosed) {uClosed = vClosed = TRUE;}

		// Allow an object to return extended Properties fields
		// Return TRUE if you take advantage of these, and fill in all strings
		virtual BOOL GetExtendedProperties(TimeValue t, TSTR &prop1Label, TSTR &prop1Data, TSTR &prop2Label, TSTR &prop2Data) {return FALSE;}

		// Animatable Overides...
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
		CoreExport bool SvHandleDoubleClick(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited);
		CoreExport COLORREF SvHighlightColor(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvIsSelected(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport MultiSelectCallback* SvGetMultiSelectCallback(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvCanSelect(IGraphObjectManager *gom, IGraphNode *gNode);

		//ExtensionChannel Access :
		
		// Adds an extension object into the pipeline. The methods (Display,
		// PreChanChangedNotify etc) of higher priority XTCObjects will becalled 
		// before those of lower priority XTCObjects
		CoreExport void AddXTCObject(XTCObject *pObj, int priority = 0, int branchID = -1);
		CoreExport int NumXTCObjects();
		CoreExport XTCObject *GetXTCObject(int index);
		CoreExport void RemoveXTCObject(int index);
		CoreExport void SetXTCObjectPriority(int index,int priority);
		CoreExport int GetXTCObjectPriority(int index);
		CoreExport void SetXTCObjectBranchID(int index,int branchID);
		CoreExport int GetXTCObjectBranchID(int index);
		
		// This method has to be called whenever the CompoundObject updates a branch 
		// (calling Eval on it). Object *from is the object returned from Eval 
		// (os.obj);branchID is an int, that specifies that branch. The extension 
		// channel will get a callback to RemoveXTCObjectOnMergeBranches and MergeXTCObject. 
		// By default it returns true to RemoveXTCObjectOnMergeBranches, which means,
		// that the existing XTCObjects with that branchID will be deleted. The method 
		// MergeXTCObject simply copies the XTCObjects from the incoming branch into the 
		// compound object.

		CoreExport void MergeAdditionalChannels(Object *from, int branchID);
		
		// This method has to be called on the CompoundObject, so it can delete the 
		// XTCObjects for the specific branch. The XTCObject will have again the final 
		// decision if the XTCObject gets really deleted or not in a callback to 
		// RemoveXTCObjectOnBranchDeleted(), which will return true, if the XTCOject 
		// should be removed.

		CoreExport void BranchDeleted(int branchID, bool reorderChannels);
		
		// This method copies all extension objects from the "from" objects into the 
		// current object. In case deleteOld is false, the objects will be appended. 
		// In case it is true, the old XTCObjects will be deleted.
		CoreExport void CopyAdditionalChannels(Object *from, bool deleteOld = true, bool bShallowCopy = false);
		CoreExport void DeleteAllAdditionalChannels();

        // If this return FALSE, then no selection brackets are displayed on the object
        virtual BOOL UseSelectionBrackets() { return TRUE; }
        // returns TRUE for manipulator objcts and FALSE for all others
        virtual BOOL IsManipulator() { return FALSE; }

		void* GetInterface(ULONG id) { return BaseObject::GetInterface(id);}
		CoreExport BaseInterface* GetInterface(Interface_ID id);

	};


// This function should be used to count polygons in an object.
// It uses Object::PolygonCount() if it is supported, and converts to
// a TriObject and counts faces and vertices otherwise.
CoreExport void GetPolygonCount(TimeValue t, Object* pObj, int& numFaces, int& numVerts);


// mjm - begin - 07.17.00
class  CameraObject;

// -------------------------------
// multi-pass render camera effect
// -------------------------------
class IMultiPassCameraEffect : public ReferenceTarget
{
public:
	// allows effect to declare it's compatibility with the current camera object
	virtual bool IsCompatible(CameraObject *pCameraObject) = 0;
	// indicates that the renderer should display each pass as it is rendered (not used by viewport renderer)
	virtual bool DisplayPasses(TimeValue renderTime) = 0;
	// indicates the total number of passes to be rendered
	virtual int TotalPasses(TimeValue renderTime) = 0;
	// called for each render pass. the effect can alter the camera node, camera object, or override the render time
	virtual ViewParams *Apply(INode *pCameraNode, CameraObject *pCameraObject, int passNum, TimeValue &overrideRenderTime) = 0;
	// allows the effect to blend its own passes (not used by the viewport renderer)
	virtual void AccumulateBitmap(Bitmap *pDest, Bitmap *pSrc, int passNum, TimeValue renderTime) = 0;
	// convenience function, called after all passes have been rendered. can be ignored.
	virtual void PostRenderFrame() = 0;

	// from class ReferenceMaker
	virtual RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message) { return REF_SUCCEED; }

	// from class Animatable
	virtual SClass_ID SuperClassID() { return MPASS_CAM_EFFECT_CLASS_ID; }
	virtual void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev=NULL) {}
	virtual void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next=NULL) {}
};
// mjm - end


// ------------
// CameraObject  
// ------------
#define CAM_HITHER_CLIP         1
#define CAM_YON_CLIP            2

#define ENV_NEAR_RANGE          0
#define ENV_FAR_RANGE           1

struct CameraState {
	inline CameraState() {cbStruct = sizeof(CameraState);}
    DWORD cbStruct;
	BOOL isOrtho;	// true if cam is ortho, false for persp
	float fov;      // field-of-view for persp cams, width for ortho cams
	float tdist;    // target distance for free cameras
	BOOL horzLine;  // horizon line display state
	int manualClip;
	float hither;
	float yon;
	float nearRange;
	float farRange;
	};

class  CameraObject : public Object
{
public:
	SClass_ID SuperClassID() { return CAMERA_CLASS_ID; }
	int IsRenderable() { return(0);}
	virtual void InitNodeName(TSTR& s) { s = _T("Camera"); }
	virtual int UsesWireColor() { return FALSE; } // TRUE if the object color is used for display
	
	// Method specific to cameras:
	virtual RefResult EvalCameraState(TimeValue time, Interval& valid, CameraState* cs)=0;
	virtual void SetOrtho(BOOL b)=0;
	virtual BOOL IsOrtho()=0;
	virtual void SetFOV(TimeValue time, float f)=0; 
	virtual float GetFOV(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetTDist(TimeValue time, float f)=0; 
	virtual float GetTDist(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual int GetManualClip()=0;
	virtual void SetManualClip(int onOff)=0;
	virtual float GetClipDist(TimeValue t, int which, Interval &valid=Interval(0,0))=0;
	virtual void SetClipDist(TimeValue t, int which, float val)=0;
	virtual void SetEnvRange(TimeValue time, int which, float f)=0; 
	virtual float GetEnvRange(TimeValue t, int which, Interval& valid = Interval(0,0))=0;
	virtual void SetEnvDisplay(BOOL b, int notify=TRUE)=0;
	virtual BOOL GetEnvDisplay(void)=0;
	virtual void RenderApertureChanged(TimeValue t)=0;
	virtual void UpdateTargDistance(TimeValue t, INode* inode) { }
// mjm - begin - 07.17.00
	virtual void SetMultiPassEffectEnabled(TimeValue t, BOOL enabled) { }
	virtual BOOL GetMultiPassEffectEnabled(TimeValue t, Interval& valid) { return FALSE; }
	virtual void SetMPEffect_REffectPerPass(BOOL enabled) { }
	virtual BOOL GetMPEffect_REffectPerPass() { return FALSE; }
	virtual void SetIMultiPassCameraEffect(IMultiPassCameraEffect *pIMultiPassCameraEffect) { }
	virtual IMultiPassCameraEffect *GetIMultiPassCameraEffect() { return NULL; }
// mjm - end
};


/*------------------------------------------------------------------- 
  LightObject:   
---------------------------------------------------------------------*/

#define LIGHT_ATTEN_START       0
#define LIGHT_ATTEN_END         1

struct LightState {
    LightType type;
	Matrix3 tm;
	Color color;
	float 	intens;  // multiplier value
	float 	hotsize; 
	float 	fallsize;
	int		useNearAtten;
	float	nearAttenStart;
	float	nearAttenEnd;
	int   	useAtten;
	float 	attenStart;
	float 	attenEnd;
	int   	shape;
	float 	aspect;
		BOOL   	overshoot;
	BOOL   	shadow;
    BOOL 	on;      // light is on
	BOOL	affectDiffuse;
	BOOL	affectSpecular;
	BOOL 	ambientOnly;  // affect only ambient component
	DWORD   extra;
	};

class LightDesc;
class RendContext;


// This is a callback class that can be given to a ObjLightDesc
// to have a ray traced through the light volume.
class LightRayTraversal {
	public:
		// This is called for every step (return FALSE to halt the integration).
		// t0 and t1 define the segment in terms of the given ray.
		// illum is the light intensity over the entire segment. It can be
		// assumed that the light intensty is constant for the segment.
		virtual BOOL Step(float t0, float t1, Color illum, float distAtten)=0;
	};

// Flags passed to TraverseVolume
#define TRAVERSE_LOWFILTSHADOWS (1<<0)
#define TRAVERSE_HIFILTSHADOWS  (1<<1)
#define TRAVERSE_USESAMPLESIZE	(1<<2)

// A light must be able to create one of these to give to the renderer.
// The Illuminate() method (inherited from LightDesc) is called by the renderer
// to illuminate a surface point.
class ObjLightDesc : public LightDesc {
	public:         
		// This data will be set up by the default implementation of Update()
		LightState ls;
		INode *inode;
		BOOL uniformScale; // for optimizing
		Point3 lightPos;
		Matrix3 lightToWorld;
		Matrix3 worldToLight;
		Matrix3 lightToCam;   // updated in UpdateViewDepParams
		Matrix3 camToLight;   // updated in UpdateViewDepParams
		int renderNumber;   // set by the renderer. Used in RenderInstance::CastsShadowsFrom()

		CoreExport ObjLightDesc(INode *n);
		CoreExport virtual ~ObjLightDesc();

		virtual ExclList* GetExclList() { return NULL; }  

		// update light state that depends on position of objects&lights in world.
		CoreExport virtual int Update(TimeValue t, const RendContext &rc, RenderGlobalContext *rgc, BOOL shadows, BOOL shadowGeomChanged);

		// update light state that depends on global light level.
		CoreExport virtual void UpdateGlobalLightLevel(Color globLightLevel) {}

		// update light state that depends on view matrix.
		CoreExport virtual int UpdateViewDepParams(const Matrix3& worldToCam);

		// default implementation 
		CoreExport virtual Point3 LightPosition() { return lightPos; }
		
		// This function traverses a ray through the light volume.
		// 'ray' defines the parameter line that will be traversed.
		// 'minStep' is the smallest step size that caller requires, Note that
		// the callback may be called in smaller steps if they light needs to
		// take smaller steps to avoid under sampling the volume.
		// 'tStop' is the point at which the traversal will stop (ray.p+tStop*ray.dir).
		// Note that the traversal can terminate earlier if the callback returns FALSE.
		// 'proc' is the callback object.
		//
		// attenStart/End specify a percent of the light attenuation distances
		// that should be used for lighting durring the traversal.
		//
		// The shade context passed in should only be used for state (like are
		// shadows globaly disabled). The position, normal, etc. serve no purpose.
		virtual void TraverseVolume(
			ShadeContext& sc,       
			const Ray &ray, int samples, float tStop,
			float attenStart, float attenEnd,
			DWORD flags,
			LightRayTraversal *proc) {}
	};

// Values returned from GetShadowMethod()
#define LIGHTSHADOW_NONE                0
#define LIGHTSHADOW_MAPPED              1
#define LIGHTSHADOW_RAYTRACED   2


class  LightObject: public Object {
	public:
	SClass_ID SuperClassID() { return LIGHT_CLASS_ID; }
	int IsRenderable() { return(0);}
	virtual void InitNodeName(TSTR& s) { s = _T("Light"); }

	// Methods specific to Lights:
	virtual RefResult EvalLightState(TimeValue time, Interval& valid, LightState *ls)=0;
	virtual ObjLightDesc *CreateLightDesc(INode *n, BOOL forceShadowBuffer=FALSE) {return NULL;}
	virtual void SetUseLight(int onOff)=0;
	virtual BOOL GetUseLight(void)=0;
	virtual void SetHotspot(TimeValue time, float f)=0; 
	virtual float GetHotspot(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetFallsize(TimeValue time, float f)=0; 
	virtual float GetFallsize(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetAtten(TimeValue time, int which, float f)=0; 
	virtual float GetAtten(TimeValue t, int which, Interval& valid = Interval(0,0))=0;
	virtual void SetTDist(TimeValue time, float f)=0; 
	virtual float GetTDist(TimeValue t, Interval& valid = Interval(0,0))=0;
	virtual void SetConeDisplay(int s, int notify=TRUE)=0;
	virtual BOOL GetConeDisplay(void)=0;
	virtual int GetShadowMethod() {return LIGHTSHADOW_NONE;}
	virtual void SetRGBColor(TimeValue t, Point3& rgb) {}
	virtual Point3 GetRGBColor(TimeValue t, Interval &valid = Interval(0,0)) {return Point3(0,0,0);}        
	virtual void SetIntensity(TimeValue time, float f) {}
	virtual float GetIntensity(TimeValue t, Interval& valid = Interval(0,0)) {return 0.0f;}
	virtual void SetAspect(TimeValue t, float f) {}
	virtual float GetAspect(TimeValue t, Interval& valid = Interval(0,0)) {return 0.0f;}    
	virtual void SetUseAtten(int s) {}
	virtual BOOL GetUseAtten(void) {return FALSE;}
	virtual void SetAttenDisplay(int s) {}
	virtual BOOL GetAttenDisplay(void) {return FALSE;}      
	virtual void Enable(int enab) {}
	virtual void SetMapBias(TimeValue t, float f) {}
	virtual float GetMapBias(TimeValue t, Interval& valid = Interval(0,0)) {return 0.0f;}
	virtual void SetMapRange(TimeValue t, float f) {}
	virtual float GetMapRange(TimeValue t, Interval& valid = Interval(0,0)) {return 0.0f;}
	virtual void SetMapSize(TimeValue t, int f) {}
	virtual int GetMapSize(TimeValue t, Interval& valid = Interval(0,0)) {return 0;}
	virtual void SetRayBias(TimeValue t, float f) {}
	virtual float GetRayBias(TimeValue t, Interval& valid = Interval(0,0)) {return 0.0f;}
	virtual int GetUseGlobal() {return 0;}
	virtual void SetUseGlobal(int a) {}
	virtual int GetShadow() {return 0;}
	virtual void SetShadow(int a) {}
	virtual int GetShadowType() {return 0;}
	virtual void SetShadowType(int a) {}
	virtual int GetAbsMapBias() {return 0;}
	virtual void SetAbsMapBias(int a) {}
	virtual int GetOvershoot() {return 0;}
	virtual void SetOvershoot(int a) {}
	virtual int GetProjector() {return 0;}
	virtual void SetProjector(int a) {}
	virtual ExclList* GetExclList() {return NULL;}
	virtual BOOL Include() {return FALSE;}
	virtual Texmap* GetProjMap() {return NULL;}
	virtual void SetProjMap(Texmap* pmap) {}
	virtual void UpdateTargDistance(TimeValue t, INode* inode) {}
	};

/*------------------------------------------------------------------- 
  HelperObject:
---------------------------------------------------------------------*/

class  HelperObject: public Object {
	public:
	SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
	int IsRenderable() { return(0); }
	virtual void InitNodeName(TSTR& s) { s = _T("Helper"); }
	virtual int UsesWireColor() { return FALSE; }   // TRUE if the object color is used for display
	virtual BOOL NormalAlignVector(TimeValue t,Point3 &pt, Point3 &norm) {pt=Point3(0,0,0);norm=Point3(0,0,-1);return TRUE;}
	};

/*------------------------------------------------------------------- 
  ConstObject:
---------------------------------------------------------------------*/

#define GRID_PLANE_NONE		-1
#define GRID_PLANE_TOP		0
#define GRID_PLANE_LEFT		1
#define GRID_PLANE_FRONT	2
#define GRID_PLANE_BOTTOM	3
#define GRID_PLANE_RIGHT	4
#define GRID_PLANE_BACK		5

class  ConstObject: public HelperObject {
	private:
		bool m_transient;
	public:
		ConstObject():m_transient(false){};
	
	// Override this function in HelperObject!
	int IsConstObject() { return 1; }

	// Methods specific to construction grids:
	virtual void GetConstructionTM( TimeValue t, INode* inode, ViewExp *vpt, Matrix3 &tm ) = 0;     // Get the transform for this view
	virtual void SetConstructionPlane(int which, int notify=TRUE) = 0;
	virtual int  GetConstructionPlane(void) = 0;
	virtual Point3 GetSnaps( TimeValue t ) = 0;    // Get snap values
	virtual void SetSnaps(TimeValue t, Point3 p) = 0;

	virtual BOOL NormalAlignVector(TimeValue t,Point3 &pt, Point3 &norm) {pt=Point3(0,0,0);norm=Point3(0,0,-1);return TRUE;}

	//JH 09/28/98 for design ver
	bool IsTransient()const {return m_transient;}
	void SetTransient(bool state = true) {m_transient = state;}

	//JH 11/16/98
	virtual void SetExtents(TimeValue t, Point3 halfbox)=0;
	virtual Point3 GetExtents(TimeValue t)=0;
	//JH 09/28/98 for design ver
//	bool IsImplicit()const {return m_implicit;}
//	void SetImplicit(bool state = true) {m_implicit = state;}

	};

/*------------------------------------------------------------------- 
  GeomObject: these are the Renderable objects.  
---------------------------------------------------------------------*/

class  GeomObject: public Object {
	public:         
		virtual void InitNodeName(TSTR& s) { s = _T("Object"); }
		SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }
		
		virtual int IsRenderable() { return(1); }               

		// If an object creates different  meshes depending on the 
		// particular instance (view-dependent) it should return 1.
		virtual int IsInstanceDependent() { return 0; }

		// GetRenderMesh should be implemented by all renderable GeomObjects.
		// set needDelete to TRUE if the render should delete the mesh, FALSE otherwise
		// Primitives that already have a mesh cached can just return a pointer
		// to it (and set needDelete = FALSE).
		CoreExport virtual Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);

		// Objects may now supply multiple render meshes ( e.g. particle systems). If this function
		// returns a positive number, then GetMultipleRenderMesh and GetMultipleRenderMeshTM will be 
		// called for each mesh, instead of calling GetRenderMesh // DS 5/10/00
		virtual int NumberOfRenderMeshes() { return 0; } // 0 indicates multiple meshes not supported.

		// For multiple render meshes, this method must be implemented. 
		// set needDelete to TRUE if the render should delete the mesh, FALSE otherwise
		// meshNumber specifies which of the multiplie meshes is being asked for.// DS 5/10/00
	    virtual	Mesh* GetMultipleRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete, int meshNumber) { return NULL; }

		// For multiple render meshes, this method must be implemented. 
		// meshTM should be returned with the transform defining the offset of the particular mesh in object space.
		// meshTMValid should contain the validity interval of meshTM // DS 5/10/00
		virtual void GetMultipleRenderMeshTM(TimeValue t, INode *inode, View& view, int meshNumber, 
			Matrix3& meshTM, Interval& meshTMValid) { return;  }

		// If this returns NULL, then GetRenderMesh will be called
		CoreExport virtual PatchMesh* GetRenderPatchMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);

		CoreExport Class_ID PreferredCollapseType();

		virtual BOOL CanDoDisplacementMapping() { return 0; }

	private:
	};


//-- Particle Systems ---------------------------------------------------

// A force field can be applied to a particle system by a SpaceWarp.
// The force field provides a function of position in space, velocity
// and time that gives a force.
// The force is then used to compute an acceleration on a particle
// which modifies its velocity. Typically, particles are assumed to
// to have a normalized uniform mass==1 so the acceleration is F/M = F.
class ForceField {
	public:
		virtual Point3 Force(TimeValue t,const Point3 &pos, const Point3 &vel, int index)=0;
		virtual void DeleteThis() {}
	};

// A collision object can be applied to a particle system by a SpaceWarp.
// The collision object checks a particle's position and velocity and
// determines if the particle will colide with it in the next dt amount of
// time. If so, it modifies the position and velocity.
class CollisionObject : public InterfaceServer {
	public:
		// Check for collision. Return TRUE if there was a collision and the position and velocity have been modified.
		virtual BOOL CheckCollision(TimeValue t,Point3 &pos, Point3 &vel, float dt,int index, float *ct=NULL, BOOL UpdatePastCollide=TRUE)=0;
		virtual Object *GetSWObject()=0;
		virtual void DeleteThis() {}
	};


// Values returned from ParticleCenter()
#define PARTCENTER_HEAD		1  // Particle geometry lies behind the particle position
#define PARTCENTER_CENTER	2  // Particle geometry is centered around particle position
#define PARTCENTER_TAIL		3  // Particle geometry lies in front of the particle position

// The particle system class derived from GeomObject and still has
// GEOMOBJECT_CLASS_ID as its super class.
//
// Given an object, to determine if it is a ParticleObject, call
// GetInterface() with the ID  I_PARTICLEOBJ or use the macro
// GetParticleInterface(anim) which returns a ParticleObject* or NULL.
class ParticleObject: public GeomObject {
	public:
		BOOL IsParticleSystem() {return TRUE;}

		virtual void ApplyForceField(ForceField *ff)=0;
		virtual BOOL ApplyCollisionObject(CollisionObject *co)=0; // a particle can choose no to support this and return FALSE

		// A particle object IS deformable, but does not let itself be
		// deformed using the usual GetPoint/SetPoint methods. Instead
		// a space warp must apply a force field to deform the particle system.
		int IsDeformable() {return TRUE;} 

		// Particle objects don't actually do a shallow copy and therefore 
		// cannot be cached.
		BOOL CanCacheObject() {return FALSE;}


		virtual BOOL NormalAlignVector(TimeValue t,Point3 &pt, Point3 &norm) {pt=Point3(0,0,0);norm=Point3(0,0,-1);return TRUE;}


		// These methods provide information about individual particles
		virtual Point3 ParticlePosition(TimeValue t,int i) {return Point3(0,0,0);}
		virtual Point3 ParticleVelocity(TimeValue t,int i) {return Point3(0,0,0);}
		virtual float ParticleSize(TimeValue t,int i) {return 0.0f;}
		virtual int ParticleCenter(TimeValue t,int i) {return PARTCENTER_CENTER;}
		virtual TimeValue ParticleAge(TimeValue t, int i) {return -1;}
		virtual TimeValue ParticleLife(TimeValue t, int i) {return -1;}

		// This tells the renderer if the ParticleObject's topology doesn't change over time
		// so it can do better motion blur.  This means the correspondence of vertex id to
		// geometrical vertex must be invariant.
		virtual BOOL HasConstantTopology() { return FALSE; }
	};

//----------------------------------------------------------------------


/*------------------------------------------------------------------- 
  ShapeObject: these are the open or closed hierarchical shape objects.  
---------------------------------------------------------------------*/

class PolyShape;
class BezierShape;
class MeshCapInfo;
class PatchCapInfo;
class ShapeHierarchy;

// This class may be requested in the pipeline via the GENERIC_SHAPE_CLASS_ID,
// also set up in the Class_ID object genericShapeClassID

// Options for steps in MakePolyShape (>=0: Use fixed steps)
#define PSHAPE_BUILTIN_STEPS -2         // Use the shape's built-in steps/adaptive settings (default)
#define PSHAPE_ADAPTIVE_STEPS -1        // Force adaptive steps

// Parameter types for shape interpolation (Must match types in spline3d.h & polyshp.h)
#define PARAM_SIMPLE 0		// Parameter space based on segments
#define PARAM_NORMALIZED 1	// Parameter space normalized to curve length

// GenerateMesh Options
#define GENMESH_DEFAULT -1	// Use whatever is stored in the ShapeObject's UseViewport flag
#define GENMESH_VIEWPORT 0
#define GENMESH_RENDER 1

// Defines for number of ShapeObject references/subanims
#define SHAPE_OBJ_NUM_REFS 1
#define SHAPE_OBJ_NUM_SUBS 1

class IParamBlock;
class IParamMap;

class ShapeObject: public GeomObject {
		friend class SObjRenderingDlgProc;
		IObjParam *ip;
		IParamBlock *sopblock;	// New for r4, renderable version parameter block
		static IParamMap *pmap;
		int loadVersion;
		// Display mesh cache stuff
		Mesh meshCache;
		Interval cacheValid;
		int cacheType;
	public:
		CoreExport ShapeObject();
		CoreExport ~ShapeObject();	// Must call this on destruction

        virtual BOOL IsShapeObject() { return TRUE; }

		virtual int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm) {return FALSE;}
		virtual void InitNodeName(TSTR& s) { s = _T("Shape"); }
		SClass_ID SuperClassID() { return SHAPE_CLASS_ID; }
		CoreExport virtual int IsRenderable();
		CoreExport virtual void CopyBaseData(ShapeObject &from);
		// Access methods
		CoreExport float GetThickness(TimeValue t, Interval &ivalid);
		CoreExport int GetSides(TimeValue t, Interval &ivalid);
		CoreExport float GetAngle(TimeValue t, Interval &ivalid);
		CoreExport float GetViewportThickness();
		CoreExport int GetViewportSides();
		CoreExport float GetViewportAngle();
		CoreExport BOOL GetRenderable();
		CoreExport BOOL GetGenUVs();
		CoreExport BOOL GetDispRenderMesh();
		CoreExport BOOL GetUseViewport();
		CoreExport BOOL GetViewportOrRenderer();
		CoreExport void SetThickness(TimeValue t, float thick);
		CoreExport void SetSides(TimeValue t, int s);
		CoreExport void SetAngle(TimeValue t, float a);
		CoreExport void SetViewportThickness(float thick);
		CoreExport void SetViewportSides(int s);
		CoreExport void SetViewportAngle(float a);
		CoreExport void SetRenderable(BOOL sw);
		CoreExport void SetGenUVs(BOOL sw);
		CoreExport void SetDispRenderMesh(BOOL sw);
		CoreExport void SetUseViewport(BOOL sw);
		CoreExport void SetViewportOrRenderer(BOOL sw);
		CoreExport virtual Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);
		CoreExport virtual void GetRenderMeshInfo(TimeValue t, INode *inode, View& view, int &nverts, int &nfaces);	// Get info on the rendering mesh
		CoreExport virtual void GenerateMesh(TimeValue t, int option, Mesh *mesh);
		virtual int NumberOfVertices(TimeValue t, int curve = -1) { return 0; }	// Informational only, curve = -1: total in all curves
		virtual int NumberOfCurves()=0;                 // Number of curve polygons in the shape
		virtual BOOL CurveClosed(TimeValue t, int curve)=0;     // Returns TRUE if the curve is closed
		virtual Point3 InterpCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE)=0;    // Interpolate from 0-1 on a curve
		virtual Point3 TangentCurve3D(TimeValue t, int curve, float param, int ptype=PARAM_SIMPLE)=0;   // Get tangent at point on a curve
		virtual float LengthOfCurve(TimeValue t, int curve)=0;  // Get the length of a curve
		virtual int NumberOfPieces(TimeValue t, int curve)=0;   // Number of sub-curves in a curve
		virtual Point3 InterpPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE)=0; // Interpolate from 0-1 on a sub-curve
		virtual Point3 TangentPiece3D(TimeValue t, int curve, int piece, float param, int ptype=PARAM_SIMPLE)=0;        // Get tangent on a sub-curve
		virtual MtlID GetMatID(TimeValue t, int curve, int piece) { return 0; }
		virtual BOOL CanMakeBezier() { return FALSE; }                  // Return TRUE if can turn into a bezier representation
		virtual void MakeBezier(TimeValue t, BezierShape &shape) {}     // Create the bezier representation
		virtual ShapeHierarchy &OrganizeCurves(TimeValue t, ShapeHierarchy *hier=NULL)=0;       // Ready for lofting, extrusion, etc.
		virtual void MakePolyShape(TimeValue t, PolyShape &shape, int steps = PSHAPE_BUILTIN_STEPS, BOOL optimize = FALSE)=0;   // Create a PolyShape representation with optional fixed steps & optimization
		virtual int MakeCap(TimeValue t, MeshCapInfo &capInfo, int capType)=0;  // Generate mesh capping info for the shape
		virtual int MakeCap(TimeValue t, PatchCapInfo &capInfo) { return 0; }	// Only implement if CanMakeBezier=TRUE -- Gen patch cap info
		virtual BOOL AttachShape(TimeValue t, INode *thisNode, INode *attachNode, BOOL weldEnds=FALSE, float weldThreshold=0.0f) { return FALSE; }	// Return TRUE if attached
		// UVW Mapping switch access
		virtual BOOL HasUVW() { return GetGenUVs(); }
		virtual void SetGenUVW(BOOL sw) { SetGenUVs(sw); }
		// These handle loading and saving the data in this class. Should be called
		// by derived class BEFORE it loads or saves any chunks
		CoreExport virtual IOResult Save(ISave *isave);
		CoreExport virtual IOResult Load(ILoad *iload);		

		CoreExport virtual Class_ID PreferredCollapseType();
		CoreExport virtual BOOL GetExtendedProperties(TimeValue t, TSTR &prop1Label, TSTR &prop1Data, TSTR &prop2Label, TSTR &prop2Data);
		CoreExport virtual void RescaleWorldUnits(float f);
		// New reference support for r4
		CoreExport virtual RefResult ShapeObject::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message );
		CoreExport virtual RefTargetHandle GetReference(int i);
		CoreExport virtual void SetReference(int i, RefTargetHandle rtarg);
		CoreExport virtual Animatable* SubAnim(int i);
		CoreExport virtual TSTR SubAnimName(int i);
		CoreExport ParamDimension *GetParameterDim(int pbIndex);
		CoreExport TSTR GetParameterName(int pbIndex);
		CoreExport virtual int RemapRefOnLoad(int iref);
		virtual int NumRefs() {return 1;}
		virtual int NumSubs() {return 1;}
		CoreExport void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		CoreExport void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
		// Get the validity of the ShapeObject parts
		CoreExport Interval GetShapeObjValidity(TimeValue t);
		// The following displays the shape's generated mesh if necessary
		CoreExport int Display(TimeValue t, INode *inode, ViewExp* vpt, int flags);
		// Get the bounding box for the shape, if it's active.  Leaves bbox unchanged if not.
		CoreExport virtual Box3 GetBoundingBox(TimeValue t, Matrix3 *tm=NULL);
		CoreExport virtual void InvalidateGeomCache();
	private:
	};

// Set ShapeObject's global Constant Cross-Section angle threshold (angle in radians) --
// Used for renderable shapes.
CoreExport void SetShapeObjectCCSThreshold(float angle);

/*------------------------------------------------------------------- 
  WSMObject : This is the helper object for the WSM modifier
---------------------------------------------------------------------*/

class  WSMObject: public Object {
	public:                                         
		SClass_ID SuperClassID() { return WSM_OBJECT_CLASS_ID; }                
		virtual Modifier *CreateWSMMod(INode *node)=0;
		virtual int UsesWireColor() { return FALSE; }   // TRUE if the object color is used for display
		virtual BOOL NormalAlignVector(TimeValue t,Point3 &pt, Point3 &norm) {pt=Point3(0,0,0);norm=Point3(0,0,-1);return TRUE;}
		virtual BOOL SupportsDynamics() { return FALSE; } // TRUE if spacewarp or collision object supports Dynamics

		virtual ForceField *GetForceField(INode *node) {return NULL;}		
		virtual CollisionObject *GetCollisionObject(INode *node) {return NULL;}		

	private:
	};



//--- Interface to XRef objects ---------------------------------------------------

class IXRefObject: public Object {
	public:
		Class_ID ClassID() {return Class_ID(XREFOBJ_CLASS_ID,0);}
		SClass_ID SuperClassID() {return SYSTEM_CLASS_ID;}

		// Initialize a new XRef object
		virtual void Init(TSTR &fname, TSTR &oname, Object *ob, BOOL asProxy=FALSE)=0;

		virtual void SetFileName(TCHAR *name, BOOL proxy=FALSE, BOOL update=TRUE)=0;
		virtual void SetObjName(TCHAR *name, BOOL proxy=FALSE)=0;
		virtual void SetUseProxy(BOOL onOff,BOOL redraw=TRUE)=0;
		virtual void SetRenderProxy(BOOL onOff)=0;
		virtual void SetUpdateMats(BOOL onOff)=0;
		virtual void SetIgnoreAnim(BOOL onOff,BOOL redraw=TRUE)=0;
		
		virtual TSTR GetFileName(BOOL proxy=FALSE)=0;
		virtual TSTR GetObjName(BOOL proxy=FALSE)=0;
		virtual TSTR &GetCurFileName()=0;
		virtual TSTR &GetCurObjName()=0;
		virtual BOOL GetUseProxy()=0;
		virtual BOOL GetRenderProxy()=0;
		virtual BOOL GetUpdateMats()=0;
		virtual BOOL GetIgnoreAnim()=0;		
		
		// Causes browse dialogs to appear
		virtual void BrowseObject(BOOL proxy)=0;
		virtual void BrowseFile(BOOL proxy)=0;

		// Try to reload ref
		virtual void ReloadXRef()=0;
	};



//---------------------------------------------------------------------------------


class ControlMatrix3;

// Used with EnumModContexts()
class ModContextEnumProc {
	public:
		virtual BOOL proc(ModContext *mc)=0;  // Return FALSE to stop, TRUE to continue.
	};

/*------------------------------------------------------------------- 
  Modifier: these are the ObjectSpace and World Space modifiers: They are 
  subclassed off of BaseObject so that they can put up a graphical 
  representation in the viewport. 
---------------------------------------------------------------------*/

class  Modifier: public BaseObject {
		friend class ModNameRestore;
		TSTR modName;
	public:
		
		CoreExport virtual TSTR GetName();
		CoreExport virtual void SetName(TSTR n);

		SClass_ID SuperClassID() { return OSM_CLASS_ID; }
		
		// Disables all mod apps that reference this modifier _and_ have a select
		// anim flag turned on.
		void DisableModApps() { NotifyDependents(FOREVER,PART_OBJ,REFMSG_DISABLE); }
		void EnableModApps() {  NotifyDependents(FOREVER,PART_OBJ,REFMSG_ENABLE); }
		
		// This disables or enables the mod. All mod apps referencing will be affected.
		void DisableMod() { 
			SetAFlag(A_MOD_DISABLED);
			NotifyDependents(FOREVER,PART_ALL|PART_OBJECT_TYPE,REFMSG_CHANGE); 
			}
		void EnableMod() {      
			ClearAFlag(A_MOD_DISABLED);
			NotifyDependents(FOREVER,PART_ALL|PART_OBJECT_TYPE,REFMSG_CHANGE); 
			}
		int IsEnabled() { return !TestAFlag(A_MOD_DISABLED); }

		// Same as above but for viewports only
		void DisableModInViews() { 
			SetAFlag(A_MOD_DISABLED_INVIEWS);
			NotifyDependents(FOREVER,PART_ALL|PART_OBJECT_TYPE,REFMSG_CHANGE); 
			}
		void EnableModInViews() {      
			ClearAFlag(A_MOD_DISABLED_INVIEWS);
			NotifyDependents(FOREVER,PART_ALL|PART_OBJECT_TYPE,REFMSG_CHANGE); 
			}
		int IsEnabledInViews() { return !TestAFlag(A_MOD_DISABLED_INVIEWS); }

		// Same as above but for renderer only
		void DisableModInRender() { 
			SetAFlag(A_MOD_DISABLED_INRENDER);
			NotifyDependents(FOREVER,PART_ALL|PART_OBJECT_TYPE,REFMSG_CHANGE); 
			}
		void EnableModInRender() {      
			ClearAFlag(A_MOD_DISABLED_INRENDER);
			NotifyDependents(FOREVER,PART_ALL|PART_OBJECT_TYPE,REFMSG_CHANGE); 
			}
		int IsEnabledInRender() { return !TestAFlag(A_MOD_DISABLED_INRENDER); }

		CoreExport virtual Interval LocalValidity(TimeValue t);
		virtual ChannelMask ChannelsUsed()=0;
		virtual ChannelMask ChannelsChanged()=0;
		// this is used to invalidate cache's in Edit Modifiers:
		virtual void NotifyInputChanged(Interval changeInt, PartID partID, RefMessage message, ModContext *mc) {}
		
		// This method indicates if the modifier changes the selection type channel or not.
		// If a modifier wants to change dynamically if it changes the subobj sel type
		// or not, it can overwrite this method.
		// ChannelsChanged() however can not be dynamically implemented.
		virtual bool ChangesSelType(){ return ChannelsChanged()&SUBSEL_TYPE_CHANNEL ? true : false;}

		// These call ChannelsUsed/Changed() but also OR in GFX_DATA_CHANNEL as appropriate.
		CoreExport ChannelMask TotalChannelsUsed();
		CoreExport ChannelMask TotalChannelsChanged();
		
		// This is the method that is called when the modifier is needed to 
		// apply its effect to the object. Note that the INode* is always NULL
		// for object space modifiers.
		virtual void ModifyObject(TimeValue t, ModContext &mc, ObjectState* os, INode *node)=0;

		// this should return FALSE for things like edit modifiers
		virtual int NeedUseSubselButton() { return 1; }
			  
		// Modifiers that place a dependency on topology should return TRUE
		// for this method. An example would be a modifier that stores a selection
		// set base on vertex indices.
		virtual BOOL DependOnTopology(ModContext &mc) {return FALSE;}

		// this can return:
		//   DEFORM_OBJ_CLASS_ID -- not really a class, but so what
		//   MAPPABLE_OBJ_CLASS_ID -- ditto
		//   TRIOBJ_CLASS_ID
		//   BEZIER_PATCH_OBJ_CLASS_ID
		virtual Class_ID InputType()=0;

		virtual void ForceNotify(Interval& i) 
			{NotifyDependents(i,ChannelsChanged(),REFMSG_CHANGE );}

		virtual IOResult SaveLocalData(ISave *isave, LocalModData *ld) { return IO_OK; }  
		virtual IOResult LoadLocalData(ILoad *iload, LocalModData **pld) { return IO_OK; }  

		// These handle loading and saving the modifier name. Should be called
		// by derived class BEFORE it loads or saves any chunks
		CoreExport IOResult Save(ISave *isave);
		CoreExport IOResult Load(ILoad *iload);

		// This will call proc->proc once for each application of the modifier.
		CoreExport void EnumModContexts(ModContextEnumProc *proc);
		
		// This method will return the IDerivedObject and index of this modifier 
		// for a given modifier context.
		CoreExport void GetIDerivedObject(ModContext *mc, IDerivedObject *&derObj, int &modIndex);

		// In case the modifier changes the object type (basically the os->obj pointer in ModifyObject)
		// *and* changes the ExtensionChannel, it has to overwrite this method and copy only the channels
		// that it doesn't modify/added already to the new object.
		CoreExport virtual void CopyAdditionalChannels(Object *fromObj, Object *toObj) { toObj->CopyAdditionalChannels(fromObj);}

		// Animatable Overides...
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
		CoreExport TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited);
		CoreExport bool SvCanSetName(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvSetName(IGraphObjectManager *gom, IGraphNode *gNode, TSTR &name);
		CoreExport bool SvHandleDoubleClick(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport COLORREF SvHighlightColor(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvIsSelected(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport MultiSelectCallback* SvGetMultiSelectCallback(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvCanSelect(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvCanInitiateLink(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvCanConcludeLink(IGraphObjectManager *gom, IGraphNode *gNode, IGraphNode *gNodeChild);
		CoreExport bool SvLinkChild(IGraphObjectManager *gom, IGraphNode *gNodeThis, IGraphNode *gNodeChild);
		CoreExport bool SvCanRemoveThis(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvRemoveThis(IGraphObjectManager *gom, IGraphNode *gNode);
	private:
	};

class  OSModifier: public Modifier {
	public:
		SClass_ID SuperClassID() { return OSM_CLASS_ID; }
	};

class  WSModifier: public Modifier {
	public:
		SClass_ID SuperClassID() { return WSM_CLASS_ID; }
	};

void CoreExport MakeHitRegion(HitRegion& hr, int type, int crossing, int epsi, IPoint2 *p);

class PolyLineProc {
	public:
	virtual int proc(Point3 *p, int n)=0;
	virtual void SetLineColor(float r, float g, float b) {}
	virtual void SetLineColor(Point3 c) {}
	virtual void Marker(Point3 *p,MarkerType type) {}
	};

class DrawLineProc:public PolyLineProc {
	GraphicsWindow *gw;
	public:
		DrawLineProc() { gw = NULL; }
		DrawLineProc(GraphicsWindow *g) { gw = g; }
		int proc(Point3 *p, int n) { gw->polyline(n, p, NULL, NULL, 0, NULL); return 0; }
		void SetLineColor(float r, float g, float b) {gw->setColor(LINE_COLOR,r,g,b);}
		void SetLineColor(Point3 c) {gw->setColor(LINE_COLOR,c);}
		void Marker(Point3 *p,MarkerType type) {gw->marker(p,type);}
	};

class BoxLineProc:public PolyLineProc {
	Box3 box;
	Matrix3 *tm;
	public:
		BoxLineProc() { box.Init();}
		BoxLineProc(Matrix3* m) { tm = m;  box.Init(); }
		Box3& Box() { return box; }
		CoreExport int proc(Point3 *p, int n);
		CoreExport void Marker(Point3 *p,MarkerType type);
	};


// Apply the PolyLineProc to each edge (represented by an array of Point3's) of the box
// after passing it through the Deformer def.
void CoreExport DoModifiedBox(Box3& box, Deformer &def, PolyLineProc& lp);
void CoreExport DoModifiedLimit(Box3& box, float z, int axis, Deformer &def, PolyLineProc& lp);
void CoreExport DrawCenterMark(PolyLineProc& lp, Box3& box );

// Some functions to draw mapping icons
void CoreExport DoSphericalMapIcon(BOOL sel,float radius, PolyLineProc& lp);
void CoreExport DoCylindricalMapIcon(BOOL sel,float radius, float height, PolyLineProc& lp);
void CoreExport DoPlanarMapIcon(BOOL sel,float width, float length, PolyLineProc& lp);

//---------------------------------------------------------------------
// Data structures for keeping log of hits during sub-object hit-testing.
//---------------------------------------------------------------------

class HitLog;
class HitRecord {
	friend class HitLog;    
	HitRecord *next;
	public:         
		INode *nodeRef;
		ModContext *modContext;
		DWORD distance;
		ulong hitInfo;
		HitData *hitData;
		HitRecord() { next = NULL; modContext = NULL; distance = 0; hitInfo = 0; hitData = NULL;}
		HitRecord(INode *nr, ModContext *mc, DWORD d, ulong inf, HitData *hitdat) {
			next = NULL;
			nodeRef = nr; modContext = mc; distance = d; hitInfo = inf; hitData = hitdat;
			}               
		HitRecord(HitRecord *n,INode *nr, ModContext *mc, DWORD d, ulong inf, HitData *hitdat) {
			next = n;
			nodeRef = nr; modContext = mc; distance = d; hitInfo = inf; hitData = hitdat;
			}               
		HitRecord *     Next() { return next; }
		~HitRecord() { if (hitData) { delete hitData; hitData = NULL; } }
	};                                      

class HitLog {
	HitRecord *first;
	int hitIndex;
	public:
		HitLog()  { first = NULL; hitIndex = 0; }
		~HitLog() { Clear(); }
		CoreExport void Clear();
		CoreExport void ClearHitIndex()		{ hitIndex = 0; }
		CoreExport void IncrHitIndex()		{ hitIndex++; }
		HitRecord* First() { return first; }
		CoreExport HitRecord* ClosestHit();
		CoreExport void LogHit(INode *nr, ModContext *mc, DWORD dist, ulong info, HitData *hitdat = NULL);
	};


// Creates a new empty derived object, sets it to point at the given
// object and returns a pointer to the derived object.
CoreExport Object *MakeObjectDerivedObject(Object *obj);


// Category strings for space warp objects:

#define SPACEWARP_CAT_GEOMDEF		1
#define SPACEWARP_CAT_MODBASED		2
#define SPACEWARP_CAT_PARTICLE		3

CoreExport TCHAR *GetSpaceWarpCatString(int id);

// ObjectConverter - allows users to register methods to (for example)
// allow Max to convert TriObjects directly into their plug-in object type.

class ObjectConverter : public InterfaceServer {
public:
	virtual Class_ID ConvertsFrom ()=0;
	virtual Class_ID ConvertsTo ()=0;
	// NOTE: There's a serious problem in that this method does not accept a TimeValue.
	// See below for details.
	virtual Object *Convert (Object *from)=0;
	virtual void DeleteThis () { }
};

// There was a problem in the above ObjectConverter class in that its Convert
// method doesn't accept a time parameter.  It's too late to change that, so we've
// implemented the following interface to supply the correct method.  Users'
// ObjectConverters should subclass off of both ObjectConverter and
// ITimeBasedConverter.  They should implement the GetInterface method as
// follows:
//BaseInterface *MyConverter::GetInterface (Interface_ID id) {
	//if (id == INTERFACE_TIME_BASED_CONVERTER) return (ITimeBasedConverter *)this;
	//return ObjectConverter::GetInterface (id);
//}
// They should then implement ConvertWithTime properly, and use Convert only
// to call ConvertWithTime with a time of GetCOREInterface()->GetTime().
// Convert should not be called (and won't be called by the system if this
// interface is properly set up).

#define INTERFACE_TIME_BASED_CONVERTER Interface_ID(0x1e064bad,0x716643db)

class ITimeBasedConverter : public BaseInterface {
public:
	Interface_ID GetID () { return INTERFACE_TIME_BASED_CONVERTER; }
	// This is the method that should be used to do the right conversion:
	virtual Object *ConvertWithTime (TimeValue t, Object *from)=0;
};

CoreExport bool RegisterObjectConverter (ObjectConverter *conv);
CoreExport int CanConvertTriObject (Class_ID to);
CoreExport int CanConvertPatchObject (Class_ID to);
CoreExport int CanConvertSplineShape (Class_ID to);
CoreExport void RegisterStaticEditTri (Object *triob);
CoreExport void RegisterCollapseType (Class_ID cid, TSTR name, bool canSelfConvert=false);

// Developers have to return a class derived from this class with implementations for 
// all memberfunctions when implementing subobjects for obejcts and modifiers (see GetSubObjType())

class ISubObjType : public InterfaceServer
{
public:
	virtual MaxIcon *GetIcon()=0;
	virtual TCHAR *GetName()=0;
};

// Generic implementation for subobject types. This SubObjectType will either use the 
// SubObjectIcons_16i.bmp and  SubObjectIcons_16a.bmp bitmaps in the UI directory 
// (for the GenSubObjType(int idx) constructor), or any other bmp file that is specified
// in the GenSubObjType(TCHAR *nm, TCHAR* pFilePrefix, int idx) constructor. The 
// bitmap files have to reside in the UI directory.

class GenSubObjType : public ISubObjType {
	TSTR name;
	MaxIcon *mIcon;
	int mIdx;
	TSTR mFilePrefix;

public:
	GenSubObjType(TCHAR *nm, TCHAR* pFilePrefix, int idx) : name(nm), mIcon(NULL), mIdx(idx), mFilePrefix(pFilePrefix) {}

	GenSubObjType(int idx) : mIcon(NULL), mIdx(idx), mFilePrefix(_T("SubObjectIcons")) {}
	
	CoreExport ~GenSubObjType();
	void SetName(TCHAR *nm){name = nm;}
	TCHAR *GetName() { return name;}
	CoreExport MaxIcon *GetIcon(); 
};


#endif //_OBJECT_
