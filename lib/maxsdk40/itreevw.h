/**********************************************************************
 *<
	FILE: treevw.h

	DESCRIPTION: New Tree View

	CREATED BY:	Rolf Berteig

	HISTORY: Created 17 April 1995
			 Moved into public SDK, JBW 5.25.00

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/


#ifndef __ITREEVW__
#define __ITREEVW__

#define WM_TV_SELCHANGED			WM_USER + 0x03001
#define WM_TV_MEDIT_TV_DESTROYED	WM_USER + 0x03b49

// Sent to a track view window to force it to do a horizontal zoom extents
#define WM_TV_DOHZOOMEXTENTS	WM_USER + 0xb9a1

// Has a maximize button for TV in a viewport.
#define TVSTYLE_MAXIMIZEBUT		(1<<0)
#define TVSTYLE_INVIEWPORT		(1<<1)
#define TVSTYLE_NAMEABLE		(1<<2)
#define TVSTYLE_INMOTIONPAN		(1<<3)

// Major modes
#define MODE_EDITKEYS		0
#define MODE_EDITTIME		1
#define MODE_EDITRANGES		2
#define MODE_POSRANGES		3
#define MODE_EDITFCURVE		4

class TrackViewFilter;
class ITreeViewOps;
class TrackViewPick;

typedef Animatable* AnimatablePtr;

#include "iFnPub.h"


//Added by AF (09/07/00)
//A CORE interface to get the trackview windows
//Use GetCOREInterface(ITRACKVIEWS) to get a pointer to this interface
//**************************************************
#define ITRACKVIEWS Interface_ID(0x531c5f2c, 0x6fdf29cf)

class ITrackViewArray : public FPStaticInterface
	{
	public:	
		int GetNumAvailableTrackViews();
		ITreeViewOps* GetTrackView(int index);
		ITreeViewOps* GetTrackView(TSTR name);
		Tab<ITreeViewOps*> GetAvaliableTrackViews();
		ITreeViewOps* GetLastActiveTrackView();

		BOOL IsTrackViewOpen(TSTR name);
		BOOL IsTrackViewOpen(int index);
		BOOL OpenTrackViewWindow(TSTR name);
		BOOL OpenTrackViewWindow(int index);
		BOOL OpenLastActiveTrackViewWindow();

		BOOL CloseTrackView(TSTR name);
		BOOL CloseTrackView(int index);
		void DeleteTrackView(TSTR name);
		void DeleteTrackView(int index);

		TCHAR* GetTrackViewName(int index);
		TCHAR* GetLastUsedTrackViewName();

		BOOL IsTrackViewCurrent(int index);
		BOOL IsTrackViewCurrent(TSTR name);
		BOOL SetTrackViewCurrent(int index);
		BOOL SetTrackViewCurrent(TSTR name);

		BOOL TrackViewZoomSelected(TSTR tvName);
		BOOL TrackViewZoomOn(TSTR tvName, Animatable* anim, int subNum);
	
		enum{ getTrackView, getAvaliableTrackViews, getNumAvailableTrackViews,
			  openTrackView, closeTrackView, getTrackViewName, trackViewZoomSelected, trackViewZoomOn,
			  setFilter, clearFilter, pickTrackDlg, isOpen, openLastTrackView, currentTrackViewProp,
			  lastUsedTrackViewNameProp, deleteTrackView, isTrackViewCurrent, setTrackViewCurrent,
			};

		DECLARE_DESCRIPTOR(ITrackViewArray);

		BEGIN_FUNCTION_MAP
		FN_1(getTrackView, TYPE_INTERFACE, fpGetTrackView, TYPE_FPVALUE);
		FN_0(getAvaliableTrackViews, TYPE_INTERFACE_TAB_BV, GetAvaliableTrackViews);
		FN_0(getNumAvailableTrackViews, TYPE_INT, GetNumAvailableTrackViews);

		FN_1(openTrackView, TYPE_BOOL, fpOpenTrackViewWindow, TYPE_FPVALUE);
		FN_1(closeTrackView, TYPE_BOOL, fpCloseTrackView, TYPE_FPVALUE);
		VFN_1(deleteTrackView, fpDeleteTrackView, TYPE_FPVALUE);

		FN_1(getTrackViewName, TYPE_STRING, GetTrackViewName, TYPE_INDEX);
		FN_1(trackViewZoomSelected, TYPE_BOOL, TrackViewZoomSelected, TYPE_TSTR);
		FN_3(trackViewZoomOn, TYPE_BOOL, TrackViewZoomOn, TYPE_TSTR, TYPE_REFTARG, TYPE_INDEX);
		FN_VA(setFilter, TYPE_BOOL, fpSetTrackViewFilter);
		FN_VA(clearFilter, TYPE_BOOL, fpClearTrackViewFilter);
		FN_VA(pickTrackDlg, TYPE_FPVALUE_BV, fpDoPickTrackDlg);
		FN_1(isOpen, TYPE_BOOL, fpIsTrackViewOpen, TYPE_FPVALUE);
		FN_0(openLastTrackView, TYPE_BOOL, OpenLastActiveTrackViewWindow);		
		RO_PROP_FN(currentTrackViewProp, GetLastActiveTrackView, TYPE_INTERFACE);
		RO_PROP_FN(lastUsedTrackViewNameProp, GetLastUsedTrackViewName, TYPE_STRING);
		FN_1(isTrackViewCurrent, TYPE_BOOL, fpIsTrackViewCurrent, TYPE_FPVALUE);
		FN_1(setTrackViewCurrent, TYPE_BOOL, fpSetTrackViewCurrent, TYPE_FPVALUE);
        END_FUNCTION_MAP

	private:
		// these functions are wrapper functions to massage maxscript specific values into standard values
		// These methods just call one of the corresponding public methods
		BOOL fpSetTrackViewFilter(FPParams* val);
		BOOL fpClearTrackViewFilter(FPParams* val);
		FPValue fpDoPickTrackDlg(FPParams* val);
		BOOL fpIsTrackViewOpen(FPValue* val);
		BOOL fpCloseTrackView(FPValue* val);
		void fpDeleteTrackView(FPValue* val);
		BOOL fpIsTrackViewCurrent(FPValue* val);
		BOOL fpSetTrackViewCurrent(FPValue* val);
		ITreeViewOps* fpGetTrackView(FPValue* val);
		BOOL fpOpenTrackViewWindow(FPValue* val);

	};


#define TREEVIEW_OPS_INTERFACE Interface_ID(0x60fb7eef, 0x1f6d6dd3)
//These are the operations you can do on any open trackview
//Added by AF (09/12/00)
//*********************************************************
class ITreeViewOps : public FPMixinInterface {
	public:
		virtual ~ITreeViewOps() {}

		virtual int GetNumTracks()=0;
		virtual int NumSelTracks()=0;
 		virtual void GetSelTrack(int i,AnimatablePtr &anim,AnimatablePtr &client,int &subNum)=0;
		virtual ReferenceTarget* GetAnim(int index)=0;
		virtual ReferenceTarget* GetClient(int index)=0;

		virtual BOOL CanAssignController()=0;
		virtual void DoAssignController(BOOL clearMot=TRUE)=0;
		virtual void ShowControllerType(BOOL show)=0;	

		virtual TCHAR *GetTVName()=0;
		// added for scripter access, JBW - 11/11/98
		virtual void SetTVName(TCHAR *)=0;
		virtual void CloseTreeView()=0;

		virtual void SetFilter(DWORD mask,int which, BOOL redraw=TRUE)=0;
		virtual void ClearFilter(DWORD mask,int which, BOOL redraw=TRUE)=0; 
		virtual DWORD TestFilter(DWORD mask,int which)=0; 
		
		// added for param wiring, JBW - 5.26.00
		virtual void ZoomOn(Animatable* owner, int subnum)=0;
		virtual void ZoomSelected()=0;
		virtual void ExpandTracks()=0;

		//added for completeness by AF (09/12/00) 
		virtual int GetIndex(Animatable *anim)=0;
		virtual void SelectTrackByIndex(int index, BOOL clearSelection=TRUE)=0;
		virtual void SelectTrack(Animatable* anim, BOOL clearSelection=TRUE)=0;
		virtual BOOL AssignControllerToSelected(Animatable* ctrl)=0;

		//added by AF (09/25/00) for MAXScript exposure
		virtual void SetEditMode(int mode)=0;
		virtual int GetEditMode()=0;

		//added by AF (09/25/00) for more MAXScript exposure
		//These differ from "active" because the trackview 
		//doesn't have to be selected for it to be the currently used trackview
		virtual BOOL IsCurrent()=0;
		virtual void SetCurrent()=0;

		enum {  tv_getName, tv_setName, tv_close, tv_numSelTracks, tv_getNumTracks, tv_getSelTrack, 
				tv_canAssignController, tv_doAssignController, tv_assignController, tv_showControllerTypes, 
				tv_expandTracks, tv_zoomSelected, tv_zoomOnTrack,
				tv_getAnim, tv_getClient, tv_getSelAnim, tv_getSelClient, tv_getSelAnimSubNum, 
				tv_getIndex, tv_selectTrackByIndex, tv_selectTrack,
				tv_setFilter, tv_clearFilter, 
				tv_setEditMode, tv_getEditMode, tv_setEditModeProp, tv_getEditModeProp, tv_editModeTypes,
				tv_setCurrent, tv_getCurrent,
			};

		BEGIN_FUNCTION_MAP
			FN_0(tv_getName, TYPE_STRING, GetTVName);
			VFN_1(tv_setName, SetTVName, TYPE_STRING);
			VFN_0(tv_close, CloseTreeView);
			FN_0(tv_getNumTracks, TYPE_INT, GetNumTracks);
			FN_0(tv_numSelTracks, TYPE_INT, NumSelTracks);
			
			FN_0(tv_canAssignController, TYPE_BOOL, CanAssignController);
			VFN_0(tv_doAssignController, DoAssignController);
			FN_1(tv_assignController, TYPE_BOOL, AssignControllerToSelected, TYPE_REFTARG);
			VFN_1(tv_showControllerTypes, ShowControllerType, TYPE_BOOL);
			
			VFN_0(tv_expandTracks, ExpandTracks);
			VFN_0(tv_zoomSelected, ZoomSelected);
			VFN_2(tv_zoomOnTrack, ZoomOn, TYPE_REFTARG, TYPE_INT);

			FN_1(tv_getAnim, TYPE_REFTARG, GetAnim, TYPE_INDEX);
			FN_1(tv_getClient, TYPE_REFTARG, GetClient, TYPE_INDEX);

			FN_1(tv_getSelAnim, TYPE_REFTARG, fpGetSelectedAnimatable, TYPE_INDEX);
			FN_1(tv_getSelClient, TYPE_REFTARG, fpGetSelectedClient, TYPE_INDEX);
			FN_1(tv_getSelAnimSubNum, TYPE_INDEX, fpGetSelectedAnimSubNum, TYPE_INDEX);

			FN_1(tv_getIndex, TYPE_INDEX, GetIndex, TYPE_REFTARG);
			VFN_2(tv_selectTrackByIndex, SelectTrackByIndex, TYPE_INDEX, TYPE_BOOL);
			VFN_2(tv_selectTrack, SelectTrack, TYPE_REFTARG, TYPE_BOOL);

			FN_VA(tv_setFilter, TYPE_BOOL, fpSetFilter);
			FN_VA(tv_clearFilter, TYPE_BOOL, fpClearFilter);

			VFN_1(tv_setEditMode, SetEditMode, TYPE_ENUM);
			FN_0(tv_getEditMode, TYPE_ENUM, GetEditMode);
			PROP_FNS(tv_getEditModeProp, GetEditMode, tv_setEditModeProp, SetEditMode, TYPE_ENUM);
			FN_0(tv_getCurrent, TYPE_BOOL, IsCurrent);
			VFN_0(tv_setCurrent, SetCurrent);
		END_FUNCTION_MAP
		
		FPInterfaceDesc* GetDesc();

	private:
		//these methods are created to massage data into a format the function publishing system can interpret
		//these functions just call other public functions above
		//Added by AF (09/12/00)
		virtual Animatable* fpGetSelectedAnimatable(int index)=0;
		virtual Animatable* fpGetSelectedClient(int index)=0;
		virtual int fpGetSelectedAnimSubNum(int index)=0;
		virtual BOOL fpSetFilter(FPParams* val)=0;
		virtual BOOL fpClearFilter(FPParams* val)=0;

};


class ITreeView : public ITreeViewOps, public IObject {
	public:
		
		virtual ~ITreeView() {}
		virtual void SetPos(int x, int y, int w, int h)=0;
		virtual void Show()=0;
		virtual void Hide()=0;
		virtual BOOL IsVisible()=0;
		virtual BOOL InViewPort()=0; 

		virtual void SetTreeRoot(ReferenceTarget *root,ReferenceTarget *client=NULL,int subNum=0)=0;
		virtual void SetLabelOnly(BOOL only)=0;

		virtual void SetMultiSel(BOOL on)=0;
		virtual void SetSelFilter(TrackViewFilter *f=NULL)=0;
		virtual void SetActive(BOOL active)=0;
		virtual BOOL IsActive()=0;
		virtual HWND GetHWnd()=0;
		virtual int GetTrackViewParent(int index)=0; // returns -1 if no parent is found

		virtual void Flush()=0;
		virtual void UnFlush()=0;
		virtual void SetMatBrowse()=0;
		virtual DWORD GetTVID()=0;
	};

#define OPENTV_NEW		0
#define OPENTV_SPECIAL	-2
#define OPENTV_LAST		-1


// Sent by a tree view window to its parent when the user right clicks
// on dead area of the toolbar.
// Mouse points are relative to the client area of the tree view window
//
// LOWORD(wParam) = mouse x
// HIWORD(wParam) = mouse y
// lparam         = tree view window handle
#define WM_TV_TOOLBAR_RIGHTCLICK	WM_USER + 0x8ac1

// Sent by a tree view window when the user double
// clicks on a track label.
// wParam = 0
// lParam = HWND of track view window
#define WM_TV_LABEL_DOUBLE_CLICK	WM_USER + 0x8ac2

class TrackViewActionCallback: public ActionCallback {
public:
    BOOL ExecuteAction(int id);
    void SetHWnd(HWND hWnd) { mhWnd = hWnd; }

    HWND mhWnd;
};

//-----------------------------------------------------------------
//
// Button IDs for the track view
	
#define ID_TV_TOOLBAR			200    // the toolbar itself
//#define ID_TV_DELETE			210
#define ID_TV_MOVE				220
#define ID_TV_SCALE				230
//#define ID_TV_FUNCTION_CURVE	240
#define ID_TV_SNAPKEYS			260
#define ID_TV_ALIGNKEYS			270
#define ID_TV_ADD				280
//#define ID_TV_EDITKEYS			290
//#define ID_TV_EDITTIME			300
//#define ID_TV_EDITRANGE			310
//#define ID_TV_POSITIONRANGE		320
#define ID_TV_FILTERS			330
#define ID_TV_INSERT			340
#define ID_TV_CUT				350
#define ID_TV_COPY				360
#define ID_TV_PASTE				370
#define ID_TV_SLIDE				380
#define ID_TV_SELECT			390
#define ID_TV_REVERSE			400
#define ID_TV_LEFTEND			410
#define ID_TV_RIGHTEND			420
#define ID_TV_SUBTREE			430
#define ID_TV_ASSIGNCONTROL		440
#define ID_TV_MAKEUNIQUE		450
#define ID_TV_CHOOSEORT			460
#define ID_TV_SHOWTANGENTS		470
#define ID_TV_SCALEVALUES		480
#define ID_TV_FREEZESEL			490
#define ID_TV_TEMPLATE			500
#define ID_TV_LOCKTAN			510
#define ID_TV_PROPERTIES		520
#define ID_TV_NEWEASE			530
#define ID_TV_DELEASE			540
#define ID_TV_TOGGLEEASE		550
#define ID_TV_CHOOSE_EASE_ORT	560
#define ID_TV_CHOOSE_MULT_ORT	570
#define ID_TV_ADDNOTE			580
#define ID_TV_DELETENOTE		590
#define ID_TV_RECOUPLERANGE		600
#define ID_TV_COPYTRACK			610
#define ID_TV_PASTETRACK		620
#define ID_TV_REDUCEKEYS		630
#define ID_TV_ADDVIS			640
#define ID_TV_DELVIS			650
#define ID_TV_TVNAME			660
#define ID_TV_TVUTIL			670
//watje
#define ID_TV_GETSELECTED		680
#define ID_TV_DELETECONTROL		690

// Status tool button IDs
#define ID_TV_STATUS			1000
#define ID_TV_ZOOMREGION		1020
#define ID_TV_PAN				1030
#define ID_TV_VFITTOWINDOW		1040
#define ID_TV_HFITTOWINDOW		1050
#define ID_TV_SHOWSTATS			1060
#define ID_TV_TIMETYPEIN		1070
#define ID_TV_VALUETYPEIN		1080
#define ID_TV_ZOOM				1090
#define ID_TV_MAXIMIZE			1100
#define ID_TV_SELWILDCARD		1110
#define ID_TV_ZOOMSEL			1120

// From accelerators
#define ID_TV_K_SNAP			2000
//#define ID_TV_K_LOCKKEYS		2010
#define ID_TV_K_MOVEKEYS		2020
#define ID_TV_K_MOVEVERT		2030
#define ID_TV_K_MOVEHORZ		2040
#define ID_TV_K_SELTIME			2050
#define ID_TV_K_SUBTREE			2060
#define ID_TV_K_LEFTEND			2070
#define ID_TV_K_RIGHTEND		2080
#define ID_TV_K_TEMPLATE		2090
#define ID_TV_K_SHOWTAN			2100
#define ID_TV_K_LOCKTAN			2110
#define ID_TV_K_APPLYEASE		2120
#define ID_TV_K_APPLYMULT		2130
#define ID_TV_K_ACCESSTNAME		2140
#define ID_TV_K_ACCESSSELNAME	2150
#define ID_TV_K_ACCESSTIME		2160
#define ID_TV_K_ACCESSVAL		2170
#define ID_TV_K_ZOOMHORZ		2180
#define ID_TV_K_ZOOMHORZKEYS	2190
#define ID_TV_K_ZOOM			2200
#define ID_TV_K_ZOOMTIME		2210
#define ID_TV_K_ZOOMVALUE		2220
//#define ID_TV_K_NUDGELEFT		2230
//#define ID_TV_K_NUDGERIGHT		2240
//#define ID_TV_K_MOVEUP			2250
//#define ID_TV_K_MOVEDOWN		2260
//#define ID_TV_K_TOGGLENODE		2270
//#define ID_TV_K_TOGGLEANIM		2280
#define ID_TV_K_SHOWSTAT		2290
#define ID_TV_K_MOVECHILDUP		2300
#define ID_TV_K_MOVECHILDDOWN	2310


// trackview filter name to mask map
#define FILTER_SELOBJECTS		(1<<0)
#define FILTER_SELCHANNELS		(1<<1)
#define FILTER_ANIMCHANNELS		(1<<2)

#define FILTER_WORLDMODS		(1<<3)
#define FILTER_OBJECTMODS		(1<<4)
#define FILTER_TRANSFORM		(1<<5)
#define FILTER_BASEPARAMS		(1<<6)

#define FILTER_POSX				(1<<7)
#define FILTER_POSY				(1<<8)
#define FILTER_POSZ				(1<<9)
#define FILTER_ROTX				(1<<10)
#define FILTER_ROTY				(1<<11)
#define FILTER_ROTZ				(1<<12)
#define FILTER_SCALEX			(1<<13)
#define FILTER_SCALEY			(1<<14)
#define FILTER_SCALEZ			(1<<15)
#define FILTER_RED				(1<<16)
#define FILTER_GREEN			(1<<17)
#define FILTER_BLUE				(1<<18)

#define FILTER_CONTTYPES		(1<<19)
#define FILTER_NOTETRACKS		(1<<20)
#define FILTER_SOUND			(1<<21)
#define FILTER_MATMAPS			(1<<22)
#define FILTER_MATPARAMS		(1<<23)
#define FILTER_VISTRACKS		(1<<24)

// More filter bits. These are stored in the 2nd DWORD.
#define FILTER_GEOM				(1<<0)
#define FILTER_SHAPES			(1<<1)
#define FILTER_LIGHTS			(1<<2)
#define FILTER_CAMERAS			(1<<3)
#define FILTER_HELPERS			(1<<4)
#define FILTER_WARPS			(1<<5)
#define FILTER_VISIBLE_OBJS		(1<<6)
#define FILTER_POSITION			(1<<7)
#define FILTER_ROTATION			(1<<8)
#define FILTER_SCALE			(1<<9)
#define FILTER_CONTX			(1<<10)
#define FILTER_CONTY			(1<<11)
#define FILTER_CONTZ			(1<<12)
#define FILTER_STATICVALS		(1<<13)
#define FILTER_HIERARCHY		(1<<14)
#define FILTER_NODES			(1<<15)

#define DEFAULT_TREEVIEW_FILTER0	(FILTER_WORLDMODS|FILTER_OBJECTMODS|FILTER_TRANSFORM|FILTER_BASEPARAMS| \
	FILTER_POSX|FILTER_POSY|FILTER_POSZ|FILTER_ROTX|FILTER_ROTY|FILTER_ROTZ| \
	FILTER_SCALEX|FILTER_SCALEY|FILTER_SCALEZ|FILTER_RED|FILTER_GREEN|FILTER_BLUE|FILTER_NOTETRACKS| \
	FILTER_MATMAPS|FILTER_MATPARAMS|FILTER_VISTRACKS|FILTER_SOUND)

#define DEFAULT_TREEVIEW_FILTER1	(FILTER_POSITION|FILTER_ROTATION|FILTER_SCALE| \
	FILTER_CONTX|FILTER_CONTY|FILTER_CONTZ|FILTER_VISIBLE_OBJS|FILTER_STATICVALS| \
	FILTER_HIERARCHY|FILTER_NODES)


#endif

