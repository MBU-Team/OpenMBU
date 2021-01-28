/**********************************************************************
 *<
	FILE: imtl.h

	DESCRIPTION: Renderer materials

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __IMTL__H
#define __IMTL__H


#include <plugapi.h>
#include <RenderElements.h>

//#include "gport.h"	
#include "custcont.h" 
#include "shape.h"

#define PROJ_PERSPECTIVE 0   // perspective projection
#define PROJ_PARALLEL 1      // parallel projection

#define AXIS_UV 0
#define AXIS_VW 1
#define AXIS_WU 2

// Values for SymFlags:
#define U_WRAP   (1<<0)
#define V_WRAP   (1<<1)  
#define U_MIRROR (1<<2)
#define V_MIRROR (1<<3)

#define	X_AXIS 0
#define	Y_AXIS 1
#define	Z_AXIS 2

static inline float Intens(const AColor& c) {	return (c.r+c.g+c.b)/3.0f;	}
static inline float Intens(const Color& c) {	return (c.r+c.g+c.b)/3.0f;	}

// Meta-materials post this message to the MtlEditor when user
// clicks on a sub-material button.
#define WM_SUB_MTL_BUTTON	WM_USER + 0x04001

// Materials or Texture maps post this message to the MtlEditor when user
// clicks on a texture map button.
#define WM_TEXMAP_BUTTON	WM_USER + 0x04002

class ShadeContext;
class Bitmap;
class RenderMapsContext; 
class Object;
class UVGen;
class XYZGen;
class Sampler;

// Postage Stamp images ----------
// GetImage fills an array RGB triples.
// Width() and Height() return the dimensions of the image in pixels. 
// The width in bytes of the array is given by the following macro, where w is pixel width. 

#define PS_SMALL 0
#define PS_LARGE 1
#define PS_TINY  2

#define PS_TINY_SIZE  24  // size of the tiny postage stamps
#define PS_SMALL_SIZE 32  // size of the small postage stamps
#define PS_LARGE_SIZE 88  // size of the large postage stamps
#define PSDIM(isz) ((isz==0)?PS_SMALL_SIZE:(isz==1)?PS_LARGE_SIZE:PS_TINY_SIZE)

#define ByteWidth(w) (((w*3+3)/4)*4)

class PStamp: public AnimProperty {
	public:
	virtual int Width()=0;
	virtual int Height()=0;
	virtual void SetImage(UBYTE *img)=0;
	virtual void GetImage(UBYTE *img)=0;
	virtual void DeleteThis()=0;
	virtual IOResult Load(ILoad *iload)=0;
	virtual IOResult Save(ISave *isave)=0;
	};

class TexHandle {
	public:
		virtual DWORD_PTR GetHandle() = 0;
		virtual void DeleteThis() = 0;
	};		

// Values for extraFlags:
#define EX_MULT_ALPHA     1    // if alpha is NOT premultiplied in the Bitmap, set this 
#define EX_RGB_FROM_ALPHA 2    // make map using alpha to define gray level 
#define EX_OPAQUE_ALPHA   4    // make map using opaque alpha 
#define EX_ALPHA_FROM_RGB 8    // make alpha from intensity 

class TexHandleMaker{
	public: 
		// choice of two ways to create a texture handle.
		// From a 3DSMax Bitmap
		virtual TexHandle* CreateHandle(Bitmap *bm, int symflags=0, int extraFlags=0)=0;

		// From a 32 bit DIB
		virtual TexHandle* CreateHandle(BITMAPINFO *bminf, int symflags=0, int extraFlags=0)=0;

		// Create an 32 bit DIB with symmetry, extraflags already incorporated.
		// iff forceW is non-zero, it is used as the width of the final DIB 
		// iff forceH is non-zero, it is used as the height of the final DIB 
		virtual BITMAPINFO *BitmapToDIB(Bitmap *bm, int symflags, int extraFlags, BOOL forceW=0, BOOL forceH=0)=0;

		// From a 32 bit DIB that has symflags, extraflags already incorporated.
		virtual TexHandle* MakeHandle(BITMAPINFO*bminf)=0;  // this takes ownership of the BITMAPINFO*

		// This tells you the size desired of the bitmap. It ultimately
		// needs a bitmap that is a power of 2 in width and height.  If 
		// UseClosestPowerOf2 returns TRUE, then the bitmap need not be square.
		// If you already have a bitmap around, just pass it in to CreateHandle
		// and it will be converted.  If you are making a bitmap from scratch
		// (i.e.) a procedural texture, then you should make it Size() in 
		// width in height, and save us an extra step.  In either case
		// You own your bitmap, and are responsible for ultimately freeing it.

		virtual BOOL UseClosestPowerOf2()=0;
		virtual int Size()=0;
	};


// Callback used with the new multi-texture interface by function SetGfxMtl. 
class MtlMakerCallback: public TexHandleMaker {
	public:
	// This fills all the fields of texinf exept the texhandle and the texture ops.
	// note TexInfo is defined in mlt.h
	virtual void GetGfxTexInfoFromTexmap(TimeValue t, TextureInfo& texinf, Texmap *txm)=0;
	virtual BOOL NumberTexturesSupported()=0; // how many textures does the hardware+driver support
	};


// passed to SetPickMode. This is a callback that gets called as
// the user tries to pick objects in the scene.
class PickObjectProc {
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


// Interface that is passed in to the Mtl or Texture Map when it is in the mtl
// editor.
class IMtlParams : public InterfaceServer {
	public:
	// call after mouse up's in mtls params
	// It causes the viewports to be redrawn.
	virtual void MtlChanged()=0;  

	// Adds rollup pages to the Material Params. Returns the window
	// handle of the dialog that makes up the page.
	virtual HWND AddRollupPage( HINSTANCE hInst, TCHAR *dlgTemplate, 
		DLGPROC dlgProc, TCHAR *title, LPARAM param=0,DWORD flags=0, int category = ROLLUP_CAT_STANDARD )=0;

	virtual HWND AddRollupPage( HINSTANCE hInst, DLGTEMPLATE *dlgTemplate, 
		DLGPROC dlgProc, TCHAR *title, LPARAM param=0,DWORD flags=0, int category = ROLLUP_CAT_STANDARD )=0;


	// Adds rollup pages to the Material Params. Returns the window
	// handle of the dialog that makes up the page.
	virtual HWND ReplaceRollupPage( HWND hOldRollup, HINSTANCE hInst, TCHAR *dlgTemplate, 
		DLGPROC dlgProc, TCHAR *title, LPARAM param=0,DWORD flags=0, int category = ROLLUP_CAT_STANDARD  )=0;

	virtual HWND ReplaceRollupPage( HWND hOldRollup, HINSTANCE hInst, DLGTEMPLATE *dlgTemplate, 
		DLGPROC dlgProc, TCHAR *title, LPARAM param=0,DWORD flags=0, int category = ROLLUP_CAT_STANDARD  )=0;

	// Removes a rollup page and destroys it.  When a dialog is destroyed
	// it need not delete all its rollup pages: the Mtl Editor will do
	// this for it, and it is more efficient.
	virtual void DeleteRollupPage( HWND hRollup )=0;

	// When the user mouses down in dead area, the plug-in should pass
	// mouse messages to this function which will pass them on to the rollup.
	virtual void RollupMouseMessage( HWND hDlg, UINT message, 
				WPARAM wParam, LPARAM lParam )=0;

	virtual int IsRollupPanelOpen(HWND hwnd)=0;
	virtual int GetRollupScrollPos()=0;
	virtual void SetRollupScrollPos(int spos)=0;

	virtual void RegisterTimeChangeCallback(TimeChangeCallback *tc)=0;
	virtual void UnRegisterTimeChangeCallback(TimeChangeCallback *tc)=0;

	// Registers a dialog window so IsDlgMesage() gets called for it.
	// This is done automatically for Rollup Pages.
	virtual void RegisterDlgWnd( HWND hDlg )=0;
	virtual int UnRegisterDlgWnd( HWND hDlg )=0;

	// get the current time.
	virtual TimeValue GetTime()=0;	

	// Pick an object from the scene
	virtual void SetPickMode(PickObjectProc *proc)=0;
	virtual void EndPickMode()=0;

	// JBW 10/19/98: get interface to mtl editor rollup
	virtual IRollupWindow *GetMtlEditorRollup()=0;

	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
	};

class ParamDlg;
class Texmap;

class RenderData : public InterfaceServer {
	public:
		virtual void DeleteThis() {delete this;/*should be pure virtual*/}
		virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0;} 
	};

class LightDesc : public RenderData {
	public:
		BOOL affectDiffuse;
		BOOL affectSpecular;
		BOOL  ambientOnly;
		DWORD  extra;
		LightDesc() { affectDiffuse = affectSpecular = TRUE; }
		// determine color and direction of illumination: return FALSE if light behind surface.
		// also computes dot product with normal dot_nl, and diffCoef which is to be used in
		// place of dot_n for diffuse light computations.
	    virtual BOOL Illuminate(ShadeContext& sc, Point3& normal, Color& color, Point3 &dir, float &dot_nl, float &diffuseCoef){ return 0;}
		
		virtual Point3 LightPosition() { return Point3(0,0,0); } 

	};

#define SHADELIM_FLAT 1
#define SHADELIM_GOURAUD 2
#define SHADELIM_PHONG 3

// Transform Reference frames: 
enum RefFrame { REF_CAMERA=0, REF_WORLD, REF_OBJECT };

#define N_MAX_RENDER_ELEMENTS	32

class ShadeOutput : public BaseInterfaceServer {
	public:
		ULONG flags;
		Color c;  // shaded color
		Color t;  // transparency
		float ior;  // index of refraction
		int gbufId;

