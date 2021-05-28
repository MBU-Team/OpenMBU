/**********************************************************************
 *<
	FILE: render.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __RENDER__H
#define __RENDER__H

// includes
#include "sfx.h"


#define FIELD_EVEN 0
#define FIELD_ODD 1


// Render Types   RB: I didn't want to include render.h in MAXAPI.H...
#ifndef _REND_TYPE_DEFINED
#define _REND_TYPE_DEFINED
enum RendType
{ 
	RENDTYPE_NORMAL,
	RENDTYPE_REGION,
	RENDTYPE_BLOWUP,
	RENDTYPE_SELECT,
	RENDTYPE_REGIONCROP,
	// The following 2 types are to NOT be passed into plugin renderers.  There purpose is for passing to
	//  the function Interface::OpenCurRenderer, which converts them into RENDTYPE_REGION and RENDTYPE_REGIONCROP, resp.
	RENDTYPE_REGION_SEL, // do a region render using the bounding rectangle of the selection
	RENDTYPE_CROP_SEL	// do a crop render using the bounding rectangle of the selection
};
#endif


class DefaultLight
{
public:
	LightState ls;
	Matrix3 tm;	
};

class ViewParams : public BaseInterfaceServer {
	public:
		Matrix3 prevAffineTM; // world space to camera space transform 2 ticks previous 
		Matrix3 affineTM;  // world space to camera space transform
		int projType;      // PROJ_PERSPECTIVE or PROJ_PARALLEL
		float hither,yon;
		float distance; // to view plane
		// Parallel projection params
		float zoom;  // Zoom factor 
		// Perspective params
		float fov; 	// field of view
		float nearRange; // for fog effects
		float farRange;  // for fog effects
		// Generic expansion function
		virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
	};


// Common renderer parameters

/*
class Atmospheric;
class IRenderElement;
*/
class RadiosityEffect;
class ToneOperator;

#ifdef DESIGN_VER
// Even if rendering to another device (i.e., Printer) is supported in the renderer,
// render to a preview window instead if it is supported. This goes in the extraFlags 
// field of RendParams.
#define RENDER_REDIRECT_TO_WINDOW     (1L << 1)
#endif

// This removes a fix made for R3 that  cleaned up the edges of shadows where objects intersect-- DS 8/28/00
// This goes in the extraFlags field of RendParams.
#define RENDER_R25SHADOWS    		  (1L << 2)

// Tell the renderer to hide frozen objects
// This goes in the extraFlags field of RendParams.
#define RENDER_HIDE_FROZEN    		  (1L << 3)

// These parameters are passed to the renderer when the renderer is opend.
class RendParams
{
private:
	IRenderElementMgr *mpIRenderElementMgr; // The render element manager, may be NULL -- declared in maxsdk/include/RenderElements.h

public:
	enum RenderMode
	{
		RM_Default,			// normal rendering mode
		RM_IReshade,		// render is being used for interactive reshading
	};

	RendType rendType;	 	// normal, region, blowup, selection

	BOOL isNetRender;  		// is this a render on a network slave?	
	BOOL fieldRender;
	int fieldOrder;    		// 0->even, 1-> odd
	TimeValue frameDur; 	// duration of one frame
	
	BOOL colorCheck;
	int vidCorrectMethod; 	// 0->FLAG, 1->SCALE_LUMA 2->SCALE_SAT
	int ntscPAL;  			// 0 ->NTSC,  1 ->PAL
	BOOL superBlack;		// impose superBlack minimum intensity?
	int sbThresh;			// value used for superBlack
	BOOL rendHidden;		// render hidden objects?
	BOOL force2Side;		// force two-sided rendering?
	BOOL inMtlEdit;	  		// rendering in the mtl editor?
	float mtlEditTile; 		// if in mtl editor, scale tiling by this value
	BOOL mtlEditAA;   		// if in mtl editor, antialias? 
	BOOL multiThread; 		// for testing only
	BOOL useEnvironAlpha;  	// use alpha from the environment map.
	BOOL dontAntialiasBG; 	// Don't antialias against background (for video games)		
	BOOL useDisplacement; 	// Apply displacment mapping		
	bool useRadiosity;		// Include radiosity in rendering
	bool computeRadiosity;	// Compute radiosity before rendering
	Texmap *envMap;			// The environment map, may be NULL
	Atmospheric *atmos; 	// The atmosphere effects, may be NULL.
	Effect *effect; 	    // The postprocessing effects, may be NULL.
	RadiosityEffect* pRadiosity;	// The radiosity effect
	ToneOperator* pToneOp;	// The tone operator, may be NULL
	TimeValue firstFrame; 	// The first frame that will be rendered
	int scanBandHeight;		// height of a scan band (default scanline renderer)
	ULONG extraFlags;		// for expansion
	int width,height;		// image width,height.
	BOOL filterBG;			// filter background
	BOOL alphaOutOnAdditive;// produce alpha on additive transparency

