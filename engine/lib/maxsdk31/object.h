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
class Texmap;
  
typedef Object* ObjectHandle;

MakeTab(TextMapIndex)
typedef TextMapIndexTab TextTab;

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

class View {
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
		virtual int Execute(int cmd, ULONG arg1=0, ULONG arg2=0, ULONG arg3=0) { return 0; } 

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

typedef unsigned long ChannelMask;

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
		CoreExport void DeleteObj(BOOL checkLock=FALSE);
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

class LocalModData {
	public:
		virtual ~LocalModData() {}
		virtual LocalModData *Clone()=0;
		virtual void* GetInterface(ULONG id) { return NULL; }  // to access sub-obj selection interfaces, JBW 2/5/99
	}; 

class ModContext {
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
	public:
		CoreExport void* GetInterface(ULONG id);

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
		
	private:
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
#define OBJECT_LOCKED 0x8000000

class ShapeObject;

class Object: public BaseObject {
		ulong locked;   // lock flags for each channel + object locked flag
		Interval noEvalInterval;  // used in ReducingCaches
	public:
		Object() { locked = OBJECT_LOCKED; noEvalInterval = FOREVER; }

		virtual int IsRenderable()=0;  // is this a renderable object?
		virtual void InitNodeName(TSTR& s)=0;
		virtual int UsesWireColor() { return TRUE; }    // TRUE if the object color is used for display
		virtual int DoOwnSelectHilite() { return 0; }
		// validity interval of Object as a whole at current time
		virtual Interval ObjectValidity(TimeValue t) { return FOREVER; }

		// This used to be in GeomObject but I realized that other types of objects may
		// want this (mainly to participate in normal align) such as grid helper objects.
		virtual int IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm) {return FALSE;}

		// Objects that don't support IntersectRay() (like helpers) can implement this
		// method to provide a default vector for normal align.
		virtual BOOL NormalAlignVector(TimeValue t,Point3 &pt, Point3 &norm) {return FALSE;}

		// locking of object as whole. defaults to NOT modifiable.
		void LockObject() { locked |= OBJECT_LOCKED; }
		void UnlockObject() { locked &= ~OBJECT_LOCKED; }
		int  IsObjectLocked() { return locked&OBJECT_LOCKED; }

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

		// access the current validity interval for the nth channel
		CoreExport virtual Interval ChannelValidity(TimeValue t, int nchan);
		virtual void SetChannelValidity(int nchan, Interval v) { }
		CoreExport void UpdateValidity(int nchan, Interval v);  // AND in interval v to channel validity

		// invalidate the specified channels
		virtual void InvalidateChannels(ChannelMask channels) { }

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
		CoreExport virtual Object* ConvertToType(TimeValue t, Class_ID obtype);
		
		// Indicate the types this object can collapse to
		virtual Class_ID PreferredCollapseType() {return Class_ID(0,0);}
		CoreExport virtual void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);

		// return the current sub-selection state
		virtual DWORD GetSubselState() {return 0;} 
		virtual void SetSubSelState(DWORD s) {}

		// If the requested channels are locked, replace their data
		// with a copy/ and unlock them, otherwise leave them alone
		CoreExport void ReadyChannelsForMod(ChannelMask channels);

		// Virtual methods to be implemented by plug-in object:-----
		
		// Makes a copy of its "shell" and shallow copies only the
		// specified channels.  Also copies the validity intervals of
		// the copied channels, and sets Invalidates the other intervals.
		virtual Object *MakeShallowCopy(ChannelMask channels) { return NULL; }

		// Shallow-copies the specified channels from the fromOb to this.
		// Also copies the validity intervals. 
		virtual void ShallowCopy(Object* fromOb, ChannelMask channels) {}

		// This replaces locked channels with newly allocated copies.
		// It will only be called if the channel is locked.
		virtual void NewAndCopyChannels(ChannelMask channels) {}                

		// Free the specified channels
		virtual void FreeChannels(ChannelMask channels) {}                                      
	  
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
		//
		virtual int NumPipeBranches() {return 0;}
		virtual Object *GetPipeBranch(int i) {return NULL;}
		
