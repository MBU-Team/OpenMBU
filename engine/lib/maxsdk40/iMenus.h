/* -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

   FILE: iMenus.h

	 DESCRIPTION: abstract classes for menus

	 CREATED BY: michael malone (mjm)

	 HISTORY: created February 17, 2000

   	 Copyright (c) 2000, All Rights Reserved

// -----------------------------------------------------------------------------
// -------------------------------------------------------------------------- */

#if !defined( IMENUS_H_INCLUDED )
#define IMENUS_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef MENUS_EXPORTS
#define MENUS_API __declspec(dllexport)
#else
#define MENUS_API __declspec(dllimport)
#endif

class TSTR;

enum Event
{
	EVENT_BEGIN_TRACK = 0,
	EVENT_CURSOR_DOWN,
    EVENT_RIGHT_CURSOR_DOWN,
	EVENT_CURSOR_MOVED,
	EVENT_CURSOR_UP,
	EVENT_END_TRACK,
	EVENT_KEY,
    EVENT_RIGHT_CURSOR_UP
};

enum EventParam { EP_NULL = 0, EP_SHOW_SUBMENU, EP_HIDE_SUBMENU };

struct MenuEvent
{
	Event mEvent;
	unsigned int mEventParam;
};

enum QuadIndex { QUAD_ONE = 0, QUAD_TWO, QUAD_THREE, QUAD_FOUR };

enum DisplayMethod { DM_NORMAL = 0, DM_STRETCH, DM_FADE, DM_NUM_METHODS };



// predeclarations
class IMenu;
class IMenuItem;
class ActionItem;


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class ItemID
{
public:
	IMenu* mpMenu;
	IMenuItem* mpItem;

	ItemID() : mpMenu(NULL), mpItem(NULL) { }
	void Null() { mpMenu = NULL; mpItem = NULL; }

	friend bool operator==(ItemID& a, ItemID& b);
	friend bool operator!=(ItemID& a, ItemID& b) { return !(a == b); }
};

inline bool operator==(ItemID& a, ItemID& b)
{
	if ( a.mpMenu  != b.mpMenu  ||
		 a.mpItem  != b.mpItem )
		return false;
	else
		return true;
}



// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IMenuTimer

//  PURPOSE:
//    Abstract class (Interface) for for a timer
//
//  NOTES:
//    created:  04.04.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// checks if timer is running
	virtual bool IsRunning() = 0;

	// (re)starts the timer
	virtual void Start(IMenu* pIMenu, EventParam timingType) = 0;

	// stops the timer
	virtual void Stop() = 0;

	// tells timer to check time. if elapsed, will notify its IMenu client
	virtual void CheckTime() = 0;

	// checks if timer has elapsed
	virtual bool HasElapsed() = 0;

	// sets/gets the elapse time
	virtual void SetElapseTime(unsigned int elapseTime) = 0;
	virtual unsigned int GetElapseTime() const = 0;
	virtual IMenu* GetIMenu() const = 0;
	virtual EventParam GetTimingType() const = 0;
};


typedef unsigned int ValidityToken;


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class MenuColors

//  PURPOSE:
//    class declaration for a menu's color settings
//
//  NOTES:
//    created:  08.28.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	MenuColors() { ResetDefaults(); }

	// sets values to defaults
	void ResetDefaults()
	{
		mTitleBarBackgroundColor = Color(.0f, .0f, .0f);
		mTitleBarTextColor = Color(.75f, .75f, .75f);
		mItemBackgroundColor = Color(.75f, .75f, .75f);
		mItemTextColor = Color(.0f, .0f, .0f);
		mLastExecutedItemTextColor = Color(.95f, .85f, .0f);
		mHighlightedItemBackgroundColor = Color(.95f, .85f, .0f);
		mHighlightedItemTextColor = Color(.0f, .0f, .0f);
		mBorderColor = Color(.0f, .0f, .0f);
		mDisabledShadowColor = Color(.5f, .5f, .5f);
		mDisabledHighlightColor = Color(1.0f, 1.0f, 1.0f);
	}

	Color mTitleBarBackgroundColor,
		  mTitleBarTextColor,
		  mItemBackgroundColor,
		  mItemTextColor,
		  mLastExecutedItemTextColor,
		  mHighlightedItemBackgroundColor,
		  mHighlightedItemTextColor,
		  mBorderColor,
		  mDisabledShadowColor,
		  mDisabledHighlightColor;
};