		// render elements
		int nElements;
		AColor	elementVals[ N_MAX_RENDER_ELEMENTS ];

		CoreExport void MixIn(ShadeOutput& a, float f);  // (*this) =  (1-f)*(*this) + f*a;
//		ShadeOutput(){};
		ShadeOutput():nElements(0), flags(0), gbufId(0){};
		ShadeOutput( int nEle ):flags(0), gbufId(0){ nElements = nEle; }
		void Reset( int nEle = -1 )
		{ 
			flags = 0;
			gbufId = 0;
			c.Black(); t.Black(); ior = 1.0f;
			// clear nEle's worth of render elements
			if (nEle >= 0)
				nElements = nEle;
			for( int i=0; i < nElements; ++i )
				elementVals[i].Black();
		}
		ShadeOutput* Clone(){
			ShadeOutput* pNew = new ShadeOutput;
			pNew->flags = flags; pNew->c = c; pNew->t = t; pNew->ior = ior;
			pNew->gbufId = gbufId; pNew->nElements = nElements;
			for( int i=0; i < nElements; ++i )
				pNew->elementVals[i] = elementVals[i];
			return pNew;
		}

		// Render Element interface
		// allocate new val, return index
		int NewElementVal(){
			DbgAssert((nElements+1)<N_MAX_RENDER_ELEMENTS); 
			return nElements++; 
		}
		int NElementVals(){ return nElements; }
		// override new val & set nElements directly, for clear mostly
		void SetNElements( int n ){
			DbgAssert( n>=0 && n<N_MAX_RENDER_ELEMENTS); 
			nElements = n;
		}
		// return rgba value of element n
		AColor ElementVal( int n ){ 
			DbgAssert( n>=0 && n<N_MAX_RENDER_ELEMENTS); 
			return elementVals[n]; 
		}
		// set rgba value of element n
		void SetElementVal( int n, AColor c ){ 
			DbgAssert( n>=0 && n<N_MAX_RENDER_ELEMENTS); 
			elementVals[n] = c; 
		}
		// sum rgba value intof element n
		void AccumElementVal( int n, AColor c ){ 
			DbgAssert( n>=0 && n<N_MAX_RENDER_ELEMENTS); 
			elementVals[n] += c; 
		}
		// sum all color components of a shade output
		void Sum( ShadeOutput* pSample ){
			c += pSample->c; t += pSample->t;
			for( int i = 0; i < nElements; ++i )
				elementVals[i] += pSample->elementVals[i];
		}
		void Scale( float x ){
			c *= x; t *= x;
			for( int i = 0; i < nElements; ++i )
				elementVals[i] *= x;
		}
		// sum all color components, w/o alpha of a shade output
		void SumClr( ShadeOutput* pSample ){
			c += pSample->c; 
			for( int i = 0; i < nElements; ++i ){
				elementVals[i].r += pSample->elementVals[i].r;
				elementVals[i].g += pSample->elementVals[i].g;
				elementVals[i].b += pSample->elementVals[i].b;
			}
		}

		ShadeOutput& operator += ( ShadeOutput& a ){
			c += a.c;  
			t += a.t; 
			for( int i = 0; i < nElements; ++i )
				elementVals[i] += a.elementVals[i];
			return *this;
		}
/*		ShadeOutput& operator *= ( float x ){
			c *= x; t *= x; 
			for( int i = 0; i < nElements; ++i )
				elementVals[i] *= x;
			return *this;
		}
*/
		ShadeOutput& operator *= ( Color x ){
			c *= x; // t *= x; 
			AColor ac( x );
			ac.a = Intens( x );
			for( int i = 0; i < nElements; ++i )
				elementVals[i] *= ac;
			return *this;
		}

		ShadeOutput& operator = ( ShadeOutput& a ){
			flags = a.flags;
			c = a.c; t = a.t; 
			ior = a.ior;
			gbufId = a.gbufId;
			nElements = a.nElements;
			for( int i = 0; i < nElements; ++i )
				elementVals[i] = a.elementVals[i];
			return *this;
		}


	};


class RenderInstance;

struct ISect {
	float t;   // ray parameter
	BOOL exit;
	BOOL backFace;
	RenderInstance *inst;
	int fnum;  // face number
	Point3 bc; // barycentric coords
	Point3 p;  // intersection point, object coords
	Point3 pc;  // intersection point, camera coords
	ULONG matreq;
	int mtlNum;
	ISect *next;
	};

class ISectList {
	public:
	ISect *first;
	ISectList() { first = NULL; }
	~ISectList() { Init(); }
	BOOL IsEmpty() { return first?0:1; }
	CoreExport void Add(ISect *is); 
	CoreExport void Prune(float a); 
	CoreExport void Init();
	};
	
CoreExport ISect *GetNewISect();
CoreExport void DiscardISect(ISect *is);

// Value given to blurFrame for non blurred objects  ( Object motion blur)
#define NO_MOTBLUR (-1) 

class FilterKernel;
class IRenderElement;
class ToneOperator;
typedef 	Tab<IRenderElement*>	RenderElementTab;

class RenderGlobalContext : public BaseInterfaceServer
{
protected:
	IRenderElementMgr *mpIRenderElementMgr; // current render element manager

public:
	Renderer *renderer;
	int projType; 	// PROJ_PERSPECTIVE or PROJ_PARALLEL
	int devWidth, devHeight;
	float xscale, yscale;
	float xc,yc;
	BOOL antialias;
	Matrix3 camToWorld, worldToCam;
	float nearRange, farRange;
	float devAspect;          // PIXEL aspect ratio of device pixel H/W
	float frameDur;
	Texmap *envMap;
	Color globalLightLevel;
	Atmospheric *atmos;
	ToneOperator* pToneOp;	// The tone operator, may be NULL
	TimeValue time;
	BOOL wireMode;		// wire frame render mode?
	float wire_thick;	// global wire thickness
	BOOL force2Side;	// is force two-sided rendering enabled
	BOOL inMtlEdit;		// rendering in mtl editor?
	BOOL fieldRender;	// are we rendering fields
	BOOL first_field;	// is this the first field or the second?
	BOOL field_order;	// which field is first: 0->even first,  1->odd first

	BOOL objMotBlur;	// is object motion blur enabled
	int nBlurFrames;	// number of object motion blur time slices


	// Methods
	RenderGlobalContext() : pToneOp(NULL), mpIRenderElementMgr(NULL) { }

	// render element methods
	IRenderElementMgr *GetRenderElementMgr() { return mpIRenderElementMgr; }
	void SetRenderElementMgr(IRenderElementMgr *pIRenderElementMgr) { mpIRenderElementMgr = pIRenderElementMgr; }
	int NRenderElements()
	{
		return mpIRenderElementMgr ? mpIRenderElementMgr->NumRenderElements() : 0;
	} 
	IRenderElement* GetRenderElement(int n)
	{
		return mpIRenderElementMgr ? mpIRenderElementMgr->GetRenderElement(n) : NULL;
	}
	
	Point2 MapToScreen(Point3 p)
	{ 	
		if (projType==1)
		{
			return Point2(  xc + xscale*p.x , yc + yscale*p.y);
		}
		else
		{
			if (p.z==0.0f)
				p.z = .000001f;
			return Point2( xc + xscale*p.x/p.z,  yc + yscale*p.y/p.z);
		}
	}

	virtual FilterKernel* GetAAFilterKernel(){ return NULL; }
	virtual float GetAAFilterSize(){ return 0.0f; }

	// Access RenderInstances
	virtual int NumRenderInstances() { return 0; }
	virtual RenderInstance* GetRenderInstance( int i ) { return NULL; }

	// Evaluate the global environment map using ray as point of view,
	// optionally with atmospheric effect.
	virtual AColor EvalGlobalEnvironMap(ShadeContext &sc, Ray &r, BOOL applyAtmos)
	{ 
		return AColor(0.0f,0.0f,0.0f,1.0f); 
	}

	virtual void IntersectRay(RenderInstance *inst, Ray& ray, ISect &isct, ISectList &xpList, BOOL findExit) {}
	virtual BOOL IntersectWorld(Ray &ray, int skipID, ISect &hit, ISectList &xplist, int blurFrame = NO_MOTBLUR) { return FALSE; }

	virtual ViewParams *GetViewParams() { return NULL; } 

	virtual FILE* DebugFile() { return NULL; }

	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
};


#define SCMODE_NORMAL  0
#define SCMODE_SHADOW  1

//
// Shade Context: passed into Mtls and Texmaps
//
class ShadeContext : public InterfaceServer {
	public:
	ULONG mode;							// normal, shadow ...
	BOOL doMaps;						// apply texture maps?
	BOOL filterMaps;					// should texture be filtered		            
	BOOL shadow;						// apply shadows?
	BOOL backFace;						// are we on the back side of a 2-Sided face?
	int mtlNum;							// sub-mtl number for multi-materials
	Color ambientLight;					// ambient light 
	int nLights;						// number of lights;
	int rayLevel;
	int xshadeID;  						// different for every shade call in a render.
	LightDesc *atmosSkipLight;
	RenderGlobalContext *globContext;
	ShadeOutput out;                    // where a shading leaves its results

	// Reset output & render elements: 
	// param < 0 clears existing elements, leaving nElements unchanged
	// n >= 0 clears n elements & set nElements to n, default is 0
	void ResetOutput( int n = -1 ) { out.Reset(n); }	
	
