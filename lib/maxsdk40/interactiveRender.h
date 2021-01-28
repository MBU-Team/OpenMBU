/* -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

   FILE: interactiveRender.h

	 DESCRIPTION: interactive rendering interface

	 CREATED BY: michael malone (mjm)

	 HISTORY: created September 9, 2000

   	 Copyright (c) 2000, All Rights Reserved

// -----------------------------------------------------------------------------
// -------------------------------------------------------------------------- */

#if !defined( INTERACTIVE_RENDER_H_INCLUDED )
#define INTERACTIVE_RENDER_H_INCLUDED


#include "notify.h"
#include "iImageViewer.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IRenderProgressCallback : public RendProgressCallback

//  PURPOSE:
//    Progress callback for interactive rendering
//
//  NOTES:
//    created:  04.13.00 - mjm
//
//    information set via RendProgressCallback::SetCurField() or RendProgressCallback::SetSceneStats() will be
//    ignored. if a title is set via the inherited method SetTitle(), it will appear in the main status bar,
//    but will be replaced by the 'IRenderTitle' when necessary.
//
//    an interactive renderer should abort if RendProgressCallback::Progress() returns RENDPROG_ABORT
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// LO_Horizontal indicates that a single pixel-wide line will grow from left to right,
	// at the top of the rendered region, as the interactive rendering progresses
	// LO_Vertical indicates that the line will grow from top to bottom,
	// at the right of the rendered region.
	enum LineOrientation { LO_Horizontal = 0, LO_Vertical };

	// sets/gets progress line orientation
	virtual void SetProgressLineOrientation(LineOrientation orientation) = 0;
	virtual LineOrientation GetProgressLineOrientation() const = 0;

	// sets/gets progress line color
	virtual void SetProgressLineColor(const Color& color) = 0;
	virtual const Color& GetProgressLineColor() const = 0;

	// sets/gets the current title. it will appear in the main status bar as "'Title': xx% complete".
	// if no title is provided, 'ActiveShade' will be used instead.
	virtual void SetIRenderTitle(const TCHAR *pProgressTitle) = 0;
	virtual const TCHAR *GetIRenderTitle() const = 0;
};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IIRenderMgrSelector : public InterfaceServer

//  PURPOSE:
//    Abstract class (Interface) that is used to determine
//    which nodes are selected by the interactive rendering manager
//
//  NOTES:
//    created:  11/16/00 - ca
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// Is the node selected? Default behavious shades all nodes
	virtual BOOL IsSelected(INode* pINode) { return TRUE; } // shade ALL
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IIRenderMgr : public InterfaceServer

//  PURPOSE:
//    Abstract class (Interface) for an interactive rendering manager
//
//  NOTES:
//    created:  04.13.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	enum CommandMode
	{
		CMD_MODE_NULL = 0,
		CMD_MODE_DRAW_REGION,
		CMD_MODE_SELECT_OBJECT,
	}; // may later add modes for camera control

	virtual bool CanExecute() = 0;
	virtual void SetActive(bool active) = 0;
	virtual TCHAR* GetName() = 0;
	virtual bool IsActive() = 0;
	virtual HWND GetHWnd() const = 0;
	virtual ViewExp *GetViewExp() = 0;
	virtual void SetPos(int X, int Y, int W, int H) = 0;
	virtual void Show() = 0;
	virtual void Hide() = 0;
	virtual void UpdateDisplay() = 0;
	virtual void Render() = 0;
	virtual void SetDelayTime(int msecDelay) = 0;
	virtual int GetDelayTime() = 0;
	virtual void Close() = 0;
	virtual void Delete() = 0;

	// sets and gets the command mode
	virtual void SetCommandMode(CommandMode commandMode) = 0;
	virtual CommandMode GetCommandMode() const = 0;

	// sets and gets the update state
	virtual void SetActOnlyOnMouseUp(bool actOnlyOnMouseUp) = 0;
	virtual bool GetActOnlyOnMouseUp() const = 0;

	// toggles the toolbar display mode (for docked windows)
	virtual void ToggleToolbar() const = 0;

	// gets the display style
	virtual IImageViewer::DisplayStyle GetDisplayStyle() const = 0;

	// find out if the renderer is currently rendering
	virtual BOOL IsRendering() = 0;

	// Has the rendering manager selected any nodes.
	virtual BOOL AreAnyNodesSelected() const = 0;

	// Get interface that determines whether nodes are selected
	virtual IIRenderMgrSelector* GetNodeSelector() = 0;

	// ---------------------
	// static public methods
	// ---------------------
	// returns a pointer to the active IIRenderMgr (NULL if none exist)
	static IIRenderMgr* GetActiveIIRenderMgr();
	// returns number of IIRenderMgrs
	static unsigned int GetNumIIRenderMgrs();
	// returns pointer to i'th IIRenderMgr (NULL if doesn't exist)
	static IIRenderMgr* GetIIRenderMgr(unsigned int i);
};