inline COLORREF MakeCOLORREF(const Color& c) { return RGB( FLto255(c.r), FLto255(c.g), FLto255(c.b) ); }

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IMenuSettings

//  PURPOSE:
//    Abstract class (Interface) for general menu settings
//
//  NOTES:
//    created:  02.17.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// to determine validity of settings
	virtual bool IsTokenValid(const ValidityToken& token) = 0;
	virtual void UpdateValidityToken(ValidityToken& token) const = 0;

	// sets values to defaults
	virtual void ResetDefaults() = 0;

	// sets and gets the border size
	virtual void SetBorderSz(int borderSz) = 0;
	virtual int GetBorderSz() const = 0;

	// sets and gets the horizontal margin size (in points)
	virtual void SetHorizontalMarginInPoints(int horizontalMarginInPoints) = 0;
	virtual int GetHorizontalMarginInPoints() const = 0;

	// sets and gets the vertical margin size (in points)
	virtual void SetVerticalMarginInPoints(int verticalMarginInPoints) = 0;
	virtual int GetVerticalMarginInPoints() const = 0;

	// gets the margins in pixels
	virtual int GetHorizontalMargin(HDC hDC) const = 0;
	virtual int GetVerticalMargin(HDC hDC) const = 0;

	// sets and gets the item font face
	virtual void SetItemFontFace(TCHAR* szItemFontFace) = 0;
	virtual const TCHAR* GetItemFontFace() const = 0;

	// sets and gets the title font face
	virtual void SetTitleFontFace(TCHAR* szTitleFontFace) = 0;
	virtual const TCHAR* GetTitleFontFace() const = 0;

	// sets and gets the item font size
	virtual void SetItemFontSize(int itemFontSize) = 0;
	virtual int GetItemFontSize() const = 0;

	// sets and gets the title font size
	virtual void SetTitleFontSize(int titleFontSize) = 0;
	virtual int GetTitleFontSize() const = 0;

	// sets and gets whether menu item's have uniform height
	virtual void SetUseUniformItemHeight(bool useUniformItemHeight) = 0;
	virtual bool GetUseUniformItemHeight() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetUseUniformItemHeightBOOL(BOOL useUniformItemHeight) = 0;
	virtual BOOL GetUseUniformItemHeightBOOL() const = 0;

	// sets and gets the opacity - 0 to 1
	virtual void SetOpacity(float opacity) = 0;
	virtual float GetOpacity() const = 0;

	// sets and gets the display method
	virtual void SetDisplayMethod(DisplayMethod displayMethod) = 0;
	virtual DisplayMethod GetDisplayMethod() const = 0;

	// sets and gets the number of animation steps
	virtual void SetAnimatedSteps(unsigned int steps) = 0;
	virtual unsigned int GetAnimatedSteps() const = 0;

	// sets and gets the duration of an animation step (milliseconds)
	virtual void SetAnimatedStepTime(unsigned int ms) = 0;
	virtual unsigned int GetAnimatedStepTime() const = 0;

	// sets and gets the delay before a submenu is displayed (milliseconds)
	virtual void SetSubMenuPauseTime(unsigned int ms) = 0;
	virtual unsigned int GetSubMenuPauseTime() const = 0;

	// sets and gets whether to use the menu's last executed item (when user clicks in title bar)
	virtual void SetUseLastExecutedItem(bool useLastExecutedItem) = 0;
	virtual bool GetUseLastExecutedItem() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetUseLastExecutedItemBOOL(BOOL useLastExecutedItem) = 0;
	virtual BOOL GetUseLastExecutedItemBOOL() const = 0;

	// sets and gets whether the menu is repositioned when near the edge of the screen
	virtual void SetRepositionWhenClipped(bool repositionWhenClipped) = 0;
	virtual bool GetRepositionWhenClipped() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetRepositionWhenClippedBOOL(BOOL repositionWhenClipped) = 0;
	virtual BOOL GetRepositionWhenClippedBOOL() const = 0;

	// sets and gets whether the menu should remove redundant separators
	virtual void SetRemoveRedundantSeparators(bool removeRedundantSeparators) = 0;
	virtual bool GetRemoveRedundantSeparators() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetRemoveRedundantSeparatorsBOOL(BOOL removeRedundantSeparators) = 0;
	virtual BOOL GetRemoveRedundantSeparatorsBOOL() const = 0;
};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IMenuGlobalContext

