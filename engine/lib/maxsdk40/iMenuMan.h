/* -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

   FILE: iMenuMan.h

	 DESCRIPTION: abstract classes for the menu manager

	 CREATED BY: Scott Morrison

	 HISTORY: created March 21, 2000

   	 Copyright (c) 2000, All Rights Reserved

// -----------------------------------------------------------------------------
// -------------------------------------------------------------------------- */

// Only include this one time
#pragma once

#include "iMenus.h"

typedef int MenuContextId;

// Define some menu contexts used by the code of MAX

const MenuContextId kMainMenuBar       = 1;
const MenuContextId kViewportQuad      = 2;
const MenuContextId kSchematicViewQuad = 3;
const MenuContextId kIReshadeQuad      = 4;
//watje for the uvw unwrap quad menu
const MenuContextId kIUVWUnwrapQuad    = 5;


enum MenuContextType {
    kMenuContextMenuBar = 0,
    kMenuContextQuadMenu,
    kMenuContextPopupMenu,
};
    

class IMenuContext : public InterfaceServer {

public:
    virtual MenuContextId GetId() = 0;
    virtual MenuContextType GetType() = 0;
    virtual TSTR& GetName() = 0;
};

class IMenuBarContext: public IMenuContext {

public:

    virtual void SetMenu(IMenu* pMenu) = 0;

    virtual IMenu* GetMenu() = 0;
    virtual HMENU CreateWindowsMenu() = 0;
    virtual void UpdateWindowsMenu() = 0;

    virtual HMENU GetCurWindowsMenu() = 0;

    virtual void ExecuteAction(int cmdId) = 0;
    virtual bool CommandIDInRange(int cmdId) = 0;
};

class IQuadMenuContext: public IMenuContext {

public:

    // Add a new quad menu to the context.
    virtual bool AddQuadMenu(IQuadMenu* pMenu, TCHAR* pName) = 0;
    // Set the quad menu for a slot in the context.
    virtual void SetMenu(int index, IQuadMenu* pMenu, TCHAR* pName) = 0;
    // Remove a Quad menu from the context.
    virtual void RemoveMenu(int index) = 0; 

    // Accessors for all the Quad menus in the context
    virtual int MenuCount() = 0;
    virtual IQuadMenu* GetMenu(int index) = 0;

    // Get/Set the index of the current default right-click menu
    virtual int GetCurrentMenuIndex() = 0;
    virtual void SetCurrentMenuIndex(int index) = 0;

    // Get set the falg that indicates we should display all quads in the menu
    virtual bool GetShowAllQuads(int index) = 0;
    virtual void SetShowAllQuads(int index, bool showAll) = 0;

    // The list of contexts (modifiers keys pressed) for right-click menus
    enum RightClickContext {
        kNonePressed,
        kShiftPressed,
        kAltPressed,
        kControlPressed,
        kShiftAndAltPressed,
        kShiftAndControlPressed,
        kControlAndAltPressed,
        kShiftAndAltAndControlPressed,
    };

    // This method queries the state of the modifier keys and returns the
    // appropriate context.
    virtual RightClickContext GetRightClickContext() = 0;

    // Get/Set the right-click menu associated with a context
    virtual IQuadMenu* GetRightClickMenu(RightClickContext context) = 0;
    virtual void SetRightClickMenu(RightClickContext context, IQuadMenu *pMenu) = 0;

    // return the index for the given quad menu.  Return -1 if the menu is not in the context.
    virtual int FindMenu(IQuadMenu* pMenu) = 0;
    // Find a Quad menu based on its name
    virtual IQuadMenu* FindMenuByTitle(TCHAR* pTitle) = 0;

};

const DWORD kMenuMenuBar  = (1 << 0);
const DWORD kMenuQuadMenu = (1 << 1);

#define MENU_MGR_INTERFACE  Interface_ID(0xadc20bd, 0x7491741d)
class IMenuManager : public FPStaticInterface {

public:
    // Add a menu to the manager return false of menu already is registered.
    virtual bool RegisterMenu(IMenu* pMenu, DWORD flags = 0) = 0;
    // Remove a menu form the mananger.  return false of the menu is not registered
    virtual bool UnRegisterMenu(IMenu* pMenu) = 0;

    // Find a menu based on its name
    virtual IMenu* FindMenu(TCHAR* pTitle) = 0;

    // Find a Quad menu based on its name
    virtual IQuadMenu* FindQuadMenu(TCHAR* pTitle) = 0;

    // Register a new 
    // Register a new context
    virtual bool RegisterMenuBarContext(MenuContextId contextId, TCHAR* pName) = 0;
    virtual bool RegisterQuadMenuContext(MenuContextId contextId, TCHAR* pName) = 0;

    virtual int NumContexts() = 0;
    virtual IMenuContext* GetContextByIndex(int index) = 0;
    virtual IMenuContext* GetContext(MenuContextId contextId) = 0;

    // Update MAX's main menu bar after adding sub-menus or menu items
    virtual void UpdateMenuBar() = 0;

    // Load a menu file and update everything.  
    virtual BOOL LoadMenuFile(TCHAR* pMenuFile) = 0;

    // Save a menu file.
    virtual BOOL SaveMenuFile(TCHAR* pMenuFile) = 0;

    // Get the name of the current menu file.
    virtual TCHAR* GetMenuFile() = 0;

     // Set the given menu to be used as the main menu bar
    virtual BOOL SetMainMenuBar(IMenu* pMenu) = 0;

    // Set the the viewport right-click menu to be the given quad menu
    virtual BOOL SetViewportRightClickMenu(IQuadMenuContext::RightClickContext context,
                                           IQuadMenu* pQuadMenu) = 0;

    // Get the the current viewport right-click menu 
    virtual IQuadMenu* GetViewportRightClickMenu(IQuadMenuContext::RightClickContext context) = 0;

    // Set the the given viewport right-click menu to be the named quad menu
    virtual IMenu* GetMainMenuBar() = 0;

    // Get/Set the show all wuad flags on a quad menu
    virtual bool GetShowAllQuads(IQuadMenu* pQuadMenu) = 0;
    virtual void SetShowAllQuads(IQuadMenu* pQuadMenu, bool showAll) = 0;

    // Get/Set the name of a quad menu
    virtual TCHAR* GetQuadMenuName(IQuadMenu* pQuadMenu) = 0;
    virtual void SetQuadMenuName(IQuadMenu* pQuadMenu, TCHAR* pName) = 0;

	// Function Publishing IDs
	enum { loadMenuFile,
           saveMenuFile,
           getMenuFile,
           unRegisterMenu,
           unRegisterQuadMenu,
           registerMenuContext,
           findMenu,
           findQuadMenu,
           updateMenuBar,
           createQuadMenu,
           createMenu,
           createSubMenuItem,
           createSeparatorItem,
           createActionItem,
           setViewportRightClickMenu,
           getViewportRightClickMenu,
           getMainMenuBar,
           setMainMenuBar,
           getShowAllQuads,
           setShowAllQuads,
           numMenus,
           getMenu,
           numQuadMenus,
           getQuadMenu,
           getQuadMenuName,
           setQuadMenuName,
    };

	// enumeration IDs
	enum { rightClickContext, };
};