	virtual Class_ID ClassID() { return Class_ID(0,0); }  // to distinguish different ShadeContexts.
	virtual BOOL InMtlEditor()=0;	// is this rendering the mtl editor sample sphere?
	virtual int Antialias() {return 0;}
	virtual int ProjType() { return 0;} // returns: 0: perspective, 1: parallel
	virtual LightDesc* Light(int n)=0;	// get the nth light. 
	virtual TimeValue CurTime()=0;     	// current time value
	virtual int NodeID() { return -1; }
	virtual INode *Node() { return NULL; }
	virtual Object *GetEvalObject() { return NULL; } // Returns the evaluated object for this node. 
												   // Will be NULL if object is motion blurred	
	virtual Point3 BarycentricCoords() { return Point3(0,0,0);}  // coords relative to triangular face 
	virtual int FaceNumber()=0;			// 
	virtual Point3 Normal()=0;  		// interpolated surface normal, in cameara coords: affected by SetNormal()
	virtual void SetNormal(Point3 p) {}	// used for perturbing normal
	virtual Point3 OrigNormal() { return Normal(); } // original surface normal: not affected by SetNormal();
	virtual Point3 GNormal()=0; 		// geometric (face) normal
	virtual float  Curve() { return 0.0f; }   	    // estimate of dN/dsx, dN/dsy
	virtual Point3 V()=0;       		// Unit view vector: from camera towards P 
	virtual void SetView(Point3 p)=0;	// Set the view vector
	virtual Point3 OrigView() { return V(); } // Original view vector: not affected by SetView();
	virtual	Point3 ReflectVector()=0;	// reflection vector
	virtual	Point3 RefractVector(float ior)=0;	// refraction vector
	virtual void SetIOR(float ior) {} // Set index of refraction
	virtual float GetIOR() { return 1.0f; } // Get index of refraction
	virtual Point3 CamPos()=0;			// camera position
	virtual Point3 P()=0;				// point to be shaded;
	virtual Point3 DP()=0;    		  	// deriv of P, relative to pixel, for AA
	virtual void DP(Point3& dpdx, Point3& dpdy){};  // deriv of P, relative to pixel
	virtual Point3 PObj()=0;   		  	// point in obj coords
	virtual Point3 DPObj()=0;   	   	// deriv of PObj, rel to pixel, for AA
	virtual Box3 ObjectBox()=0; 	  	// Object extents box in obj coords
	virtual Point3 PObjRelBox()=0;	  	// Point rel to obj box [-1 .. +1 ] 
	virtual Point3 DPObjRelBox()=0;	  	// deriv of Point rel to obj box [-1 .. +1 ] 
	virtual void ScreenUV(Point2& uv, Point2 &duv)=0; // screen relative uv (from lower left)
	virtual IPoint2 ScreenCoord()=0; // integer screen coordinate (from upper left)
	virtual Point2 SurfacePtScreen(){ return Point2(0.0,0.0); } // return the surface point in screen coords

	virtual Point3 UVW(int channel=0)=0;  			// return UVW coords for point
	virtual Point3 DUVW(int channel=0)=0; 			// return UVW derivs for point
	virtual void DPdUVW(Point3 dP[3],int channel=0)=0; // Bump vectors for UVW (camera space)

	// BumpBasisVectors: this is going to replace DPdUVW: Now compute bump vectors for 
	//  specific 2D cases only.
	//    axis = AXIS_UV, AXIS_VW, AXIS_WU		
	//    return:     0 => not implemented,  1=> implemented
	// If this function is implemented, it will be used instead of DPdUVW.
	virtual int BumpBasisVectors(Point3 dP[2], int axis, int channel=0) { return 0; } 


	virtual BOOL IsSuperSampleOn(){ return FALSE; }
	virtual BOOL IsTextureSuperSampleOn(){ return FALSE; }
	virtual int GetNSuperSample(){ return 0; }
	virtual float GetSampleSizeScale(){ return 1.0f; }

	// UVWNormal: returns a vector in UVW space normal to the face in UVW space. This can 
	// be CrossProd(U[1]-U[0],U[2]-U[1]), where U[i] is the texture coordinate
	// at the ith vertex of the current face.  Used for hiding textures on
	// back side of objects.
	virtual Point3 UVWNormal(int channel=0) { return Point3(0,0,1); }  

	// diameter of view ray at this point
	virtual float RayDiam() { return Length(DP()); } 

	// angle of ray cone hitting this point: gets increased/decreased by curvature 
	// on reflection
	virtual float RayConeAngle() { return 0.0f; } 

	CoreExport virtual AColor EvalEnvironMap(Texmap *map, Point3 view); //evaluate map with given viewDir
	virtual void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG=TRUE)=0;   // returns Background color, bg transparency

	// Camera ranges set by user in camera's UI.
	virtual float CamNearRange() {return 0.0f;}
	virtual float CamFarRange() {return 0.0f;}

	// Transform to and from internal space
	virtual Point3 PointTo(const Point3& p, RefFrame ito)=0; 
	virtual Point3 PointFrom(const Point3& p, RefFrame ifrom)=0; 
	virtual Point3 VectorTo(const Point3& p, RefFrame ito)=0; 
	virtual Point3 VectorFrom(const Point3& p, RefFrame ifrom)=0; 
	CoreExport virtual Point3 VectorToNoScale(const Point3& p, RefFrame ito); 
	CoreExport virtual Point3 VectorFromNoScale(const Point3& p, RefFrame ifrom); 

	// After being evaluated, if a map or material has a non-zero GBufID, it should
	// call this routine to store it into the shade context.
	virtual void SetGBufferID(int gbid) { out.gbufId = gbid; }

	virtual FILE* DebugFile() { return NULL; }

	virtual AColor EvalGlobalEnvironMap(Point3 dir) { return AColor(0,0,0,0); }

	virtual BOOL GetCache(Texmap *map, AColor &c){ return FALSE; }
	virtual BOOL GetCache(Texmap *map, float &f) { return FALSE; }
	virtual BOOL GetCache(Texmap *map, Point3 &p){ return FALSE; }
	virtual void PutCache(Texmap *map, const AColor &c){}
	virtual void PutCache(Texmap *map, const float f) {}
	virtual void PutCache(Texmap *map, const Point3 &p){}
	virtual void TossCache(Texmap *map) {}
	// Generic expansion function
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 

	// These are used to prevent self shadowing by volumetric lights
	LightDesc *GetAtmosSkipLight() { return atmosSkipLight; }
	void SetAtmosSkipLight(LightDesc *lt) { atmosSkipLight = lt; }

	// render element methods
	virtual int NRenderElements() { 
		return globContext ? globContext->NRenderElements():0;
	}
	virtual IRenderElement *GetRenderElement(int n) { 
		return globContext ? globContext->GetRenderElement(n) : NULL;
	}

	// diffuse illum utility, computes incoming diffuse illumination, for matte/shadow
	CoreExport virtual Color DiffuseIllum();

	// used to preshade in different ways
//	virtual IRenderMode GetIRenderMode(){ return IRM_NORMAL; }
//	IRenderMode GetIRenderMode(){ return iRenderMode; }
//	void SetIRenderMode( IRenderMode mode ){ iRenderMode = mode; }

	// NOTE: All of these methods are inline. The inline definitions not
	// declared here are declared in toneop.h

	// Does this tone operator really map physical values to RGB. This method
	// is provided so shaders can determine whether the shading calculations
	// are in physical or RGB space.
	bool IsPhysicalSpace() const;

	// Map an scaled energy value into RGB. The first version of the
	// method converts a color value and the second converts a monochrome
	// value. The converted color value is stored in <i>energy</i>.
	// The converted monochrome value is returned.
	template< class T >
	void ScaledToRGB( T& energy ) const
		{ if ( globContext != NULL && globContext->pToneOp != NULL )
			globContext->pToneOp->ScaledToRGB( energy ); }
	float ScaledToRGB( float energy ) const;

	// Map an energy value int out.c into RGB. The converted value is stored in
	// out.c.
	void ScaledToRGB( );

	// Scale physical values so they can be used in the renderer. The
	// first version of the method converts a color value and the second
	// converts a monochrome value. The converted color value is stored
	// in <i>energy</i>. The converted monochrome value is returned.
	template< class T >
	void ScalePhysical(T& energy) const
		{ if ( globContext != NULL && globContext->pToneOp != NULL )
			globContext->pToneOp->ScalePhysical( energy ); }
	float ScalePhysical(float energy) const;

	// Scale RGB values, just supplied to invert ScalePhysical. The first
	// version of the method converts a color value and the second
	// converts a monochrome value. The converted color value is stored
	// in <i>energy</i>. The converted monochrome value is returned.
	template< class T >
	void ScaleRGB(T& energy) const
		{ if ( globContext != NULL && globContext->pToneOp != NULL )
			globContext->pToneOp->ScaleRGB( energy ); }
	float ScaleRGB(float energy) const;

	ShadeContext() {
		mode = SCMODE_NORMAL; nLights = 0; shadow = TRUE;  rayLevel = 0; 
		globContext = NULL; atmosSkipLight = NULL; 
	}
};

		
// Material and Texmap flags values
#define MTL_IN_SCENE          	(1<<0)
#define MTL_BEING_EDITED     	(1<<1)  // mtl params being displayed in medit
#define MTL_SUB_BEING_EDITED 	(1<<2)  // sub mtl or tex being displayed in medit 
#define MTL_TEX_DISPLAY_ENABLED (1<<3)  // Viewport display enabled for THIS mtl/tex (see MTL_SUB_DISPLAY_ENABLED)
#define MTL_MEDIT_BACKGROUND    (1<<8)  // Show background in Mtl Editor
#define MTL_MEDIT_BACKLIGHT		(1<<9)  // Backlight in Mtl Editor

#define MTL_OBJTYPE_SHIFT		10
#define MTL_MEDIT_OBJTYPE		(1<<MTL_OBJTYPE_SHIFT) // Object type displayed in Mtl Editor
#define MTL_MEDIT_OBJTYPE_MASK	((1<<MTL_OBJTYPE_SHIFT)|(1<<(MTL_OBJTYPE_SHIFT+1))|(1<<(MTL_OBJTYPE_SHIFT+2)))

