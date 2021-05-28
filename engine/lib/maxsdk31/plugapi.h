/*******************************************************************
 *
 *    DESCRIPTION: DLL Plugin API
 *
 *    AUTHOR: Dan Silva
 *
 *    HISTORY: 11/30/94 Started coding
 *
 *******************************************************************/

#include "maxtypes.h"

#ifndef PLUGAPI_H_DEFINED


#define PLUGAPI_H_DEFINED

// 3DSMAX Version number to be compared against that returned by DLL's
// LibVersion function to detect obsolete DLLs.
//#define VERSION_3DSMAX 100      // changed  4/ 9/96 -- DB
//#define VERSION_3DSMAX 110      // changed  7/17/96 -- DS
//#define VERSION_3DSMAX 111      // changed  9/24/96 -- DS
//#define VERSION_3DSMAX 120      // changed  9/30/96 -- DB
//#define VERSION_3DSMAX 200      // changed  10/30/96 -- DS


// MAX release number X 1000
//#define MAX_RELEASE  2000      // DDS 9/30/97  
//#define MAX_RELEASE  2500      // JMK 2/25/98  
//#define MAX_RELEASE  3000        // DDS 9/3/98  
#define MAX_RELEASE  3100        // CCJ 10/21/99  


// This is the max API number.  When a change in the API requires DLL's
// to be recompiled, increment this number and set MAX_SDK_REV to 0
// THis will make all DLL's with the former MAX_API_NUM unloadable.
//#define MAX_API_NUM     4	   // DDS 9/30/97
//#define MAX_API_NUM     5	   // DDS 10/06/97
//#define MAX_API_NUM     6	   // DDS 9/3/98
#define MAX_API_NUM     7	   // CCJ 5/14/99

// This denotes the revision of the SDK for a given API. Increment this
// when the SDK functionality changes in some significant way (for instance
// a new GetProperty() query  response is added), but the headers have 
// not been changed.
#define MAX_SDK_REV     0	  // DDS 9/20/97

// This value should be returned by the LibVersion() method for DLL's:
#define VERSION_3DSMAX ((MAX_RELEASE<<16)+(MAX_API_NUM<<8)+MAX_SDK_REV)
#ifdef DESIGN_VER
#define VERSION_DMAX ((MAX_RELEASE<<16)+(MAX_API_NUM<<8)+MAX_SDK_REV)
#endif

typedef enum {kAPP_NONE, kAPP_MAX, kAPP_VIZ} APPLICATION_ID;

//This method returns the ApplicationID, either VIZ or MAX. If a plugin
//is designed to work only in one product, then you could use this method
//in your IsPublic() call to switch between exposing the plug-in or not.
CoreExport APPLICATION_ID GetAppID();

// Macros for extracting parts of VERSION_3DSMAX
#define GET_MAX_RELEASE(x)  (((x)>>16)&0xffff)
#define GET_MAX_API_NUM(x)  (((x)>>8)&0xff)
#define GET_MAX_SDK_REV(x)  ((x)&0xff)
#define GET_MAX_SDK_NUMREV(x)  ((x)&0xffff) 



// Internal super classes, that plug-in developers need not know about
#define GEN_MODAPP_CLASS_ID 		0x00000b
#define MODAPP_CLASS_ID 			0x00000c
#define OBREF_MODAPP_CLASS_ID 		0x00000d
#define BASENODE_CLASS_ID			0x000001
#define GEN_DERIVOB_CLASS_ID 		0x000002
#define DERIVOB_CLASS_ID 			0x000003
#define WSM_DERIVOB_CLASS_ID 		0x000004
#define PARAMETER_BLOCK_CLASS_ID 	0x000008	
#define PARAMETER_BLOCK2_CLASS_ID 	0x000082	// <JBW> ParamBlocks Ed. 2
#define EASE_LIST_CLASS_ID			0x000009
#define AXIS_DISPLAY_CLASS_ID		0x00000e
#define MULT_LIST_CLASS_ID			0x00000f
#define NOTETRACK_CLASS_ID			0x0000ff
#define TREE_VIEW_CLASS_ID			0xffffff00
#define SCENE_CLASS_ID				0xfffffd00
#define THE_GRIDREF_CLASS_ID		0xfffffe00
#define VIEWREF_CLASS_ID			0xffffff01
#define BITMAPDAD_CLASS_ID			0xffffff02 // for drag/drop of bitmaps
#define PARTICLE_SYS_CLASS_ID		0xffffff03 // NOTE: this is for internal use only. Particle systems return GEOMOBJECT_CLASS_ID -- use IsParticleObject() to determine if an object is a particle system.
#define XREF_CLASS_ID				0xffffff04 // Object XRefs
#define AGGMAN_CLASS_ID				0xffffff05 // Object aggregation, VIZ
#define MAXSCRIPT_WRAPPER_CLASS_ID	0xffffff06 // JBW, 5/14/99, MAX object wrappers within MAXScript

