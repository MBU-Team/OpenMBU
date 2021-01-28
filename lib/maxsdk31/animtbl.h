/**********************************************************************
 *<
	FILE:  animtbl.h

	DESCRIPTION:  Defines Animatable Classes

	CREATED BY: Rolf Berteig & Dan Silva

	HISTORY: created 9 September 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef _ANIMTBL_H_
#define _ANIMTBL_H_


#define ANIMTYPE_NODE		1
#define ANIMTYPE_ROOTNODE	3
#define ANIMTYPE_CONTROL	2

/*---------------------------------------------------*/

class TreeListExp; 
class AnimEnum;
class ReferenceTarget;
class DWORDTab;
class IObjParam;
class INodeTab;
class AppDataChunk;
class IParamBlock2;

// The maximum number of track views that can be opened. Each
// animatable stores 3 bits for each track to identify it's open/close
// state and selection state.
#define MAX_TRACK_VIEWS	16

#define ALL_TRACK_VIEWS	0xffff

// The maximum number of track view selection sets
#define MAX_TRACKVIEW_SELSETS	16


// Values for flags in aflag
#define A_EVALUATING 		1
#define A_NOTIFYDEP  		(1<<1)
#define A_CHILD_TREE_OPEN 	(1<<2)
#define A_SUBANIM_TREE_OPEN	(1<<3)
#define A_OBJECT_REDUCED	(1<<4)

// BITS 5-11 are reserved for specific sub-class use.

// Atmospheric flags
#define A_ATMOS_DISABLED	(1<<5)

// OBJECT flags
#define A_OBJ_CREATING		(1<<5)		// The object is being created. It doesn't want to snap to itself.
#define A_OBJ_BEING_EDITED (1<<7)
#ifdef _OSNAP
#define A_OBJ_LONG_CREATE   (1<<6)		// Persists throughout the wholle creation process as 
										// opposed to the previous flag which gets cleared as 
										// as the object is added to the scene.
#endif

// MODIFIER flags . 
#define A_MOD_DISABLED			(1<<5)     // Modifier flag
#define A_MOD_BEING_EDITED		(1<<6)
#define A_MOD_USE_SEL			(1<<7)     // Modifier flag (use sub-ob selection)
#define A_MOD_DISABLED_INVIEWS	(1<<8)     // Modifier is disabled in viewports only

// MODAPP flags. 
#define A_MODAPP_DISABLED		(1<<5)  // ModApp flag
#define A_MODAPP_SELECTED		(1<<6)  // ModApp flag (parent node is selected)
#define A_MODAPP_DISPLAY_ACTIVE (1<<7)  // ModApp flag
#define A_MODAPP_DYNAMIC_BOX  	(1<<8)  // ModApp flag
#define A_MODAPP_RENDERING		(1<<9)  // Render begin turns this on and render end turns it off

// Derived Object Flags
#define A_DERIVEDOBJ_DONTDELETE	(1<<9)	// When the last modifier is deleted form this derived object, don't delete the derived object

// CONTROL flags
#define A_ORT_MASK				7
#define A_ORT_BEFORESHIFT		5	// Uses bit 5,6,7,8,9 and 10 to store ORT
#define A_ORT_AFTERSHIFT		8
#define A_CTRL_DISABLED			(1<<11)
#define A_ORT_DISABLED			A_SUPERCLASS1  // indicates that the ORT is disabled

// INODE flags
#define A_INODE_IK_TERMINATOR	(1<<5)	// Terminates the top of an IK chain
#define A_INODE_IK_POS_PINNED	(1<<6)
#define A_INODE_IK_ROT_PINNED	(1<<7)
#ifdef _OSNAP
#define A_INODE_CLONE_TARGET	(1<<8)
#endif
#define A_COMPONENT_LOCKED		(1<<19)	// needed for CompositeBase and its children

// Flags for Hold and Restore logic, for "lazy holding",
// to avoid multiple holding.
#define A_HELD				(1<<12)
#define A_SET				(1<<13)

// Deleted but kept around for UNDO
#define A_IS_DELETED		(1<<14)

// To prevent AutoDelete from being re-entered.
#define A_BEING_AUTO_DELETED  (1<<15)

// Reserved for superclass use
#define A_SUPERCLASS1		(1<<20)
#define A_SUPERCLASS2		(1<<21)

// These are reserved for use by the plug-in. No should will set these flags (except the plug-in class itself)
#define A_PLUGIN1			(1<<22)
#define A_PLUGIN2			(1<<23)
#define A_PLUGIN3			(1<<24)
#define A_PLUGIN4			(1<<25)

// Used to test for a dependency
#define A_DEPENDENCY_TEST	(1<<26)

// Ref target isn't deleted when dependents goes to 0 if this flag is set.
#define A_LOCK_TARGET		(1<<27)

#define A_WORK1				(1<<28)
#define A_WORK2				(1<<29)
#define A_WORK3				(1<<30)
#define A_WORK4				(1<<31)

#define A_OPENFLAG(t) ((t==0)?A_CHILD_TREE_OPEN:A_SUBANIM_TREE_OPEN)


class TimeMap {
	public:
		virtual TimeValue map(TimeValue t)=0;
		virtual TimeValue prevmap(TimeValue t)=0;
	};

class TrackHitRecord  {
	public:
		DWORD	hit;
		DWORD	flags;
		TrackHitRecord(DWORD h=0,DWORD f=0) {hit=h;flags=f;}
	};
typedef Tab<TrackHitRecord> TrackHitTab;


// Flags passed to MapKeys and DeleteKeys
#define TRACK_DOSEL			(1<<0)
#define TRACK_DOALL			(1<<1)  // ignore selection
#define TRACK_SLIDEUNSEL	(1<<2)  // Slide unselected keys to the right
#define TRACK_RIGHTTOLEFT	(1<<3)  // Enumerate right to left. If TRACK_SLIDEUNSEL is set, keys will slide to the left.
#define TRACK_DOSUBANIMS	(1<<4)  // This flag is only passed to MapKeys
#define TRACK_DOCHILDNODES	(1<<5)	// This flag is only passed to MapKeys
#define TRACK_MAPRANGE		(1<<6)	// The range, if not locked to first and last key, should be mapped as well.

// Flags passed to EditTimeRange
#define EDITRANGE_LINKTOKEYS	(1<<0)	// This means if one of the ends of the interval is at a key, link it to the key so that if the key moves, the interval moves.

// Flags passed to hit test tracks and fcurves.
#define HITTRACK_SELONLY		(1<<0)
#define HITTRACK_UNSELONLY		(1<<1)
#define HITTRACK_ABORTONHIT		(1<<2)
#define HITCURVE_TESTTANGENTS	(1<<3)
#define HITTRACK_SUBTREEMODE	(1<<4)  // Subtree mode is on so the anim is being asked to hittest itself in one of its ancestor's tracks