	RendParams()
	{
		rendType = RENDTYPE_NORMAL;
		isNetRender = FALSE;
		fieldRender = FALSE;
		fieldOrder = 0;
		frameDur = 0;
		colorCheck = 0;
		vidCorrectMethod = 0;
		ntscPAL = 0;
		superBlack = 0;
		sbThresh = 0;
		rendHidden = 0;
		force2Side = 0;
		inMtlEdit = 0;
		mtlEditTile = 0;
		mtlEditAA = 0;
		multiThread = 0;
		useEnvironAlpha = 0;
		dontAntialiasBG = 0;
		useDisplacement = 0;
		useRadiosity = true;
		computeRadiosity = true;
		envMap = NULL;
		atmos = NULL;
		mpIRenderElementMgr = NULL;
		effect = NULL;
		pRadiosity = NULL;
		pToneOp = NULL;
		firstFrame = 0;
		scanBandHeight = 0;
		extraFlags = 0;
		width=height = 0;
		filterBG = 0;
	}

	RenderMode GetRenderMode() { return RM_Default; } // mjm - 06.08.00
	void SetRenderElementMgr(IRenderElementMgr *pIRenderElementMgr) { mpIRenderElementMgr = pIRenderElementMgr; } // mjm - 06.30.00
	IRenderElementMgr *GetRenderElementMgr() { return mpIRenderElementMgr; } // mjm - 06.30.00

	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; }
};

// These are passed to the renderer on every frame
class FrameRendParams : public BaseInterfaceServer {
	public:
	Color ambient;
	Color background;
	Color globalLightLevel;
	float frameDuration; // duration of one frame, in current frames
	float relSubFrameDuration;  // relative fraction of frameDuration used by subframe.

	// boundaries of the region for render region or crop (device coords).
	int regxmin,regxmax;
	int regymin,regymax;

	// parameters for render blowup.
	Point2 blowupCenter;
	Point2 blowupFactor;

	FrameRendParams() { frameDuration = 1.0f; relSubFrameDuration = 1.0f; }
	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
	};

// Since this dialog is modless and non-interactive, as the user changes
// parameters in the dialog, the renderer does not need to update it's
// state. When the user is through, they may choose 'OK' or 'Cancel'.
//
// If the user OKs the dialog, AcceptParams() will be called, at which time the
// renderer can read the parameter out of the UI and modify its state.
// 
// If RejectParams() is called, typically the renderer will not have to do anything
// since it has not yet modify its state, but if for some reason it has, it
// should restore its state.
class RendParamDlg {
	public:
		virtual void AcceptParams()=0;
		virtual void RejectParams() {}
		virtual void DeleteThis()=0;		
	};

// Flag bits for DoMaterialBrowseDlg()
#define BROWSE_MATSONLY		(1<<0)
#define BROWSE_MAPSONLY		(1<<1)
#define BROWSE_INCNONE		(1<<2) 	// Include 'None' as an option
#define BROWSE_INSTANCEONLY	(1<<3) 	// Only allow instances, no copy
#define BROWSE_TO_MEDIT_SLOT (1<<4) // browsing to medit slot
 
// passed to SetPickMode. This is a callback that gets called as
// the user tries to pick objects in the scene.
class RendPickProc
{
public:
	// Called when the user picks something.
	// return TRUE to end the pick mode.
	virtual BOOL Pick(INode *node)=0;

	// Return TRUE if this is an acceptable hit, FALSE otherwise.
	virtual BOOL Filter(INode *node)=0;

	// These are called as the mode is entered and exited
	virtual void EnterMode() {}
	virtual void ExitMode() {}

	// Provides two cursor, 1 when over a pickable object and 1 when not.
	virtual HCURSOR GetDefCursor() {return NULL;}
	virtual HCURSOR GetHitCursor() {return NULL;}

	// Return TRUE to allow the user to pick more than one thing.
	// In this case the Pick method may be called more than once.
	virtual BOOL AllowMultiSelect() {return FALSE;}
};