//-------------------------------------------
// Deformable object psuedo-super-class
#define DEFORM_OBJ_CLASS_ID 		0x000005

//-------------------------------------------
// Mappable object psuedo-super-class
#define MAPPABLE_OBJ_CLASS_ID 		0x000006

//-------------------------------------------
// Shape psuedo-super-class
#define GENERIC_SHAPE_CLASS_ID		0x0000ab

//-------------------------------------------
// Super-classes that are plugable.
#define GEOMOBJECT_CLASS_ID			0x000010
#define CAMERA_CLASS_ID				0x000020
#define LIGHT_CLASS_ID				0x000030
#define SHAPE_CLASS_ID				0x000040
#define HELPER_CLASS_ID				0x000050
#define SYSTEM_CLASS_ID	 			0x000060 
#define REF_MAKER_CLASS_ID			0x000100 	
#define REF_TARGET_CLASS_ID	 		0x000200
#define OSM_CLASS_ID				0x000810
#define WSM_CLASS_ID				0x000820
#define WSM_OBJECT_CLASS_ID			0x000830
#define SCENE_IMPORT_CLASS_ID		0x000A10
#define SCENE_EXPORT_CLASS_ID		0x000A20
#define BMM_STORAGE_CLASS_ID		0x000B10
#define BMM_FILTER_CLASS_ID			0x000B20
#define BMM_IO_CLASS_ID				0x000B30
#define BMM_DITHER_CLASS_ID			0x000B40
#define BMM_COLORCUT_CLASS_ID		0x000B50
#define USERDATATYPE_CLASS_ID		0x000B60
#define MATERIAL_CLASS_ID			0x000C00    // Materials
#define TEXMAP_CLASS_ID				0x000C10    // Texture maps
#define UVGEN_CLASS_ID				0x0000C20   // UV Generator
#define XYZGEN_CLASS_ID				0x0000C30   // XYZ Generator
#define TEXOUTPUT_CLASS_ID			0x0000C40   // Texture output filter 
#define SOUNDOBJ_CLASS_ID			0x000D00
#define FLT_CLASS_ID				0x000E00
#define RENDERER_CLASS_ID			0x000F00
#define BEZFONT_LOADER_CLASS_ID		0x001000
#define ATMOSPHERIC_CLASS_ID		0x001010
#define UTILITY_CLASS_ID			0x001020	// Utility plug-ins
#define TRACKVIEW_UTILITY_CLASS_ID	0x001030
#define FRONTEND_CONTROL_CLASS_ID	0x001040
#define MOT_CAP_DEV_CLASS_ID		0x001060
#define MOT_CAP_DEVBINDING_CLASS_ID	0x001050
#define OSNAP_CLASS_ID				0x001070
#define TEXMAP_CONTAINER_CLASS_ID	0x001080    // Texture map container
#define RENDER_EFFECT_CLASS_ID      0x001090    // Render post-effects
#define FILTER_KERNEL_CLASS_ID      0x0010a0    // AA Filter kernel
#define SHADER_CLASS_ID				0x0010b0    // plugable shading into stdmtl
#define COLPICK_CLASS_ID		  	0x0010c0    // color picker
#define SHADOW_TYPE_CLASS_ID		0x0010d0    // shadow generators
//-- GUPSTART
#define GUP_CLASS_ID		  		0x0010e0    // Global Utility Plug-In
//-- GUPEND
#define LAYER_CLASS_ID				0x0010f0
#define SCHEMATICVIEW_UTILITY_CLASS_ID	0x001100
#define SAMPLER_CLASS_ID			0x001110
#define ASSOC_CLASS_ID				0x001120
#define GLOBAL_ASSOC_CLASS_ID		0x001130