//  PURPOSE:
//    Abstract class (Interface) for context global to all menus that might be
//    displayed during a user's menuing action
//
//  NOTES:
//    created:  02.17.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// sets and gets the menu settings
	virtual void SetIMenuSettings(IMenuSettings* pIMenuSettings) = 0;
	virtual IMenuSettings* GetIMenuSettings() const = 0;

	// updates cursor position from win32
	virtual void UpdateCursorPosition() = 0;

	// gets the cached cursor position
	virtual const IPoint2& GetCursorPosition() const = 0;

	// sets and gets the cached initial cursor position. This is where the user clicked
	virtual const IPoint2& GetInitialCursorPosition() const = 0;
	virtual void SetInitialCursorPosition(IPoint2& initPos) = 0;

	// sets and gets the global timer
	virtual void SetIMenuTimer(IMenuTimer* pIMenuTimer) = 0;
	virtual IMenuTimer* GetIMenuTimer() const = 0;

	// sets and gets the handle to the display window
	virtual void SetHDisplayWnd(HWND hDisplayWnd) = 0;
	virtual HWND GetHDisplayWnd() const = 0;

	// sets and gets the handle to the messsage window
	virtual void SetHMessageWnd(HWND hDisplayWnd) = 0;
	virtual HWND GetHMessageWnd() const = 0;

	// sets and gets the handle to the display device context
	virtual void SetHDisplayDC(HDC hDisplayDC) = 0;
	virtual HDC GetHDisplayDC() const = 0;

	// sets and gets the handle to the title font
	virtual void SetTitleHFont(HFONT hTitleFont) = 0;
	virtual HFONT GetTitleHFont() const = 0;

	// sets and gets the handle to the item font
	virtual void SetItemHFont(HFONT hItemFont) = 0;
	virtual HFONT GetItemHFont() const = 0;

	// sets and gets the handle to the accelerator font
	virtual void SetAcceleratorHFont(HFONT hItemFont) = 0;
	virtual HFONT GetAcceleratorHFont() const = 0;

	// sets and gets the menu's maximum item size
	virtual void SetUniformItemSize(const IPoint2& itemSize) = 0;
	virtual const IPoint2& GetUniformItemSize() const = 0;

	// gets the height of a title bar, not counting the border
	virtual int GetTitleBarHeight() = 0;

	// gets the ItemID of the menu/item triplet currently being traversed
	virtual ItemID& GetCurrentItemID() = 0;

	// gets the ItemID of the menu/item triplet currently selected
	virtual ItemID& GetSelectionItemID() = 0;

	// convenience functions to determine selection is available
	virtual bool HasSelection() = 0;

	// convenience functions to determine selection status
	virtual bool IsCurrentMenuSelected() = 0;  // current menu is selected
	virtual bool IsCurrentItemSelected() = 0;  // current menu and item are selected

	// convenience function to select current menu item
	virtual void SelectCurrentItem() = 0;
};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IMenuLocalContext

