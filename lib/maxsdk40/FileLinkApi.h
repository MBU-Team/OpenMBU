/*********************************************************************
 *<
	FILE: FileLinkApi.h

	DESCRIPTION: File Link interface class

	CREATED BY:	Nikolai Sander

	HISTORY: Created 29 January 1998

 *>	Copyright (c) 1997-1999, All Rights Reserved.
 **********************************************************************/

#ifndef FILELINKAPI_H
#define FILELINKAPI_H

#include <maxtypes.h>


/******
	Accessing the Link Table:
	If you want your plugin to be able to access the IVizLinkTable interface
	without incurring a load-time dependency, add the following function to
	your plugin. With this, your plugin will load whether or not the
	VizLink plugin is available.

// Returns NULL if the LinkTable plugin is not present on this system.
IVizLinkTable* GetLinkTable()
{
	// Look for the LinkTable node in track view.
	ITrackViewNode* tvRoot = GetCOREInterface()->GetTrackViewRootNode();
	int i = tvRoot->FindItem(VIZLINKTABLE_CLASS_ID);
	if (i < 0)
		return NULL;

	// Get the node's controller.
	ITrackViewNode* tvNode = tvRoot->GetNode(i);
	Control* pc = tvNode->GetController(VIZLINKTABLE_CLASS_ID);
	if (pc == NULL)
		return NULL;

	// Call GetInterface to confirm that this is the proper instance.
	return GetVizLinkTable(pc);
}
******/

#define VIZLINKTABLE_CLASS_ID Class_ID(0xa20bbe82, 0x70c763d)

#define I_VIZLINKCONTROLLER (I_USERINTERFACE+0x1739)
#define GetVizLinkTable(anim) ((IVizLinkTable*)anim->GetInterface(I_VIZLINKCONTROLLER))

#define kFILES_FORMAT -1


class VizLinkList;
class FormatRegistry;
class FormatFactory;
class LinkTableRecord;
class LinkedObjectsEnum;
class IFileLinkManager;


// Interface to the underlying implementation. Also designed as 
// a Facade to the more complicated linking process.
//
class IVizLinkTable : public StdControl
{
public :
	typedef int Iterator;

	// Access to the UI driver.
	virtual IFileLinkManager* GetFileLinkManager() = 0;

	// If you pass in kFILES_FORMAT for the format argument, the format type will be
	// determined from the filename.
	virtual BOOL DoAttach(const TCHAR* filename,
						int format = 0,
						BOOL suppressPrompts = FALSE,
						BOOL readOnly = TRUE) = 0;
	virtual int NumLinkedFiles() const = 0;
	virtual bool GetLinkID(int i, Iterator& iter) const = 0;
	virtual bool DoReload(Iterator iter, BOOL suppressPrompts = FALSE) = 0;
	virtual bool DoDetach(Iterator iter) = 0;
	virtual bool DoBind(Iterator iter) = 0;
	virtual LinkTableRecord* RecordAt(IVizLinkTable::Iterator id) = 0;
	virtual bool ChangeLinkFile(Iterator iter, const TSTR& str) = 0;

// Auto-reload event handling.
protected:
	friend class DBManUI;
	// Only DBManUI can turn this on and off.
	virtual void EnableAutoReload(bool enable) = 0;
public:
	virtual void WaitForReloadThread() const = 0;

public:
	// List updating notification.
	virtual void RegisterForListUpdates(VizLinkList*) = 0;
	virtual void UnregisterForListUpdates(VizLinkList*) = 0;
	virtual void UpdateList() = 0;

	// For iterating over all linked nodes.
	virtual void EnumerateLinkedObjects(LinkedObjectsEnum* EnumProc) = 0;

	// Linked splines can be rendered.
	virtual void SetRenderSplines(BOOL b) = 0;
	virtual BOOL GetRenderSplines() const = 0;
	virtual BOOL SetSplineRenderThickness(float f) = 0;
	virtual float GetSplineRenderThickness() const = 0;
	virtual void SetGenUVs(BOOL b) = 0;
	virtual BOOL GetGenUVs() const = 0;
	virtual void SetShapeRenderFlags(Object *pObj) const = 0;

	// If you are supporting a new format, register your factory class
	// here.
	virtual BOOL RegisterFactory(Class_ID& cid) = 0;
	virtual FormatRegistry* Registry() = 0;

};


// This interface provides access to some of the link manager's functions.
class IFileLinkManager
{
public:
	// Select a file, prompt for settings, and link the file.
	virtual BOOL DoAttach(BOOL suppressPrompts = FALSE) = 0;

	// Display the manager.
	virtual void OpenFileLinkManager(Interface* ip) = 0;
	// Enable/Disable the manager
	virtual void EnableFileLinkManager(BOOL enable) = 0;
};


// If you have a list that you want dynamically updated whenever a 
// linked file's status changes, derive your class from this, and
// implement the inherited method(s). For example, a utility plugin
// would derive from both UtilityObj and from VizLinkList.
class VizLinkList
{
public:
	virtual void RefreshLinkedFileList() = 0;
};


#endif //FILELINKAPI_H