// Super-class ID's of various controls
#define	CTRL_SHORT_CLASS_ID 	   	0x9001
#define	CTRL_INTEGER_CLASS_ID		0x9002
#define	CTRL_FLOAT_CLASS_ID			0x9003
#define	CTRL_POINT2_CLASS_ID	   	0x9004
#define	CTRL_POINT3_CLASS_ID	   	0x9005
#define	CTRL_POS_CLASS_ID		   	0x9006
#define	CTRL_QUAT_CLASS_ID			0x9007
#define	CTRL_MATRIX3_CLASS_ID		0x9008
#define	CTRL_COLOR_CLASS_ID     	0x9009	// float color
#define	CTRL_COLOR24_CLASS_ID   	0x900A   // 24 bit color
#define	CTRL_POSITION_CLASS_ID		0x900B
#define	CTRL_ROTATION_CLASS_ID		0x900C
#define	CTRL_SCALE_CLASS_ID			0x900D
#define CTRL_MORPH_CLASS_ID			0x900E
#define CTRL_USERTYPE_CLASS_ID		0x900F  // User defined type
#define CTRL_MASTERPOINT_CLASS_ID	0x9010	
#define MASTERBLOCK_SUPER_CLASS_ID	0x9011

#define PATH_CONTROL_CLASS_ID				0x2011
#define EULER_CONTROL_CLASS_ID				0x2012
#define EXPR_POS_CONTROL_CLASS_ID			0x2013
#define EXPR_P3_CONTROL_CLASS_ID			0x2014
#define EXPR_FLOAT_CONTROL_CLASS_ID			0x2015
#define EXPR_SCALE_CONTROL_CLASS_ID			0x2016
#define EXPR_ROT_CONTROL_CLASS_ID			0x2017
#define LOCAL_EULER_CONTROL_CLASS_ID		0x2018

#define FLOATNOISE_CONTROL_CLASS_ID		0x87a6df24
#define POSITIONNOISE_CONTROL_CLASS_ID	0x87a6df25
#define POINT3NOISE_CONTROL_CLASS_ID	0x87a6df26
#define ROTATIONNOISE_CONTROL_CLASS_ID	0x87a6df27
#define SCALENOISE_CONTROL_CLASS_ID		0x87a6df28
#define SURF_CONTROL_CLASSID			Class_ID(0xe8334011,0xaeb330c8)
#define LINKCTRL_CLASSID				Class_ID(0x873fe764,0xaabe8601)


// Class ID's of built-in classes. The value is the first ULONG of the 
// 8 byte Class ID: the second ULONG is 0 for all built-in classes.
// NOTE: Only built-in classes should have the second ULONG == 0.

//--------------------- subclasses of GEOMOBJECT_CLASS_ID:

// Built into CORE
#define TRIOBJ_CLASS_ID 	 	0x0009	  
#define EDITTRIOBJ_CLASS_ID	0xe44f10b3	// base triangle mesh
#define POLYOBJ_CLASS_ID		0x5d21369a	// polygon mesh
#define PATCHOBJ_CLASS_ID  		0x1030
#define NURBSOBJ_CLASS_ID		0x4135

// Primitives
#define BOXOBJ_CLASS_ID 		0x0010
#define SPHERE_CLASS_ID 		0x0011 
#define CYLINDER_CLASS_ID 		0x0012
#define CONE_CLASS_ID			0xa86c23dd
#define TORUS_CLASS_ID			0x0020
#define TUBE_CLASS_ID			0x7B21
#define HEDRA_CLASS_ID			0xf21c5e23
#define BOOLOBJ_CLASS_ID		0x387BB2BB
// Above used in obselete Boolean -- henceforth use following:
#define NEWBOOL_CLASS_ID Class_ID(0x51db4f2f,0x1c596b1a)