// Flags passed to SelectKeys
// Either SELECT, DESELECT, or a combination of CLEARKEYS and CLEARCURVE
// will be specified.
#define SELKEYS_SELECT			(1<<0)	
#define SELKEYS_DESELECT		(1<<1)
#define SELKEYS_CLEARKEYS		(1<<2)		 
#define SELKEYS_CLEARCURVE		(1<<3) 
#define SELKEYS_FCURVE			(1<<4) 	// indicates that were operating on keys of a function curve, not a track  


// Flags passed to GetTimeRange
#define TIMERANGE_SELONLY		(1<<0)	// The bounding interval of selected keys
#define TIMERANGE_ALL			(1<<1)  // Whatever the channel's time range is - usually the bunding interval of all keys.
#define TIMERANGE_CHILDNODES	(1<<2)  // A node's time range should include child nodes.
#define TIMERANGE_CHILDANIMS	(1<<3)  // A animatable's child anim ranges should be included

// Passed to the functions that modify a time range such as copy,paste,delete,reverse
#define TIME_INCLEFT		(1<<10)  // Include left endpoint
#define TIME_INCRIGHT		(1<<11)  // Include right endpoint
#define TIME_NOSLIDE		(1<<12)  // Delete any keys in the interval but don't actually remove the block of time.

// In addition to the TIME_ flags above, the following flag may be passed to PasteTrck()
#define PASTE_RELATIVE		(1<<20)	// Add the keys relative to existing keys

// Flags passed to AddKey
#define ADDKEY_SELECT		(1<<0)  // Select the new key and deselect any other selected keys
#define ADDKEY_INTERP		(1<<1)  // Init the new key to the interpolated value at that time. Otherwise, init the key to the value of the previous key.
#define ADDKEY_FLAGGED		(1<<2)  // Flag the newly created key as if FlagKey() was called for it

// Flags passed to CopyKeysFromTime()
#define COPYKEY_POS			(1<<0)	// These filter flags are passed to a tm controller. The tm
#define COPYKEY_ROT			(1<<1)	// can decide what to do with them... they have obvious meaning
#define COPYKEY_SCALE		(1<<2)	// For the PRS controller.

// Flags passed to GetNextKeyTime()
#define NEXTKEY_LEFT		(1<<0)	// Search to the left.
#define NEXTKEY_RIGHT		(1<<1)  // Search to the right.
#define NEXTKEY_POS			(1<<2)
#define NEXTKEY_ROT			(1<<3)
#define NEXTKEY_SCALE		(1<<4)

// Flags passed to IsKeyAtTime
#define KEYAT_POSITION		(1<<0)
#define KEYAT_ROTATION		(1<<1)
#define KEYAT_SCALE			(1<<2)

// Flags passed to PaintTrack and PaintFCurves
#define PAINTTRACK_SHOWSEL			(1<<0)
#define PAINTTRACK_SHOWSTATS		(1<<1)	// Show for selected keys
#define PAINTCURVE_SHOWTANGENTS		(1<<2)	// Show for selected keys
#define PAINTCURVE_FROZEN			(1<<3)  // Curve is in a frozen state
#define PAINTCURVE_GENCOLOR			(1<<4)	// Draw curve in generic color
#define PAINTCURVE_XCOLOR			(1<<5)	// Draw curve in red
#define PAINTCURVE_YCOLOR			(1<<6)	// Draw curve in green
#define PAINTCURVE_ZCOLOR			(1<<7)	// Draw curve in blue
#define PAINTTRACK_SUBTREEMODE		(1<<8)  // Subtree mode is on so the anim is being asked to paint itself in one of its ancestor's tracks
#define PAINTTRACK_HIDESTATICVALUES (1<<9)  // Indicates that static values shouldn't be displayed in tracks.

// Flags passed to GetFCurveExtents
#define EXTENTS_SHOWTANGENTS		(1<<0) // Tangents are visible for selected keys


// Values returned from PaintTrack, PaintFCurve and HitTestTrack
#define TRACK_DONE			1		// Track was successfully painted/hittested
#define TRACK_DORANGE		2		// This anim has no track. Draw/hittest the bounding range of it's subanims
#define TRACK_ASKCLIENT		3		// Ask client anim to paint the track

// Values returned from HitTestFCurve
#define HITCURVE_KEY		1	// Hit one or more keys
#define HITCURVE_WHOLE		2   // Hit the curve (anywhere)
#define HITCURVE_TANGENT	3	// Hit a tangent handle
#define HITCURVE_NONE		4	// Didn't hit squat
#define HITCURVE_ASKCLIENT	5	// Ask client to hit test fcurve.

// These flags are passed into PaintFCurves, HitTestFCurves, and GetFCurveExtnents
// They are filters for controllers with more than one curve
// NOTE: RGB controllers interpret X as red, Y as green and Z as blue.
#define DISPLAY_XCURVE		(1<<29)
#define DISPLAY_YCURVE		(1<<30)
#define DISPLAY_ZCURVE		(1<<31)

// Values returned from GetSelKeyCoords()
#define KEYS_NONESELECTED	(1<<0)
#define KEYS_MULTISELECTED	(1<<1)
#define KEYS_COMMONTIME		(1<<2)  // Both of these last two bits
#define KEYS_COMMONVALUE	(1<<3)  // could be set.

// Flags passed to GetSelKeyCoords()
#define KEYCOORDS_TIMEONLY		(1<<0)
#define KEYCOORDS_VALUEONLY		(1<<1)

// Variable definitions for SetSelKeyCoordsExpr()
#define KEYCOORDS_TIMEVAR	_T("n")
#define KEYCOORDS_VALVAR	_T("n")

// Return values from SetSelKeyCoordsExpr()
#define KEYCOORDS_EXPR_UNSUPPORTED	0	// Don't implement this method
#define	KEYCOORDS_EXPR_ERROR		1   // Error in expression
#define KEYCOORDS_EXPR_OK			2   // Expression evaluated

// Returned from NumKeys() if the animatable is not keyframeable
#define NOT_KEYFRAMEABLE	-1

// Flags passed to AdjustTangents
#define ADJTAN_LOCK		(1<<0)
#define ADJTAN_BREAK	(1<<1)

// Flags passed to EditTrackParams
#define EDITTRACK_FCURVE	(1<<0)	// The user is in the function curve editor.
#define EDITTRACK_TRACK		(1<<1) 	// The user is in one of the track views.
#define EDITTRACK_SCENE		(1<<2)	// The user is editing a path in the scene.
#define EDITTRACK_BUTTON	(1<<3)	// The user invoked by choosing the properties button. In this case the time parameter is NOT valid.
#define EDITTRACK_MOUSE		(1<<4)	// The user invoked by right clicking with the mouse. In this case the time parameter is valid.


