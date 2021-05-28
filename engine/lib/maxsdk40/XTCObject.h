/**********************************************************************
 *<
	FILE: XTCObject.h
				  
	DESCRIPTION:  Defines the Extension Channel Object

	CREATED BY: Nikolai Sander

	HISTORY: created 3 March 2000

 *>     Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#include "maxapi.h"
#include "plugapi.h"
#include "iFnPub.h"
/*------------------------------------------------------------------- 
  XTCObject:  
---------------------------------------------------------------------*/

class XTCObject : public InterfaceServer {
public:
	// Unique identifier
	virtual Class_ID ExtensionID()=0;
	virtual XTCObject *Clone()=0;
	
	// Channels that the XTCObject depends on. If a modifier changes a channel, 
	// that a XTCObject depends on, its Pre- and PostChanChangedNotify
	// methods will be called
	virtual ChannelMask DependsOn(){return 0;}
	
	// These are the channels, that the extension object changes in the Pre- 
	// or PostChanChangedNotify methods
	virtual ChannelMask ChannelsChanged(){return 0;}

	// These are the channels, that the extension object changes in the Pre- 
	// or PostChanChangedNotify methods
	virtual ChannelMask ChannelsUsed(){return 0;}
	
	// If an XTCObject wants to display itself in the viewport, it can 
	// overwrite this method
	virtual int  Display(TimeValue t, INode* inode, ViewExp *vpt, int flags,Object *pObj){return 0;};
	
	// This method will be called before a modifier is applied, that 
	// changes a channel, that the XTCObject depends on. In case the 
	// modifier is the last in the stack, bEndOfPipleine is true,
	// otherwise false

	virtual void PreChanChangedNotify(TimeValue t, ModContext &mc, ObjectState* os, INode *node,Modifier *mod, bool bEndOfPipeline){};

	// This method will be called after a modifier is applied, that 
	// changes a channel, that the XTC object depends on. In case the 
	// modifier is the last in the stack, bEndOfPipleine is true
	// otherwise false
	virtual void PostChanChangedNotify(TimeValue t, ModContext &mc, ObjectState* os, INode *node,Modifier *mod, bool bEndOfPipeline){};
	
	// If the XTC object returns true from this method, the object 
	// is not displayed in the viewport
	virtual BOOL SuspendObjectDisplay(){ return false; }
	virtual void DeleteThis()=0;
	
	// This method allows the object to enlarge its viewport rectangle,
	// if it wants to. The system will call this method for all XTCObjects 
	// when calculating the viewport rectangle; the XTCObject can enlarge the 
	// rectangle if desired
	virtual void MaybeEnlargeViewportRect(GraphicsWindow *gw, Rect &rect){}

	// by default the existing XTCObjects will be deleted, if a branch updates. 
	// In case the XTCObject wants to do more intelligent branching (not just 
	// deleted and add), it might want to return false to this method, so that 
	// it can later (see MergeXTCObject) copy the data from this and other 
	// branches into an existing XTCObject.
	virtual bool RemoveXTCObjectOnMergeBranches(Object *obFrom, Object *obTo) { return true; }
	

	// The default implementation just adds the XTCObject to the to object
	// In case the XTCObject should do a more intelligent merge with already
	// existing XTCObjects in the obTo, it has to overwrite this method
	virtual bool MergeXTCObject(Object *obFrom, Object *obTo, int prio, int branchID) { obTo->AddXTCObject(this,prio,branchID); return true;}

	// In case a branch of a compound object is deleted the XTCObject will be asked,
	// if the XTCObject should be deleted as well. In case the XTCObject represents a
	// merge of all branches the TCObject might want to return false to this
	// method and reassign itself to another branch, so that the merged information is 
	// not lost.
	virtual bool RemoveXTCObjectOnBranchDeleted(Object *ComObj,int branchID, bool branchWillBeReordered) { return true; }
};
