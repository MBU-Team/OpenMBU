/*********************************************************************
 *<
	FILE: nodeDisp.h

	DESCRIPTION: Interface for node display callbacks

	CREATED BY:	Cleve Ard

	HISTORY: Created April 3, 2000

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __NODEDISP__
#define __NODEDISP__

// A callback to allow plug-ins that aren't actually objects (such as utilities)
// to control a Node's display
class NodeDisplayCallback : public InterfaceServer {
	public:

		// StartDisplay is called right before the tree of nodes is drawn
      virtual void StartDisplay(TimeValue t, ViewExp *vpt, int flags)=0;
		// EndDisplay is called right after the tree of nodes is drawn
       virtual void EndDisplay(TimeValue t, ViewExp *vpt, int flags)=0;
		// Display: Called for every node
		virtual bool Display(TimeValue t, ViewExp *vpt, int flags, INode *node,Object *pObj)=0;		
		// HideObject: Queries if the normal node mesh should be displayed
		virtual bool SuspendObjectDisplay(TimeValue t,INode *node)=0;
		// AddNodeCallbackBox: Asks the callback to participate in the bounding box calculation
		virtual void AddNodeCallbackBox(TimeValue t, INode *node, ViewExp *vpt, Box3& box,Object *pObj)=0;
		// HitTest: Hit testing on the callback's mesh
		virtual bool HitTest(TimeValue t, INode *node, int type, int crossing, int flags, IPoint2 *p, ViewExp* vpt,Object *pObj)=0;
		// Activate: Called when the callback gets activated, it's up to the callback to invalidate the screen
		virtual void Activate()=0;
		// Deactivate: Called when the callback is deactivated
		virtual void Deactivate()=0;
		// GetName: Queries the name of the callback. (Used for display in the menu)
		virtual TSTR GetName() const = 0;// user must delete the string

	};

#define I_NODEDISPLAYCONTROL  0x00001000	

// Gets a pointer to the INodeDisplayControl interface, the caller should pass a pointer to "Interface"
#define GetNodeDisplayControl(i)  ((INodeDisplayControl*)i->GetInterface(I_NODEDISPLAYCONTROL))

// An interface that is used to register the node display callback.
class INodeDisplayControl : public InterfaceServer
{
	public:

		// Register a NodeDisplayCallback
		virtual void RegisterNodeDisplayCallback(NodeDisplayCallback *cb)=0;
		virtual void UnRegisterNodeDisplayCallback(NodeDisplayCallback *cb)=0;

		//Set and get the current current callback, the callback must be registered
		virtual bool SetNodeCallback(NodeDisplayCallback* hook)=0;
		virtual NodeDisplayCallback* GetNodeCallback()=0;


		// Viewport refresh routine
		// this function only invalidates the display, it's up to the callback to select the correct redraw technique.
		virtual void InvalidateNodeDisplay()=0;

};

#endif