// These are returned from TrackParamsType(). They define how the track parameters are invoked.
#define TRACKPARAMS_NONE	0	// Has no track parameters
#define TRACKPARAMS_KEY		1	// Entered by right clicking on a selected key
#define TRACKPARAMS_WHOLE	2	// Entered by right clicking anywhere in the track.

// Flags passed into RenderBegin
#define RENDERBEGIN_IN_MEDIT   1   // Indicates that the render is occuring in the material editor.


// Macros for converting track screen coords to time and back.
#define TimeToScreen(t,scale,scroll) (int(floor((t)*(scale)+0.5)) - (scroll))
#define ScreenToTime(s,scale,scroll) ((int)floor((s)/(scale) + (scroll)/(scale)+0.5))
#define ValueToScreen(v,h,scale,scroll) (h-int(floor((v)*(scale)+0.5)) - (scroll))
#define ScreenToValue(s,h,scale,scroll) ((float(h)-(float(s)+float(scroll)))/(scale))

// Scales a value about an origin
#define ScaleAboutOrigin(val,origin,scale) ((((val)-(origin))*(scale))+(origin))


class TrackClipObject {
	public:
		// Specifies the interval of time clipped.
		Interval clip;		

		// Identifies the creator of the clip object
		virtual SClass_ID 	SuperClassID()=0;
		virtual Class_ID	ClassID()=0;

		TrackClipObject(Interval iv) {clip = iv;}
		virtual void DeleteThis()=0;

		virtual int NumKeys() {return 0;}
		virtual BOOL GetKeyVal(int i, void *val) {return FALSE;}
		virtual BOOL SetKeyVal(int i, void *val) {return FALSE;}
	};


// This must be updated if a new entry is added to DimType!
#define NUM_BUILTIN_DIMS	10

enum DimType {
	DIM_WORLD,
	DIM_ANGLE,
	DIM_COLOR,	 	//0-1
	DIM_COLOR255,	//0-255
	DIM_PERCENT,  	//0-100
	DIM_NORMALIZED,	//0-1
	DIM_SEGMENTS,
	DIM_TIME,
	DIM_CUSTOM,
	DIM_NONE
	};

// These two classes describes the dimension of a parameter (sub-anim).
// The dimension type and possibly the dimension scale (if the type is
// custom) are used to determine a scale factor for the parameter.
// When a controller is drawing a function curve, it only needs to
// use the Convert() function - the scale factor is rolled into the single
// 'vzoom' parameter passed to PaintFCurves.
// So, for a controller to plot a value 'val' at time t it would do the
// following:
// int x = TimeToScreen(t,tzoom,tscroll);
// int y = ValueToScreen(dim->Convert(val),rect.h()-1,vzoom,vscroll);
//
class ParamDimensionBase {
	public:
		virtual DimType DimensionType()=0;
		virtual float Convert(float value)=0;
		virtual float UnConvert(float value)=0;
	}; 
class ParamDimension : public ParamDimensionBase {
	public:
		// If the DimType is custom than these must be implemented.
		virtual float GetDimScale() {return 1.0f;}
		virtual void SetDimScale() {}
		virtual TCHAR *DimensionName() {return _T("");}		
	};

// These point to default implementations for the standard DIM types.
CoreExport extern ParamDimension *defaultDim;
CoreExport extern ParamDimension *stdWorldDim;
CoreExport extern ParamDimension *stdAngleDim;
CoreExport extern ParamDimension *stdColorDim;
CoreExport extern ParamDimension *stdColor255Dim;
CoreExport extern ParamDimension *stdPercentDim;
CoreExport extern ParamDimension *stdNormalizedDim;
CoreExport extern ParamDimension *stdSegmentsDim;
CoreExport extern ParamDimension *stdTimeDim;

// Interface IDs for GetInterface() - NOTE: doesn't need to be released.
#define I_CONTROL			0x00001001		
#define I_MASTER			0x00001010
#define I_EASELIST			0x00001020
#define I_MULTLIST			0x00001030
#define I_BASEOBJECT		0x00001040
#define I_PARTICLEOBJ		0x00001050
#define I_KEYCONTROL		0x00001060
#define I_TEXTOBJECT		0x00001070
#define I_WAVESOUND			0x00001080
#ifdef _SUBMTLASSIGNMENT
	#define I_SUBMTLAPI			0x00001090
#endif
#define I_MESHSELECT		0x000010A0
#define I_MESHSELECTDATA	0x000010B0
#define I_MAXSCRIPTPLUGIN	0x000010C0
#define I_MESHDELTAUSER		0x000010D0
#define I_MESHDELTAUSERDATA	0x000010E0
#define I_SPLINESELECT		0x000010F0  // JBW: add interface to spline selection & ops (for SplineShape & EditSplineMod)
#define I_SPLINESELECTDATA	0x00001100
#define I_SPLINEOPS			0x00001110
#define I_PATCHSELECT		0x00001120  
#define I_PATCHSELECTDATA	0x00001130
#define I_PATCHOPS			0x00001140

#define I_COMPONENT			0x0000F010
#define I_REFARRAY			0x0000F030
#ifdef DESIGN_VER
//VIZ specific range begins here 0x0000F000
#define I_LINK_TARGET		0x0000F020
#define I_LAYER				0x0000F040
#define I_LAYER_MANAGER		0x0000F050
#endif
#define I_REAGENT			0x0000F060
#define I_GEOMIMP			0x0000F070 //JH 3/7/99 implementaion neutral interface to geometry caches
#define I_AGGREGATION		0x0000F080
#define I_VALENCE			0x0000F090
#define I_GUEST				0x0000F100
#define I_HOST				0x0000F110
#define I_SCENEOBJECT		0x0000F120
// Plug-in defined interfaces should be > this id
#define I_USERINTERFACE		0x0000ffff

