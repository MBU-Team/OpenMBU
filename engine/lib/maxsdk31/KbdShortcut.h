 /**********************************************************************
 *<
	FILE: KbdShortcut.h

	DESCRIPTION: Keyboard Shortcut table definitions

	CREATED BY:	Scott Morrison

	HISTORY: Created 10 July, 1998

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

#ifndef __KBDSHORTCUT__
#define __KBDSHORTCUT__

// ShortcutTableIds used by the system
const ShortcutTableId kShortcutMainUI = 0;
const ShortcutTableId kShortcutTrackView = 1;
const ShortcutTableId kShortcutMaterialEditor = 2;
const ShortcutTableId kShortcutVideoPost = 3;
const ShortcutTableId kShortcutSchematicView = 5;


// Description of a command for building shortcut tables from static data
struct ShortcutDescription {
	int mCmdID;
	int mResourceID;
};

// Describes an operation that can be attached to a Shortcut
class ShortcutOperation {

public:
    CoreExport ShortcutOperation()
        {
            mCmdId = 0;
            mpName = NULL;
        }

    CoreExport int    GetId() { return mCmdId; }
    CoreExport void SetId(int id) { mCmdId = id; }
    CoreExport TCHAR*  GetName() { return mpName;}
    CoreExport void SetName(TCHAR* pName) { mpName = pName; }

private:
    int     mCmdId;     // The command id sent to the window proc
    TCHAR*  mpName;     // The name of the operation the user sees

};

// A table of accerators used by plug-ins
class ShortcutTable {

public:
    CoreExport ShortcutTable(ShortcutTableId id, TSTR& name, HACCEL hDefaults,
                             int numIds, ShortcutDescription* pOps,
                             HINSTANCE hInst);
    CoreExport ~ShortcutTable();

    CoreExport HACCEL GetHAccel() { return mhAccel; }
    CoreExport void SetHAccel(HACCEL hAccel) { mhAccel = hAccel; }
    CoreExport HACCEL GetDefaultHAccel() { return mhDefaultAccel; }
    CoreExport TSTR& GetName() { return mName; }
    CoreExport ShortcutTableId GetId() { return mId; }
    CoreExport ShortcutOperation& operator[](int i) { return mOps[i]; }
    CoreExport int Count() { return mOps.Count(); }
    CoreExport void DeleteThis() { delete this; }

    CoreExport TCHAR* GetString(int commandId);

private:
    // These values are set by the plug-in to describe a shortcut table

    // Unique identifier of table (like a class id)
    ShortcutTableId  mId;
    // Name to use in preference dlg drop-down
    TSTR mName;
    // Descriptors of all operations that can have Shortcuts
    Tab<ShortcutOperation>  mOps; 

    // The windows accelerator table in use
    HACCEL mhDefaultAccel;
    // The windows accelerator table in use
    HACCEL mhAccel;

};

class ShortcutCallback {
public:
    virtual BOOL KeyboardShortcut(int id) = 0;
};

#endif