//  PURPOSE:
//    Abstract class (Interface) for context local to a specific menu
//
//  NOTES:
//    created:  02.17.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// sets and gets the handle to the drawing device context
	virtual void SetHDrawDC(HDC hDrawDC) = 0;
	virtual HDC GetHDrawDC() const = 0;

	// sets and gets the cursor position in the local coordinates
	virtual void SetLocalCursorPosition(const IPoint2& localCursorPos) = 0;
	virtual const IPoint2& GetLocalCursorPosition() const = 0;

	// sets and gets the menu's current width
	virtual void SetMenuItemWidth(int menuWidth) = 0;
	virtual int GetMenuItemWidth() const = 0;

	// sets and gets the menu's current level (submenus have level > 0)
	virtual void SetLevel(int level) = 0;
	virtual int GetLevel() const = 0;

	// sets and gets the menu's last executed item path (a tab of IMenuItems, listing the selected item at each menu level)
	virtual void SetLastExecutedItemPath(Tab<IMenuItem *> *pExecutedItemPath) = 0;
	virtual Tab<IMenuItem *> *GetLastExecutedItemPath() = 0;

	// sets and gets the menu's current colors
	virtual void SetMenuColors(const MenuColors *pMenuColors) = 0;
	virtual const MenuColors *GetMenuColors() const = 0;

	// sets and gets the global menu context
	virtual void SetIMenuGlobalContext(IMenuGlobalContext* pIMenuGlobalContext, int level, Tab<IMenuItem *> *pExecutedItemPath, const MenuColors *pMenuColors) = 0;
	virtual IMenuGlobalContext* GetIMenuGlobalContext() const = 0;
};


#include <iFnPub.h>

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IQuadMenuSettings : public IMenuSettings, public FPStaticInterface