#define GetControlInterface(anim)	((Control*)anim->GetInterface(I_CONTROL))
#define GetObjectInterface(anim)	((BaseObject*)anim->GetInterface(I_BASEOBJECT))
#define GetParticleInterface(anim) 	((ParticleObject*)anim->GetInterface(I_PARTICLEOBJ))
#define GetKeyControlInterface(anim) ((IKeyControl*)anim->GetInterface(I_KEYCONTROL))
#define GetMasterController(anim) ((ReferenceTarget*)anim->GetInterface(I_MASTER))
#define GetTextObjectInterface(anim) ((ITextObject*)anim->GetInterface(I_TEXTOBJECT))
#define GetWaveSoundInterface(anim) ((IWaveSound*)anim->GetInterface(I_WAVESOUND))
#define GetMeshSelectInterface(anim) ((IMeshSelect*)anim->GetInterface(I_MESHSELECT))
#define GetMeshSelectDataInterface(anim) ((IMeshSelectData*)anim->GetInterface(I_MESHSELECTDATA))
#define GetMeshDeltaUserInterface(anim) ((MeshDeltaUser*)anim->GetInterface(I_MESHDELTAUSER))
#define GetMeshDeltaUserDataInterface(anim) ((MeshDeltaUserData*)anim->GetInterface(I_MESHDELTAUSERDATA))

// This is the base class for classes that can be hung off an animatable's
// property list. When an animatable is deleted, it's properties will be
// deleted and their virtual destructor will be called.
class AnimProperty {
	public:
		virtual BOOL DontDelete() {return FALSE;}
		virtual ~AnimProperty() {}
		virtual DWORD ID()=0;
	};

class AnimPropertyList : public Tab<AnimProperty*> {
	public:
		CoreExport int FindProperty(DWORD id,int start=0);
	};


// Property IDs
#define PROPID_APPDATA		0x00000010
#define PROPID_EASELIST		0x00000020
#define PROPID_MULTLIST		0x00000030
#define PROPID_NOTETRACK	0x00000040
#define PROPID_CLEARCACHES	0x00000050
#define PROPID_HAS_WSM		0x00000060
#define PROPID_PSTAMP_SMALL 0x00000070
#define PROPID_PSTAMP_LARGE 0x00000071
#define PROPID_SVDATA			0x00000080
#define PROPID_FORCE_RENDER_MESH_COPY 0x000000100
#define PROPID_EVAL_STEPSIZE_BUG_FIXED 0x1000
#define PROPID_USER			0x0000FFFF	
// Values above PROPID_USER can be used by plug-ins. 
// Note: that a plug-in should only put user defined properties on it's
// own list. So IDs only have to be unique within a plug-in. If a plug-in
// needs to attach data to another object, it can do so via APP_DATA.

// BeginEditParams flags values
#define BEGIN_EDIT_CREATE  		(1<<0)
#define BEGIN_EDIT_MOTION		(1<<1)	// Controller is being edited in the motion branch
#define BEGIN_EDIT_HIERARCHY	(1<<2)	// Same as BEGIN_EDIT_IK
#define BEGIN_EDIT_IK			(1<<2)  // Controller is being edited in the IK subtask of the hierarchy branch
#define BEGIN_EDIT_LINKINFO		(1<<3)  // Controller is being edited in the Link Info  subtask of the hierarchy branch

// EndEditParams flags values
#define END_EDIT_REMOVEUI  (1<<0)

// Flags passed to EnumAuxFiles
#define FILE_ENUM_INACTIVE 		(1<<0) 			// enumerate inactive files
#define FILE_ENUM_VP			(1<<1)   	  	// enumerate video post files
#define FILE_ENUM_RENDER		(1<<2)   	  	// enumerate render files 
#define FILE_ENUM_ALL  (FILE_ENUM_INACTIVE|FILE_ENUM_VP|FILE_ENUM_RENDER)	// enumerate ALL files
#define FILE_ENUM_MISSING_ONLY	(1<<8)  	  	// enumerate missing files only
#define FILE_ENUM_1STSUB_MISSING (1<<9) 	    // just enumerate 1st file named by ifl if missing
#define FILE_ENUM_DONT_RECURSE   (1<<10) 	    // don't enumerate references
#define FILE_ENUM_CHECK_AWORK1   (1<<11) 	    // don't enumerate things with flag A_WORK1 set

// To enumerate all active but missing files
#define FILE_ENUM_MISSING_ACTIVE (FILE_ENUM_VP|FILE_ENUM_RENDER|FILE_ENUM_MISSING_ONLY)

// To enumerate all active but missing files, but only enumerate first subfile pointed
// to by an ifl (enumerating all of them can be very slow).
#define FILE_ENUM_MISSING_ACTIVE1 (FILE_ENUM_MISSING_ACTIVE|FILE_ENUM_1STSUB_MISSING )

// A callback object passed to EnumAuxFiles().
class NameEnumCallback {
	public:
		virtual void RecordName(TCHAR *name)=0;
	};

class NoteTrack;

class Animatable {
		friend class ISaveImp;
		friend class ILoadImp;

	protected:
		unsigned long aflag;
		AnimPropertyList aprops;
		DWORD tvflags1, tvflags2;

	public:	
		void SetAFlag(int mask) { aflag|=mask; }
		void ClearAFlag(int mask) { aflag &= ~mask; }
		int TestAFlag(int mask) { return(aflag&mask?1:0); }
		CoreExport Animatable();
		Animatable& operator=(const Animatable& an)  { aflag = an.aflag; return *this; }		
		virtual void GetClassName(TSTR& s) { s= TSTR(_T("Animatable")); }  
		CoreExport virtual Class_ID ClassID();
		CoreExport virtual SClass_ID SuperClassID();		
		CoreExport virtual ~Animatable();

		// This is how things are actually deleted. 
		// Since they are created with the ClassDesc::Create()  function, and 
		// deleted via this function, they can use a different memory allocator
		// than the core code. (theoretically)
		CoreExport virtual void DeleteThis();
		
		// Get rid of anything that can be rebuilt 
		// Objects have a default implementation.
		virtual void FreeCaches() {}