class IQuadMenuContext;

// I_RENDER is passed as the id parameter to a renderer's implementation of
// virtual void* Animatable::GetInterface(ULONG id)
// the renderer returns a pointer to a class IInteractiveRender instance if it supports interactive rendering,
// otherwise the default implementation will return NULL, indicating that interactive rendering is not supported.
enum { I_RENDER_ID = 0x12345678 }; // value must be > I_USERINTERFACE in maxsdk/include/animtbl.h - but don't want to include animtbl.h

// Abstract interface class for a renderer supporting reshading - version 1
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IInteractiveRender : public InterfaceServer

//  PURPOSE:
//    Abstract class (Interface) for a a renderer supporting interactive rendering
//
//  NOTES:
//    created:  04.13.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// notification of begin and end of interactive rendering
	virtual void BeginSession() = 0;
	virtual void EndSession() = 0;

	// sets/gets the owner window (to be passed to renderer, if necessary)
	virtual void SetOwnerWnd(HWND hOwnerWnd) = 0;
	virtual HWND GetOwnerWnd() const = 0;

	// sets/gets the pointer to the interactive rendering manager
	virtual void SetIIRenderMgr(IIRenderMgr *pIIRenderMgr) = 0;
	virtual IIRenderMgr *GetIIRenderMgr(IIRenderMgr *pIIRenderMgr) const = 0;

	// sets/gets the bitmap to be rendered to
	virtual void SetBitmap(Bitmap *pDestBitmap) = 0;
	virtual Bitmap *GetBitmap(Bitmap *pDestBitmap) const = 0;

	// sets/gets the scene root node
	virtual void SetSceneINode(INode *pSceneINode) = 0;
	virtual INode *GetSceneINode() const = 0;

	// sets/gets whether to use the ViewINode. if false, ViewParams should be used
	virtual void SetUseViewINode(bool bUseViewINode) = 0;
	virtual bool GetUseViewINode() const = 0;

	// sets/gets the ViewINode
	virtual void SetViewINode(INode *pViewINode) = 0;
	virtual INode *GetViewINode() const = 0;

	// sets/gets the ViewExp
	virtual void SetViewExp(ViewExp *pViewExp) = 0;
	virtual ViewExp *GetViewExp() const = 0;

	// sets/gets the region of the bitmap to be rendered. if Box2::IsEmpty() returns true, it indicates to render entire bitmap
	virtual void SetRegion(const Box2 &region) = 0;
	virtual const Box2 &GetRegion() const = 0;

	// sets/gets the lights to be used in abscence of scene lights
	virtual void SetDefaultLights(DefaultLight *pDefLights, int numDefLights) = 0;
	virtual const DefaultLight *GetDefaultLights(int &numDefLights) const = 0;

	// sets/gets the pointer to progress callback object
	virtual void SetProgressCallback(IRenderProgressCallback *pProgCB) = 0;
	virtual const IRenderProgressCallback *GetProgressCallback() const = 0;

	// renders the bitmap using default rendering functionality
	virtual void Render(Bitmap *pDestBitmap) = 0;

	// returns the NodeRenderID for a given bitmap pixel location
	// Return 0 if there is no node
	virtual ULONG GetNodeHandle(int x, int y) = 0;

	// fills the sBBox parameter with the screen space bounding box for a given INode. returns true if successful, otherwise false.
	virtual bool GetScreenBBox(Box2& sBBox, INode *pINode) = 0;

	// returns ActionTableId for any action items the reshading renderer may implement. returns 0 if none.
	virtual ActionTableId GetActionTableId() = 0;

	// returns ActionCallback for any action items the reshading renderer may implement. returns NULL if none.
	virtual ActionCallback *GetActionCallback() = 0;

	// access to additional method interfaces
	virtual void *GetInterface() { return NULL; }

	// find out if the renderer is currently rendering
	virtual BOOL IsRendering() = 0;

};


#endif // !defined( INTERACTIVE_RENDER_H_INCLUDED )