//  PURPOSE:
//    Abstract class (Interface) for quad menu settings
//
//  NOTES:
//    created:  02.17.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
// function IDs 
	enum
	{
		// from IMenuSettings
		fnIdResetDefaults,
		fnIdSetBorderSize,
		fnIdGetBorderSize,
		fnIdSetHorizontalMarginInPoints,
		fnIdGetHorizontalMarginInPoints,
		fnIdSetVerticalMarginInPoints,
		fnIdGetVerticalMarginInPoints,
		fnIdSetItemFontFace,
		fnIdGetItemFontFace,
		fnIdSetTitleFontFace,
		fnIdGetTitleFontFace,
		fnIdSetItemFontSize,
		fnIdGetItemFontSize,
		fnIdSetTitleFontSize,
		fnIdGetTitleFontSize,
		fnIdSetUseUniformItemHeight,
		fnIdGetUseUniformItemHeight,
		fnIdSetOpacity,
		fnIdGetOpacity,
		fnIdSetDisplayMethod,
		fnIdGetDisplayMethod,
		fnIdSetAnimatedSteps,
		fnIdGetAnimatedSteps,
		fnIdSetAnimatedStepTime,
		fnIdGetAnimatedStepTime,
		fnIdSetSubMenuPauseTime,
		fnIdGetSubMenuPauseTime,
		fnIdSetUseLastExecutedItem,
		fnIdGetUseLastExecutedItem,
		fnIdSetRepositionWhenClipped,
		fnIdGetRepositionWhenClipped,
		fnIdSetRemoveRedundantSeparators,
		fnIdGetRemoveRedundantSeparators,

		// from IQuadMenuSettings
		fnIdSetFirstQuadDisplayed,
		fnIdGetFirstQuadDisplayed,
		fnIdSetUseUniformQuadWidth,
		fnIdGetUseUniformQuadWidth,
		fnIdSetMirrorQuad,
		fnIdGetMirrorQuad,
		fnIdSetMoveCursorOnReposition,
		fnIdGetMoveCursorOnReposition,
		fnIdSetReturnCursorAfterReposition,
		fnIdGetReturnCursorAfterReposition,
		fnIdSetInitialCursorLocInBox_0to1,
		fnIdGetInitialCursorLocXInBox_0to1,
		fnIdGetInitialCursorLocYInBox_0to1,

		fnIdSetTitleBarBackgroundColor,
		fnIdGetTitleBarBackgroundColor,
		fnIdSetTitleBarTextColor,
		fnIdGetTitleBarTextColor,
		fnIdSetItemBackgroundColor,
		fnIdGetItemBackgroundColor,
		fnIdSetItemTextColor,
		fnIdGetItemTextColor,
		fnIdSetLastExecutedItemTextColor,
		fnIdGetLastExecutedItemTextColor,
		fnIdSetHighlightedItemBackgroundColor,
		fnIdGetHighlightedItemBackgroundColor,
		fnIdSetHighlightedItemTextColor,
		fnIdGetHighlightedItemTextColor,
		fnIdSetBorderColor,
		fnIdGetBorderColor,
		fnIdSetDisabledShadowColor,
		fnIdGetDisabledShadowColor,
		fnIdSetDisabledHighlightColor,
		fnIdGetDisabledHighlightColor,
};

	// sets and gets the first quadrant displayed
	virtual void SetFirstQuadDisplayed(QuadIndex firstQuadDisplayed) = 0;
	virtual QuadIndex GetFirstQuadDisplayed() const = 0;

	// sets and gets whether the quadrants have uniform width
	virtual void SetUseUniformQuadWidth(bool useUniformQuadWidth) = 0;
	virtual bool GetUseUniformQuadWidth() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetUseUniformQuadWidthBOOL(BOOL useUniformQuadWidth) = 0;
	virtual BOOL GetUseUniformQuadWidthBOOL() const = 0;

	// sets and gets whether the quad menus are mirrored (left - right)
	virtual void SetMirrorQuad(bool mirrorQuad) = 0;
	virtual bool GetMirrorQuad() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetMirrorQuadBOOL(BOOL mirrorQuad) = 0;
	virtual BOOL GetMirrorQuadBOOL() const = 0;

	// sets and gets whether the cursor moves when the quad menu is repositioned because of clipping the edge of the screen
	virtual void SetMoveCursorOnReposition(bool moveCursorOnReposition) = 0;
	virtual bool GetMoveCursorOnReposition() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetMoveCursorOnRepositionBOOL(BOOL moveCursorOnReposition) = 0;
	virtual BOOL GetMoveCursorOnRepositionBOOL() const = 0;

	// sets and gets whether the cursor is moved the opposite distance that it was automatically moved when the quad menu is repositioned because of clipping the edge of the screen
	virtual void SetReturnCursorAfterReposition(bool returnCursorAfterReposition) = 0;
	virtual bool GetReturnCursorAfterReposition() const = 0;
	// these overrides are provided for the function publishing system
	virtual void SetReturnCursorAfterRepositionBOOL(BOOL returnCursorAfterReposition) = 0;
	virtual BOOL GetReturnCursorAfterRepositionBOOL() const = 0;

	// sets and gets the initial location of the cursor in the center box - as a ratio (0 to 1) of box size
	virtual void SetCursorLocInBox_0to1(float x, float y) = 0;
	virtual float GetCursorLocXInBox_0to1() const = 0;
	virtual float GetCursorLocYInBox_0to1() const = 0;


	// gets the color array for a specific quad (numbered 1 through 4)
	virtual const MenuColors *GetMenuColors(int quadNum) const = 0;

	// sets and gets the title bar background color for a specific quad (numbered 1 through 4)
	virtual void SetTitleBarBackgroundColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetTitleBarBackgroundColor(int quadNum) const = 0;
	virtual COLORREF GetTitleBarBackgroundColorRef(int quadNum) const = 0;

	// sets and gets the title bar text color for a specific quad (numbered 1 through 4)
	virtual void SetTitleBarTextColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetTitleBarTextColor(int quadNum) const = 0;
	virtual COLORREF GetTitleBarTextColorRef(int quadNum) const = 0;

	// sets and gets the item background color for a specific quad (numbered 1 through 4)
	virtual void SetItemBackgroundColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetItemBackgroundColor(int quadNum) const = 0;
	virtual COLORREF GetItemBackgroundColorRef(int quadNum) const = 0;

	// sets and gets the item text color for a specific quad (numbered 1 through 4)
	virtual void SetItemTextColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetItemTextColor(int quadNum) const = 0;
	virtual COLORREF GetItemTextColorRef(int quadNum) const = 0;

	// sets and gets the last executed item text color for a specific quad (numbered 1 through 4)
	virtual void SetLastExecutedItemTextColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetLastExecutedItemTextColor(int quadNum) const = 0;
	virtual COLORREF GetLastExecutedItemTextColorRef(int quadNum) const = 0;

	// sets and gets the highlighted item background color for a specific quad (numbered 1 through 4)
	virtual void SetHighlightedItemBackgroundColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetHighlightedItemBackgroundColor(int quadNum) const = 0;
	virtual COLORREF GetHighlightedItemBackgroundColorRef(int quadNum) const = 0;

	// sets and gets the highlighted item text color for a specific quad (numbered 1 through 4)
	virtual void SetHighlightedItemTextColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetHighlightedItemTextColor(int quadNum) const = 0;
	virtual COLORREF GetHighlightedItemTextColorRef(int quadNum) const = 0;

	// sets and gets the border color for a specific quad (numbered 1 through 4)
	virtual void SetBorderColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetBorderColor(int quadNum) const = 0;
	virtual COLORREF GetBorderColorRef(int quadNum) const = 0;

	// sets and gets the disabled shadow color for a specific quad (numbered 1 through 4)
	virtual void SetDisabledShadowColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetDisabledShadowColor(int quadNum) const = 0;
	virtual COLORREF GetDisabledShadowColorRef(int quadNum) const = 0;

	// sets and gets the disabled highlight color for a specific quad (numbered 1 through 4)
	virtual void SetDisabledHighlightColor(int quadNum, const Color& color) = 0;
	virtual const Color& GetDisabledHighlightColor(int quadNum) const = 0;
	virtual COLORREF GetDisabledHighlightColorRef(int quadNum) const = 0;
};