		// 'create' is TRUE if parameters are being edited in the create branch.
		// 'removeUI' is TRUE if the object's UI should be removed.
		virtual void BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev=NULL) {} 
		virtual void EndEditParams(IObjParam *ip,ULONG flags,Animatable *next=NULL) {} 

		// OLE-like method for adding new interfaces
		virtual void* GetInterface(ULONG id){ return NULL;}
		virtual void ReleaseInterface(ULONG id,void *i) {}

		// General method for adding properties, when
		// defining a new Interface would be too cumbersome
		virtual int SetProperty(ULONG id, void *data) { return 0; }
		virtual void *GetProperty(ULONG id) { return NULL; }

		virtual	int NumSubs()  { return 0; }     // How many sub-animatables are there?
		virtual	Animatable* SubAnim(int i) { return NULL; }  // access the ith sub-animatable
		CoreExport virtual TSTR SubAnimName(int i);  // get name of ith subanim
		virtual BOOL BypassTreeView() { return FALSE; } // return TRUE and you won't appear in the TreeView however your children will.
		virtual BOOL BypassTrackBar() { return BypassTreeView(); } // return TRUE and you won't appear in the TrackBar however your children will.
		virtual BOOL BypassPropertyLevel() { return FALSE; } // return TRUE and you won't appear as a property in the scripter however your children will.
		virtual BOOL InvisibleProperty() { return FALSE; } // return TRUE and you won't appear as a property in the scripter.
		virtual BOOL AssignController(Animatable *control,int subAnim) {return FALSE;}

		// Used to allow deletion of controllers from the track view
		virtual BOOL CanDeleteSubAnim(int i) {return FALSE;}
		virtual void DeleteSubAnim(int i) {}
		
		// Return the suggested color to draw a sub-anim's function curve
		// can be one of PAINTCURVE_GENCOLOR, PAINTCURVE_XCOLOR, PAINTCURVE_YCOLOR, PAINTCURVE_ZCOLOR
		virtual DWORD GetSubAnimCurveColor(int subNum) {return PAINTCURVE_GENCOLOR;}

		// Converts an anim index to a ref index or returns -1 if there is no
		// corrispondance. This is used for copying and pasting in the track
		// view. If a client does not wish an anim to be copy/pastable then
		// it can return -1 even if there is a corrisponding ref num.
		virtual int SubNumToRefNum(int subNum) {return -1;}

		// In addition to SubNumToRefNum(), if an anim doesn't want to be coppied it can return FALSE from this function
		virtual BOOL CanCopyAnim() {return TRUE;}

		// An anim can implement this to reutrn FALSE to prohibit make unique
		virtual BOOL CanMakeUnique() {return TRUE;}

		virtual	int NumChildren() {return 0; }   // Non-zero only for nodes.
		virtual Animatable* ChildAnim(int i) { return NULL; } // access the ith child
		CoreExport virtual TSTR NodeName(); 	   // For nodes only

		CoreExport int EnumAnimTree(AnimEnum *animEnum, Animatable *client, int subNum);
		CoreExport int HasSubElements(int type=0); 		

		// called once at the beginning of each render
		virtual int RenderBegin(TimeValue t, ULONG flags=0) { return 0; }
		// called once at the end of each render
		virtual int RenderEnd(TimeValue t) { return 0; }

		// edit the track or parameters
		virtual void EditTrack() { assert(0); } 
		
		// Get the number of keys and the time of the ith key.
		virtual int NumKeys() {return NOT_KEYFRAMEABLE;}
		virtual TimeValue GetKeyTime(int index) {return 0;}
		virtual int GetKeyIndex(TimeValue t) {return -1;}
		virtual BOOL GetNextKeyTime(TimeValue t,DWORD flags,TimeValue &nt) {return FALSE;}
		virtual void CopyKeysFromTime(TimeValue src,TimeValue dst,DWORD flags) {} 
		virtual void DeleteKeyAtTime(TimeValue t) {}
		virtual BOOL IsKeyAtTime(TimeValue t,DWORD flags) {return FALSE;}
		
		// The value returned from these two methods should be the number of keys skipped because their times were before
		// range.Start(). So sel[i] is the selection state for the offset+ith key.
		virtual int GetKeyTimes(Tab<TimeValue> &times,Interval range,DWORD flags) {return 0;}		
		virtual int GetKeySelState(BitArray &sel,Interval range,DWORD flags) {return 0;}

		// TreeView Methods
		/////////////////////////////////////////////////////////////
		// the 'tv' parameter specifies which track view.
		// each track view uses one bit, there can be up to MAX_TRACK_VIEWS
		CoreExport void OpenTreeEntry(int type, DWORD tv);
		CoreExport void CloseTreeEntry(int type, DWORD tv);
		CoreExport int IsTreeEntryOpen(int type, DWORD tv);
		
		// Track view selected state
		CoreExport BOOL GetSelInTrackView(DWORD tv);
		CoreExport void SetSelInTrackView(DWORD tv, BOOL sel);

		// Track view selection sets: 'which' should be >=0 and <MAX_TRACKVIEW_SELSETS
		CoreExport BOOL InTrackViewSelSet(int which);
		CoreExport void SetTrackViewSelSet(int which, BOOL inOut);

		// The tracks time range:
		CoreExport virtual Interval GetTimeRange(DWORD flags);
		virtual void EditTimeRange(Interval range,DWORD flags) {};
		
		// Operations to a selected block of time		
		virtual void DeleteTime(Interval iv, DWORD flags) {}
		virtual void ReverseTime(Interval iv, DWORD flags) {}
		virtual void ScaleTime(Interval iv, float s) {}
		virtual void InsertTime(TimeValue ins, TimeValue amount) {}
		
		// If an anim supports the above time operations it should return TRUE from this method.
		// Returning TRUE enables time selection on the track view for the track.
		virtual BOOL SupportTimeOperations() {return FALSE;}

		// Operations to keys
		CoreExport virtual void MapKeys(TimeMap *map, DWORD flags);
		virtual void DeleteKeys(DWORD flags) {}
		virtual void DeleteKeyByIndex(int index) {}
		virtual void SelectKeys(TrackHitTab& sel, DWORD flags) {}
		virtual void SelectSubKeys(int subNum,TrackHitTab& sel, DWORD flags) {} // this is called on the client when the client takes over control of an anims fcurve
		virtual void SelectSubCurve(int subNum,BOOL sel) {}
		virtual void SelectKeyByIndex(int i,BOOL sel) {}
		virtual BOOL IsKeySelected(int i) {return FALSE;}
		virtual void FlagKey(TrackHitRecord hit) {}
		virtual int GetFlagKeyIndex() {return -1;}
		virtual int NumSelKeys() {return 0;}
		virtual void CloneSelectedKeys(BOOL offset=FALSE) {}   // When offset is TRUE, set the new key time to be centered between the original key and the next key
		virtual void AddNewKey(TimeValue t,DWORD flags) {}
		virtual void MoveKeys(ParamDimensionBase *dim,float delta,DWORD flags) {}  // move selected keys vertically in the function curve editor
		virtual void ScaleKeyValues(ParamDimensionBase *dim,float origin,float scale,DWORD flags) {}
		virtual void SelectCurve(BOOL sel) {}
		virtual BOOL IsCurveSelected() {return FALSE;}		
		virtual BOOL IsSubCurveSelected(int subNum) {return FALSE;}
		virtual int GetSelKeyCoords(TimeValue &t, float &val,DWORD flags) {return KEYS_NONESELECTED;}
		virtual void SetSelKeyCoords(TimeValue t, float val,DWORD flags) {}
		virtual int SetSelKeyCoordsExpr(ParamDimension *dim,TCHAR *timeExpr, TCHAR *valExpr, DWORD flags) {return KEYCOORDS_EXPR_UNSUPPORTED;}
		virtual void AdjustTangents(
			TrackHitRecord hit,
			ParamDimensionBase *dim,
			Rect& rcGraph,
			float tzoom,
			int tscroll,
			float vzoom,
			int vscroll,
			int dx,int dy,
			DWORD flags) {};

		// Does this animatable actually have animation?
		// Default implementation returns TRUE if a child anim has animation.
		CoreExport virtual BOOL IsAnimated(); 

		// Clipboard methods:
		virtual BOOL CanCopyTrack(Interval iv, DWORD flags) {return FALSE;}
		virtual BOOL CanPasteTrack(TrackClipObject *cobj,Interval iv, DWORD flags) {return FALSE;}
		virtual TrackClipObject *CopyTrack(Interval iv, DWORD flags) {return NULL;}
		virtual void PasteTrack(TrackClipObject *cobj,Interval iv, DWORD flags) {}

		// Plug-ins can implement copying and pasting for cases where their subanims
		// don't implement it. These aren't called on the client unless the sub-anim
		// doesn't implement the above versions.
		virtual BOOL CanCopySubTrack(int subNum,Interval iv, DWORD flags) {return FALSE;}
		virtual BOOL CanPasteSubTrack(int subNum,TrackClipObject *cobj,Interval iv, DWORD flags) {return FALSE;}
		virtual TrackClipObject *CopySubTrack(int subNum,Interval iv, DWORD flags) {return NULL;}
		virtual void PasteSubTrack(int subNum,TrackClipObject *cobj,Interval iv, DWORD flags) {}


		// Drawing and hit testing tracks
		virtual int GetTrackVSpace( int lineHeight ) { return 1; }
		virtual int HitTestTrack(			
			TrackHitTab& hits,
			Rect& rcHit,
			Rect& rcTrack,			
			float zoom,
			int scroll,
			DWORD flags ) { return TRACK_DORANGE; }
		virtual int PaintTrack(			
			ParamDimensionBase *dim,
			HDC hdc,
			Rect& rcTrack,
			Rect& rcPaint,
			float zoom,
			int scroll,
			DWORD flags ) { return TRACK_DORANGE; }
		virtual int PaintSubTrack(			
			int subNum,
			ParamDimensionBase *dim,
			HDC hdc,
			Rect& rcTrack,
			Rect& rcPaint,
			float zoom,
			int scroll,
			DWORD flags) {return TRACK_DORANGE;}

		// Drawing and hit testing function curves
		virtual int PaintFCurves(			
			ParamDimensionBase *dim,
			HDC hdc,
			Rect& rcGraph,
			Rect& rcPaint,
			float tzoom,
			int tscroll,
			float vzoom,
			int vscroll,
			DWORD flags ) { return 0; }
		virtual int HitTestFCurves(			
			ParamDimensionBase *dim,
			TrackHitTab& hits,
			Rect& rcHit,
			Rect& rcGraph,			
			float tzoom,
			int tscroll,
			float vzoom,
			int vscroll,
			DWORD flags ) { return HITCURVE_NONE; }
		
		// Versions that allow clients to paint and hit test their subanims curves
		virtual int PaintSubFCurves(			
			int subNum,
			ParamDimensionBase *dim,
			HDC hdc,
			Rect& rcGraph,
			Rect& rcPaint,
			float tzoom,
			int tscroll,
			float vzoom,
			int vscroll,
			DWORD flags ) { return 0; }
		virtual int HitTestSubFCurves(
			int subNum,
			ParamDimensionBase *dim,
			TrackHitTab& hits,
			Rect& rcHit,
			Rect& rcGraph,			
			float tzoom,
			int tscroll,
			float vzoom,
			int vscroll,
			DWORD flags ) { return HITCURVE_NONE; }


		// Edit key info (or whatever) for selected keys.
		// hParent is the parent window that should be used to create any dialogs.
		// This function should not return until the user has completed editng at which
		// time any windows that were created should be destroyed. Unlike
		// BeginEditParams/EndEditParams this interface is modal.		
		virtual void EditTrackParams(
			TimeValue t,	// The horizontal position of where the user right clicked.
			ParamDimensionBase *dim,
			TCHAR *pname, // The name of the parameter as given by the client
			HWND hParent,
			IObjParam *ip,
			DWORD flags) {}

		// Returns a value indicating how track parameters are
		// are invoked. See description above by
		// TRACKPARAMS_NONE, TRACKPARAMS_KEY, TRACKPARAMS_WHOLE
		virtual int TrackParamsType() {return TRACKPARAMS_NONE;}

		// Calculate the largest and smallest values.
		// If this is processed, return non-zero.
		virtual int GetFCurveExtents(
			ParamDimensionBase *dim,
			float &min, float &max, DWORD flags) {return 0;}
		virtual int GetSubFCurveExtents(
			int subNum,
			ParamDimensionBase *dim,
			float &min, float &max, DWORD flags) {return 0;}

		// Describes the type of dimension of the ith sub-anim
		virtual ParamDimension* GetParamDimension(int i) {return defaultDim;}

		// This is not used anymore.
		virtual LRESULT CALLBACK TrackViewWinProc( HWND hwnd,  UINT message, 
		                                           WPARAM wParam, LPARAM lParam ) { return 0;}


		// Called when the user clicks on the icon of a subAnim in the track view.
		virtual BOOL SelectSubAnim(int subNum) {return FALSE;}

		// Add/delete note tracks
		CoreExport void AddNoteTrack(NoteTrack *note);
		CoreExport void DeleteNoteTrack(NoteTrack *note,BOOL delNote=TRUE); // If delNote is FALSE the note track will be removed from the anim but not deleted.
		CoreExport BOOL HasNoteTracks();
		CoreExport int NumNoteTracks();
		CoreExport NoteTrack *GetNoteTrack(int i);

		// Enumerate auxiliary files -- see ref.h for default implementation
		virtual void EnumAuxFiles(NameEnumCallback& nameEnum, DWORD flags = FILE_ENUM_ALL) {}

		// Free all bitmaps in the Animatable: don't recurse
		virtual void FreeAllBitmaps() {}

		// A master controller should implement this method to give the 
		// MAX a list of nodes that are part of the system. 
		virtual void GetSystemNodes(INodeTab &nodes) {}

		// If an object is a sub-class of a particular class, it will have a
		// different ClassID() because it is a different class. This method
		// allows an object to indicate that it is a sub-class of a particluar
		// class and therefore can be treated as one.
		// For example, a class could be derived from TriObject. This derived
		// class would have a different ClassID() then TriObject's class ID
		// however it still can be treated (cast) as a TriObject because it
		// is derived from TriObject.
		// Note the default implelementation: a class is considered to also
		// be a subclass of itself.
		virtual BOOL IsSubClassOf(Class_ID classID) {return classID==ClassID();}

		// Access app data chunks
		CoreExport void AddAppDataChunk(Class_ID cid, SClass_ID sid, DWORD sbid, DWORD len, void *d);
		CoreExport AppDataChunk *GetAppDataChunk(Class_ID cid, SClass_ID sid, DWORD sbid);
		CoreExport BOOL RemoveAppDataChunk(Class_ID cid, SClass_ID sid, DWORD sbid);		
		CoreExport void ClearAllAppData();
		
		CoreExport virtual void MouseCycleCompleted(TimeValue t);
		CoreExport virtual void MouseCycleStarted(TimeValue t);

		// JBW: direct ParamBlock2 access added
		virtual int	NumParamBlocks() { return 0; }			// return number of ParamBlocks in this instance
		virtual IParamBlock2* GetParamBlock(int i) { return NULL; } // return i'th ParamBlock
		virtual IParamBlock2* GetParamBlockByID(short id) { return NULL; } // return ParamBlock given ID


		// Save and load functions for schematic view data.
		// For classes derived from ReferenceMaker, there is
		// no need to call these.  However, IF you have stuff
		// derived from Animatable AND it appears in the
		// schematic view AND you want to save schematic view
		// properties for the object (node position, selection
		// state, etc.) THEN you have to call these guys in
		// your Save and Load functions... 
		CoreExport bool SvSaveData(ISave *isave, USHORT id);
		CoreExport bool SvLoadData(ILoad *iLoad);

		// Used internally by the schematic view.  There should
		// be no reason for plug-ins to ever call these...
		CoreExport DWORD SvGetRefIndex();
		CoreExport void SvSetRefIndex(DWORD i);
		CoreExport bool SvDeleteRefIndex();

		// Traverses the graph of objects in the MAX scene,
		// adding desired objects to the schematic view.
		// Developers can specialize this behaviour by overriding
		// this method and adding whatever objects are interesting
		// to the schematic view...
		// Objects are added to the schematic view by calling
		// IGraphObjectManager::AddAnimatable(...)
		// Reference lines are added to the schematic view by
		// calling IGraphObjectManager::AddReference(...)
		// Implementers of SvTraverseAnimGraph(...) should
		// call SvTraverseAnimGraph(...) recursively to
		// process other objects in the scene.
		CoreExport virtual SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);

		// A default graph traversal function which can be
		// called from SvTraverseAnimGraph(...) to handle
		// graph traversal in simple cases.  Follows sub-anim
		// and child references...
		CoreExport SvGraphNodeReference SvStdTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);

		// Animatable returns true if it can be the initiator of
		// a link operation in the schematic view...
		CoreExport virtual bool SvCanInitiateLink(IGraphObjectManager *gom, IGraphNode *gNode);

		// Animatable returns true if it can be the receiver
		// (parent) of a link operation in the schematic view...
		CoreExport virtual bool SvCanConcludeLink(IGraphObjectManager *gom, IGraphNode *gNode, IGraphNode *gNodeChild);

		// Returns the name of the object as it appears in the
		// schematic view...
		CoreExport virtual TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited);

		// Return true to permit the object's name to be
		// edited in the schematic view...
		CoreExport virtual bool SvCanSetName(IGraphObjectManager *gom, IGraphNode *gNode);

		// Called when the user changes the name of the object
		// in the schematic view...
		CoreExport virtual bool SvSetName(IGraphObjectManager *gom, IGraphNode *gNode, TSTR &name);

		// Return true if this object can be removed in the schematic view...
		CoreExport virtual bool SvCanRemoveThis(IGraphObjectManager *gom, IGraphNode *gNode);

		// Called when the user deletes this object in the schematic view...
		CoreExport virtual bool SvRemoveThis(IGraphObjectManager *gom, IGraphNode *gNode);

		// Returns true if the object is selected in its primary
		// editor...
		CoreExport virtual bool SvIsSelected(IGraphObjectManager *gom, IGraphNode *gNode);

		// Returns true if the object is to be highlighted in
		// the schematic view...
		CoreExport virtual bool SvIsHighlighted(IGraphObjectManager *gom, IGraphNode *gNode);

		// Returns the highlight color for this node.  The
		// highlight color is used to outline nodes in the
		// schematic view when SvIsHighlighted(...) returns
		// true...
		CoreExport virtual COLORREF SvHighlightColor(IGraphObjectManager *gom, IGraphNode *gNode);

		// Returns a color which is used to paint the triangular
		// color swatch that appears in the upper-right hand
		// corner of the node in the schematic view.  Can
		// return SV_NO_SWATCH to indicate that no swatch is
		// to be drawn...
		CoreExport virtual COLORREF SvGetSwatchColor(IGraphObjectManager *gom, IGraphNode *gNode);

		// Returns true if this object is inactive.  The schematic
		// view draws inactive nodes in a grayed-out state.
		CoreExport virtual bool SvIsInactive(IGraphObjectManager *gom, IGraphNode *gNode);

		// Links gNodeChild to this object...
		CoreExport virtual bool SvLinkChild(IGraphObjectManager *gom, IGraphNode *gNodeThis, IGraphNode *gNodeChild);

		// Called when this node is double-clicked in the
		// schematic view...
		CoreExport virtual bool SvHandleDoubleClick(IGraphObjectManager *gom, IGraphNode *gNode);

		// Called before a multiple select/deselect operation
		// in the schematic view.  Returns a callback used
		// to perform the (de)selection.  May return NULL if
		// this object cannot be selected in some principle
		// editor outside the schematic view...		
		CoreExport virtual MultiSelectCallback* SvGetMultiSelectCallback(IGraphObjectManager *gom, IGraphNode *gNode);		

		// Returns true if this object can be selected in
		// some editor (viewport, material editor, plug-in
		// specific editor, etc.).  Selection is actually
		// accomplished by via the SvGetMultiSelectCallback(...)
		// mechanism described above...
		CoreExport virtual bool SvCanSelect(IGraphObjectManager *gom, IGraphNode *gNode);

		// Called when the user edits the properties of a
		// node from the schematic view...
		CoreExport virtual bool SvEditProperties(IGraphObjectManager *gom, IGraphNode *gNode);

		// Returns a string to be displayed in the tip window
		// for this object in the schematic view...
		CoreExport virtual TSTR SvGetTip(IGraphObjectManager *gom, IGraphNode *gNode);

		// Returns a string to be displayed in the tip window
		// in the schematic view for a reference from "gNodeMaker"
		// to this...
		CoreExport virtual TSTR SvGetRefTip(IGraphObjectManager *gom, IGraphNode *gNode, IGraphNode *gNodeMaker);

		// Returns true is this object can respond to the SvDetach(...) method...
		CoreExport virtual bool SvCanDetach(IGraphObjectManager *gom, IGraphNode *gNode);

		// Detach this object from its owner...
		CoreExport virtual bool SvDetach(IGraphObjectManager *gom, IGraphNode *gNode);

	};





