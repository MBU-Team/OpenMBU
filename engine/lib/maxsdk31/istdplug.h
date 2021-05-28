/**********************************************************************
 *<
	FILE: istdplug.h

	DESCRIPTION:  Interfaces into some of the standard plug-ins 
	              that ship with MAX

	CREATED BY: Rolf Berteig	

	HISTORY: created 20 January 1996

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __ISTDPLUG__
#define __ISTDPLUG__

//----------------------------------------------------------------
// The following are parameter block IDs for procedural objects

// Arc
#define ARC_RADIUS		0
#define ARC_FROM		1
#define ARC_TO			2
#define ARC_PIE			3
#define ARC_REVERSE		4

// Box object
#define BOXOBJ_LENGTH	0
#define BOXOBJ_WIDTH	1
#define BOXOBJ_HEIGHT	2
#define BOXOBJ_WSEGS	3
#define BOXOBJ_LSEGS	4
#define BOXOBJ_HSEGS	5
#define BOXOBJ_GENUVS	6

// Circle
#define CIRCLE_RADIUS		0

// Cone
#define CONE_RADIUS1		0
#define CONE_RADIUS2		1
#define CONE_HEIGHT			2
#define CONE_SEGMENTS		3
#define CONE_CAPSEGMENTS	4
#define CONE_SIDES			5
#define CONE_SMOOTH			6
#define CONE_SLICEON		7
#define CONE_PIESLICE1		8
#define CONE_PIESLICE2		9
#define CONE_GENUVS			10

// Cylinder
#define CYLINDER_RADIUS			0
#define CYLINDER_HEIGHT			1
#define CYLINDER_SEGMENTS		2
#define CYLINDER_CAPSEGMENTS	3
#define CYLINDER_SIDES			4
#define CYLINDER_SMOOTH			5
#define CYLINDER_SLICEON		6
#define CYLINDER_PIESLICE1		7
#define CYLINDER_PIESLICE2		8
#define CYLINDER_GENUVS			9

// Donut
#define DONUT_RADIUS1		0
#define DONUT_RADIUS2		1

// Ellipse
#define ELLIPSE_LENGTH		0
#define ELLIPSE_WIDTH		1

// Hedra
#define HEDRA_RADIUS	0
#define HEDRA_FAMILY	1
#define HEDRA_P			2
#define HEDRA_Q			3
#define HEDRA_SCALEP	4
#define HEDRA_SCALEQ	5
#define HEDRA_SCALER	6
#define HEDRA_VERTS		7
#define HEDRA_GENUVS	8

// Helix
#define HELIX_RADIUS1		0
#define HELIX_RADIUS2		1
#define HELIX_HEIGHT		2
#define HELIX_TURNS			3
#define HELIX_BIAS			4
#define HELIX_DIRECTION		5

// NGon
#define NGON_RADIUS			0
#define NGON_SIDES			1
#define NGON_CIRCULAR		2

// PatchGrid
#define PATCHGRID_LENGTH	0
#define PATCHGRID_WIDTH		1
#define PATCHGRID_WSEGS		2
#define PATCHGRID_LSEGS		3
#define PATCHGRID_TEXTURE	4

// Rain/snow
#define RSPART_VPTPARTICLES		0
#define RSPART_RNDPARTICLES		1
#define RSPART_DROPSIZE			2
#define RSPART_SPEED			3
#define RSPART_VARIATION		4
#define RSPART_DISPTYPE			5
#define RSPART_STARTTIME		6
#define RSPART_LIFETIME			7
#define RSPART_EMITTERWIDTH		8
#define RSPART_EMITTERHEIGHT	9
#define RSPART_HIDEEMITTER		10
#define RSPART_BIRTHRATE		11
#define RSPART_CONSTANT			12
#define RSPART_RENDER			13
#define RSPART_TUMBLE			14
#define RSPART_SCALE			15

// Rectangle
#define RECTANGLE_LENGTH	0
#define RECTANGLE_WIDTH		1
#define RECTANGLE_FILLET	2

// Sphere
#define SPHERE_RADIUS	0
#define SPHERE_SEGS		1
#define SPHERE_SMOOTH	2
#define SPHERE_HEMI		3
#define SPHERE_SQUASH	4
#define SPHERE_RECENTER	5
#define SPHERE_GENUVS	6

// Star
#define START_RADIUS1		0
#define START_RADIUS2		1
#define START_POINTS		2
#define START_DISTORT		3
#define START_FILLET1		4
#define START_FILLET2		5


// Tea Pot
#define TEAPOT_RADIUS	0
#define TEAPOT_SEGS		1
#define TEAPOT_SMOOTH	2
#define TEAPOT_TEAPART	3
#define TEAPOT_BODY		4
#define TEAPOT_HANDLE	5
#define TEAPOT_SPOUT	6
#define TEAPOT_LID		7
#define TEAPOT_GENUVS	8


// Text
#define TEXT_SIZE 0
#define TEXT_KERNING 1
#define TEXT_LEADING 2

// torus
#define TORUS_RADIUS		0
#define TORUS_RADIUS2		1
#define TORUS_ROTATION		2
#define TORUS_TWIST			3
#define TORUS_SEGMENTS		4
#define TORUS_SIDES			5
#define TORUS_SMOOTH		6
#define TORUS_SLICEON		7
#define TORUS_PIESLICE1		8
#define TORUS_PIESLICE2		9
#define TORUS_GENUVS		10

// Tube
#define TUBE_RADIUS			0
#define TUBE_RADIUS2		1
#define TUBE_HEIGHT			2
#define TUBE_SEGMENTS		3
#define TUBE_CAPSEGMENTS	4
#define TUBE_SIDES			5
#define TUBE_SMOOTH			6
#define TUBE_SLICEON		7
#define TUBE_PIESLICE1		8
#define TUBE_PIESLICE2		9
#define TUBE_GENUVS			10

// Grid
#define GRIDHELP_LENGTH			0
#define GRIDHELP_WIDTH			1
#define GRIDHELP_GRID			2


//----------------------------------------------------------------
// The following are parameter block IDs for modifiers

// Bend
#define BEND_ANGLE		0
#define BEND_DIR		1
#define BEND_AXIS		2
#define BEND_DOREGION	3
#define BEND_FROM		4
#define BEND_TO			5

// Bomb
#define BOMB_STRENGTH		0
#define BOMB_GRAVITY		1
#define BOMB_CHAOS			2
#define BOMB_DETONATION		3

// Deflector
#define DEFLECTOR_BOUNCE	0
#define DEFLECTOR_WIDTH		1
#define DEFLECTOR_HEIGHT	2

// Displace (modifier and space warp object)
#define DISPLACE_MAPTYPE		0
#define DISPLACE_UTILE			1
#define DISPLACE_VTILE			2
#define DISPLACE_WTILE			3
#define DISPLACE_BLUR			4
#define DISPLACE_USEMAP			5
#define DISPLACE_APPLYMAP		6
#define DISPLACE_STRENGTH		7
#define DISPLACE_DECAY			8
#define DISPLACE_CENTERLUM		9
#define DISPLACE_UFLIP			10
#define DISPLACE_VFLIP			11
#define DISPLACE_WFLIP			12
#define DISPLACE_CENTERL		13
#define DISPLACE_CAP			14
#define DISPLACE_LENGTH			15
#define DISPLACE_WIDTH			16
#define DISPLACE_HEIGHT			17
#define DISPLACE_AXIS			18


// Extrude
#define EXTRUDE_AMOUNT			0
#define EXTRUDE_SEGS			1
#define EXTRUDE_CAPSTART		2
#define EXTRUDE_CAPEND			3
#define EXTRUDE_CAPTYPE			4
#define EXTRUDE_OUTPUT			5
#define EXTRUDE_MAPPING			6
#define EXTRUDE_GEN_MATIDS		7
#define EXTRUDE_USE_SHAPEIDS	8
#define EXTRUDE_SMOOTH			9

// Gravity
#define GRAVITY_STRENGTH		0
#define GRAVITY_DECAY			1
#define GRAVITY_TYPE			2
#define GRAVITY_DISPLENGTH		3

// Wind
#define WIND_STRENGTH		0
#define WIND_DECAY			1
#define WIND_TYPE			2
#define WIND_DISPLENGTH		3
#define WIND_TURBULENCE		4
#define WIND_FREQUENCY		5
#define WIND_SCALE			6

// UVW map
#define UVWMAP_MAPTYPE		0
#define UVWMAP_UTILE		1
#define UVWMAP_VTILE		2
#define UVWMAP_WTILE		3
#define UVWMAP_UFLIP		4
#define UVWMAP_VFLIP		5
#define UVWMAP_WFLIP		6
#define UVWMAP_CAP			7
#define UVWMAP_CHANNEL		8
#define UVWMAP_LENGTH		9
#define UVWMAP_WIDTH		10
#define UVWMAP_HEIGHT		11
#define UVWMAP_AXIS			12


// Noise mod
#define NOISEMOD_SEED			0
#define NOISEMOD_SCALE			1
#define NOISEMOD_FRACTAL		2
#define NOISEMOD_ROUGH			3
#define NOISEMOD_ITERATIONS		4
#define NOISEMOD_ANIMATE		5
#define NOISEMOD_FREQ			6
#define NOISEMOD_PHASE			7
#define NOISEMOD_STRENGTH		8

// Optimize
#define OPTMOD_RENDER			0
#define OPTMOD_VIEWS			1
#define OPTMOD_FACETHRESH1		2
#define OPTMOD_EDGETHRESH1		3
#define OPTMOD_BIAS1			4
#define OPTMOD_PRESERVEMAT1		5
#define OPTMOD_PRESERVESMOOTH1	6
#define OPTMOD_MAXEDGE1			7
#define OPTMOD_FACETHRESH2		8
#define OPTMOD_EDGETHRESH2		9
#define OPTMOD_BIAS2			10
#define OPTMOD_PRESERVEMAT2		11
#define OPTMOD_PRESERVESMOOTH2	12
#define OPTMOD_MAXEDGE2			13
#define OPTMOD_AUTOEDGE			14
#define OPTMOD_MANUPDATE		15

// Volume selection modifier
#define VOLSEL_LEVEL	0
#define VOLSEL_METHOD	1
#define VOLSEL_TYPE		2
#define VOLSEL_VOLUME	3
#define VOLSEL_INVERT	4

// Ripple/Wave space warp object and object space modifier
#define RWAVE_AMPLITUDE		0
#define RWAVE_AMPLITUDE2	1
#define RWAVE_WAVELEN		2
#define RWAVE_PHASE			3
#define RWAVE_DECAY			4

#define RWAVE_CIRCLES		5 // These last three are only valid for space warp objects
#define RWAVE_SEGMENTS		6
#define RWAVE_DIVISIONS		7

// Ripple/Wave binding (modifier)
#define RWAVE_FLEX			0

// Skew
#define SKEW_AMOUNT		0
#define SKEW_DIR		1
#define SKEW_AXIS		2
#define SKEW_DOREGION	3
#define SKEW_FROM		4
#define SKEW_TO			5

// Material modifier
#define MATMOD_MATID 0

// Smoothing group modifier
#define SMOOTHMOD_AUTOSMOOTH 	0
#define SMOOTHMOD_THRESHOLD		1
#define SMOOTHMOD_SMOOTHBITS	2

// Normal modifier
#define NORMMOD_UNIFY	0
#define NORMMOD_FLIP 	1

// SurfRev (Lathe) modifier
#define SURFREV_DEGREES			0
#define SURFREV_SEGS			1
#define SURFREV_CAPSTART		2
#define SURFREV_CAPEND			3
#define SURFREV_CAPTYPE			4
#define SURFREV_WELDCORE		5
#define SURFREV_OUTPUT			6
#define SURFREV_MAPPING			7
// Taper 
#define TAPER_AMT			0
#define TAPER_CRV			1
#define TAPER_AXIS			2
#define TAPER_EFFECTAXIS	3
#define TAPER_SYMMETRY		4
#define TAPER_DOREGION		5
#define TAPER_FROM			6
#define TAPER_TO			7

// Twist
#define TWIST_ANGLE		0
#define TWIST_BIAS		1
#define TWIST_AXIS		2
#define TWIST_DOREGION	3
#define TWIST_FROM		4
#define TWIST_TO		5

// Material mod
#define MATMOD_MATID	0

// Smooth mod
#define SMOOTH_AUTOSMOOTH 	0
#define SMOOTH_THRESHOLD	1
#define SMOOTH_SMOOTHBITS	2

// Normal mod
#define NORMALMOD_UNIFY		0
#define NORMALMOD_FLIP 		1

// Tessellation mod
#define TESSMOD_TYPE		0
#define TESSMOD_TENSION		1
#define TESSMOD_ITERATIONS	2
#define TESSMOD_FACE_TYPE	3

// UVW xform
#define UVWXFORM_UTILE		0
#define UVWXFORM_VTILE		1
#define UVWXFORM_WTILE		2
#define UVWXFORM_UOFFSET	3
#define UVWXFORM_VOFFSET	4
#define UVWXFORM_WOFFSET	5
#define UVWXFORM_UFLIP		6
#define UVWXFORM_VFLIP		7
#define UVWXFORM_WFLIP		8
#define UVWXFORM_CHANNEL	9


//-- Text shape object interface -------------------------

// Use GetTextObjectInterface() to get a pointer to an 
// ITextObject given a pointer to an Object. 


// Flags passed to ChangeFont()
#define TEXTOBJ_ITALIC		(1<<1)
#define TEXTOBJ_UNDERLINE	(1<<2)

// Alignment types
#define TEXTOBJ_LEFT 0
#define TEXTOBJ_CENTER 1
#define TEXTOBJ_RIGHT 2
#define TEXTOBJ_JUSTIFIED 3

class ITextObject {
	public:
		// Returns TRUE if string is changed. Can't change string if current font is not installed
		virtual BOOL ChangeText(TSTR string)=0;
		
		// Returns TRUE if font is successfully changed.
		virtual BOOL ChangeFont(TSTR name, DWORD flags)=0;

		// Get fount and string
		virtual TSTR GetFont()=0;
		virtual TSTR GetString()=0;
		
		// Get/Set styles
		virtual BOOL GetItalic()=0;
		virtual BOOL GetUnderline()=0;
		virtual void SetItalic(BOOL sw)=0;
		virtual void SetUnderline(BOOL sw)=0;

		// Get/Set alignment
		virtual BOOL SetAlignment(int type)=0;
		virtual int GetAlignment()=0;
	};



//-- Controller interfaces -------------------------------

// Base key class
class IKey {
	public:
		TimeValue time;
		DWORD flags;
		IKey() {time=0;flags=0;}
	};

//--- TCB keys -------------

class ITCBKey : public IKey {
	public:		
		float tens, cont, bias, easeIn, easeOut;
	};

class ITCBFloatKey : public ITCBKey {
	public:
		float val;		
	};

class ITCBPoint3Key : public ITCBKey {
	public:
		Point3 val;		
	};

class ITCBRotKey : public ITCBKey {
	public:
		AngAxis val;		
	};

class ITCBScaleKey : public ITCBKey {
	public:
		ScaleValue val;		
	};


//--- Bezier keys -------------

class IBezFloatKey : public IKey {
	public:
		float intan, outtan;
		float val;
	};

class IBezPoint3Key : public IKey {
	public:
		Point3 intan, outtan;
		Point3 val;
	};

class IBezQuatKey : public IKey  {
	public:		
		Quat val;
	};

class IBezScaleKey : public IKey  {
	public:
		Point3 intan, outtan;
		ScaleValue val;
	};


//--- Linear Keys --------------

class ILinFloatKey : public IKey {
	public:
		float val;
	};

class ILinPoint3Key : public IKey {
	public:
		Point3 val;
	};

class ILinRotKey : public IKey {
	public:
		Quat val;
	};

class ILinScaleKey : public IKey {
	public:
		ScaleValue val;
	};


// --- Flag bits for keys -------------------------------

// General flags
#define IKEY_SELECTED	(1<<0)
#define IKEY_XSEL		(1<<1)
#define IKEY_YSEL		(1<<2)
#define IKEY_ZSEL		(1<<3)
#define IKEY_FLAGGED	(1<<13)
#define IKEY_TIME_LOCK	(1<<14)

// TCB specific key flags
#define TCBKEY_QUATVALID	(1<<4) // When this bit is set the angle/axis is derived from the quat instead of vice/versa

// Bezier specific key flags
#define BEZKEY_XBROKEN		(1<<4) // Broken means not locked
#define BEZKEY_YBROKEN		(1<<5)
#define BEZKEY_ZBROKEN		(1<<6)

// The in and out types are stored in bits 7-13
#define BEZKEY_NUMTYPEBITS	3
#define BEZKEY_INTYPESHIFT	7
#define	BEZKEY_OUTTYPESHIFT	(BEZKEY_INTYPESHIFT+BEZKEY_NUMTYPEBITS)
#define BEZKEY_TYPEMASK		7

// Bezier tangent types
#define BEZKEY_SMOOTH	0
#define BEZKEY_LINEAR	1
#define BEZKEY_STEP		2
#define BEZKEY_FAST		3
#define BEZKEY_SLOW		4
#define BEZKEY_USER		5

#define NUM_TANGENTTYPES	6

// This key is interpolated using arclength as the interpolation parameter
#define BEZKEY_CONSTVELOCITY	(1<<15)

#define TangentsLocked(f,j) (!(f&(BEZKEY_XBROKEN<<j)))
#define SetTangentLock(f,j,l) {if (l) (f)=(f)&(~(BEZKEY_XBROKEN<<j)); else (f)|=(BEZKEY_XBROKEN<<j);}

// Macros to access hybrid tangent types
#define GetInTanType(f)  int(((f)>>BEZKEY_INTYPESHIFT)&BEZKEY_TYPEMASK)
#define GetOutTanType(f) int(((f)>>BEZKEY_OUTTYPESHIFT)&BEZKEY_TYPEMASK)
#define SetInTanType(f,t)  {(f) = ((f)&(~(BEZKEY_TYPEMASK<<BEZKEY_INTYPESHIFT)))|(t<<BEZKEY_INTYPESHIFT);}
#define SetOutTanType(f,t) {(f) = ((f)&(~(BEZKEY_TYPEMASK<<BEZKEY_OUTTYPESHIFT)))|(t<<BEZKEY_OUTTYPESHIFT);}


// Track flags
#define TFLAG_CURVESEL			(1<<0)
#define TFLAG_RANGE_UNLOCKED	(1<<1)
#define TFLAG_LOOPEDIN			(1<<3)
#define TFLAG_LOOPEDOUT			(1<<4)
#define TFLAG_COLOR				(1<<5)	// Set for Bezier Point3 controlers that are color controllers
#define TFLAG_HSV				(1<<6)	// Set for color controls that interpolate in HSV


//-------------------------------------------------------
// This is an interface into either a TCB or Bezier key
// frame controller. It is up to the client to make sure
// that the IKey* point to a key of the approriate derived
// class based on the ClassID() of the controller.

class IKeyControl {
	public:
		// Total number of keys.
		virtual int GetNumKeys()=0;
		
		// Sets the number of keys allocated. 
		// May add blank keys or delete existing keys
		virtual void SetNumKeys(int n)=0;
		
		// Fill in 'key' with the ith key
		virtual void GetKey(int i,IKey *key)=0;
		
		// Set the ith key
		virtual void SetKey(int i,IKey *key)=0;

		// Append a new key onto the end. Note that the
		// key list will ultimately be sorted by time. Returns
		// the key's index.
		virtual int AppendKey(IKey *key)=0;

		// If any changes are made that would require the keys to be sorted
		// this method should be called.
		virtual void SortKeys()=0;		

		// Access track flags
		virtual DWORD &GetTrackFlags()=0;
	};

// To get a pointer to the above interface given a pointer to a controller
// use the macro defined in animtbl.h: GetKeyControlInterface()

// Access to default key parameters
CoreExport void SetBezierDefaultTangentType(int in, int out);
CoreExport void GetBezierDefaultTangentType(int &in, int &out);

CoreExport void SetTCBDefaultParams(float t, float c, float b, float easeIn, float easeOut);
CoreExport void GetTCBDefaultParams(float &t, float &c, float &b, float &easeIn, float &easeOut);


//-----------------------------------------------------------
// A plug-in can register itself to read a particular APP_DATA 
// chunk when a 3DS file is loaded. If a chunk is encountered
// that matches a registered plug-in, that plug-in will be
// asked to create an instance of itself based on the contents
// of the APP_DATA chunk.

class TriObject;

class ObjectDataReaderCallback {
	public:
		// Chunk name
		virtual char *DataName()=0;

		// Create an instance of an object based on the data and the original mesh object
		virtual Object *ReadData(TriObject *obj, void *data, DWORD len)=0;

		virtual void DeleteThis()=0;
	};
 
CoreExport void RegisterObjectAppDataReader(ObjectDataReaderCallback *cb);
 
CoreExport Object *ObjectFromAppData(TriObject *obj, char *name, void *data, DWORD len);


// Note about 3DS App Data:
// If app data is encountered and no plug-in has registered to
// convert it, then it is just hung off the object (or INode in
// the case of KXP app data).
// For object app data, TriObject's super class and class ID are used
// to identify the chunk and the sub ID is set to 0.
// For node app data, INode's super class and class ID are used
// to identify the chunk and the sub ID is set to 0.
//
// This single MAX app data chunk will contain the entire
// 3DS app data chunk, which may have sub chunks (see IPAS SDK).
// The following routines will aid in parsing 3DS app data.

// Get the ID string out of an XDATA_ENTRY chunk and null terminates it
CoreExport void GetIDStr(char *chunk, char *idstring);

// Returns the offset into 'appd' of the specified chunk 
// or -1 if it is not found
CoreExport int FindAppDataChunk(void *appd, DWORD len, char *idstring);

// Similar to Find, but actually returns a pointer to the chunk
// or NULL if it is not found
CoreExport void *GetAppDataChunk(void *appd, DWORD len, char *idstring);

// Adds the chunk to the appdata chunk, preserving existing chunks.
// 'chunk' should point to the new chunk header followed by its data.
CoreExport int SetAppDataChunk(void **pappd, DWORD &len, void *chunk);

// Deletes a chunk from the appdata while preserving other chunks.
CoreExport int DeleteAppDataChunk(void **pappd, DWORD &len, char *idstring);

// Known sub chunks inside an appdata chunk
#define XDATA_ENTRY		0x8001
#define XDATA_APPNAME	0x8002



//---------------------------------------------------------
// Interface into MAX's default WAV sound object
// use the Interface method GetSoundObject() to get a pointer
// to the current sound object and then use the
// GetWaveSoundInterface() on the result to see if it supports
// this interface.

class IWaveSound {
	public:
		// Retreives the name of the current sound file
		virtual TSTR GetSoundFileName()=0;
		
		// Sets the sound file. This will cause the WAV to
		// be loaded into the tack view. Returns FALSE if
		// the file can't be opened or no wave track exist.
		virtual BOOL SetSoundFileName(TSTR name)=0;

		// Set the time offset for the wave
		virtual void SetStartTime(TimeValue t)=0;

		// Get the time offset for the wave
		virtual TimeValue GetStartTime()=0;		
		virtual TimeValue GetEndTime()=0;
	};


//-----------------------------------------------------------
//
// Access to the boolean object's parameters. Given a pointer to
// an object whose class ID is Class_ID(BOOLOBJ_CLASS_ID,0) or
// NEWBOOL_CLASS_ID, you can cast that pointer to the following
// class.  Note that some options do not work in the old Boolean
// (BOOLOBJ_CLASS_ID), and there is no Optimize parameter in
// the new Boolean.
//

#define BOOLOP_UNION			0
#define BOOLOP_INTERSECTION		1
#define BOOLOP_SUB_AB			2
#define BOOLOP_SUB_BA			3
#define BOOLOP_CUT				4

#define BOOLOP_CUT_REFINE  0
#define BOOLOP_CUT_SEPARATE  1
#define BOOLOP_CUT_REMOVE_IN  2
#define BOOLOP_CUT_REMOVE_OUT  3

#define BOOLUPDATE_ALWAYS		0
#define BOOLUPDATE_SELECTED		1
#define BOOLUPDATE_RENDER		2
#define BOOLUPDATE_MANUAL		3

#define BOOL_ADDOP_REFERENCE 0
#define BOOL_ADDOP_INSTANCE 1
#define BOOL_ADDOP_COPY 2
#define BOOL_ADDOP_MOVE 3

#define BOOL_MAT_NO_MODIFY 0
#define BOOL_MAT_IDTOMAT 1
#define BOOL_MAT_MATTOID 2
#define BOOL_MAT_DISCARD_ORIG 3
#define BOOL_MAT_DISCARD_NEW 4

class IBoolObject : public GeomObject {
public:
	virtual BOOL GetOperandSel(int which)=0;
	virtual void SetOperandSel(int which,BOOL sel)=0;
	virtual int GetBoolOp()=0;
	virtual void SetBoolOp(int op)=0;
	virtual int GetBoolCutType()=0;
	virtual void SetBoolCutType(int ct)=0;
	virtual BOOL GetDisplayResult()=0;
	virtual void SetDisplayResult(BOOL onOff)=0;
	virtual BOOL GetShowHiddenOps()=0;
	virtual void SetShowHiddenOps(BOOL onOff)=0;
	virtual int GetUpdateMode()=0;
	virtual void SetUpdateMode(int mode)=0;
	virtual BOOL GetOptimize()=0;
	virtual void SetOptimize(BOOL onOff)=0;
	virtual void SetOperandA (TimeValue t, INode *node)=0;
	virtual void SetOperandB (TimeValue t, INode *node, INode *boolNode,
		int addOpMethod=0, int matMergeMethod=0, bool *canUndo=NULL)=0;
};

// The boolean object has five references. 2 references to the
// operand objects, 2 references to transform controllers 
// providing a transformation matrix for the 2 operands,
// and one to the parameter block.
#define BOOLREF_OBJECT1		0
#define BOOLREF_OBJECT2		1
#define BOOLREF_CONT1		2
#define BOOLREF_CONT2		3
#define BOOLREF_PBLOCK     4

//-------------------------------------------------------------
// Access to path controller's parameters.
//

class IPathPosition : public Control {
	public:
		virtual BOOL SetPathNode(INode *node)=0;
		virtual void SetFollow(BOOL f)=0;
		virtual BOOL GetFollow()=0;
		virtual void SetBankAmount(float a)=0;
		virtual float GetBankAmount()=0;
		virtual void SetBank(BOOL b)=0;
		virtual BOOL GetBank()=0;
		virtual void SetTracking(float t)=0;		// smoothness
		virtual float GetTracking()=0;
		virtual void SetAllowFlip(BOOL f)=0;
		virtual BOOL GetAllowFlip()=0;
		virtual void SetConstVel(BOOL cv)=0;
		virtual BOOL GetConstVel()=0;
		virtual void SetFlip(BOOL onOff)=0;
		virtual BOOL GetFlip()=0;
		virtual void SetAxis(int axis)=0;
		virtual int GetAxis()=0;
	};

// Bank and tracking are scaled in the UI.
#define BANKSCALE 100.0f
#define FromBankUI(a) ((a)*BANKSCALE)
#define ToBankUI(a)	  ((a)/BANKSCALE)

#define TRACKSCALE 0.04f
#define FromTrackUI(a) ((a)*TRACKSCALE)
#define ToTrackUI(a)   ((a)/TRACKSCALE)

// percent controller, path node and paramblock2 refs
// #define PATHPOS_PERCENT_REF	0    // obsolete in Ed. 2, percent is an animatable in the ParamBlock
#define PATHPOS_PATH_REF	1
#define PATHPOS_PBLOCK_REF	2

//-------------------------------------------------------------
// Access to noise controller's parameters.
// All noise controllers are derived from this class
//

class INoiseControl : public StdControl {
	public:
		virtual void SetSeed(int seed)=0;
		virtual int GetSeed()=0;
		virtual void SetFrequency(float f)=0;
		virtual float GetFrequency()=0;
		virtual void SetFractal(BOOL f)=0;
		virtual BOOL GetFractal()=0;
		virtual void SetRoughness(float f)=0;
		virtual float GetRoughness()=0;
		virtual void SetRampIn(TimeValue in)=0;
		virtual TimeValue GetRampIn()=0;
		virtual void SetRampOut(TimeValue out)=0;
		virtual TimeValue GetRampOut()=0;
		virtual void SetPositiveOnly(int which,BOOL onOff)=0;
		virtual BOOL GetPositiveOnly(int which)=0;
		virtual Control *GetStrengthController()=0;
		virtual void SetStrengthController(Control *c)=0;
	};

//-------------------------------------------------------------
// Access to SurfPosition controller
//

class ISurfPosition : public Control {
	public:
		virtual void SetSurface(INode *node)=0;
		virtual int GetAlign()=0;
		virtual void SetAlign(int a)=0;
		virtual BOOL GetFlip()=0;
		virtual void SetFlip(BOOL f)=0;
	};

// Surface controller references
#define SURFCONT_U_REF			0
#define SURFCONT_V_REF			1
#define SURFCONT_SURFOBJ_REF	2


//-------------------------------------------------------------
// Access to the LinkCtrl
//

class ILinkCtrl : public Control {
	public:
		virtual int GetParentCount()=0;
		virtual TimeValue GetLinkTime(int i)=0;
		virtual void SetLinkTime(int i,TimeValue t)=0;
		virtual void LinkTimeChanged()=0; // call after changing link times
		virtual void AddNewLink(INode *node,TimeValue t)=0;
		virtual void DeleteLink(int i)=0;
	};

// LinkCtrl references
#define LINKCTRL_CONTROL_REF		0	// the TM controller
#define LINKCTRL_FIRSTPARENT_REF	1	// parent nodes... refs 1-n


//-------------------------------------------------------------
// Access to LookatControl
//

class ILookatControl : public Control {
	public:
		virtual void SetFlip(BOOL f)=0;
		virtual BOOL GetFlip()=0;
		virtual void SetAxis(int a)=0;
		virtual int GetAxis()=0;
	};

// References for the lookat controller
#define LOOKAT_TARGET_REF	0
#define LOOKAT_POS_REF		1
#define LOOKAT_ROLL_REF		2
#define LOOKAT_SCL_REF		3


//-------------------------------------------------------------
// Access to FFD modifiers
//

// Can either be casted to IFFDMod<Modifier> or IFFDMod<WSMObject> based on the ClassID
template <class T>
class IFFDMod : public T {
	public:
		virtual int			NumPts()=0;								// number of lattice control points 
		virtual int			NumPtConts()=0;							// number of CP's having controllers
		virtual Control*	GetPtCont(int i)=0;						// get i'th CP controller
		virtual void		SetPtCont(int i,Control *c)=0;			// set i'th CP controller
		virtual Point3		GetPt(int i)=0;							// get i'th CP
		virtual	void		SetPt(int i, Point3 p)=0;				// set i'th CP
		virtual	void		SetGridDim(IPoint3 d) { }				// set the lattice dimensions
		virtual	IPoint3		GetGridDim() { return IPoint3(0,0,0); }	// get the lattice dimensions
		virtual void		AnimateAll() { }						// assign controllers to all CP's				
		virtual	void		Conform() { }							// not valid for WSMObject's
		virtual void		SelectPt(int i, BOOL sel, BOOL clearAll=FALSE) { }
};

//-------------------------------------------------------------
// Access to mesh selections in editable mesh and edit mesh mod
//

#include "namesel.h"

// Selection levels:
#define IMESHSEL_OBJECT 0
#define IMESHSEL_VERTEX 1
#define IMESHSEL_FACE 2
#define IMESHSEL_EDGE 3

class IMeshSelect {
public:
	virtual DWORD GetSelLevel()=0;
	virtual void SetSelLevel(DWORD level)=0;
	virtual void LocalDataChanged()=0;
	virtual BOOL HasWeightedVertSel () { return FALSE; }
	virtual BOOL CanAssignWeightedVertSel () { return FALSE; }
};

class IMeshSelectData {
public:
	virtual BitArray GetVertSel()=0;
	virtual BitArray GetFaceSel()=0;
	virtual BitArray GetEdgeSel()=0;
	
	virtual void SetVertSel(BitArray &set, IMeshSelect *imod, TimeValue t)=0;
	virtual void SetFaceSel(BitArray &set, IMeshSelect *imod, TimeValue t)=0;
	virtual void SetEdgeSel(BitArray &set, IMeshSelect *imod, TimeValue t)=0;

	virtual GenericNamedSelSetList & GetNamedVertSelList ()=0;
	virtual GenericNamedSelSetList & GetNamedEdgeSelList ()=0;
	virtual GenericNamedSelSetList & GetNamedFaceSelList ()=0;

	virtual void GetWeightedVertSel (int nv, float *sel) {}
	virtual void SetWeightedVertSel (int nv, float *sel, IMeshSelect *imod, TimeValue t) {}
};

//-------------------------------------------------------------
// Access to spline selections and operations in SplineShape and EditSplineMod
//

// selection levels defined in splshape.h   

class ISplineSelect					// accessed via GetInterface(I_SPLINESELECT)
{
public:
	virtual DWORD GetSelLevel()=0;
	virtual void SetSelLevel(DWORD level)=0;
	virtual void LocalDataChanged()=0;
};

class ISplineSelectData				// accessed via GetInterface(I_SPLINESELECTDATA)
{
public:
	// access spline sub-object selections, current & named
	virtual BitArray GetVertSel()=0;
	virtual BitArray GetSegmentSel()=0;
	virtual BitArray GetSplineSel()=0;
	
	virtual void SetVertSel(BitArray &set, ISplineSelect *imod, TimeValue t)=0;
	virtual void SetSegmentSel(BitArray &set, ISplineSelect *imod, TimeValue t)=0;
	virtual void SetSplineSel(BitArray &set, ISplineSelect *imod, TimeValue t)=0;

	virtual GenericNamedSelSetList & GetNamedVertSelList ()=0;
	virtual GenericNamedSelSetList & GetNamedSegmentSelList ()=0;
	virtual GenericNamedSelSetList & GetNamedSplineSelList ()=0;
};

enum splineCommandMode { ScmCreateLine, ScmAttach, ScmInsert, ScmConnect, ScmRefine, ScmFillet, ScmChamfer, 
					     ScmBind, ScmRefineConnect, ScmOutline, ScmTrim, ScmExtend, ScmCrossInsert,
						 ScmBreak, ScmUnion, ScmSubtract,  };
enum splineButtonOp    { SopHide, SopUnhideAll, SopDelete, SopDetach, SopDivide, SopCycle,
						 SopUnbind, SopWeld, SopMakeFirst, SopAttachMultiple, SopExplode, SopReverse, 
						 SopClose, SopIntersect, SopMirrorHoriz, SopMirrorVert,
						 SopMirrorBoth, SopSelectByID, };

class ISplineOps				// accessed via GetInterface(I_SPLINEOPS)
{
public:
	// start up interactive command mode, uses mode enum above
	virtual void StartCommandMode(splineCommandMode mode)=0;
	// perform button op, uses op enum above
	virtual void ButtonOp(splineButtonOp opcode)=0;
};

//-------------------------------------------------------------
// Access to spline selections and operations in PatchObject and EditPatchMod
//

// selection levels defined in patchobj.h   

class IPatchSelect					// accessed via GetInterface(I_PATCHSELECT)
{
public:
	virtual DWORD GetSelLevel()=0;
	virtual void SetSelLevel(DWORD level)=0;
	virtual void LocalDataChanged()=0;
};

class IPatchSelectData				// accessed via GetInterface(I_PATCHSELECTDATA)
{
public:
	// access patch sub-object selections, current & named
	virtual BitArray GetVertSel()=0;
	virtual BitArray GetEdgeSel()=0;
	virtual BitArray GetPatchSel()=0;
	
	virtual void SetVertSel(BitArray &set, IPatchSelect *imod, TimeValue t)=0;
	virtual void SetEdgeSel(BitArray &set, IPatchSelect *imod, TimeValue t)=0;
	virtual void SetPatchSel(BitArray &set, IPatchSelect *imod, TimeValue t)=0;

	virtual GenericNamedSelSetList & GetNamedVertSelList ()=0;
	virtual GenericNamedSelSetList & GetNamedEdgeSelList ()=0;
	virtual GenericNamedSelSetList & GetNamedPatchSelList ()=0;
};

enum patchCommandMode { PcmAttach, PcmExtrude, PcmBevel, PcmBind };
enum patchButtonOp    { PopUnbind, PopHide, PopUnhideAll, PopWeld, PopDelete, PopSubdivide,
						PopAddTri, PopAddQuad, PopDetach };

class IPatchOps				// accessed via GetInterface(I_PATCHOPS)
{
public:
	// start up interactive command mode, uses mode enum above
	virtual void StartCommandMode(patchCommandMode mode)=0;
	// perform button op, uses op enum above
	virtual void ButtonOp(patchButtonOp opcode)=0;
};


#endif //__ISTDPLUG__
