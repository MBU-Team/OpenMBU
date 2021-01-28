/* -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

   FILE: sfx.h

	 DESCRIPTION: class declarations for the special effect class hierarchy

	 CREATED BY: michael malone (mjm)

	 HISTORY: created June 30, 2000

   	 Copyright (c) 2000, All Rights Reserved

// -----------------------------------------------------------------------------
// -------------------------------------------------------------------------- */
#ifndef __EFFECT__H
#define __EFFECT__H

// includes
#include "iFnPub.h"

// predeclarations
class IRendParams;
class ShadeContext;
class RenderGlobalContext;
class IReshadeFragment;

// ------------------------------
//
// check abort callback class
//
// used to check for abort during
// an extended operation
// and to update progress
//
// ------------------------------

#define CAC_INTERFACE Interface_ID(0x666534, 0x50101) 


class CheckAbortCallback : public FPMixinInterface
{
public:
	virtual BOOL Check()=0;  // returns TRUE if user has done something to cause an abort.
	virtual	BOOL Progress(int done, int total)=0;
	virtual void SetTitle(const TCHAR *title)=0;

    // FPInterface stuff
	enum { cac_check, cac_progress, cac_setTitle, };

	BEGIN_FUNCTION_MAP	
      FN_0(cac_check, TYPE_BOOL, Check); 
      FN_2(cac_progress, TYPE_BOOL, Progress, TYPE_INT, TYPE_INT); 
      VFN_1(cac_setTitle, SetTitle, TYPE_STRING); 
    END_FUNCTION_MAP 

	FPInterfaceDesc* GetDesc() { return (FPInterfaceDesc*)GetCOREInterface(CAC_INTERFACE); }
};

// -------------------------------------------------------------
//
// special effect plug-in base class
//
// base class for Atmospherics, Effects, RenderElements, Shaders
//
// -------------------------------------------------------------

class SFXParamDlg : public InterfaceServer
{
public:
	virtual Class_ID ClassID()=0;
	virtual void SetThing(ReferenceTarget *m)=0;
	virtual ReferenceTarget* GetThing()=0;
	virtual void SetTime(TimeValue t) { }		
	virtual void DeleteThis()=0;		
	virtual INT_PTR Execute(int cmd, ULONG_PTR arg1=0, ULONG_PTR arg2=0, ULONG_PTR arg3=0) { return 0; } 
	virtual void InvalidateUI() { }	
};


class SpecialFX: public ReferenceTarget
{
public:
	TSTR name;
	// This name will appear in the track view and the list of current atmospheric effects.
	virtual TSTR GetName() { return _T(""); }		

	// Is effect active 
	virtual BOOL Active(TimeValue t) { return !TestAFlag(A_ATMOS_DISABLED); }

	// Called when the render steps to a new frame
	virtual	void Update(TimeValue t, Interval& valid) { }

	// Saves and loads name. These should be called at the start of
	// a plug-in's save and load methods.
	CoreExport IOResult Save(ISave *isave);
	CoreExport IOResult Load(ILoad *iload);

	// Put up a dialog that lets the user edit the plug-ins parameters.
	virtual SFXParamDlg *CreateParamDialog(IRendParams *ip) { return NULL; }
	// Implement this if you are using the ParamMap2 AUTO_UI system and the 
	// effect has secondary dialogs that don't have the effect as their 'thing'.
	// Called once for each secondary dialog for you to install the correct thing.
	// Return TRUE if you process the dialog, false otherwise.
	virtual BOOL SetDlgThing(SFXParamDlg* dlg) { return FALSE; }

	// If an atmospheric has references to gizmos or other objects in the scene it can optionally 
	// provide access to the object list.
	virtual int NumGizmos() { return 0; }
	virtual INode *GetGizmo(int i) { return NULL; }
	virtual void DeleteGizmo(int i) { }
	virtual void AppendGizmo(INode *node) { }
	virtual BOOL OKGizmo(INode *node) { return FALSE; } // approve a node for possible use as gizmo
	virtual void EditGizmo(INode *node) { } // selects this gizmo & displays params for it if any
	virtual	void InsertGizmo(int i, INode *node) { assert(0); } // must be defined to use DeleteGizmoRestore