// This is the interface given to a renderer when it needs to display its parameters
// It is also given to atmospheric effects to display thier parameters.
class IRendParams : public InterfaceServer
{
public:
	// The current position of the frame slider
	virtual TimeValue GetTime()=0;

	// Register a callback object that will get called every time the
	// user changes the frame slider.
	virtual void RegisterTimeChangeCallback(TimeChangeCallback *tc)=0;
	virtual void UnRegisterTimeChangeCallback(TimeChangeCallback *tc)=0;

	// Brings up the material browse dialog allowing the user to select a material.
	// newMat will be set to TRUE if the material is new OR cloned.
	// Cancel will be set to TRUE if the user cancels the dialog.
	// The material returned will be NULL if the user selects 'None'
	virtual MtlBase *DoMaterialBrowseDlg(HWND hParent,DWORD flags,BOOL &newMat,BOOL &cancel)=0;

	// Adds rollup pages to the render params dialog. Returns the window
	// handle of the dialog that makes up the page.
	virtual HWND AddRollupPage(HINSTANCE hInst, TCHAR *dlgTemplate, 
		DLGPROC dlgProc, TCHAR *title, LPARAM param=0, DWORD flags=0, int category = ROLLUP_CAT_STANDARD)=0;

	virtual HWND AddRollupPage(HINSTANCE hInst, DLGTEMPLATE *dlgTemplate, 
		DLGPROC dlgProc, TCHAR *title, LPARAM param=0, DWORD flags=0, int category = ROLLUP_CAT_STANDARD)=0;

	// Removes a rollup page and destroys it.
	virtual void DeleteRollupPage(HWND hRollup)=0;

	// When the user mouses down in dead area, the plug-in should pass
	// mouse messages to this function which will pass them on to the rollup.
	virtual void RollupMouseMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)=0;

	// This will set the command mode to a standard pick mode.
	// The callback implements hit testing and a method that is
	// called when the user actually picks an item.
	virtual void SetPickMode(RendPickProc *proc)=0;
	
	// If a plug-in is finished editing its parameters it should not
	// leave the user in a pick mode. This will flush out any pick modes
	// in the command stack.
	virtual void EndPickMode()=0;
		
	// When a plugin has a Texmap, clicking on the button
	// associated with that map should cause this routine
	// to be called.
	virtual void PutMtlToMtlEditor(MtlBase *mb)=0;

	// This is for use only by the scanline renderer.
	virtual float GetMaxPixelSize() = 0;
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 

	// JBW 12/1/98: get interface to rollup window interface
	virtual IRollupWindow* GetIRollup()=0;
};


//----------------------------------------------------------------
// Render Instance flags
#define INST_HIDE	  		(1<<0) // instance is hidden
#define INST_CLIP			(1<<1) // clip instance: ray tracers should skip it 
#define INST_BLUR			(1<<2) // secondary motion blur instance 
#define INST_RCV_SHADOWS	(1<<3) // instance receives shadows
#define INST_TM_NEGPARITY	(1<<4) // mesh is inside-out: need to reverse normals on-the-fly
#define INST_MTL_BYFACE     (1<<5)	//instance's object supports mtl-by-face interface ( chkmtlapi.h)


class RenderInstance
{
public:
	ULONG flags;
	Mtl *mtl;       		// from inode, for convenience
	float wireSize;         // Mtl wireframe size
	Mesh *mesh;				// result of GetRenderMesh call
	float vis;				// Object visibility
	int nodeID;				// unique within scene during render- corresponds to ShadeContext::NodeID()
	int objMotBlurFrame;  	// Object motion blur sub frame (= NO_MOTBLUR for non-blurred objects)
	int objBlurID;		    // Blur instances for an object share a objBlurID value.
	Matrix3 objToWorld;		// transforms object coords to world coords
	Matrix3 objToCam;		// transforms object coords to cam coords
	Matrix3 normalObjToCam; // for transforming surface normals from obj to camera space
	Matrix3 camToObj;    	// transforms camera coords to object coords
	Box3 obBox;				// Object space extents
	Point3 center;			// Bounding sphere center (camera coords)
	float radsq;			// square of bounding sphere's radius

	void SetFlag(ULONG f, BOOL b) { if (b) flags |= f; else flags &= ~f; }
	void SetFlag(ULONG f) {  flags |= f; }
	void ClearFlag(ULONG f) {  flags &= ~f; }
	BOOL TestFlag(ULONG f) { return flags&f?1:0; }
	BOOL Hidden() { return TestFlag(INST_HIDE); }
	BOOL IsClip() { return TestFlag(INST_CLIP); }