#define MTL_TILING_SHIFT		13
#define MTL_MEDIT_TILING		(1<<MTL_TILING_SHIFT) // Object type displayed in Mtl Editor
#define MTL_MEDIT_TILING_MASK	((1<<MTL_TILING_SHIFT)|(1<<(MTL_TILING_SHIFT+1))|(1<<(MTL_TILING_SHIFT+2)))
#define MTL_MEDIT_VIDCHECK		(1<<16)
#define MTL_BROWSE_OPEN1		(1<<18)
#define MTL_BROWSE_OPEN2		(1<<19)
#define MTL_SUB_DISPLAY_ENABLED (1<<20)  // flag indicates that texture display is enabled for map or mtl in this subtree
#define MTL_WORK_FLAG			(1<<31)

#define MTL_DISPLAY_ENABLE_FLAGS (MTL_TEX_DISPLAY_ENABLED|MTL_SUB_DISPLAY_ENABLED)  // Interactive texture display enabled for THIS mtl base

// Material Requirements flags: returned by Requirements() function
#define MTLREQ_2SIDE    		(1<<0)  // 2-sided material
#define MTLREQ_WIRE     		(1<<1)  // Wire frame
#define MTLREQ_WIRE_ABS 		(1<<2)  // Wire frame, absolute size
#define MTLREQ_TRANSP   		(1<<3) 	// transparency
#define MTLREQ_UV				(1<<4)  // requires UVW coords
#define MTLREQ_FACEMAP			(1<<5)  // use "face map" UV coords
#define MTLREQ_XYZ				(1<<6)  // requires object XYZ coords
#define MTLREQ_OXYZ 			(1<<7)  // requires object ORIGINAL XYZ coords
#define MTLREQ_BUMPUV			(1<<8)  // requires UV bump vectors
#define MTLREQ_BGCOL			(1<<9)  // requires background color (e.g. Matte mtl)
#define MTLREQ_PHONG			(1<<10) // requires interpolated normal
#define MTLREQ_AUTOREFLECT 		(1<<11) // needs to build auto-reflect map
#define MTLREQ_AUTOMIRROR 		(1<<12) // needs to build auto-mirror map
#define MTLREQ_NOATMOS 			(1<<13) // suppress atmospheric shader (used by Matte mtl)
#define MTLREQ_ADDITIVE_TRANSP	(1<<14) // composite additively 
#define MTLREQ_VIEW_DEP			(1<<15) // mtl/map is view dependent 
#define MTLREQ_UV2				(1<<16)  // requires second uv channel values (vertex colors)
#define MTLREQ_BUMPUV2			(1<<17)  // requires second uv channel bump vectors
#define MTLREQ_PREPRO	    	(1<<18)  // Pre-processing. BuildMaps will be called on every frame
#define MTLREQ_DONTMERGE_FRAGMENTS 	(1<<19) // inhibit sub-pixel fragment merging, when mtl creates
										    // discontinuities across edges that would normally be smooth.
#define MTLREQ_DISPLACEMAP 		(1<<20) //Material has Displacement map channel
#define MTLREQ_SUPERSAMPLE 		(1<<21) //Material requires supersampling
#define MTLREQ_WORLDCOORDS 		(1<<22) //World coordinates are used in mtl/map evaluation
#define MTLREQ_TRANSP_IN_VP  	(1<<23) //transparent in viewport ( opacity < 100, or opacity map & ShowMapInVP set)
#define MTLREQ_FACETED		  	(1<<24) //render faceted in the viewports

#define MTLREQ_REND1  	(1<<25) // USED IN RENDERER.
#define MTLREQ_REND2  	(1<<26) // USED IN RENDERER.
#define MTLREQ_REND3  	(1<<27) // USED IN RENDERER.
#define MTLREQ_REND4  	(1<<28) // USED IN RENDERER.
#define MTLREQ_REND5 	(1<<29) // USED IN RENDERER.
#define MTLREQ_REND6 	(1<<30) // USED IN RENDERER.
#define MTLREQ_REND7 	(1<<31) // USED IN RENDERER.

#define MAPSLOT_TEXTURE      0 	// texture maps: 
#define MAPSLOT_ENVIRON      1 	// environment maps: generate UVW on-the-fly using view vector, default to spherical
#define MAPSLOT_DISPLACEMENT 2 	// displacement maps: a type of texture map.
#define MAPSLOT_BACKGROUND   3 	// background maps: generate UVW on-the-fly using view vector, default to screen

// Where to get texture vertices: returned by GetUVWSource();
#define UVWSRC_EXPLICIT   0		// use explicit mesh texture vertices (whichever map channel)
#define UVWSRC_OBJXYZ	  1 	// generate planar uvw mapping from object xyz on-the-fly
#define UVWSRC_EXPLICIT2  2  	// use explicit mesh map color vertices as a tex map
#define UVWSRC_WORLDXYZ   3  	// generate planar uvw mapping from world xyz on-the-fly
#ifdef DESIGN_VER
#define UVWSRC_GEOXYZ	  4		// generate planar uvw mapping from geo referenced world xyz on-the-fly
#endif

class ISubMap {
public:
	
	// Methods to access sub texture maps of material or texmap
	virtual int NumSubTexmaps() { return 0; }
	virtual Texmap* GetSubTexmap(int i) { return NULL; }
	virtual int MapSlotType(int i)=0;
	virtual void SetSubTexmap(int i, Texmap *m) { }

	// query ISubMap about the On/Off state of each sub map.
	virtual int SubTexmapOn(int i) { return 1; } 

	CoreExport virtual TSTR GetSubTexmapSlotName(int i);
	CoreExport TSTR GetSubTexmapTVName(int i);

	// called by the ParamMap2 AUTO_UI system if the material/map is letting
	// the system build an AutoMParamDlg for it.  SetDlgThing() is called on
	// a material/map coming into an exsting set of ParamDlgs, once for each 
	// secondary ParamDlg and it should set the appropriate 'thing' into the
	// given dlg (for example, Texout* or UVGen*).  Return FALSE if dlg is unrecognized.
	// See Gradient::SetDlgThing() for an example.
	virtual BOOL SetDlgThing(ParamDlg* dlg) { return FALSE; }

	// use this for drag-and-drop of texmaps
	CoreExport void CopySubTexmap(HWND hwnd, int ifrom, int ito);
	
	virtual ReferenceTarget *GetRefTarget()=0;
};

// Base class from which materials and textures are subclassed.
class MtlBase: public ReferenceTarget, public ISubMap {
	friend class Texmap;
	TSTR name;
	ULONG mtlFlags;
	int defaultSlotType;
	int activeCount;
	public:
		MtlBase *activeMB; 
		Quat meditRotate;
		ULONG gbufID;
		CoreExport MtlBase();
		TSTR& GetName() { return name; }
		CoreExport void SetName(TSTR s);
		void SetMtlFlag(int mask, BOOL val=TRUE) { 
			if (val) mtlFlags |= mask; else mtlFlags &= ~mask;
			}
		void ClearMtlFlag(int mask) { mtlFlags &= ~mask; }
		int TestMtlFlag(int mask) { return(mtlFlags&mask?1:0); }

		// Used internally by materials editor.
		int GetMeditObjType() { return (mtlFlags&MTL_MEDIT_OBJTYPE_MASK)>>MTL_OBJTYPE_SHIFT; }	
		void SetMeditObjType(int t) { mtlFlags &= ~MTL_MEDIT_OBJTYPE_MASK; mtlFlags |= t<<MTL_OBJTYPE_SHIFT; }
		int GetMeditTiling() { return (mtlFlags&MTL_MEDIT_TILING_MASK)>>MTL_TILING_SHIFT; }	
		void SetMeditTiling(int t) { mtlFlags &= ~MTL_MEDIT_TILING_MASK; mtlFlags |= t<<MTL_TILING_SHIFT; }

		// recursively determine if there are any multi-materials or texmaps 
		// in tree
		CoreExport BOOL AnyMulti();

		BOOL TextureDisplayEnabled() { return TestMtlFlag(MTL_TEX_DISPLAY_ENABLED); }

		// Return the "className(instance Name)". 
		// The default implementation should be used in most cases.
	    CoreExport virtual TSTR GetFullName();

		// Mtls and Texmaps must use this to copy the common portion of 
		// themselves when cloning
		CoreExport MtlBase& operator=(const MtlBase& m);

		virtual int BuildMaps(TimeValue t, RenderMapsContext &rmc) { return 1; }

		// This gives the cumulative requirements of the mtl and its
		// tree. The default just OR's together the local requirements
		// of the Mtl with the requirements of all its children.
		// For most mtls, all they need to implement is LocalRequirements,
		// if any.
		CoreExport virtual ULONG Requirements(int subMtlNum); 

		// Specifies various requirements for the material: Should NOT
		// include requirements of its sub-mtls and sub-maps.
		virtual ULONG LocalRequirements(int subMtlNum) { return 0; } 