	// Animatable overides...
	CoreExport SvGraphNodeReference SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags);
};

#define SFXBASE_CHUNK	0x39bf
#define SFXNAME_CHUNK	0x0100


// Classes used for implementing UNDO in Atmosphere and Effects classes.
class AppendGizmoRestore : public RestoreObj
{
public:
	SpecialFX *fx;
	INode *node;
	AppendGizmoRestore(SpecialFX *f, INode *n) { fx= f; node = n; }
	void Redo() { fx->AppendGizmo(node); }
	void Restore(int isUndo) { fx->DeleteGizmo(fx->NumGizmos()-1); } 
	TSTR Description() { return TSTR("AppendGizmoRestore"); }
};

class DeleteGizmoRestore: public RestoreObj
{
public:
	SpecialFX *fx;
	INode *node;
	int num;
	DeleteGizmoRestore(SpecialFX *a, INode *n, int i) {	fx = a; node = n; num = i;	}
	void Redo() { fx->DeleteGizmo(num); }
	void Restore(int isUndo) { fx->InsertGizmo(num,node); } 
	TSTR Description() { return TSTR("DeleteGizmoRestore"); }
};

// ------------------------------
// atmospheric plug-in base class
// ------------------------------

// Returned by an Atmospheric when it is asked to put up its rollup page.
typedef SFXParamDlg AtmosParamDlg;

class Atmospheric : public SpecialFX
{
public:
	RefResult NotifyRefChanged(Interval changeInt,
		                       RefTargetHandle hTarget,
							   PartID& partID,
							   RefMessage message) { return REF_SUCCEED; }
	SClass_ID SuperClassID() { return ATMOSPHERIC_CLASS_ID; }
	
	// Saves and loads name. These should be called at the start of
	// a plug-in's save and load methods.
	IOResult Save(ISave *isave) { return SpecialFX::Save(isave); }
	IOResult Load(ILoad *iload) { return SpecialFX::Load(iload); }

	// Put up a modal dialog that lets the user edit the plug-ins parameters.
	virtual AtmosParamDlg *CreateParamDialog(IRendParams *ip) { return NULL; }
	// Implement this if you are using the ParamMap2 AUTO_UI system and the 
	// atmosphere has secondary dialogs that don't have the effect as their 'thing'.
	// Called once for each secondary dialog for you to install the correct thing.
	// Return TRUE if you process the dialog, false otherwise.
	virtual BOOL SetDlgThing(AtmosParamDlg* dlg) { return FALSE; }

	// This is the function that is called to apply the effect.
	virtual void Shade(ShadeContext& sc,const Point3& p0,const Point3& p1,Color& color, Color& trans, BOOL isBG=FALSE)=0;
// begin - ke/mjm - 03.16.00 - merge reshading code
//	virtual void PreShade(ShadeContext& sc,const Point3& p0,const Point3& p1,Color& color, Color& trans, IReshadeFragment* pFrag, BOOL isBG=FALSE) { }
//	virtual void ReShade(ShadeContext& sc,const Point3& p0,const Point3& p1,Color& color, Color& trans, IReshadeFragment* pFrag, BOOL isBG=FALSE) { Shade(sc,p0,p1,color,trans,isBG); }
// end - ke/mjm - 03.16.00 - merge reshading code
};

// Chunk IDs saved by base class
#define ATMOSHPERICBASE_CHUNK	SFXBASE_CHUNK
#define ATMOSHPERICNAME_CHUNK	SFXNAME_CHUNK


// --------------------------------
// render effect plug-in base class
// --------------------------------

// Returned by an  effect when it is asked to put up its rollup page.
typedef SFXParamDlg EffectParamDlg;

class Effect : public SpecialFX
{
public:
	RefResult NotifyRefChanged(Interval changeInt,
		                       RefTargetHandle hTarget,
							   PartID& partID,
							   RefMessage message) {return REF_SUCCEED;}
	SClass_ID SuperClassID() { return RENDER_EFFECT_CLASS_ID; }

