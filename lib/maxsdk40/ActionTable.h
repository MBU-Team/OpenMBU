 /**********************************************************************
 *<
	FILE: ActionTable.h

	DESCRIPTION: Action Table definitions

	CREATED BY:	Scott Morrison

	HISTORY: Created 8 February, 2000,
             Based on KbdAction.h.

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

// The ActionTable class is used by plug-ins (and core) to export
// tables of items that can be used by the UI to attach to keyboard
// shorcuts, assign to toolbar buttons, and add to menus.
#pragma once

#include "stack.h"
#include "iFnPub.h"

class MaxIcon;
class MacroEntry;

typedef DWORD ActionTableId;
typedef DWORD ActionContextId;

// ActionTableIds used by the system
const ActionTableId kActionMainUI           = 0;
const ActionTableId kActionTrackView        = 1;
const ActionTableId kActionMaterialEditor   = 2;
const ActionTableId kActionVideoPost        = 3;
const ActionTableId kActionSchematicView    = 5;
const ActionTableId kActionCommonIReshade   = 6;
const ActionTableId kActionScanlineIReshade = 7;

class ActionTable;
class IMenu;

// Context IDs used by the system.  Several tables may share the same context id.
const ActionContextId kActionMainUIContext         = 0;
const ActionContextId kActionTrackViewContext      = 1;
const ActionContextId kActionMaterialEditorContext = 2;
const ActionContextId kActionVideoPostContext      = 3;
const ActionContextId kActionSchematicViewContext  = 5;
const ActionContextId kActionIReshadeContext       = 6;

// Description of a command for building action tables from static data

struct ActionDescription {

    // A unique identifier for the command (must be uniqe per table)
	int mCmdID;

    // A string resource id that describes the command
	int mDescriptionResourceID;

    // A string resource ID for a short name for the action
    int mShortNameResourceID;

    // A string resource for the category of an operation
    int mCategoryResourceID;

};

// Describes an single operation that can be attached to a UI elements

class ActionItem : public InterfaceServer {

public:
    virtual int    GetId() = 0;

    // return true if action executed, and FALSE otherwise
    virtual BOOL ExecuteAction() = 0;

	// override this if you wish to customize macroRecorder output for this action
	CoreExport virtual void EmitMacro();

	// internal Execute(), handles macroRecording, etc. and call ExecuteAction()  - jbw 9.9.00
	CoreExport BOOL Execute(); 

    // Get the text to put on a button
    virtual void GetButtonText(TSTR& buttonText) = 0;
    // Get the text to use in a menu
    virtual void GetMenuText(TSTR& menuText) = 0;
    // Get the long description text for tool tips etc.
    virtual void GetDescriptionText(TSTR& descText) = 0;
    // Get the string describing the category of the item
    virtual void GetCategoryText(TSTR& catText) = 0;

    // check to see if menu items should be checked, or button pressed
    virtual BOOL IsChecked() = 0;
    // Check to see if menu item should show up in context menu
    virtual BOOL IsItemVisible() = 0;
    // Check to see if menu item should be enabled in a menu
    virtual BOOL IsEnabled() = 0;
    
    virtual MaxIcon* GetIcon() = 0;

    virtual void DeleteThis() = 0;

    CoreExport ActionTable* GetTable() { return mpTable; }
    CoreExport void SetTable(ActionTable* pTable) { mpTable = pTable; }

    CoreExport TCHAR* GetShortcutString();

	virtual MacroEntry* GetMacroScript() { return NULL; }

    // If this method returns true, then the ActionItem creates
    // a menu instead of performing an action
    CoreExport virtual BOOL IsDynamicMenu() { return FALSE; }
    // This can be called after an action item is created to tell the
    // system that is is a dynamic menu action.
    CoreExport virtual void SetIsDynamicMenu() {}
    // If the ActionItem does produce a menu, this method is called To
    // get the menu.  See the DynamicMenu class below for an easy way
    // to produce these menus.  If the menu is requested by a
    // right-click quad menu, then hwnd is the window where the click
    // occurred, and m is the point in the window where the user
    // clicked.  If the item is used from a menu bar, hwnd will be NULL.
    CoreExport virtual IMenu* GetDynamicMenu(HWND hwnd, IPoint2& m) { return NULL; }

    // ActionItems that are deleted after they execute should return TRUE.
    CoreExport virtual BOOL IsDynamicAction() { return FALSE; }

protected:

    ActionTable* mpTable;  // The table that owns the action
};

class ActionCallback;

// A table of actions that can be tied to UI elements (buttons, menus, RC menu,
// keyboard shortcuts)

class ActionTable : public BaseInterfaceServer {

public:
    CoreExport ActionTable(ActionTableId id,
                           ActionContextId contextId,
                           TSTR& name,
                           HACCEL hDefaults,
                           int numIds,
                           ActionDescription* pOps,
                           HINSTANCE hInst);
    CoreExport ActionTable(ActionTableId id,
                           ActionContextId contextId,
                           TSTR& name);

    CoreExport ~ActionTable();

    // Get/Set the current keyboard accelerators for the table
    CoreExport HACCEL GetHAccel() { return mhAccel; }
    CoreExport void SetHAccel(HACCEL hAccel) { mhAccel = hAccel; }
    // Get the default keyboard accelerator table.  This is used when
    // the user has not assigned any accelerators.
    CoreExport HACCEL GetDefaultHAccel() { return mhDefaultAccel; }
    CoreExport void SetDefaultHAccel(HACCEL accel) { mhDefaultAccel = accel; }

    CoreExport TSTR& GetName() { return mName; }
    CoreExport ActionTableId GetId() { return mId; }
    CoreExport ActionContextId GetContextId() { return mContextId; }

    // Get the current callback assocuated with this table.
    // returns NULL if the table is not active.
    CoreExport ActionCallback* GetCallback() { return mpCallback; }
    CoreExport void SetCallback(ActionCallback* pCallback) { mpCallback = pCallback; }

    // Methods to iterate over the actions in the table
    CoreExport int Count() { return mOps.Count(); }
    CoreExport ActionItem* operator[](int i) { return mOps[i]; }

    // Get an action by its command id.
    CoreExport ActionItem* GetAction(int cmdId);

    // Add an operation to the table
    CoreExport void AppendOperation(ActionItem* pAction);
    // Remove an operation from the table
    CoreExport BOOL DeleteOperation(ActionItem* pAction);

    CoreExport void DeleteThis() { delete this; }

    // Get the text to put on a button
    CoreExport virtual BOOL GetButtonText(int cmdId, TSTR& buttonText);
    // Get the text to use in a menu
    CoreExport virtual BOOL GetMenuText(int cmdId, TSTR& menuText)
        { return GetButtonText(cmdId, menuText); }
    // Get the long description text for tool tips etc.
    CoreExport virtual BOOL GetDescriptionText(int cmdId, TSTR& descText);

    // check to see if menu items should be checked, or button pressed
    CoreExport virtual BOOL IsChecked(int cmdId) { return FALSE; }
    // Check to see if menu item should show up in context menu
    CoreExport virtual BOOL IsItemVisible(int cmdId) { return TRUE; }
    // Check to see if menu item should be enabled in a menu
    CoreExport virtual BOOL IsEnabled(int cmdId) { return TRUE; }

    // Write an action identifier to a CUI file or KBD file
    // Default implementation is to write the integer ID.
    // This is over-riden when command IDs are not persistent
    CoreExport virtual void WritePersistentActionId(int cmdId, TSTR& idString);
    // Read an action identifier from a CUI file or KBD file
    // Default implementation is to read the integer ID.
    // This is over-riden when command IDs are not persistent
    // Returns -1 if the command is not found in the table
    CoreExport virtual int ReadPersistentActionId(TSTR& idString);

    // return an optional icon for the command
    CoreExport virtual MaxIcon* GetIcon(int cmdId) { return NULL; };

    // Fill the action table with the given action descriptions
    CoreExport void BuildActionTable(HACCEL hDefaults,
                                int numIds,
                                ActionDescription* pOps,
                                HINSTANCE hInst);

    // Get the action assigned to the given accelerator, if any
    CoreExport ActionItem* GetCurrentAssignment(ACCEL accel);
    // Assign the command to th given accelerator.  Also removes any
    // previous assignment to that accelerator
    CoreExport void AssignKey(int cmdId, ACCEL accel);
    // removes the given assignment from the shortcut table
    void RemoveShortcutFromTable(ACCEL accel);

private:
    // These values are set by the plug-in to describe a action table

    // Unique identifier of table (like a class id)
    ActionTableId  mId;

    // An identifier to group tables use the same context.  Tables with the
    // same context cannot have overlapping keyboard shortcuts.
    ActionContextId mContextId;

    // Name to use in preference dlg drop-down
    TSTR mName;

    // Descriptors of all operations that can have Actions
    Tab<ActionItem*>  mOps; 

    // The windows accelerator table in use when no keyboard shortcuts saved
    HACCEL mhDefaultAccel;
    // The windows accelerator table in use
    HACCEL mhAccel;

    // The currently active callback
    ActionCallback* mpCallback;
};

class ActionCallback : public BaseInterfaceServer {
public:
	CoreExport virtual ~ActionCallback(){};
    CoreExport virtual BOOL ExecuteAction(int id) { return FALSE; }
    // called when an action item says it is a dynamic menu
    CoreExport virtual IMenu* GetDynamicMenu(int id, HWND hwnd, IPoint2& m) { return NULL; }

    // Access to the table the callback uses
    ActionTable* GetTable() { return mpTable; }
    void SetTable(ActionTable* pTable) { mpTable = pTable; }

private:
    ActionTable *mpTable;
};

// An ActionContext is an identifer of a group of keyboard shortcuts.
// Examples are Main UI, Tack View, and Editable Mesh.  They are
// registered using Interface::RegisterActionContext().
//
class ActionContext {
public:
    ActionContext(ActionContextId contextId, TCHAR *pName)
        { mContextId = contextId; mName = pName; mActive = true; }

    TCHAR* GetName() { return mName.data(); }
    ActionContextId GetContextId() { return mContextId; }

    bool IsActive() { return mActive; }
    void SetActive(bool active) { mActive = active; }
    
private:
    ActionContextId  mContextId;
    TSTR             mName;
    bool             mActive;
};

// The ActionManager manages a set of ActionTables, callbacks and ActionContexts.
// The manager handles the keyboard accelerator tables for each ActionTable
// as well.  You get a pointer to this class using Interface::GetActionManager().

#define ACTION_MGR_INTERFACE  Interface_ID(0x4bb71a79, 0x4e531e4f)

class IActionManager : public FPStaticInterface  {

public:
    // Register an action table with the manager.
    // Note that most plug-ins will not need this method.  Instead,
    // plug-ins export action table with the methods in ClassDesc.
    virtual void RegisterActionTable(ActionTable* pTable) = 0;

    // Methods to iterate over the action table
    virtual int NumActionTables() = 0;
    virtual ActionTable* GetTable(int i) = 0;

    // These methods are used to turn a table on and off.
    virtual int ActivateActionTable(ActionCallback* pCallback, ActionTableId id) = 0;
    virtual int DeactivateActionTable(ActionCallback* pCallback, ActionTableId id) = 0;

    // Find a table based on its id.
    virtual ActionTable* FindTable(ActionTableId id) = 0;

    // Get the string that describes the keyboard shortcut for the operation
    virtual BOOL GetShortcutString(ActionTableId tableId, int commandId, TCHAR* buf) = 0;
    // Get A string the descibes an operation
    virtual BOOL GetActionDescription(ActionTableId tableId, int commandId, TCHAR* buf) = 0;

    // Register an action context.  This is called when you create the
    // action table that uses this context.
    virtual BOOL RegisterActionContext(ActionContextId contextId, TCHAR* pName) = 0;
    // Methods to iterate over the action contexts
    virtual int NumActionContexts() = 0;
    virtual ActionContext* GetActionContext(int i) = 0;
    // Find a context based on it's ID.
    virtual ActionContext* FindContext(ActionContextId contextId) = 0;

    // Query whether a context is active.
    virtual BOOL IsContextActive(ActionContextId contextId) = 0;

    // Internal methods used by the keyboard shotcut UI
    virtual TCHAR* GetShortcutFile() = 0;
    virtual TCHAR* GetShortcutDir() = 0;
    virtual int IdToIndex(ActionTableId id) = 0;
    virtual void SaveAllContextsToINI() = 0;

    virtual int MakeActionSetCurrent(TCHAR* pDir, TCHAR* pFile) = 0;
    virtual int LoadAccelConfig(LPACCEL *accel, int *cts, ActionTableId tableId = -1,
                                BOOL forceDefault = FALSE) = 0;
    virtual int SaveAccelConfig(LPACCEL *accel, int *cts) = 0;
    virtual int GetCurrentActionSet(TCHAR *buf) = 0;

    virtual BOOL SaveKeyboardFile(TCHAR* pFileName) = 0;
    virtual BOOL LoadKeyboardFile(TCHAR* pFileName) = 0;
    virtual TCHAR* GetKeyboardFile() = 0;

	// Function IDs
    enum {
        executeAction,
        saveKeyboardFile,
        loadKeyboardFile,
        getKeyboardFile,
    };
};

// The DynamicMenu class provides a way for plugins to produce
// the menu needed in the ActionItem::GetDynamicMenu() method.

class DynamicMenuCallback {
public:
    virtual void MenuItemSelected(int itemId) = 0;
};

class DynamicMenu {
public:

    CoreExport DynamicMenu(DynamicMenuCallback* pCallback);

    // Called after menu creation to get the IMenu created.
    // This is the value returned from  ActionItem::GetDynamicMenu()
    CoreExport IMenu* GetMenu();

    enum DynamicMenuFlags {
        kDisabled   = 1 << 0,
        kChecked    = 1 << 1,
        kSeparator  = 1 << 2,
    };

    // Add an item to the dynamic menu.
    CoreExport void AddItem(DWORD flags, UINT itemId, TCHAR* pItemTitle);
    CoreExport void BeginSubMenu(TCHAR* pTitle);
    CoreExport void EndSubMenu();

private:

    Stack<IMenu*> mMenuStack;
    DynamicMenuCallback *mpCallback;
};