#define MENU_SETTINGS Interface_ID(0x31561ddb, 0x1a2f4619)
inline IQuadMenuSettings* GetQuadSettings() { return (IQuadMenuSettings*)GetCOREInterface(MENU_SETTINGS); }


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class IMenuElement

//  PURPOSE:
//    Abstract class (Interface) for any menu element
//
//  NOTES:
//    created:  02.17.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	// location indicates origin location relative to element rectangle
	enum OriginLocation { UPPER_LEFT, LOWER_LEFT, LOWER_RIGHT, UPPER_RIGHT };

	// sets and gets the element's origin, and origin location
	virtual void SetOrigin(const IPoint2& origin, OriginLocation location) = 0;
	virtual const IPoint2& GetOrigin() const = 0;

	// sets and gets the item's visibility
	virtual void SetVisible(bool visible) = 0;
	virtual bool GetVisible() = 0;

	// sets and gets the item's title
	virtual void SetTitle(const TCHAR *customTitle) = 0;
	virtual const TSTR& GetTitle() = 0;

	// sets and gets the enabled state of the element
	virtual void SetEnabled(bool enabled) = 0;
	virtual bool GetEnabled() = 0;

	// gets the element's size, in the menu's coordinate space
	virtual const IPoint2& GetSize() = 0;

	// gets the element's rectangle, in the menu's coordinate space
	virtual const Box2& GetRect() = 0;

	// determines if point is in element's rectangle
	virtual bool IsInRect(const IPoint2& point) = 0;
};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#define MENU_ITEM_INTERFACE Interface_ID(0x2e926bd1, 0x296e68f6)

class IMenuItem : public FPMixinInterface, public IMenuElement

//  PURPOSE:
//    Abstract class (Interface) for a menu item
//
//  NOTES:
//    created:  02.17.00 - mjm
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

