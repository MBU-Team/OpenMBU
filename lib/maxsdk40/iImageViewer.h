/* -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

   FILE: iImageViewer.h

	 DESCRIPTION: abstract class for image viewers

	 CREATED BY: michael malone (mjm)

	 HISTORY: created April 13, 2000

   	 Copyright (c) 2000, All Rights Reserved

// -----------------------------------------------------------------------------
// -------------------------------------------------------------------------- */

#if !defined( I_IMAGE_VIEWER_H_INCLUDED )
#define I_IMAGE_VIEWER_H_INCLUDED

#include "maxtypes.h"		// Interface_ID

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef IMAGE_VIEW_EXPORTS
#define IMAGE_VIEW_API __declspec(dllexport)
#else
#define IMAGE_VIEW_API __declspec(dllimport)
#endif

class CropCallback;

// sent by a docked bitmap viewer window to its parent when the user right clicks on dead area of the toolbar
// mouse points are relative to the client area of the schematic view window
//
// LOWORD(wParam) = mouse x
// HIWORD(wParam) = mouse y
// lparam         = bitmap viewer window handle
#define WM_BV_TOOLBAR_RIGHTCLICK	WM_USER + 0x8ac4

#define MAX_BITMAP_VIEWER_CLASS _T("MaxBitmapViewerClass") // windows class name


// aszabo | Nov.16.2000
// For the exported abstract classes in this header I am just adding
// a GetInterface method to them instead of deriving them from InterfaceServer
// in order to make it ready for extending it later with additional interfaces
class BaseInterface;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IMAGE_VIEW_API IImageViewer

//  PURPOSE:
//    Abstract class (Interface) for an image viewer
//
//  NOTES:
//    created:  04.13.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	enum DisplayStyle { IV_FLOATING = 0, IV_DOCKED };

	enum WindowPosition
	{
		WPos_NULL = 0,
		WPos_UpperLeft,
		WPos_LowerLeft,
		WPos_UpperRight,
		WPos_LowerRight,
		WPos_Center,

		// the following are used for automatic save and restore
		WPos_Renderer = 10,
		WPos_VideoPostPrimary,
		WPos_VideoPostSecondary,
	};

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	class PostDisplayCallback

	//  PURPOSE:
	//    Abstract class (Interface) for a callback that allows post-display access
	//    to an image viewer
	//
	//  NOTES:
	//    created:  04.13.00 - mjm
	//
	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	{
	public:
		virtual void PostDisplayCB(HWND hWnd) = 0;

		// provides a way for extending it with interfaces
		virtual		BaseInterface* GetInterface	(Interface_ID id) { return NULL; }
	};


	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	class PreEventHandlerCallback

	//  PURPOSE:
	//    Abstract class (Interface) for a callback that allows pre-event handler
	//    access to an image viewer
	//
	//  NOTES:
	//    created:  04.13.00 - mjm
	//
	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	{
	public:
		virtual LRESULT EventHandlerCB(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool &propagate) = 0;
		// provides a way for extending it with interfaces
		virtual		BaseInterface* GetInterface	(Interface_ID id) { return NULL; }
	};

	// shows/hides the viewer window
	virtual void Show() = 0;
	virtual void Hide() = 0;

	// gets the handle to display window
	virtual HWND GetHDisplayWindow() = 0;

	// sets the screen
	virtual void SetPos(int x, int y, int w, int h) = 0;

	// gets the display style
	virtual DisplayStyle GetDisplayStyle() const = 0;

	// sets/gets the context help id
	virtual void SetContextHelpId(DWORD helpID) = 0;
	virtual DWORD GetContextHelpId() const = 0;

	// sets/gets the DADMgr (allows override of DADMgr methods)
	virtual void SetDADMgr(DADMgr *pDADMgr) = 0;
	virtual DADMgr *GetDADMgr() const = 0;

	// sets/gets the pre-event handler callback
	virtual void SetPreEventHandlerCallback(PreEventHandlerCallback* pPreEventHandlerCB) = 0;
	virtual PreEventHandlerCallback* GetPreEventHandlerCallback() const = 0;

	// sets/gets the post-display callback
	virtual void SetPostDisplayCallback(PostDisplayCallback* pPostDisplayCB) = 0;
	virtual PostDisplayCallback* GetPostDisplayCallback() const = 0;

	// provides a way for extending it with interfaces
	virtual		BaseInterface* GetInterface	(Interface_ID id) { return NULL; }
};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IMAGE_VIEW_API IMaxBitmapViewer : public IImageViewer

//  PURPOSE:
//    Abstract class (Interface) for an default max bitmap viewer
//
//  NOTES:
//    created:  04.13.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// sets/gets the bitmap
	virtual void SetBitmap(Bitmap* pBitmap) = 0;
	virtual Bitmap* GetBitmap() const = 0;

	// sets/gets the crop callback
	virtual void SetCropCB(CropCallback* pCropCallback) = 0;
	virtual CropCallback* GetCropCB() const = 0;

	// sets/gets the autonomous state
	virtual void SetAutonomous(bool isAutonomous) = 0;
	virtual bool GetAutonomous() const = 0;

	// sets/gets the current position
	virtual void SetCurrentPosition(WindowPosition currentPosition) = 0;
	virtual WindowPosition GetCurrentPosition() const = 0;

	// sets/gets whether to show the save button
	virtual void SetShowSaveButton(bool showSaveButton) = 0;
	virtual bool GetShowSaveButton() const = 0;

	// (un)displays the viewer
	virtual bool Display(TCHAR *title, WindowPosition position = WPos_Center) = 0;
	// parameters x, y, w, & h will be ignored unless 'Current Position' is WPos_NULL
	virtual bool Display(TCHAR *title, HWND hParent, int x, int y, int w, int h) = 0;
	virtual bool UnDisplay() = 0;
	virtual void ClearScreen() = 0;

	// transforms point/rect between window's client coords and bitmap's coords
	virtual POINT XFormScreenToBitmap(const POINT &pt) const = 0;
	virtual POINT XFormBitmapToScreen(const POINT &pt) const = 0;
	virtual Rect XFormScreenToBitmap(const Rect &rect) const = 0;
	virtual Rect XFormBitmapToScreen(const Rect &rect) const = 0;

	// shows/hides/toggles the toolbar
	virtual void ShowToolbar(bool show) = 0;
	virtual void ToggleToolbar() = 0;

	// gets the portion of the window's client area that is safe to draw in (in client coordinates)
	virtual void GetDrawableRect(Rect& drawableRect) = 0;

	// refreshes the region of the window, or the entire window if region is NULL
	virtual void RefreshWindow(Rect* pRefreshRegion = NULL) = 0;

	// provides a way for extending it with interfaces
	virtual		BaseInterface* GetInterface	(Interface_ID id) { return NULL; }
};


IMAGE_VIEW_API IMaxBitmapViewer* CreateIMaxBitmapViewer(Bitmap* pBitmap, IImageViewer::DisplayStyle displayStyle);
IMAGE_VIEW_API void ReleaseIMaxBitmapViewer(IMaxBitmapViewer *);


#endif // !defined( I_IMAGE_VIEWER_H_INCLUDED )