	// Saves and loads name. These should be called at the start of
	// a plug-in's save and load methods.
	IOResult Save(ISave *isave) { return SpecialFX::Save(isave); }
	IOResult Load(ILoad *iload) { return SpecialFX::Load(iload); }

	// Put up a dialog that lets the user edit the plug-ins parameters.
	virtual EffectParamDlg *CreateParamDialog(IRendParams *ip) { return NULL; }
	// Implement this if you are using the ParamMap2 AUTO_UI system and the 
	// effect has secondary dialogs that don't have the effect as their 'thing'.
	// Called once for each secondary dialog for you to install the correct thing.
	// Return TRUE if you process the dialog, false otherwise.
	virtual BOOL SetDlgThing(EffectParamDlg* dlg) { return FALSE; }

	// What G-buffer channels does this Effect require in the output bitmap?
	virtual DWORD GBufferChannelsRequired(TimeValue t) { return 0; }

	// This is the function that is called to apply the effect.
	virtual void Apply(TimeValue t, Bitmap *bm, RenderGlobalContext *gc, CheckAbortCallback *cb )=0;

};

// Chunk IDs saved by base class
#define EFFECTBASE_CHUNK	SFXBASE_CHUNK
#define EFFECTNAME_CHUNK	SFXNAME_CHUNK


// --------------------------------
// filter kernel plug-in base class
// --------------------------------

#define AREA_KERNEL_CLASS_ID			0x77912301
#define DEFAULT_KERNEL_CLASS_ID			AREA_KERNEL_CLASS_ID

// Returned by a kernel when it is asked to put up its rollup page.
typedef SFXParamDlg FilterKernelParamDlg;

class FilterKernel : public SpecialFX
{
public:
	RefResult NotifyRefChanged(Interval changeInt, 
							   RefTargetHandle hTarget, 
							   PartID& partID, 
							   RefMessage message ) { return REF_SUCCEED; }

	SClass_ID SuperClassID() { return FILTER_KERNEL_CLASS_ID; }
	
	// Saves and loads name. These should be called at the start of
	// a plug-in's save and load methods.
	IOResult Save(ISave *isave) { return SpecialFX::Save(isave); }
	IOResult Load(ILoad *iload) { return SpecialFX::Load(iload); }

	// Put up a dialog that lets the user edit the plug-ins parameters.
	virtual FilterKernelParamDlg *CreateParamDialog( IRendParams *ip ) { return NULL; }

	// filter kernel section
	// This is the function that is called to sample kernel values.
	virtual double KernelFn( double x, double y = 0.0 )=0;

	// integer number of pixels from center to filter 0 edge, must not truncate filter
	// x dimension for 2D filters
	virtual long GetKernelSupport()=0;
	// for 2d returns y support, for 1d returns 0
	virtual long GetKernelSupportY()=0;

	virtual bool Is2DKernel()=0;
	virtual bool IsVariableSz()=0;
	// 1-D filters ignore the y parameter
	virtual void SetKernelSz( double x, double y = 0.0 )=0;
	virtual void GetKernelSz( double& x, double& y )=0;

	// returning true will disable the built-in normalizer
	virtual bool IsNormalized(){ return FALSE; }

	// this is for possible future optimizations, not sure its needed
	virtual bool HasNegativeLobes()=0;

	virtual TCHAR* GetDefaultComment()=0;

	// there are 2 optional 0.0 ...1.0 parameters, for whatever
	virtual long GetNFilterParams(){ return 0; }
	virtual TCHAR * GetFilterParamName( long nParam ){ return _T(""); }
	virtual double GetFilterParamMax( long nParam ){ return 1.0; }
	virtual double GetFilterParam( long nParam ){ return 0.0; }
	virtual void SetFilterParam( long nParam, double val ){};
};

// Chunk IDs saved by base class
#define FILTERKERNELBASE_CHUNK	0x39bf
#define FILTERKERNELNAME_CHUNK	0x0100


#endif