// Class ID for XRef object
#define XREFOBJ_CLASS_ID		0x92aab38c

// Class ID for XRef object
#define XREFOBJ_CLASS_ID		0x92aab38c

//Subclasses of object snaps.
#define GRID_OSNAP_CLASS_ID Class_ID(0x62f565d6, 0x110a1f97)


// The teapot is unique in that it uses both DWORDs in its class IDs
// Note that this is what 3rd party plug-ins SHOULD do.
#define TEAPOT_CLASS_ID1		0xACAD13D3
#define TEAPOT_CLASS_ID2		0xACAD26D9

#define PATCHGRID_CLASS_ID  	0x1070

// Particles
#define RAIN_CLASS_ID			0x9bd61aa0
#define SNOW_CLASS_ID			0x9bd61aa1

// Space Warp Objects
#define WAVEOBJ_CLASS_ID 		0x0013

// The basic lofter class
#define LOFTOBJ_CLASS_ID		0x1035
#define LOFT_DEFCURVE_CLASS_ID	0x1036

// Our implementation of the lofter
#define LOFT_GENERIC_CLASS_ID	0x10B0

#define TARGET_CLASS_ID  		0x1020  // should this be a helper?
#define MORPHOBJ_CLASS_ID		0x1021

// Subclasses of SHAPE_CLASS_ID
#define SPLINESHAPE_CLASS_ID 	0x00000a
#define LINEARSHAPE_CLASS_ID 	0x0000aa
#define SPLINE3D_CLASS_ID  		0x1040
#define NGON_CLASS_ID  			0x1050
#define DONUT_CLASS_ID  		0x1060
#define STAR_CLASS_ID			0x1995
#define RECTANGLE_CLASS_ID		0x1065
#define HELIX_CLASS_ID			0x1994
#define ELLIPSE_CLASS_ID		0x1097
#define CIRCLE_CLASS_ID			0x1999
#define TEXT_CLASS_ID			0x1993
#define ARC_CLASS_ID			0x1996

// subclasses of CAMERA_CLASS_ID:
#define SIMPLE_CAM_CLASS_ID  	0x1001
#define LOOKAT_CAM_CLASS_ID  	0x1002

// subclasses of LIGHT_CLASS_ID:
#define OMNI_LIGHT_CLASS_ID  	0x1011
#define SPOT_LIGHT_CLASS_ID  	0x1012
#define DIR_LIGHT_CLASS_ID  	0x1013
#define FSPOT_LIGHT_CLASS_ID  	0x1014
#define TDIR_LIGHT_CLASS_ID  	0x1015

// subclasses of HELPER_CLASS_ID
#define DUMMY_CLASS_ID 			0x876234
#define BONE_CLASS_ID 			0x8a63c0
#define TAPEHELP_CLASS_ID 		0x02011
#define GRIDHELP_CLASS_ID		0x02010
#define POINTHELP_CLASS_ID		0x02013
#define PROTHELP_CLASS_ID		0x02014

//subclasses of UVGEN_CLASS_ID
#define STDUV_CLASS_ID 			0x0000100

//subclasses of XYZGEN_CLASS_ID
#define STDXYZ_CLASS_ID 		0x0000100

//subclasses of TEXOUT_CLASS_ID
#define STDTEXOUT_CLASS_ID 		0x0000100

// subclasses of MATERIAL_CLASS_ID	
#define DMTL_CLASS_ID  			0x00000002	// Origninal Stdmtl
#define DMTL2_CLASS_ID  		0x00000003	// R2.5 stdmtl
#define CMTL_CLASS_ID 			0x0000100  // top-bottom material 
#define MULTI_CLASS_ID 			0x0000200  // multi material
#define DOUBLESIDED_CLASS_ID 	0x0000210  // double-sided mtl
#define MIXMAT_CLASS_ID 		0x0000250  // blend mtl
#define MATTE_CLASS_ID 			0x0000260  // Matte mtl