		// This gives the UVW channel requirements of the mtl and its
		// tree. The default implementation just OR's together the local mapping requirements
		// of the Mtl with the requirements of all its children.
		// For most mtls, all they need to implement is LocalMappingsRequired, if any.
		// mapreq and bumpreq will be initialized to empty sets with MAX_MESHMAPS elements
		// set 1's in mapreq for uvw channels required
		// set 1's in bumpreq for bump mapping channels required
		CoreExport virtual void MappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq);

		// Specifies UVW channel requirements for the material: Should NOT
		// include UVW channel requirements of its sub-mtls and sub-maps.
		virtual void LocalMappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq) {  }

		// This returns true for materials or texmaps that select sub-
		// materials based on mesh faceMtlIndex. Used in 
		// interactive render.
		virtual	BOOL IsMultiMtl() { return FALSE; }

		// Methods to access sub texture maps of material or texmap
		virtual int MapSlotType(int i) { return defaultSlotType; }

		// This must be called on a sub-Mtl or sub-Map when it is removed,
		// in case it or any of its submaps are active in the viewport.
		CoreExport void DeactivateMapsInTree();

		// To make texture & material evaluation more efficient, this function is
		// called whenever time has changed.  It will be called at the
		// beginning of every frame during rendering.
		virtual	void Update(TimeValue t, Interval& valid)=0;

		// set back to default values
		virtual void Reset()=0;

		// call this to determine the validity interval of the mtl or texture
		virtual Interval Validity(TimeValue t)=0;
		
		// this gets called when the mtl or texture is to be displayed in the
		// mtl editor params area.
		virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)=0;
		
		// save the common mtlbase stuff.
		// these must be called in a chunk at the beginning of every mtl and
		// texmap.
		CoreExport IOResult Save(ISave *isave);
        CoreExport IOResult Load(ILoad *iload);
		
		// GBuffer functions
		ULONG GetGBufID() { return gbufID; }
		void SetGBufID(ULONG id) { gbufID = id; }
		
		// Default File enumerator.
		CoreExport void EnumAuxFiles(NameEnumCallback& nameEnum, DWORD flags);

		// Postage Stamp
		CoreExport PStamp* GetPStamp(int sz); // sz = 0: small(32x32), 1: large(88x88), 2: tiny(24x24)  		
		CoreExport PStamp* CreatePStamp(int sz, BOOL Render = FALSE);    		
		CoreExport void DiscardPStamp(int sz);   		

		// Schematic View Animatable Overides...
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
		CoreExport bool SvHandleDoubleClick(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited);
		CoreExport bool SvCanSetName(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvSetName(IGraphObjectManager *gom, IGraphNode *gNode, TSTR &name);
		CoreExport COLORREF SvHighlightColor(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvIsSelected(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport MultiSelectCallback* SvGetMultiSelectCallback(IGraphObjectManager *gom, IGraphNode *gNode);
		CoreExport bool SvCanSelect(IGraphObjectManager *gom, IGraphNode *gNode);

		// Methods for doing interactive texture display
		virtual BOOL SupportTexDisplay() { return FALSE; }   // supports map display in viewport
		virtual DWORD_PTR GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker) {return 0;}
		CoreExport void IncrActive();
		CoreExport void DecrActive(); 
		int Active() { return activeCount; }
		virtual void ActivateTexDisplay(BOOL onoff) {}

		// Multiple textures in viewports:
		virtual BOOL SupportsMultiMapsInViewport() { return FALSE; }
		virtual void SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb) {}
		virtual void* GetInterface(ULONG id){ if(id==I_SUBMAP) return (ISubMap*)this; else return ReferenceTarget::GetInterface(id); }
		virtual BaseInterface *GetInterface(Interface_ID id) { return ReferenceTarget::GetInterface(id); }
		virtual ReferenceTarget *GetRefTarget(){return this;}
	};


// Every MtlBase sub-class defines a ParamDlg to manage its part of
// the material editor.
class ParamDlg : public InterfaceServer {
	public:
		virtual Class_ID ClassID()=0;
		virtual void SetThing(ReferenceTarget *m)=0;
		virtual ReferenceTarget* GetThing()=0;
		virtual void SetTime(TimeValue t)=0;
		virtual	void ReloadDialog()=0;
		virtual void DeleteThis()=0;
		virtual void ActivateDlg(BOOL onOff)=0;
		// These need to be implemented if using a TexDADMgr or a MtlDADMgr
		virtual int FindSubTexFromHWND(HWND hwnd){ return -1;} 
		virtual int FindSubMtlFromHWND(HWND hwnd){ return -1;} 
		// Generic expansion function
		virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 

	};

// Pre-defined categories of texture maps
CoreExport TCHAR TEXMAP_CAT_2D[];  // 2D maps
CoreExport TCHAR TEXMAP_CAT_3D[];  // 3D maps
CoreExport TCHAR TEXMAP_CAT_COMP[]; // Composite
CoreExport TCHAR TEXMAP_CAT_COLMOD[]; // Color modifier
CoreExport TCHAR TEXMAP_CAT_ENV[];  // Environment

class NameAccum {
	public:
		virtual	void AddMapName(TCHAR *name)=0;
	};


// virual texture map interface
class Texmap: public MtlBase {
	public:
		int cacheID;    // used by renderer for caching returned values
				
		CoreExport Texmap();

		SClass_ID SuperClassID() { return TEXMAP_CLASS_ID; }
		virtual void GetClassName(TSTR& s) { s= TSTR(_T("Texture")); }  

		// Evaluate the color of map for the context.
		virtual	AColor EvalColor(ShadeContext& sc)=0;
	
		// Evaluate the map for a "mono" channel.
		// this just permits a bit of optimization 
		virtual	float  EvalMono(ShadeContext& sc) {
			return Intens(EvalColor(sc));
			}
		
		// For Bump mapping, need a perturbation to apply to a normal.
		// Leave it up to the Texmap to determine how to do this.
		virtual	Point3 EvalNormalPerturb(ShadeContext& sc)=0;

		// This query is made of maps plugged into the Reflection or 
		// Refraction slots:  Normally the view vector is replaced with
		// a reflected or refracted one before calling the map: if the 
		// plugged in map doesn't need this , it should return TRUE.
		virtual BOOL HandleOwnViewPerturb() { return FALSE; }


		// This gets the viewport display bitmap in DIB format, useful when combining several maps
		// for hardware-supported multiple texture display.  // DS 4/17/00
		// IF mono=TRUE, the map should do a mono evaluation and place the result in RGB.
		// forceW and forceH, if non-zero, overide dimensions specified by thmaker.
		virtual CoreExport BITMAPINFO* GetVPDisplayDIB(TimeValue t, TexHandleMaker& thmaker, Interval &valid, BOOL mono=FALSE, int forceW=0, int forceH=0);  

		virtual	void GetUVTransform(Matrix3 &uvtrans) { uvtrans.IdentityMatrix(); }
		virtual int GetTextureTiling() { return  U_WRAP|V_WRAP; }
		virtual void InitSlotType(int sType) { defaultSlotType = sType; }			   
		virtual int MapSlotType(int i) { return defaultSlotType; }
		virtual int GetUVWSource() { return UVWSRC_EXPLICIT; }
		virtual int GetMapChannel () { return 1; }	// only relevant if above returns UVWSRC_EXPLICIT

		virtual UVGen *GetTheUVGen() { return NULL; }  // maps with a UVGen should implement this
		virtual XYZGen *GetTheXYZGen() { return NULL; } // maps with a XYZGen should implement this

		// System function to set slot type for all subtexmaps in a tree.
		CoreExport void RecursInitSlotType(int sType);			   
		virtual void SetOutputLevel(TimeValue t, float v) {}

		// called prior to render: missing map names should be added to NameAccum.
		// return 1: success,   0:failure. 
		virtual int LoadMapFiles(TimeValue t) { return 1; } 

		// render a 2-d bitmap version of map.
		CoreExport virtual void RenderBitmap(TimeValue t, Bitmap *bm, float scale3D=1.0f, BOOL filter = FALSE);

		CoreExport void RefAdded(RefMakerHandle rm);

		// Schematic View Animatable Overides...
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);

		// --- Texmap evaluation ---

		// Returns true only if all submaps and itself have a meaningful output.
		// Returns false if at least one sutexmap or itself does not have a 
		// meaningful output,
		CoreExport virtual bool IsOutputMeaningful( ShadeContext& sc );

		// The default implementation returns false
		virtual bool IsLocalOutputMeaningful( ShadeContext& sc ) { return false; }

		// Is the texture High Dynamic Range?
		virtual int IsHighDynamicRange( ) const { return false; }
	};


class TexmapContainer: public ReferenceTarget {
	SClass_ID SuperClassID() { return TEXMAP_CONTAINER_CLASS_ID; }
	CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
	};

// ID's for different DynamicProperties: passed into
// GetDynamicProperty()

#define DYN_BOUNCE 1
#define DYN_STATIC_FRICTION 2
#define DYN_SLIDING_FRICTION 3

// begin - ke/mjm - 03.16.00 - merge reshading code
// class IReshadeFragment;
// end - ke/mjm - 03.16.00 - merge reshading code


// virtual material interface
class Mtl: public MtlBase {
	RenderData *renderData;
	public:

		CoreExport Mtl();
		SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
		virtual void GetClassName(TSTR& s) { s= TSTR(_T("Mtl")); }  

		MtlBase* GetActiveTexmap() { return activeMB; }
		void SetActiveTexmap( MtlBase *txm) { activeMB = txm; }

		CoreExport void RefDeleted();
		CoreExport void RefAdded(RefMakerHandle rm);  

		// Must call Update(t) before calling these functions!
		// Their purpose is for setting up the material for the
		// GraphicsWindow renderer.
		virtual Color GetAmbient(int mtlNum=0, BOOL backFace=FALSE)=0;
		virtual Color GetDiffuse(int mtlNum=0, BOOL backFace=FALSE)=0;	    
		virtual Color GetSpecular(int mtlNum=0, BOOL backFace=FALSE)=0;
		virtual float GetShininess(int mtlNum=0, BOOL backFace=FALSE)=0;
		virtual	float GetShinStr(int mtlNum=0, BOOL backFace=FALSE)=0;		
		virtual float GetXParency(int mtlNum=0, BOOL backFace=FALSE)=0;

