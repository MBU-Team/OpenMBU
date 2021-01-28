/**********************************************************************
 *<
	FILE: IParamM.h

	DESCRIPTION:  Parameter Maps

	CREATED BY: Rolf Berteig

	HISTORY: created 10/10/95

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/


#ifndef __IPARAMM__
#define __IPARAMM__


class IParamMap;
class IRendParams;

// If custom handling of controls needs to be done, ParameterMap
// client can't implement one of these and set is as the ParameterMap's
// user callback.
class ParamMapUserDlgProc {
	public:
		virtual BOOL DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)=0;
		virtual void DeleteThis()=0;
		virtual void Update(TimeValue t) {}
	};

// Return this from DlgProc to get the viewports redrawn.
#define REDRAW_VIEWS	2



class IParamMap {
	public:
	 	// Mark the UI as needing to be updated.
	 	virtual void Invalidate()=0;
		
		// Swaps the existing parameter block with a new one and updates UI.
		virtual void SetParamBlock(IParamArray *pb)=0;

		// The given proc will be called _after_ default processing is done.
		// The callback can then apply constraints to controls.
		// Note that if the proc is non-NULL when the ParamMap is deleted
		// its DeleteThis() method will be called.
		virtual void SetUserDlgProc(ParamMapUserDlgProc *proc=NULL)=0;
		virtual ParamMapUserDlgProc *GetUserDlgProc()=0;

		// Changes a map entry to refer to a different item in the parameter block.
		virtual void SetPBlockIndex(int mapIndex, int blockIndex)=0;

		// Access the dialog window.
		virtual HWND GetHWnd()=0;

		// Access the parameter block
		virtual IParamArray *GetParamBlock()=0;

		// Is the dialog proc active
		virtual BOOL DlgActive()=0;
	};




// Giving this value for scale specifies autoscale
#define SPIN_AUTOSCALE	-1.0f

class ParamUIDesc {
	public:
		// Float or int controlled by a single spinner
		CoreExport ParamUIDesc(
			int index,EditSpinnerType spinType,int idEdit,int idSpin,
			float lowLim,float highLim,float scale,ParamDimension *dim=defaultDim);

		// int controlelled by n radio buttons
		// vals[i] represents the value if ctrlIDs[i] is checked.
		// if vals=NULL then ctrlIDs[i] represents a value of i.
		//
		// OR
		// 
		// int controlled by multiple check boxes where each
		// check boxes controlls a single bit.
		// vals[i] specifies which bit ctrlIds[i] controls.
		// If vals=NULL ctrlIDs[i] controls the ith bit.		
		CoreExport ParamUIDesc(
			int index,ControlType type,int *ctrlIDs,int count,int *vals=NULL);

		// int controlled by a single check box (BOOL)
		// or Point3 controlled by a color swatch.
		CoreExport ParamUIDesc(int index,ControlType type,int id);

		// Point3 controlled by 3 spinners
		CoreExport ParamUIDesc(int index,
				EditSpinnerType spinType,
				int idEdit1,int idSpin1,
				int idEdit2,int idSpin2,
				int idEdit3,int idSpin3,
				float lowLim,float highLim,float scale,
				ParamDimension *dim=defaultDim);
		
		
		int	pbIndex;
		ParamType 	ptype;
		ControlType	ctype;
		int id[6];
		int *ids;
		int *vals;
		int count;

		EditSpinnerType spinType;
		float lowLim;
		float highLim;
		float scale;
		ParamDimension *dim;
	};

// Creates a parameter map that will handle a parameter block in a modeless
// dialog where time does not change and the viewport is not redrawn.
// Note that there is no need to destroy it. It executes the dialog and then
// destorys itself. Returns TRUE if the user selected OK, FALSE otherwise.
CoreExport BOOL CreateModalParamMap(
		ParamUIDesc *desc,int count,
		IParamArray *pb,
		TimeValue t,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		HWND hParent,
		ParamMapUserDlgProc *proc=NULL);


// Creates a parameter map to handle the display of parameters in the command panal.
// 
// This will add a rollup page to the command panel.
// DestroyCPParamMap().
//
CoreExport IParamMap *CreateCPParamMap(
		ParamUIDesc *desc,int count,
		IParamArray *pb,
		Interface *ip,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		TCHAR *title,
		DWORD flags);


CoreExport IParamMap *ReplaceCPParamMap(
		HWND oldhw,
		ParamUIDesc *desc,int count,
		IParamArray *pb,
		Interface *ip,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		TCHAR *title,
		DWORD flags);

CoreExport void DestroyCPParamMap(IParamMap *m);


// Creates a parameter map to handle the display of render parameters or
// atmospheric plug-in parameters.
CoreExport IParamMap *CreateRParamMap(
		ParamUIDesc *desc,int count,
		IParamArray *pb,
		IRendParams *ip,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		TCHAR *title,
		DWORD flags);
CoreExport void DestroyRParamMap(IParamMap *m);

class IMtlParams;

// Creates a parameter map to handle the display of texture map or
// material parameters in the material editor.
CoreExport IParamMap *CreateMParamMap(
		ParamUIDesc *desc,int count,
		IParamArray *pb,
		IMtlParams *ip,
		HINSTANCE hInst,
		TCHAR *dlgTemplate,
		TCHAR *title,
		DWORD flags);
CoreExport void DestroyMParamMap(IParamMap *m);



#endif // __IPARAMM__