// subclasses of TEXMAP_CLASS_ID	
#define CHECKER_CLASS_ID 		0x0000200
#define MARBLE_CLASS_ID 		0x0000210
#define MASK_CLASS_ID 			0x0000220  // mask texture
#define MIX_CLASS_ID 			0x0000230
#define NOISE_CLASS_ID 			0x0000234
#define GRADIENT_CLASS_ID 		0x0000270
#define TINT_CLASS_ID 			0x0000224  // Tint texture
#define BMTEX_CLASS_ID 			0x0000240  // Bitmap texture
#define ACUBIC_CLASS_ID 		0x0000250  // Reflect/refract
#define MIRROR_CLASS_ID 		0x0000260  // Flat mirror
#define COMPOSITE_CLASS_ID 		0x0000280   // Composite texture
#define RGBMULT_CLASS_ID 		0x0000290   // RGB Multiply texture
#define FALLOFF_CLASS_ID 		0x00002A0   // Falloff texture
#define OUTPUT_CLASS_ID 		0x00002B0   // Output texture
#define PLATET_CLASS_ID 		0x00002C0   // Plate glass texture
#define VCOL_CLASS_ID 			0x0934851	// Vertex color map

// Subclasses of SHADER_CLASS_ID
#define STDSHADERS_CLASS_ID		0x00000035	// to 39 

// Subclasses of SHADOW_TYPE_CLASS_ID	
#define STD_SHADOW_MAP_CLASS_ID       0x0000100
#define STD_RAYTRACE_SHADOW_CLASS_ID  0x0000200
		
// subclasses of RENDERER_CLASS_ID		  
#define SREND_CLASS_ID 			0x000001 // default scan-line renderer

// subclasses of REF_MAKER_CLASS_ID			
#define MTL_LIB_CLASS_ID 		0x001111
#define MTLBASE_LIB_CLASS_ID 	0x003333
#define THE_SCENE_CLASS_ID   	0x002222
#define MEDIT_CLASS_ID 	 		0x000C80

// subclass of all classes
#define STANDIN_CLASS_ID   		0xffffffff  // subclass of all super classes



// Default sound object
#define DEF_SOUNDOBJ_CLASS_ID	0x0000001

// Default atmosphere
#define FOG_CLASS_ID 0x10000001

//------------------ Modifier sub classes --------
#define SKEWOSM_CLASS_ID			0x6f3cc2aa
#define BENDOSM_CLASS_ID 			0x00010
#define TAPEROSM_CLASS_ID 			0x00020
#define TWISTOSM_CLASS_ID 			0x00090

#define UVWMAPOSM_CLASS_ID			0xf72b1
#define SELECTOSM_CLASS_ID			0xf8611
#define MATERIALOSM_CLASS_ID		0xf8612
#define SMOOTHOSM_CLASS_ID			0xf8613
#define NORMALOSM_CLASS_ID			0xf8614
#define OPTIMIZEOSM_CLASS_ID		0xc4d31
#define AFFECTREGION_CLASS_ID		0xc4e32
#define SUB_EXTRUDE_CLASS_ID		0xc3a32
#define TESSELLATE_CLASS_ID			0xa3b26ff2
#define DELETE_CLASS_ID				0xf826ee01
#define MESHSELECT_CLASS_ID			0x73d8ff93
#define UVW_XFORM_CLASS_ID			0x5f32de12

#define EXTRUDEOSM_CLASS_ID 		0x000A0
#define SURFREVOSM_CLASS_ID 		0x000B0

#define DISPLACEOSM_CLASS_ID		0xc4d32
#define DISPLACE_OBJECT_CLASS_ID	0xe5240
#define DISPLACE_WSM_CLASS_ID		0xe5241

#define SINEWAVE_OBJECT_CLASS_ID 	0x00030
#define SINEWAVE_CLASS_ID 			0x00040
#define SINEWAVE_OMOD_CLASS_ID 		0x00045
#define LINWAVE_OBJECT_CLASS_ID 	0x00035
#define LINWAVE_CLASS_ID 			0x00042
#define LINWAVE_OMOD_CLASS_ID 		0x00047