		// >>>>> Self-Illum
		virtual BOOL GetSelfIllumColorOn(int mtlNum=0, BOOL backFace=FALSE) { return TRUE; }
		virtual float GetSelfIllum(int mtlNum=0, BOOL backFace=FALSE) { return 0.0f; }
		virtual Color GetSelfIllumColor(int mtlNum=0, BOOL backFace=FALSE){ Color c( .0f,.0f,.0f); return c; }

		// sampler
		virtual Sampler * GetPixelSampler(int mtlNum=0, BOOL backFace=FALSE){ return NULL; }

		// used by the scanline renderer
		virtual float WireSize(int mtlNum=0, BOOL backFace=FALSE) { return 1.0f; }

		// why incomplete sets?? 
		// used onlt for default material creation....add no more
		virtual void SetAmbient(Color c, TimeValue t)=0;		
		virtual void SetDiffuse(Color c, TimeValue t)=0;		
		virtual void SetSpecular(Color c, TimeValue t)=0;
		virtual void SetShininess(float v, TimeValue t)=0;		
		virtual BOOL SupportsShaders() { return FALSE; } // moved from class StdMat

		// if supporting render elements
		virtual BOOL SupportsRenderElements() { return FALSE; } 

/*************
// begin - ke/mjm - 03.16.00 - merge reshading code
		// if supporting shaders, plug-in mtl should inherit from StdMat2
		enum ReshadeRequirements{ RR_None = 0, RR_NeedPreshade, RR_NeedReshade }; // mjm - 06.02.00

		virtual BOOL SupportsReShading(ShadeContext& sc) { return FALSE; }
		// GetReshadeValidity is called by the interactive reshader after being notified that the material has changed.
		// the return value indicates if the material is still valid, or needs to be preshaded or reshaded. the value
		// should pertain only to the latest change to the material
		// if a material doesn't support reshading, it doesn't need to override this function -- any change will cause
		// the nodes to which it's attached to be rerendered.
		virtual ReshadeRequirements GetReshadeRequirements() { return RR_NeedPreshade; } // mjm - 06.02.00
		virtual void PreShade(ShadeContext& sc, IReshadeFragment* pFrag ) { }
		virtual void PostShade(ShadeContext& sc, IReshadeFragment* pFrag, int& nextTexIndex) { }
// end - ke/mjm - 03.16.00 - merge reshading code
***********/

		// The main method: called by the renderer to compute color and transparency
		// output returned in sc.out
		virtual void Shade(ShadeContext& sc)=0;

		// Methods to access sub-materials of meta-materials 
		virtual int NumSubMtls() { return 0; }
		virtual Mtl* GetSubMtl(int i) { return NULL; }
		virtual void SetSubMtl(int i, Mtl *m) { }
//		virtual int SubMtlOn(int i) { return 1; } 
		virtual int VPDisplaySubMtl() { return -1; }  // which sub-mtl to display in the viewport: -1 indicates not implemented.
		CoreExport virtual TSTR GetSubMtlSlotName(int i);
		CoreExport TSTR GetSubMtlTVName(int i);					  
		CoreExport void CopySubMtl(HWND hwnd, int ifrom, int ito);

		// Dynamics properties
		virtual CoreExport float GetDynamicsProperty(TimeValue t, int mtlNum, int propID);
		virtual CoreExport void SetDynamicsProperty(TimeValue t, int mtlNum, int propID, float value);
		
		// Displacement mapping
		virtual float EvalDisplacement(ShadeContext& sc) { return 0.0f; }
		virtual Interval DisplacementValidity(TimeValue t) { return FOREVER; }

		// Schematic View Animatable Overides...
		CoreExport bool SvCanInitiateLink(IGraphObjectManager *gom, IGraphNode *gNode);

		// allows plugins to control the display of the Discard/Keep Old Material dialog when being
		// created in a MtlEditor slot.  Added by JBW for scripted material plugins.
		// return TRUE to prevent the Keep Old Material dialog from poping.
		virtual BOOL DontKeepOldMtl() { return FALSE; }

		// allow renderer to attach auxiliary data to each material.
		virtual void SetRenderData( RenderData *rdata) { renderData = rdata; }
		virtual RenderData *GetRenderData() { return renderData; }

		// --- Material evaluation ---

		// Returns true if the evaluated color\value (output) is constant 
		// over all possible inputs. 
		virtual bool IsOutputConst( ShadeContext& sc, int stdID ) { return false; }

		// Evaluate the material on a single standard texmap channel (ID_AM, etc) 
		// at a UVW cordinated and over an area described in the ShadingContext
		virtual bool EvalColorStdChannel( ShadeContext& sc, int stdID, Color& outClr) { return false; }
		CoreExport virtual bool EvalMonoStdChannel( ShadeContext& sc, int stdID, float& outVal) ;
	};

//  A texture map implements this class and passes it into  EvalUVMap,
//  EvalUVMapMono, and EvalDeriv to evaluate itself with tiling & mirroring
class MapSampler {
	public:
		// required:
		virtual	AColor Sample(ShadeContext& sc, float u,float v)=0;
		virtual	AColor SampleFilter(ShadeContext& sc, float u,float v, float du, float dv)=0;
		// optional:
		virtual void SetTileNumbers(int iu, int iv) {} // called before SampleFunctions to tell what tile your in
		virtual	float SampleMono(ShadeContext& sc, float u,float v) { return Intens(Sample(sc,u,v)); }
		virtual	float SampleMonoFilter(ShadeContext& sc, float u,float v, float du, float dv){
			return Intens(SampleFilter(sc,u,v,du,dv)); 
			}
	};


// This class generates UV coordinates based on the results of a UV 
// Source and user specified transformation.
// A reference to one of these is referenced by all 2D texture maps.
class UVGen: public MtlBase {
	public:
		// Get texture coords and derivatives for antialiasing
		virtual void GetUV( ShadeContext& sc, Point2& UV, Point2& dUV)=0;
		
		// This is how a Texmap evaluates itself
		virtual AColor EvalUVMap(ShadeContext &sc, MapSampler* samp,  BOOL filter=TRUE)=0;
		virtual float EvalUVMapMono(ShadeContext &sc, MapSampler* samp, BOOL filter=TRUE)=0;
		virtual	Point2 EvalDeriv( ShadeContext& sc, MapSampler* samp, BOOL filter=TRUE)=0;

		// Get dPdu and dPdv for bump mapping
		virtual	void GetBumpDP( ShadeContext& sc, Point3& dPdu, Point3& dPdv)=0;

		virtual void GetUVTransform(Matrix3 &uvtrans)=0;

		virtual int GetTextureTiling()=0;
		virtual void SetTextureTiling(int t)=0;

		virtual int GetUVWSource()=0;
		virtual int GetMapChannel () { return 1; }	// only relevant if above returns UVWSRC_EXPLICIT
		virtual void SetUVWSource(int s)=0;
		virtual void SetMapChannel (int s) { }

		virtual int SymFlags()=0;
		virtual void SetSymFlags(int f)=0;

		virtual int GetSlotType()=0; 
		virtual void InitSlotType(int sType)=0; 

		virtual int GetAxis()=0;  // returns AXIS_UV, AXIS_VW, AXIS_WU
		virtual void SetAxis(int ax)=0;  //  AXIS_UV, AXIS_VW, AXIS_WU

		// The clip flag controls whether the U,V values passed to MapSampler by 
		//  EvalUVMap and EvalUVMapMono are clipped to the [0..1] interval or not. 
		// It defaults to ON (i.e., clipped).
		virtual void SetClipFlag(BOOL b)=0;	
		virtual BOOL GetClipFlag()=0;

		virtual BOOL IsStdUVGen() { return FALSE; } // class StdUVGen implements this to return TRUE

		SClass_ID SuperClassID() { return UVGEN_CLASS_ID; }

		virtual void SetRollupOpen(BOOL open)=0;
		virtual BOOL GetRollupOpen()=0;
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
		CoreExport TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited)
			{
			return Animatable::SvGetName(gom, gNode, isBeingEdited);
			}
	};

// This class generates Point3 coordinates based on the ShadeContext.
// A reference to one of these is referenced by all 3D texture maps.
class XYZGen: public MtlBase {
	public:
		// Get texture coords and derivatives for antialiasing
		virtual void GetXYZ( ShadeContext& sc, Point3& p, Point3& dp)=0;
		SClass_ID SuperClassID() { return XYZGEN_CLASS_ID; }
		virtual void SetRollupOpen(BOOL open)=0;
		virtual BOOL GetRollupOpen()=0;
		virtual BOOL IsStdXYZGen() { return FALSE; } // class StdXYZGen implements this to return TRUE
		virtual	void GetBumpDP( ShadeContext& sc, Point3* dP) {}; // returns 3 unit vectors for computing differentials
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
		CoreExport TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited)
			{
			return Animatable::SvGetName(gom, gNode, isBeingEdited);
			}
	};

// This class is used by texture maps to put up the output filter 
// rollup, and perform the output filtering.
class TextureOutput: public MtlBase {
	public:
		virtual AColor Filter(AColor c) = 0;
		virtual float Filter(float f) = 0;
		virtual Point3 Filter(Point3 p) = 0;
		virtual float GetOutputLevel(TimeValue t) = 0;
		virtual void SetOutputLevel(TimeValue t, float v) = 0;
		virtual void SetInvert(BOOL onoff)=0;
		virtual BOOL GetInvert()=0;
		virtual void SetRollupOpen(BOOL open)=0;
		virtual BOOL GetRollupOpen()=0;
		SClass_ID SuperClassID() { return TEXOUTPUT_CLASS_ID; }
		CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
		CoreExport TSTR SvGetName(IGraphObjectManager *gom, IGraphNode *gNode, bool isBeingEdited)
			{
			return Animatable::SvGetName(gom, gNode, isBeingEdited);
			}
	};