//
// Callback for EnumAnimTree:
//
// Scope values:

#define SCOPE_DOCLOSED 1   		// do "closed" animatables.
#define SCOPE_SUBANIM  2		// do the sub anims 
#define SCOPE_CHILDREN 4 		// do the node children
#define SCOPE_OPEN	(SCOPE_SUBANIM|SCOPE_CHILDREN) // do all open animatables
#define SCOPE_ALL	(SCOPE_OPEN|SCOPE_DOCLOSED)     // do all animatables

// Return values for AnimEnum procs
#define ANIM_ENUM_PROCEED 1
#define ANIM_ENUM_STOP 2
#define ANIM_ENUM_ABORT 3

class AnimEnum {
	protected:
		int depth;
		int scope;  
		DWORD tv;
	public:
	 	AnimEnum(int s = SCOPE_OPEN, int deep = 0, DWORD tv=0xffffffff) 
			{scope = s; depth = deep; this->tv = tv;}
		void SetScope(int s) { scope = s; }
		int Scope() { return scope; }
		void IncDepth() { depth++; }
		void DecDepth() { depth--; }
		int Depth() { return depth; }
		DWORD TVBits() {return tv;}
		virtual int proc(Animatable *anim, Animatable *client, int subNum)=0;
	};

// A usefule enumeration
class ClearAnimFlagEnumProc : public AnimEnum {
		DWORD flag;
	public:
		ClearAnimFlagEnumProc(DWORD f) {flag=f;}
		int proc(Animatable *anim, Animatable *client, int subNum) {
			anim->ClearAFlag(flag);
			return ANIM_ENUM_PROCEED;
			}
	};