{
public:
	typedef int IMenuItemIcon;

	// function prototype - to be called when item is selected
	typedef void (* ActionFn)(void);
	// function prototype - to be called before item is displayed
	typedef void (* PreDisplayCB)(IMenuItem& menuItem);

	// action mode - item selection instigates action item, calls function, or displays a submenu
	// these values are saved/loaded by the menu customization system. you may append new values,
	// but cannot change any existing values.
	enum ActionMode { AM_INACTIVE = 0,
                      AM_SEPARATOR,
                      AM_ITEM,
                      AM_FN,
                      AM_SUBMENU,
                      AM_ITEM_SUBMENU };

	// sets a new context for the menu, invalidating the menu's cache
	virtual void SetIMenuLocalContext(IMenuLocalContext* pIMenuLocalContext) = 0;

	// gets the current action mode
	virtual ActionMode GetActionMode() const = 0;

	// executes the current action
	virtual bool ExecuteAction() const = 0;

	// makes the item act as an item separator
	virtual void ActAsSeparator() = 0;
	// checks if the item is acting as an item separator
	virtual bool IsSeparator() const = 0;

	// sets and gets the current action item. GetActionItem returns NULL if the ActionMode is not AM_ITEM
	virtual void SetActionItem(ActionItem* pActionItem) = 0;
	virtual ActionItem* GetActionItem() const = 0;

	// sets and gets the current action function. GetActionFn returns NULL if the ActionMode is not AM_FN
	virtual void SetActionFn(ActionFn actionFn) = 0;
	virtual const ActionFn GetActionFn() const = 0;

	// sets and gets the submenu. GetSubMenu returns NULL if the ActionMode is not AM_SUBMENU
	virtual void SetSubMenu(IMenu* menu) = 0;
	virtual IMenu* GetSubMenu() = 0;

	// sets and gets the current pre-display callback
	virtual void SetPreDisplayCB(PreDisplayCB preDisplayCB) = 0;
	virtual const PreDisplayCB GetPreDisplayCB() const = 0;

	// displays the item
	virtual void Display(bool leftToRight) = 0;

	// gets the item's accelerator, returns 0 if none
	virtual TCHAR GetAccelerator() = 0;

	// sets and gets the item's icon
	virtual void SetIcon(MaxIcon* pMaxIcon) = 0;
	virtual const MaxIcon* GetIcon() const = 0;

	// sets and gets the item's checked state
	virtual void SetChecked(bool checked) = 0;
	virtual bool GetChecked() = 0;

	// sets and gets the item's highlighted state
	virtual void SetHighlighted(bool highlighted) = 0;
	virtual bool GetHighlighted() const = 0;

	// sets and gets if the item should use a custom title -- set via SetTitle()
	virtual void SetUseCustomTitle(bool useCustomTitle) = 0;
	virtual bool GetUseCustomTitle() const = 0;

	// sets and gets if the submenu-item should be displayed flat
	virtual void SetDisplayFlat(bool displayFlat) = 0;
	virtual bool GetDisplayFlat() const = 0;

	// called after a user/menu interaction
	virtual void PostMenuInteraction() = 0;

    // Function publishing function ids.
    enum
    {
        setTitle,
        getTitle,
        setUseCustomTitle,
        getUseCustomTitle,
        setDisplayFlat,
        getDisplayFlat,
        getIsSeparator,
        getSubMenu,
        getMacroScript,
    };
};


#define MENU_INTERFACE Interface_ID(0x4bd57e2e, 0x6de57aeb)

class IMenu : public FPMixinInterface, public IMenuElement
{
public:
	// sets a new context for the menu, invalidating the menu's cache
	virtual void SetIMenuGlobalContext(IMenuGlobalContext* pIMenuGlobalContext, int level, Tab<IMenuItem *> *pExecutedItemPath, const MenuColors *pMenuColors) = 0;

	// gets the menu's local context
	virtual IMenuLocalContext* GetIMenuLocalContext() = 0;

	// returns number of contained items
	virtual int NumItems() const = 0;

	// retrieves an item from the menu
	virtual IMenuItem* GetItem(int position) = 0;

	// adds an item into the menu, defaulting to the end position.
	// position 0 indicates beginnng of list. a negative or otherwise invalid position defaults to end of list.
	virtual void AddItem(IMenuItem* item, int position = -1) = 0;

	// if valid position, removes item at position in menu
	virtual void RemoveItem(int position) = 0;

	// if present, removes an item from the menu
	virtual void RemoveItem(IMenuItem* item) = 0;

	// returns the maximum size of all items in menu
	virtual IPoint2 GetMaxItemSize() = 0;

	// called before menu is first displayed during a user/menu interaction
	virtual void Initialize() = 0;

	// called after a user/menu interaction
	virtual void PostMenuInteraction() = 0;

	// handles an event occuring within the menu
	virtual bool HandleEvent(MenuEvent &event) = 0;

	// shows the menu with the given show type
	virtual void Show(DisplayMethod displayMethod = DM_NORMAL, Box2 *rect = NULL) = 0;

	// hides the menu with the given show type
	virtual void Hide(DisplayMethod displayMethod = DM_NORMAL) = 0;

	virtual IMenuItem* FindAccelItem(TCHAR accelerator) = 0;
	virtual IMenuItem* FindNewSelectedItem() = 0;