//////////////////////////////////////////////////////////////
//
// post mapping params for shaders & render elements
class Shader;

#define STD2_NMAX_TEXMAPS	24

class IllumParams : public BaseInterfaceServer {
public:
	// these are the inputs to the shader, mostly textured channels
//	ULONG mtlFlags;
	Shader* pShader;	// for render elements to call the shader & mtl again, may be null
	Mtl* pMtl;			// max mtl being shaded, null if bgnd
//	Point3 N, V;
	Color channels[ STD2_NMAX_TEXMAPS ];	//inputs, to be textured

	float finalOpac;		// textured opacity w/ stdfalloff (reg opac in channels)
	float reflectAmt;				// combined reflection.a * reflectAmt 
	float refractAmt;				// combined reflection.a * amt 
	ULONG hasComponents;	// bits for active components(e.g.currently has active refl map)
	ULONG stdParams;
	int*  stdIDToChannel;	// combined shader & mtl

	// these are the component-wise outputs from the shading process
	Color ambIllumOut, diffIllumOut, transIllumOut, selfIllumOut; // the diffuse clrs
	Color specIllumOut, reflIllumOut;	// the specular colors

	// User Illumination outputs for render elements, name matched
	int	nUserIllumOut;		// one set of names for all illum params instances
	TCHAR** userIllumNames;  // we just keep ptr to shared name array, not destroyed
	Color* userIllumOut;	// the user illum color array, new'd & deleted w/ the class

	float diffIllumIntens; // used only by reflection dimming, intensity of diffIllum prior to color multiply
	float finalAttenuation; // for combining components

	// these are the outputs of the combiner
	Color finalC;	// final clr: combiner composites into this value.
	Color finalT;	// shader transp clr out

public:
	// Illum params are allocated by materials during shading process to
	// communicate with shaders & render elements
	// So materials need to know how many userIllum slots they will use
	// most materials know this, but std2 will have to get it from the shader
	IllumParams( int nUserOut = 0, TCHAR** pUserNames = NULL ){ 
		nUserIllumOut = nUserOut;
		userIllumOut = ( nUserOut )? new Color[ nUserOut ] : NULL;
		userIllumNames = pUserNames;
	}

//	IllumParams(){ 
//		nUserIllumOut = 0;
//		userIllumOut = NULL;
//		userIllumNames = NULL;
//	}


	~IllumParams(){
	// We Dont destroy the name array as it's shared by all
		if( userIllumOut )
			delete [] userIllumOut;
	}

	// returns number of user illum channels for this material
	int nUserIllumChannels(){ return nUserIllumOut; }

	// returns null if no name array specified
	TCHAR* GetUserIllumName( int n ) { 
		DbgAssert( n < nUserIllumOut );
		if( userIllumNames )
			return userIllumNames[n];
		return NULL;
	}

	// render elements, mtls & shaders can use this to find the index associated with a name
	// returns -1 if it can't find the name
	int FindUserIllumName( TCHAR* name ){
		int n = -1;
		for( int i = 0; i < nUserIllumOut; ++i ){
			DbgAssert( userIllumNames );
			if( strcmp( name, userIllumNames[i] )){
				n = i;
				break;
			}
		}
		return n;
	}

	// knowing the index, these set/get the user illum output color
	void SetUserIllumOutput( int n, Color& out ){
		DbgAssert( n < nUserIllumOut );
		userIllumOut[n] = out;
	}
	void SetUserIllumOutput( TCHAR* name, Color& out ){
		for( int i = 0; i < nUserIllumOut; ++i ){
			DbgAssert( userIllumNames );
			if( strcmp( name, userIllumNames[i] )){
				userIllumOut[i] = out;
				break;
			}
		}
		DbgAssert( i < nUserIllumOut );
	}

	Color GetUserIllumOutput( int n ){
		DbgAssert( n < nUserIllumOut );
		return userIllumOut[n];
	}
	Color GetUserIllumOutput( TCHAR* name, int n ){
		for( int i = 0; i < nUserIllumOut; ++i ){
			DbgAssert( userIllumNames );
			if( strcmp( name, userIllumNames[i] )){
				return userIllumOut[i];
			}
		}
		return Color(0,0,0);
	}

	void ClearOutputs() { 
		finalC = finalT = ambIllumOut=diffIllumOut=transIllumOut=selfIllumOut=
		specIllumOut=reflIllumOut= Color( 0.0f, 0.0f, 0.0f ); 
		finalAttenuation = diffIllumIntens = 0.0f;
		for( int i=0; i < nUserIllumOut; ++i )
			userIllumOut[i] = finalC;
	}

	void ClearInputs() { 
		/*mtlFlags = */ stdParams = hasComponents = 0;
		pShader = NULL; pMtl = NULL;
		stdIDToChannel = NULL;
		refractAmt = reflectAmt = 0.0f; 
		for( int i=0; i < STD2_NMAX_TEXMAPS; ++i )
			channels[ i ] = Color( 0, 0, 0 );
	}
};

///////////////////////////////////////////////////////////////////
//
//	Reshading interfaces
//
class Color32;

class IReshadeFragment : public InterfaceServer 
{
public:
	virtual int	NChannels()=0;
	virtual int	NFirstChannel()=0; 
	virtual int	NTextures()=0;

	virtual Color32	Channel( int nChan )=0;
	virtual void SetChannel( int nChan, Color32 c )=0;
	virtual void AddChannel( Color32 c )=0;

	virtual void AddColorChannel( RGBA tex )=0;
	virtual void AddFloatChannel( float f )=0;
	virtual void AddIntChannel( int i )=0;
	virtual void AddUnitVecChannel( Point3 v )=0;
	virtual void AddPoint2Channel( Point2 p )=0;
	virtual void AddPoint3Channel( Point3 p )=0;

	virtual void SetColorChannel( int nChan, RGBA tex )=0;
	virtual void SetFloatChannel( int nChan, float f )=0;
	virtual void SetIntChannel( int nChan, int i )=0;
	virtual void SetUnitVecChannel( int nChan, Point3 v )=0;
	virtual void SetPoint2Channel( int nChan, Point2 p )=0;
	virtual void SetPoint3Channel( int nChan, Point3 p )=0;

	virtual RGBA GetColorChannel( int nChan )=0;
	virtual float GetFloatChannel( int nChan )=0;
	virtual int GetIntChannel( int nChan )=0;
	virtual Point3 GetUnitVecChannel( int nChan )=0;
	virtual Point2 GetPoint2Channel( int nChan )=0;
	virtual Point3 GetPoint3Channel( int nChan )=0;
};

// must be greater than I_USERINTERFACE in maxsdk/include/animtbl.h
#define IID_IReshading	0xabe00001

// Materials support this interface to support preshading/reshading
class IReshading
{
public:
		// if supporting shaders, plug-in mtl should inherit from StdMat2
		enum ReshadeRequirements{ RR_None = 0, RR_NeedPreshade, RR_NeedReshade }; // mjm - 06.02.00

		// this is implied by the GetInterface( IID_IReshading ) call on the mtl
//		virtual BOOL SupportsReShading(ShadeContext& sc) { return FALSE; }

		// GetReshadeRequirements is called by the interactive reshader after being notified that the material has changed.
		// the return value indicates if the material is still valid, or needs to be preshaded or reshaded. the value
		// should pertain only to the latest change to the material
		// if a material doesn't support reshading, it doesn't need to override this function -- any change will cause
		// the nodes to which it's attached to be rerendered.
		virtual ReshadeRequirements GetReshadeRequirements() { return RR_NeedPreshade; } // mjm - 06.02.00
		virtual void PreShade(ShadeContext& sc, IReshadeFragment* pFrag ) { }
		virtual void PostShade(ShadeContext& sc, IReshadeFragment* pFrag, 
						int& nextTexIndex, IllumParams* ip = NULL ) { }

};

// must be greater than I_USERINTERFACE in maxsdk/include/animtbl.h
#define IID_IValidityToken	0xabe00002

class IValidityToken
{
public:
	virtual BOOL NeedsUpdate( ULONG currentToken )= 0;
	virtual void SetToken( ULONG token )= 0;
	virtual ULONG GetToken() = 0;
	virtual void SetMessage( ULONG message )= 0;
	virtual ULONG GetMessage() = 0;
};


class IValidityTokenImp : public IValidityToken
{
protected:
	ULONG	mLocalToken;
	ULONG	mMessage;

public:
	// if the object has been changed the localtoken will equal the
	// current token
	BOOL NeedsUpdate( ULONG currentToken ){
		return mLocalToken >= currentToken;
	}
	// mtls/maps set the token to next frame when changed
	// from core interface GetNextValidityToken()
	void SetToken( ULONG token ) {
		mLocalToken = token;
	}
	// not used, but here for debug & future use
	ULONG GetToken() {
		return mLocalToken;
	}

	// once the render know which mtls maps were changed, this can be used to 
	// send a secondary message e.g. map/colorVal or map/uv, etc.
	void SetMessage( ULONG message ) {
		mMessage = message;
	}
	ULONG GetMessage() {
		return mMessage;
	}
};

//------------------------------------

typedef MtlBase* MtlBaseHandle;
typedef Mtl* MtlHandle;
typedef Texmap* TexmapHandle;


// Simple list of materials
class MtlList: public Tab<MtlHandle> {
	public:
		CoreExport int AddMtl(Mtl *m, BOOL checkUnique=TRUE);
		CoreExport int FindMtl(Mtl *m);
		CoreExport int FindMtlByName(TSTR& name);
		void RemoveEntry(int n) { Delete(n,1); }
		void Empty() { Resize(0); }
	};

