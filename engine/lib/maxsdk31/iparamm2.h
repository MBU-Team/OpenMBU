/**********************************************************************
 *<
	FILE: IParamM2.h

	DESCRIPTION:  Parameter Maps, Edition 2 for use with ParamBlock2's

	CREATED BY: Rolf Berteig
			    John Wainwright, 2nd edition

	HISTORY: created 10/10/95
			 2nd Ed. 9/8/98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

/*
 *   IParamMap2's are used to map and manage UI dialogs for the parameters
 *   in a ParamBlock2.  They work almost identically to IParamMaps except:
 *
 *    1. they only work with ParamBlock2-hosted parameters; there is no
 *       IParamArray equivalent.  Hopefully, the new capabilities of ParamBlock2's
 *       cover most of the reasons for using IParamArrays.  If not, install virtual
 *       paramters in the ParamBlock2 and supply accessor fns for them.
 *
 *	  2. they derive all UI-related metadata from the ParamBlockDesc2 structure now
 *       pointed to by a ParamBlock2; there is no ParamUIDesc equivalent.
 *
 *    3. some new methods on ClassDesc can be used to automatically construct & open
 *       rollouts, so you may not have to create these explicitly yourself.
 */

#ifndef __IPARAMM2__
#define __IPARAMM2__

class IParamMap2;
class IRendParams;
class Effect;

#include <iparamb2.h>

// If custom handling of controls needs to be done, ParameterMap
// client can't implement one of these and set is as the ParameterMap's
// user callback.
class ParamMap2UserDlgProc 
{
	public:
		virtual BOOL DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)=0;
		virtual void DeleteThis()=0;
		virtual void SetThing(ReferenceTarget *m) { }
		virtual void Update(TimeValue t) { }
		virtual void SetParamBlock(IParamBlock2 *pb) { }
};

// Return this from DlgProc to get the viewports redrawn.
#define REDRAW_VIEWS	2

class IParamMap2 
{
	public:
	 	// UI updating.
	 	virtual void Invalidate()=0;    // whole UI
	 	virtual void Validate()=0;		// uninvalidate whole UI
		virtual void Invalidate(ParamID id, int tabIndex=0)=0;  // nominated param
		virtual void UpdateUI(TimeValue t)=0;  // update UI directly for time t
		virtual void RedrawViews(TimeValue t, DWORD flag=REDRAW_NORMAL)=0;  // redraw viewport

		// Swaps the existing parameter block with a new one and updates UI.
		virtual void SetParamBlock(IParamBlock2 *pb)=0;

		// The given proc will be called _after_ default processing is done.
		// The callback can then apply constraints to controls.
		// Note that if the proc is non-NULL when the ParamMap is deleted
		// its DeleteThis() method will be called.
		virtual void SetUserDlgProc(ParamMap2UserDlgProc *proc=NULL)=0;
		virtual ParamMap2UserDlgProc *GetUserDlgProc()=0;

		// Changes a map entry to refer to a different item in the parameter block.
		virtual void ReplaceParam(ParamID curParam, ParamID newParam) { }

		// Access the dialog window.
		virtual HWND GetHWnd()=0;
		// Access the rollup window containing this rollout dialog
		virtual IRollupWindow* GetIRollup() { return NULL; }

		// Access the parameter block
		virtual IParamBlock2 *GetParamBlock()=0;

		// Is the dialog proc active
		virtual BOOL DlgActive()=0;

		// Access my descriptor
		virtual ParamBlockDesc2* GetDesc()=0;

		// sent to indicate dialog is going inactive so, among other things, ColorSwatches can be told
		virtual void ActivateDlg(BOOL onOff)=0;

		// sent to a Material Editor map to find the SubTex or SubMtl index corresponding to the control hw	
		virtual int FindSubTexFromHWND(HWND hw)=0;
		virtual int FindSubMtlFromHWND(HWND hw)=0;

		// Individual enable of param UI controls
		virtual void Enable(ParamID id, BOOL onOff, int tabIndex=0)=0;

		// Set text of param UI control
		virtual void SetText(ParamID id, TCHAR* txt, int tabIndex=0)=0;

		// Set tooltip of param UI control
		virtual void SetTooltip(ParamID id, BOOL onOf, TCHAR* txt, int tabIndex=0)=0;

		// Set range of param UI control (spinner/slider)
		virtual void SetRange(ParamID id, float low, float high, int tabIndex=0)=0;

		// show or hide assciated controls
		virtual void Show(ParamID id, BOOL showHide, int tabIndex=0)=0;

		// sent by any AutoXXParamDlg as a courtesy when it receives a SetThing()
		virtual void SetThing(ReferenceTarget *m)=0;
};

// Giving this value for scale specifies autoscale
#define SPIN_AUTOSCALE	-1.0f

// Creates a parameter map that will handle a parameter block in a modeless
// dialog where time does not change and the viewport is not redrawn.
// Note that there is no need to destroy it. It executes the dialog and then
// destorys itself. Returns TRUE if the user selected OK, FALSE otherwise.
PB2Export BOOL CreateModalParamMap2(
		IParamBlock2 *pb,
		TimeValue t,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		HWND hParent,
		ParamMap2UserDlgProc *proc=NULL);

// Creates a parameter map to handle the display of parameters in the command panal.
// This will add a rollup page to the command panel.
// DestroyCPParamMap().
PB2Export IParamMap2 *CreateCPParamMap2(
		IParamBlock2 *pb,
		Interface *ip,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		TCHAR *title,
		DWORD flags,
		ParamMap2UserDlgProc* dlgProc=NULL,
		HWND hOldRollup=NULL);