#define GRAVITYOBJECT_CLASS_ID		0xe523c
#define GRAVITYMOD_CLASS_ID			0xe523d
#define WINDOBJECT_CLASS_ID			0xe523e
#define WINDMOD_CLASS_ID			0xe523f

#define DEFLECTOBJECT_CLASS_ID		0xe5242
#define DEFLECTMOD_CLASS_ID			0xe5243

#define BOMB_OBJECT_CLASS_ID 		0xf2e32
#define BOMB_CLASS_ID 				0xf2e42

// These are the FFD Modifier's class ID's
#define FFDNMOSSQUARE_CLASS_ID		Class_ID(0x8ab36cc5,0x82d7fe74)
#define FFDNMWSSQUARE_CLASS_ID		Class_ID(0x67ea40b3,0xfe7a30c4)
#define FFDNMWSSQUARE_MOD_CLASS_ID	Class_ID(0xd6636ea2,0x9aa42bf3)

#define FFDNMOSCYL_CLASS_ID			Class_ID(0x98f37a63,0x3ffe9bca)
#define FFDNMWSCYL_CLASS_ID			Class_ID(0xfa4700be,0xbbe85051)
#define FFDNMWSCYL_MOD_CLASS_ID		Class_ID(0xf1c630a3,0xaa8ff601)

#define FFD44_CLASS_ID				Class_ID(0x21325596, 0x2cd10bd8)
#define FFD33_CLASS_ID				Class_ID(0x21325596, 0x2cd10bd9)
#define FFD22_CLASS_ID				Class_ID(0x21325596, 0x2cd10bd0)

//JH Association context modifiers
//GEOM TO GEOM
#define ACMOD_GEOM_GEOM_BOOLADD_CID	0x4e0f483a
#define ACMOD_GEOM_GEOM_BOOLSUB_CID	0x61661a5c
#define ACMOD_GEOM_GEOM_BOOLINT_CID	0x2a4f3945
#define ACMOD_GEOM_GEOM_SIMPAGG_CID	0x40cb05ab

//SHAPE To GEOM
#define ACMOD_SHAPE_GEOM_HOLE_CID	0x366307b0
#define ACMOD_SHAPE_GEOM_INT_CID	0x782d8d50
#define ACMOD_SHAPE_GEOM_EMBOSS_CID	0x7a13397c
#define ACMOD_SHAPE_GEOM_REVEAL_CID	0x55ed658c

//JH Solids
#define GENERIC_AMSOLID_CLASS_ID	Class_ID(0x5bb661e8, 0xa2c27f02)


//------------------ Controller sub classes --------
#define LININTERP_FLOAT_CLASS_ID 			0x2001
#define LININTERP_POSITION_CLASS_ID 		0x2002
#define LININTERP_ROTATION_CLASS_ID 		0x2003
#define LININTERP_SCALE_CLASS_ID			0x2004
#define PRS_CONTROL_CLASS_ID				0x2005
#define LOOKAT_CONTROL_CLASS_ID				0x2006				

#define HYBRIDINTERP_FLOAT_CLASS_ID 		0x2007
#define HYBRIDINTERP_POSITION_CLASS_ID 		0x2008
#define HYBRIDINTERP_ROTATION_CLASS_ID 		0x2009
#define HYBRIDINTERP_POINT3_CLASS_ID		0x200A
#define HYBRIDINTERP_SCALE_CLASS_ID			0x2010
#define HYBRIDINTERP_COLOR_CLASS_ID			0x2011

#define TCBINTERP_FLOAT_CLASS_ID 			0x442311
#define TCBINTERP_POSITION_CLASS_ID 		0x442312
#define TCBINTERP_ROTATION_CLASS_ID 		0x442313
#define TCBINTERP_POINT3_CLASS_ID			0x442314
#define TCBINTERP_SCALE_CLASS_ID			0x442315

#define MASTERPOINTCONT_CLASS_ID			0xd9c20ff

//--------------------------------------------------

class ISave;
class ILoad;
class Interface;
class ShortcutTable;
typedef short BlockID;
class ParamBlockDesc2;
class IParamBlock2;
class IObjParam;
class Animatable;
class ParamMap2UserDlgProc;
class IParamMap2;