// Materials library
class MtlLib: public ReferenceTarget, public MtlList {
	public:
		SClass_ID SuperClassID() { return REF_MAKER_CLASS_ID; }
		CoreExport Class_ID ClassID();
		CoreExport void DeleteAll();
		void GetClassName(TSTR& s) { s= TSTR(_T("MtlLib")); }  
		CoreExport ~MtlLib();

		int NumSubs() { 
			return Count(); 
			}  
		Animatable* SubAnim(int i) { 
			return (*this)[i]; 
			}
		CoreExport TSTR SubAnimName(int i);
		CoreExport virtual void Remove(Mtl *m);
		CoreExport virtual void Add(Mtl *m);

		// From ref
		RefResult AutoDelete() { return REF_SUCCEED; }
		CoreExport void DeleteThis();
		int NumRefs() { return Count();}
		RefTargetHandle GetReference(int i) { return (RefTargetHandle)(*this)[i];}
		CoreExport void SetReference(int i, RefTargetHandle rtarg);
		CoreExport RefTargetHandle Clone(RemapDir &remap = NoRemap());
		CoreExport RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message );

		// IO
		CoreExport IOResult Save(ISave *isave);
        CoreExport IOResult Load(ILoad *iload);

	};


// A MtlBase Version of the above
class MtlBaseList: public Tab<MtlBaseHandle> {
	public:
		CoreExport int AddMtl(MtlBase *m, BOOL checkUnique=TRUE);
		CoreExport int FindMtl(MtlBase *m);
		CoreExport int FindMtlByName(TSTR& name);
		void RemoveEntry(int n) { Delete(n,1); }
		void Empty() { Resize(0); }
	};

class MtlBaseLib : public ReferenceTarget, public MtlBaseList {
	public:
		SClass_ID SuperClassID() { return REF_MAKER_CLASS_ID; }
		CoreExport Class_ID ClassID();
		CoreExport void DeleteAll();
		void GetClassName(TSTR& s) { s= TSTR(_T("MtlBaseLib")); }  
		CoreExport ~MtlBaseLib();

		int NumSubs() {return Count();}		
		Animatable* SubAnim(int i) {return (*this)[i];}
		CoreExport TSTR SubAnimName(int i);
		
		CoreExport virtual void Remove(MtlBase *m);
		CoreExport virtual void Add(MtlBase *m);
		CoreExport virtual void RemoveDuplicates();

		// From ref
		RefResult AutoDelete() {return REF_SUCCEED;}
		CoreExport void DeleteThis();
		int NumRefs() { return Count();}
		RefTargetHandle GetReference(int i) { return (RefTargetHandle)(*this)[i];}
		CoreExport void SetReference(int i, RefTargetHandle rtarg);
		CoreExport RefTargetHandle Clone(RemapDir &remap = NoRemap());
		CoreExport RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message );

		// IO
		CoreExport IOResult Save(ISave *isave);
        CoreExport IOResult Load(ILoad *iload);
	};


// Simple list of numbers
class NumList: public Tab<int> {
	public:
		CoreExport int Add(int j, BOOL checkUnique=TRUE);
		CoreExport int Find(int j);
	};

class MtlRemap {
	public:
		virtual Mtl* Map(Mtl *oldAddr)=0;
	};

//--------------------------------------------------------------------------------
// Use this class to implement Drag-and-Drop for materials sub-Texmaps.
// If this class is used the ParamDlg method FindSubTexFromHWND() must be implemented.
//--------------------------------------------------------------------------------
class TexDADMgr: public DADMgr {
	ParamDlg *dlg;
	public:
		TexDADMgr(ParamDlg *d=NULL) { dlg = d;}
		void Init(ParamDlg *d) { dlg = d; }
		// called on the draggee to see what if anything can be dragged from this x,y
		SClass_ID GetDragType(HWND hwnd, POINT p) { return TEXMAP_CLASS_ID; }
		// called on potential dropee to see if can drop type at this x,y
		CoreExport BOOL OkToDrop(ReferenceTarget *dropThis, HWND hfrom, HWND hto, POINT p, SClass_ID type, BOOL isNew);
		int SlotOwner() { return OWNER_MTL_TEX;	}
	    CoreExport ReferenceTarget *GetInstance(HWND hwnd, POINT p, SClass_ID type);
		CoreExport void Drop(ReferenceTarget *dropThis, HWND hwnd, POINT p, SClass_ID type);
		CoreExport BOOL  LetMeHandleLocalDAD(); 
		CoreExport void  LocalDragAndDrop(HWND h1, HWND h2, POINT p1, POINT p2);
		BOOL  AutoTooltip(){ return TRUE; }
	};


//--------------------------------------------------------------------------------
// Use this class to implement Drag-and-Drop for materials sub-materials.
// If this class is used the ParamDlg method FindSubMtlFromHWND() must be implemented.
//--------------------------------------------------------------------------------
class MtlDADMgr: public DADMgr {
	ParamDlg *dlg;
	public:
		MtlDADMgr(ParamDlg *d=NULL) { dlg = d;}
		void Init(ParamDlg *d) { dlg = d; }
		// called on the draggee to see what if anything can be dragged from this x,y
		SClass_ID GetDragType(HWND hwnd, POINT p) { return MATERIAL_CLASS_ID; }
		// called on potential dropee to see if can drop type at this x,y
		BOOL OkToDrop(ReferenceTarget *dropThis, HWND hfrom, HWND hto, POINT p, SClass_ID type, BOOL isNew) {
			if (hfrom==hto) return FALSE;
			return (type==MATERIAL_CLASS_ID)?1:0;
			}
		int SlotOwner() { return OWNER_MTL_TEX;	}
	    CoreExport ReferenceTarget *GetInstance(HWND hwnd, POINT p, SClass_ID type);
		CoreExport void Drop(ReferenceTarget *dropThis, HWND hwnd, POINT p, SClass_ID type);
		CoreExport BOOL  LetMeHandleLocalDAD(); 
		CoreExport void  LocalDragAndDrop(HWND h1, HWND h2, POINT p1, POINT p2);
		BOOL  AutoTooltip(){ return TRUE; }
	};


// this class used for Drag/and/dropping Bitmaps
class DADBitmapCarrier: public ReferenceTarget {
	TSTR name;
	public:
	void SetName(TSTR &nm) { name = nm; }
	TSTR& GetName() { return name; }		

	Class_ID ClassID() { return Class_ID(0,1); }
	SClass_ID SuperClassID() { return BITMAPDAD_CLASS_ID; }		
	void DeleteThis() { }
	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
	         PartID& partID,  RefMessage message) { return REF_DONTCARE; }		
	RefTargetHandle Clone(RemapDir &remap = NoRemap()) { assert(0); return NULL; }

	};

// Get a pointer to the BitmapCarrier (their's only two of these: one for the
// source, and one for the destination:  don't try to delete these; 
CoreExport DADBitmapCarrier *GetDADBitmapCarrier();


CoreExport void SetLoadingMtlLib(MtlLib *ml);
CoreExport void SetLoadingMtlBaseLib(MtlBaseLib *ml);

CoreExport ClassDesc* GetMtlLibCD();
CoreExport ClassDesc* GetMtlBaseLibCD();

CoreExport UVGen* GetNewDefaultUVGen();
CoreExport XYZGen* GetNewDefaultXYZGen();
CoreExport TextureOutput* GetNewDefaultTextureOutput();
inline int IsMtl(Animatable *m) { return m->SuperClassID()==MATERIAL_CLASS_ID; }
inline int IsTex(Animatable *m) { return m->SuperClassID()==TEXMAP_CLASS_ID; }
inline int IsMtlBase(Animatable *m) { return IsMtl(m)||IsTex(m); }

// Combines the two materials into a multi-material.
// Either of the two input materials can themselves be multis.
// c1 and c2 will be set to the mat count for mat1 and mat2.
CoreExport Mtl *CombineMaterials(Mtl *mat1, Mtl *mat2, int &mat2Offset);

// Expand multi material size to the largest face ID
CoreExport Mtl *FitMaterialToMeshIDs(Mesh &mesh,Mtl *mat);
CoreExport Mtl *FitMaterialToShapeIDs(BezierShape &shape, Mtl *mat);
CoreExport Mtl *FitMaterialToPatchIDs(PatchMesh &patch, Mtl *mat);
CoreExport Mtl *FitMaterialToPolyMeshIDs (MNMesh & mesh, Mtl *mat);

// Adjust face IDs so that no face ID is greater then them multi size
CoreExport void FitMeshIDsToMaterial(Mesh &mesh, Mtl *mat);
CoreExport void FitShapeIDsToMaterial(BezierShape &shape, Mtl *mat);
CoreExport void FitPatchIDsToMaterial(PatchMesh &patch, Mtl *mat);
CoreExport void FitPolyMeshIDsToMaterial (MNMesh & mesh, Mtl *mat);

// Remove unused mats in a multi and shift face IDs.
CoreExport Mtl *CondenseMatAssignments(Mesh &mesh, Mtl *mat);
CoreExport Mtl *CondenseMatAssignments(BezierShape &shape, Mtl *mat);
CoreExport Mtl *CondenseMatAssignments(PatchMesh &patch, Mtl *mat);
CoreExport Mtl *CondenseMatAssignments (MNMesh &mesh, Mtl *mat);


// Attach materials dialog support -- TH 3/9/99

// Attach Mat values
#define ATTACHMAT_IDTOMAT 0
#define ATTACHMAT_MATTOID 1
#define ATTACHMAT_NEITHER 2

class AttachMatDlgUser {
	public:
		virtual int GetAttachMat()=0;
		virtual BOOL GetCondenseMat()=0;
		virtual void SetAttachMat(int value)=0;
		virtual void SetCondenseMat(BOOL sw)=0;
	};

CoreExport BOOL DoAttachMatOptionDialog(IObjParam *ip, AttachMatDlgUser *user);

#endif