	// displays the menu
	virtual void Display(IMenu* pParentMenu = NULL, bool show = true) = 0;

	virtual void DisplayItems(IPoint2& origin, bool descending, bool leftToRight, bool nextSeparatorOK) = 0;

	// removes the menu from the display
	virtual void Undisplay() = 0;

	// determines if menu is displaying a submenu
	virtual bool IsDisplayingSubMenu() = 0;

	// notifies menu that timer has elapsed
	virtual void TimerElapsed(EventParam timingType) = 0;

	// sets/gets whether to show the title
	virtual void SetShowTitle(bool showTitle) = 0;
	virtual bool GetShowTitle() const = 0;

	// sets and gets the custom title
	virtual void SetCustomTitle(const TCHAR *customTitle) = 0;
	virtual const TSTR& GetCustomTitle() const = 0;

	// sets and gets if the item should use a custom title -- set via SetCustomTitle()
	virtual void SetUseCustomTitle(bool useCustomTitle) = 0;
	virtual bool GetUseCustomTitle() const = 0;

	// sets/gets whether to show the title
	virtual void SetUseGlobalWidths(bool useGlobalWidths) = 0;
	virtual bool GetUseGlobalWidths() const = 0;

    // return true if the menu has no visible items in it.
    virtual bool NoVisibleItems() = 0;

    // Function publishing function ids.
    enum
    {
        numItems,
        getItem,
        addItem,
        removeItem,
        removeItemByPosition,
        setTitle,
        getTitle,
        getUseCustomTitle,
    };        
};


class IPopupMenu : public IMenu
{
public:
	// sets and gets the menu
	virtual void SetMenu(IMenu* menu) const = 0;
	virtual IMenu* GetMenu() const = 0;

	virtual void TrackMenu(HWND hMessageWnd, bool displayAll = false) = 0;
};


class IMultiMenu
{
public:
	// returns number of contained menus allowed, -1 indicates infinite
	virtual int NumMenusAllowed() const = 0;

	// returns number of contained menus
	virtual int NumMenus() const = 0;

	// retrieves a menu from the multi-menu
	virtual IMenu* GetMenu(int position) = 0;

	// adds a menu into the container, defaulting to the end position.
	// position of -1 means end of list, 0 means beginning.
	virtual void AddMenu(IMenu* menu, int pos = -1) = 0;

	// if valid position, removes menu at position in menu
	virtual void RemoveMenu(int position) = 0;

	// if present, removes a menu from the quad menu
	virtual void RemoveMenu(IMenu* menu) = 0;

	// sets and gets the title for the menu position indicated. used when SetUseCustomTitle(true) has been called
	virtual void SetTitle(const TCHAR *customTitle, int pos) = 0; // TODO: use 'customtitle' in item as well
	virtual const TSTR& GetTitle(int pos) = 0;

	// sets and gets if the menu at indicated position should use a custom title -- set via SetTitle()
	virtual void SetUseCustomTitle(int pos, bool useCustomTitle) = 0;
	virtual bool GetUseCustomTitle(int pos) const = 0;
};


class IMenuBar : public IMultiMenu
{
public:
	virtual void TrackMenu(HWND hMessageWnd, bool displayAll = false) = 0;
};


#define QUAD_MENU_INTERFACE Interface_ID(0x78b735e9, 0x7c001f68)

class IQuadMenu : public FPMixinInterface, public IMultiMenu
{
public:
	virtual void TrackMenu(HWND hMessageWnd, bool displayAll = false) = 0;

    // Function publishing function ids.
    enum
    {
        getMenu,
        addMenu,
        removeMenu,
        removeMenuByPosition,
        setTitle,
        getTitle,
        trackMenu,
    };
};


MENUS_API IMenuItem * GetIMenuItem();
MENUS_API void ReleaseIMenuItem(IMenuItem *);

MENUS_API IMenu * GetIMenu();
MENUS_API void ReleaseIMenu(IMenu *);

MENUS_API IQuadMenu * GetIQuadMenu();
MENUS_API void ReleaseIQuadMenu(IQuadMenu *);


#endif // !defined( IMENUS_H_INCLUDED )