// System keeps a list of the DLL's found on startup.
// This is the interface to a single class
class ClassDesc {
	public:
		virtual					~ClassDesc() {}
		virtual int				IsPublic()=0;  // Show this in create branch?
		virtual void *			Create(BOOL loading=FALSE)=0;   // return a pointer to an instance of the class.
		virtual	int 			BeginCreate(Interface *i) {return 0;}
		virtual int 			EndCreate(Interface *i) {return 0;};
		virtual const TCHAR* 	ClassName()=0;
		virtual SClass_ID		SuperClassID()=0;
		virtual Class_ID		ClassID()=0;
		virtual const TCHAR* 	Category()=0;   // primitive/spline/loft/ etc
		virtual BOOL			OkToCreate(Interface *i) { return TRUE; }	// return FALSE to disable create button		
		virtual BOOL			HasClassParams() {return FALSE;}
		virtual void			EditClassParams(HWND hParent) {}
		virtual void			ResetClassParams(BOOL fileReset=FALSE) {}

        // These functions return keyboard shortcut tables that plug-ins can use
        virtual int             NumShortcutTables() { return 0; }
        virtual ShortcutTable*  GetShortcutTable(int i) { return NULL; }

		// Class IO
		virtual BOOL			NeedsToSave() { return FALSE; }
		virtual IOResult 		Save(ISave *isave) { return IO_OK; }
		virtual IOResult 		Load(ILoad *iload) { return IO_OK; }

		// bits of dword set indicate corrresponding rollup page is closed.
		// the value 0x7fffffff is returned by the default implementation so the
		// command panel can detect this method is not being overridden, and just leave 
		// the rollups as is.
		virtual DWORD			InitialRollupPageState() { return 0x7fffffff; }

		// ParamBlock2-related metadata interface, supplied & implemented in ClassDesc2 (see maxsdk\include\iparamb2.h)
		
		virtual const TCHAR*	InternalName() { return NULL; }
		virtual HINSTANCE		HInstance() { return NULL; }
		// access parameter block descriptors for this class
		virtual int				NumParamBlockDescs() { return 0; }
		virtual ParamBlockDesc2* GetParamBlockDesc(int i) { return NULL; }
		virtual ParamBlockDesc2* GetParamBlockDescByID(BlockID id) { return NULL; }
		virtual void			AddParamBlockDesc(ParamBlockDesc2* pbd) { }
		// automatic UI management
		virtual void			BeginEditParams(IObjParam *ip, ReferenceMaker* obj, ULONG flags, Animatable *prev) { }
		virtual void			EndEditParams(IObjParam *ip, ReferenceMaker* obj, ULONG flags, Animatable *prev) { }
		virtual void			InvalidateUI(ParamBlockDesc2* pbd) { }
		// automatic ParamBlock construction
		virtual void			MakeAutoParamBlocks(ReferenceMaker* owner) { }
		// access automatically-maintained ParamMaps, by simple index or by associated ParamBlockDesc
		virtual int				NumParamMaps() { return 0; }
		virtual IParamMap2*		GetParamMap(int i) { return NULL; }
		virtual IParamMap2*		GetParamMap(ParamBlockDesc2* pbd) { return NULL; }
		// maintain user dialog procs on automatically-maintained ParamMaps
		virtual void			SetUserDlgProc(ParamBlockDesc2* pbd, ParamMap2UserDlgProc* proc=NULL) { }
		virtual ParamMap2UserDlgProc* GetUserDlgProc(ParamBlockDesc2* pbd) { return NULL; }

		// Class can draw an image to represent itself graphically...
		virtual bool DrawRepresentation(COLORREF bkColor, HDC hDC, Rect &rect) { return FALSE; }

		// Generic expansion function
		virtual int Execute(int cmd, ULONG arg1=0, ULONG arg2=0, ULONG arg3=0) { return 0; } 
	};

// Create instance of the specified class
CoreExport void *CreateInstance(SClass_ID superID, Class_ID classID);

#endif