		// When an object has sub-object branches, it is likely that the
		// sub-objects are transformed relative to the object. This method
		// gives the object a chance to modify the node's transformation so
		// that operations (like edit modifiers) will work correctly when 
		// editing the history of the sub object branch.
		virtual INode *GetBranchINode(TimeValue t,INode *node,int i) {return node;}

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

		// Allow the object to enlarge its viewport rectangle, if it wants to.
		virtual void MaybeEnlargeViewportRect(GraphicsWindow *gw, Rect &rect) {}

		// Animatable Overides...
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
		CoreExport bool SvHandleDoubleClick(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited);
		CoreExport COLORREF SvHighlightColor(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvIsSelected(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport MultiSelectCallback* SvGetMultiSelectCallback(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvCanSelect(IGraphObjectManager *gom, IGraphNode *gNode);
	};


// This function should be used to count polygons in an object.
// It uses Object::PolygonCount() if it is supported, and converts to
// a TriObject and counts faces and vertices otherwise.
CoreExport void GetPolygonCount(TimeValue t, Object* pObj, int& numFaces, int& numVerts);

/*------------------------------------------------------------------- 
  CameraObject:  
---------------------------------------------------------------------*/

#define CAM_HITHER_CLIP         1
#define CAM_YON_CLIP            2

#define ENV_NEAR_RANGE          0
#define ENV_FAR_RANGE           1

struct CameraState {
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

class  CameraObject: public Object {
	public:
	SClass_ID SuperClassID() { return CAMERA_CLASS_ID; }
	int IsRenderable() { return(0);}
	virtual void InitNodeName(TSTR& s) { s = _T("Camera"); }
	virtual int UsesWireColor() { return FALSE; }   // TRUE if the object color is used for display
	
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
	virtual void UpdateTargDistance(TimeValue t, INode* inode) {}
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

		virtual NameTab* GetExclList() { return NULL; }  

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
	virtual ObjLightDesc *CreateLightDesc(INode *n) {return NULL;}
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
	virtual NameTab* GetExclList() {return NULL;}
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
class CollisionObject {
	public:
		// Check for collision. Return TRUE if there was a collision and the position and velocity have been modified.
		virtual BOOL CheckCollision(TimeValue t,Point3 &pos, Point3 &vel, float dt,int index, float *ct=NULL, BOOL UpdatePastCollide=TRUE)=0;
		virtual Object *GetSWObject()=0;
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

class ShapeObject: public GeomObject {
		BOOL renderable;	// Is it renderable?
		float thickness;	// Renderable thickness
		BOOL generateUVs;	// Generate UV coords on renderable shape if TRUE
	public:
		CoreExport ShapeObject();
		CoreExport ~ShapeObject();	// Must call this on destruction

        virtual BOOL IsShapeObject() { return TRUE; }

		virtual int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm) {return FALSE;}
		virtual void InitNodeName(TSTR& s) { s = _T("Shape"); }
		SClass_ID SuperClassID() { return SHAPE_CLASS_ID; }
		virtual int IsRenderable() { return (int)renderable;}
		CoreExport virtual void CopyBaseData(ShapeObject &from);
		// Access methods
		virtual BOOL GetRenderable() { return renderable; }
		virtual float GetThickness() { return thickness; }
		virtual BOOL GetGenUVs() { return generateUVs; }
		CoreExport virtual void SetRenderable(BOOL sw);
		CoreExport virtual void SetThickness(float t);
		CoreExport virtual void SetGenUVs(BOOL sw);
		CoreExport virtual Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);
		CoreExport virtual void GetRenderMeshInfo(TimeValue t, INode *inode, View& view, int &nverts, int &nfaces);	// Get info on the rendering mesh
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

		CoreExport virtual Interval LocalValidity(TimeValue t);
		virtual ChannelMask ChannelsUsed()=0;
		virtual ChannelMask ChannelsChanged()=0;
		// this is used to invalidate cache's in Edit Modifiers:
		virtual void NotifyInputChanged(Interval changeInt, PartID partID, RefMessage message, ModContext *mc) {}

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


#endif //_OBJECT_