//
// The is used by the two functions GetSubObjectCenters() and
// GetSubObjectTMs() found in the classes BaseObject and Control.
//
class SubObjAxisCallback {
	public:
		virtual void Center(Point3 c,int id)=0;
		virtual void TM(Matrix3 tm,int id)=0;
		virtual int Type()=0;
	};

// Values returned by Type();
#define SO_CENTER_SELECTION	1 
#define SO_CENTER_PIVOT		2


// --- AppData ---------------------------------------------

// An individual app data chunk
class AppDataChunk {
	public:
		// Note that data pointer should be allocated with standard malloc
		// since it will be freed in the destructor.
		AppDataChunk(Class_ID cid, SClass_ID sid, DWORD sbid, DWORD len, void *d)
			{classID=cid; superClassID=sid; subID=sbid; length=len; data=d;}
		AppDataChunk() {length=0;data=NULL;}

		~AppDataChunk() {
			if (data) free(data);
			}		

		// The super class and class IDs of the object that
		// is the owner of this chunk.
		Class_ID  classID;
		SClass_ID superClassID;

		// An extra ID that lets the owner identify its sub chunks.
		DWORD subID;
		
		// The chunk data itself
		DWORD length;
		void *data;

		// IO
		CoreExport IOResult Load(ILoad *iload);
		CoreExport IOResult Save(ISave *isave);
	};