PB2Export void DestroyCPParamMap2(IParamMap2 *m);

// create a child dialog of the given parent parammap (for tabbed dialogs, etc.)
PB2Export IParamMap2 *CreateChildCPParamMap2(
		IParamBlock2 *pb,
		Interface *ip,
		HINSTANCE hInst,
		IParamMap2* parent,
		TCHAR *dlgTemplate,
		TCHAR *title,
		ParamMap2UserDlgProc* dlgProc=NULL);
PB2Export void DestroyChildCPParamMap2(IParamMap2 *m);

// Creates a parameter map to handle the display of render parameters or
// atmospheric plug-in parameters.
PB2Export IParamMap2 *CreateRParamMap2(
		IParamBlock2 *pb,
		IRendParams *ip,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		TCHAR *title,
		DWORD flags,
		ParamMap2UserDlgProc* dlgProc=NULL);
PB2Export void DestroyRParamMap2(IParamMap2 *m);

// create a parameter map for render or atmos params in a child dialog window
// of the given parent parammap, used typically to create tab child windows in 
// a tabbed rollout
PB2Export IParamMap2* CreateChildRParamMap2(
		IParamBlock2 *pb, 
		IRendParams *ip, 
		HINSTANCE hInst, 
		IParamMap2* parent,
		TCHAR *dlgTemplate, 
		TCHAR *title, 
		ParamMap2UserDlgProc* dlgProc=NULL);
PB2Export void DestroyChildRParamMap2(IParamMap2 *m);

// Creates a parameter map to handle the display of texture map or
// material parameters in the material editor.
PB2Export IParamMap2 *CreateMParamMap2(
		IParamBlock2 *pb,
		IMtlParams *ip,
		HINSTANCE hInst,
		HWND hmedit,
		TexDADMgr* tdad,
		MtlDADMgr* mdad,
		TCHAR *dlgTemplate,
		TCHAR *title,
		DWORD flags,
		ParamMap2UserDlgProc* dlgProc=NULL,
		HWND hOldRollup=NULL);
PB2Export void DestroyMParamMap2(IParamMap2 *m);

// create a child dialog of the given parent parammap (for tabbed dialogs, etc.)
PB2Export IParamMap2 *CreateChildMParamMap2(
		IParamBlock2 *pb,
		IMtlParams *ip,
		HINSTANCE hInst,
		IParamMap2* parent,
		TexDADMgr* tdad,
		MtlDADMgr* mdad,
		TCHAR *dlgTemplate,
		TCHAR *title,
		ParamMap2UserDlgProc* dlgProc=NULL);
PB2Export void DestroyChildMParamMap2(IParamMap2 *m);

//  Auto ParamDlg class for Material Editor auto-UI, instanced by ClassDesc2::CreateParamDlg()
//  It maintains a table of secondary ParamDlgs for master ParamDlgs (eg, the one returned 
//  from CreateParamDlg()) and will broadcast appropriate method calls to them
//  as the master receives them
class IAutoMParamDlg : public ParamDlg
{
	public:
		virtual void		InvalidateUI()=0;
		virtual void		MtlChanged()=0;
		// secondary dialog list management
		virtual int			NumDlgs()=0;
		virtual void		AddDlg(ParamDlg* dlg)=0;
		virtual ParamDlg*	GetDlg(int i)=0;
		virtual void		SetDlg(int i, ParamDlg* dlg)=0;
		virtual void		DeleteDlg(ParamDlg* dlg)=0;
		// access to this dlg's parammap stuff
		virtual IParamMap2* GetMap()=0;
};
// create an AutoMParamDlg for material editor
PB2Export IAutoMParamDlg* CreateAutoMParamDlg(HWND hMedit, IMtlParams *i, MtlBase* mtl,
											  IParamBlock2* pb, ClassDesc2* cd, HINSTANCE inst, 
											  TCHAR* dlgTemplate, TCHAR* title, int rollFlags,
											  ParamMap2UserDlgProc* dlgProc=NULL,
											  HWND hOldRollup=NULL);

//  Auto ParamDlg class for Effects auto-UI, instanced by ClassDesc2::CreateParamDialog()
//  It maintains a table of secondary EffectParamDlg for master EffectParamDlg (eg, the one returned 
//  from CreateParamDialog()) and will broadcast appropriate method calls to them
//  as the master receives them
class IAutoEParamDlg : public EffectParamDlg
{
	public:
		virtual void		InvalidateUI()=0;
		// secondary dialog list management
		virtual int			NumDlgs()=0;
		virtual void		AddDlg(SFXParamDlg* dlg)=0;
		virtual SFXParamDlg* GetDlg(int i)=0;
		virtual void		SetDlg(int i, SFXParamDlg* dlg)=0;
		virtual void		DeleteDlg(SFXParamDlg* dlg)=0;
		// access to this dlg's parammap stuff
		virtual IParamMap2* GetMap()=0;
};
// create an AutoEParamDlg for render effects
PB2Export IAutoEParamDlg* CreateAutoEParamDlg(IRendParams *i, Effect* e,
											  IParamBlock2* pb, ClassDesc2* cd, HINSTANCE inst, 
											  TCHAR* dlgTemplate, TCHAR* title, int rollFlags, 
											  ParamMap2UserDlgProc* dlgProc=NULL);


#endif // __IPARAMM2__