	virtual RenderInstance *Next()=0;	// next in list

	virtual Interval MeshValidity()=0;
	virtual int NumLights()=0;
	virtual LightDesc *Light(int n)=0; 

	virtual BOOL CastsShadowsFrom(const ObjLightDesc& lt)=0; // is lt shadowed by this instance?

	virtual INode *GetINode()=0;  						 // get INode for instance
	virtual Object *GetEvalObject()=0; 					 // evaluated object for instance
	virtual ULONG MtlRequirements(int mtlNum, int faceNum)=0;  	 // node's mtl requirements. DS 3/31/00: added faceNum to support mtl-per-face objects
	virtual Point3 GetFaceNormal(int faceNum)=0;         // geometric normal in camera coords
	virtual Point3 GetFaceVertNormal(int faceNum, int vertNum)=0;  // camera coords
	virtual void GetFaceVertNormals(int faceNum, Point3 n[3])=0;   // camera coords
	virtual Point3 GetCamVert(int vertnum)=0; 			 // coord for vertex in camera coords		
	virtual void GetObjVerts(int fnum, Point3 obp[3])=0; // vertices of face in object coords
	virtual void GetCamVerts(int fnum, Point3 cp[3])=0; // vertices of face in camera(view) coords
	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 

	// Material-by-face access
	// Objects can provide a material as a function of face number via the IChkMtlAPI interface (chkmtlapi.h).
	// This method will return RenderInstance::mtl if flag INST_MTL_BYFACE is not set. If INST_MTL_BYFACE is
	// set it will return the proper by-face mtl. // DS 4/3/00
	virtual Mtl *GetMtl(int faceNum)=0;  
};

//----------------------------------------------------------------


// Values returned from Progress()
#define RENDPROG_CONTINUE	1
#define RENDPROG_ABORT		0

// Values passed to SetCurField()
#define FIELD_FIRST		0
#define FIELD_SECOND	1
#define FIELD_NONE		-1

// A callback passed in to the renderer
class RendProgressCallback
{
public:
	virtual void SetTitle(const TCHAR *title)=0;
	virtual int Progress(int done, int total)=0;
	virtual void SetCurField(int which) {}
	virtual void SetSceneStats(int nlights, int nrayTraced, int nshadowed, int nobj, int nfaces) {}
};


// RB: my version of a renderer...
class Renderer : public ReferenceTarget
{
public:
	// Reference/Animatable methods.
	// In addition, the renderer would need to implement ClassID() and DeleteThis()
	// Since a renderer will probably not itself have references, this implementation should do
	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	     PartID& partID,  RefMessage message) {return REF_SUCCEED;}
	SClass_ID SuperClassID() {return RENDERER_CLASS_ID;}
	
	virtual int Open(
		INode *scene,     	// root node of scene to render
		INode *vnode,     	// view node (camera or light), or NULL
		ViewParams *viewPar,// view params for rendering ortho or user viewport
		RendParams &rp,  	// common renderer parameters
		HWND hwnd, 				// owner window, for messages
		DefaultLight* defaultLights=NULL, // Array of default lights if none in scene
		int numDefLights=0	// number of lights in defaultLights array
		)=0;
					
	//
	// Render a frame -- will use camera or view from open
	//
	virtual int Render(
		TimeValue t,   			// frame to render.
   		Bitmap *tobm, 			// optional target bitmap
		FrameRendParams &frp,	// Time dependent parameters
		HWND hwnd, 				// owner window
		RendProgressCallback *prog=NULL,
		ViewParams *viewPar=NULL  // override viewPar given on Open.
		)=0;

	// apply render effects at the requested time value - should be called between Renderer::Open() and Renderer::Close()
	// this can be used during a multi-pass rendering, in order to apply the render effects to the final, blended bitmap.
	// 'updateDisplay' indicates that Bitmap's display should be refreshed by the renderer
	// return value indicats if effects were successfully applied.
	virtual bool ApplyRenderEffects(TimeValue t, Bitmap *pBitmap, bool updateDisplay=true) { return false; }

	virtual void Close(	HWND hwnd )=0;		
	// Adds rollup page(s) to renderer configure dialog
	// If prog==TRUE then the rollup page should just display the parameters
	// so the user has them for reference while rendering, they should not be editable.
	virtual RendParamDlg *CreateParamDialog(IRendParams *ir,BOOL prog=FALSE)=0;
	virtual void ResetParams()=0;
	virtual int	GetAAFilterSupport(){ return 0; } // point sample, no reconstruction filter
};