// This list is maintained by the systems. Plug-ins need not concern themselves with it.
class AnimAppData : public AnimProperty {
	public:				
		Tab<AppDataChunk*> chunks;
		CRITICAL_SECTION csect;
		AppDataChunk *lastSearch;

		DWORD ID() {return PROPID_APPDATA;}		
		CoreExport ~AnimAppData();
		CoreExport AnimAppData();

		CoreExport AppDataChunk *FindChunk(Class_ID cid, SClass_ID sid, DWORD sbid);
		void AddChunk(AppDataChunk *newChunk) {chunks.Append(1,&newChunk);}
		CoreExport BOOL RemoveChunk(Class_ID cid, SClass_ID sid, DWORD sbid);

		CoreExport IOResult Load(ILoad *iload);
		CoreExport IOResult Save(ISave *isave);
	};

static const DWORD SV_NO_REF_INDEX = 0xFFFFFFFF;
class SchematicViewProperty : public AnimProperty
	{
	private:
	DWORD nodeRefIndex;

	public:				
	DWORD ID() { return PROPID_SVDATA; }
	CoreExport ~SchematicViewProperty() {}
	CoreExport SchematicViewProperty();

	DWORD GetRefIndex();
	void SetRefIndex(DWORD refIndex);
	bool GetRefSaveMark();

	CoreExport IOResult Load(ILoad *iload);
	CoreExport IOResult Save(ISave *isave);
	};
 
CoreExport void SetLockFailureLevel(int level);
CoreExport int GetLockFailureLevel();

class Interface;
CoreExport Interface *GetCOREInterface();
				
// JBW: macrorecorder.  For the macro-recorder to establish itself with CORE
class MacroRecorder;
CoreExport void SetMacroRecorderInterface(MacroRecorder* mri);
// JBW: to set the CIU macro scrit directory ref in CORE
class MacroDir;
CoreExport void SetMacroScriptInterface(MacroDir* msd);
						
// This API allows plug-in to query various system settings.
CoreExport int GetSystemSetting(int id);

// Values to pass to GetSystemSetting():

// Are editable meshes enabled?
#define SYSSET_ENABLE_EDITABLEMESH		1

// When GetSystemSetting is called with this the undo buffer is
// cleared. GetSystemSetting will return 0.
// Note that this will only work with version 1.1 of MAX or later.
#define SYSSET_CLEAR_UNDO				2


// Are keyboard accelerators enabled for the editable mesh.
#define SYSSET_EDITABLEMESH_ENABLE_KEYBOARD_ACCEL	3

// Is the edit meh modifier enabled?
#define SYSSET_ENABLE_EDITMESHMOD	4


// Returns the state of the VERSION_3DSMAX #define from PLUGAPI.H
// when the running version of MAX was compiled.
CoreExport DWORD Get3DSMAXVersion();


// Special access to the MAX INI file for motion capture
#define MCAP_INI_CHANNEL	1
#define MCAP_INI_PRESET		2
#define MCAP_INI_STOP		3
#define MCAP_INI_PLAY		4
#define MCAP_INI_RECORD		5
#define MCAP_INI_SSENABLE	6

CoreExport int GetMotionCaptureINISetting(int ID);
CoreExport void SetMotionCaptureINISetting(int ID, int val);

// CoreExecute: generic expansion capability
CoreExport int CoreExecute(int cmd, ULONG arg1=0, ULONG arg2=0, ULONG arg3=0);

#endif // _ANIMTBL_H_

