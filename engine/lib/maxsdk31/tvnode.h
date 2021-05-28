/**********************************************************************
 *<
	FILE: tvnode.h

	DESCRIPTION: Track View Node Class

	CREATED BY: Rolf Berteig

	HISTORY: 11-14-96

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/


#ifndef __TVNODE_H__
#define __TVNODE_H__

#define TVNODE_CLASS_ID	Class_ID(0x8d73b8aa, 0x90f2ee71)

// Default position for appending
#define TVNODE_APPEND -1


// TrackViewNodes can contain one or more sub nodes or controllers.
// Sub-nodes and controllers are identified by a unique ID in the
// form of a Class_ID variable. This does not necessarily have to
// be the class ID of an existing plug-in, however plug-ins may
// wish to use thier class ID for any items they add to be sure they
// are unique.
//
// The Interface class provides access to the root track view node.
// From this node, new nodes can be added. There are two defined
// sub nodes:

#define GLOBAL_VAR_TVNODE_CLASS_ID		Class_ID(0xb27e9f2a, 0x73fad370)
#define VIDEO_POST_TVNODE_CLASS_ID		Class_ID(0x482b8d30, 0xb72c8511)

// These can be retreived by calling GetNode() on the track view root
// node and passing in one of the above IDs.


// These can be registered with a TVNode to intercept reference notifications
class TVNodeNotify {
	public:
		virtual RefResult NotifyRefChanged(
			Interval changeInt, RefTargetHandle hTarget, 
			PartID& partID,  RefMessage message)=0;
	};
 
class ITrackViewNode : public ReferenceTarget {
	public:
		virtual void AddNode(ITrackViewNode *node, TCHAR *name, Class_ID cid, int pos=TVNODE_APPEND)=0;
		virtual void AddController(Control *c, TCHAR *name, Class_ID cid, int pos=TVNODE_APPEND)=0;
		virtual int FindItem(Class_ID cid)=0;
		virtual void RemoveItem(int i)=0;
		virtual void RemoveItem(Class_ID cid)=0;
		virtual Control *GetController(int i)=0;
		virtual Control *GetController(Class_ID cid)=0;
		virtual ITrackViewNode *GetNode(int i)=0;
		virtual ITrackViewNode *GetNode(Class_ID cid)=0;
		virtual int NumItems()=0;
		virtual void SwapPositions(int i1, int i2)=0;
		virtual TCHAR *GetName(int i)=0;
		virtual void SetName(int i,TCHAR *name)=0;
		virtual void RegisterTVNodeNotify(TVNodeNotify *notify)=0;
		virtual void UnRegisterTVNodeNotify(TVNodeNotify *notify)=0;
	};

CoreExport ITrackViewNode *CreateITrackViewNode(BOOL hidden=FALSE);

#endif // __TVNODE_H__