class ShadowBuffer;
class ShadowQuadTree;

class RendContext
{
public:
	virtual int Progress(int done, int total) const { return 1; }
	virtual Color GlobalLightLevel() const = 0;
};

struct SubRendParams : public BaseInterfaceServer
{
	RendType rendType;	
	BOOL fieldRender;
	BOOL evenLines; // when field rendering
	BOOL doingMirror;
	BOOL doEnvMap;  // do environment maps?
	int devWidth, devHeight;
	float devAspect;
	int xorg, yorg;           // location on screen of upper left corner of output bitmap
	int xmin,xmax,ymin,ymax;  // area of screen being rendered
	// parameters for render blowup.
	Point2 blowupCenter;
	Point2 blowupFactor;
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
};

// flags passed to RenderMapsContext::Render()
#define RENDMAP_SHOW_NODE  1  // *Dont* exclude this node from the render.


// A pointer to this data structure is passed to MtlBase::BuildMaps();
// when rendering reflection and refraction maps.
class RenderMapsContext
{ 
public:
	virtual INode *GetNode()=0;
	virtual int NodeRenderID()=0;
	virtual void GetCurrentViewParams(ViewParams &vp)=0;
	virtual void GetSubRendParams(SubRendParams &srp)=0;
	virtual int SubMtlIndex()=0;
	virtual void SetSubMtlIndex(int mindex)=0;
	virtual void FindMtlPlane(float pl[4])=0;
	virtual void FindMtlScreenBox(Rect &sbox, Matrix3* viewTM=NULL, int mtlIndex=-1)=0;
	virtual Box3 CameraSpaceBoundingBox()=0;
	virtual Box3 ObjectSpaceBoundingBox()=0;
	virtual Matrix3 ObjectToWorldTM()=0;
	virtual RenderGlobalContext *GetGlobalContext() { return NULL; }
	// ClipPlanes is a pointer to an array of Point4's,  each of which
	// represents a clip plane.  nClip Planes is the number of planes (up to 6);
	// The planes are in View space.
	virtual	int Render(Bitmap *bm, ViewParams &vp, SubRendParams &srp, Point4 *clipPlanes=NULL, int nClipPlanes=0)=0; 
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
};	


#define DONT_CLIP 1.0E38f


//--------------------------------------------------------------------------
// Interface into the default scanline renderer, Class_ID(SREND_CLASS_ID,0)
//---------------------------------------------------------------------------
class FilterKernel;
class IScanRenderer: public Renderer {
	public:
	virtual void SetAntialias(BOOL b) = 0;
	virtual BOOL GetAntialias() = 0;
	virtual void SetFilter(BOOL b) = 0;
	virtual BOOL GetFilter() = 0;
	virtual void SetShadows(BOOL b) = 0;
	virtual BOOL GetShadows() = 0;
	virtual void SetMapping(BOOL b) = 0;
	virtual BOOL GetMapping() = 0;
	virtual void SetForceWire(BOOL b) = 0;
	virtual BOOL GetForceWire() = 0;
	virtual	void SetAutoReflect(BOOL b)=0;
	virtual	BOOL GetAutoReflect()=0;
	virtual void SetObjMotBlur(BOOL b) = 0;
	virtual BOOL GetObjMotBlur() = 0;
	virtual void SetVelMotBlur(BOOL b) = 0;
	virtual BOOL GetVelMotBlur() = 0;

	// Obsolete, use setfiltersz. pixel sz = 1.0 for all filtering
	virtual void SetPixelSize(float size) = 0;
	
	virtual void SetAutoReflLevels(int n) = 0;
	virtual void SetWireThickness(float t) = 0;
	virtual void SetObjBlurDuration(float dur) = 0;
	virtual void SetVelBlurDuration(float dur) = 0;
	virtual void SetNBlurFrames(int n) = 0;
	virtual void SetNBlurSamples(int n) = 0;
	virtual void SetMaxRayDepth(int n) = 0;
	virtual int GetMaxRayDepth() { return 7; }

	virtual void SetAntiAliasFilter( FilterKernel * pKernel ) = 0;
	virtual FilterKernel * GetAntiAliasFilter() = 0;
	virtual void SetAntiAliasFilterSz(float size) = 0;
	virtual float GetAntiAliasFilterSz() = 0;

	virtual void SetPixelSamplerEnable( BOOL on ) = 0;
	virtual BOOL GetPixelSamplerEnable() = 0;
};


#endif

