/**********************************************************************
 *<
	FILE: baseInterface.h

	DESCRIPTION: Base classes for various interfaces in MAX

	CREATED BY: John Wainwright

	HISTORY: created 9/5/00

 *>	Copyright (c) 2000 Autodesk, All Rights Reserved.
 **********************************************************************/

#ifndef __BASEINTERFACE_H__
#define __BASEINTERFACE_H__

class InterfaceServer;
class BaseInterfaceServer;
class BaseInterface;
class InterfaceNotifyCallback;

// ID for BaseInterfaces
#define BASEINTERFACE_ID Interface_ID(0, 1)

// Base class for those classes and interfaces in MAX that can serve interfaces.  
class InterfaceServer
{
public:
	virtual BaseInterface* GetInterface(Interface_ID id) { return NULL; }
};

// The base class for interfaces in MAX R4.  
//   Provides basic identity, sub-interface access, lifetime management and 
//   cloning methods.   The base class for FPInterface in the FnPub system.
//  
class BaseInterface : public InterfaceServer
{
public:
	// from InterfaceServer
	BaseInterface* GetInterface(Interface_ID id) { if (id == BASEINTERFACE_ID) return this; else return NULL; }

	// identification
	virtual Interface_ID	GetID() { return BASEINTERFACE_ID; }

	// interface lifetime management
	//   there is an implied Acquire() whenever an interface is served via a GetInterface()
	//   nortmally requiring a matching Release() from the client
	//   can optionally use LifetimeControl() method to enquire about actual
	//   lifetime policy of client and provide a server-controlled delete notify callback
	//		'noRelease' means don't need to call release, use interface as long as you like. 
	//		'immediateRelease' means the interface is good for only one call, but the release 
	//		    is implied, you don't need to call release.
	//		'wantsRelease' means the clients are controlling lifetime so the interface needs 
	//			a Release() when the client has finished (default).
	//		'serverControlled' means the server controls lifetime and will use the InterfaceNotifyCallback 
	//			to tell you when it is gone.
	enum LifetimeType { noRelease, immediateRelease, wantsRelease, serverControlled }; 

	// LifetimeControl returns noRelease since 
	// AcquireInterface and ReleaseInterface do not perform 
	// any real acquiring and releasing (reference counting, etc.)
	// If the implementation of AcquireInterface and ReleaseInterface changes
	// in this class or derived classes, the return value of LifetimeControl 
	// needs to be updated accordingly.
	virtual LifetimeType	LifetimeControl() { return noRelease; }
	virtual void			RegisterNotifyCallback(InterfaceNotifyCallback* incb) { }
	virtual void			UnRegisterNotifyCallback(InterfaceNotifyCallback* incb) { }
	virtual BaseInterface*	AcquireInterface() { return (BaseInterface*)this; };  
	virtual void			ReleaseInterface() { };

	// direct interface delete request
	virtual void			DeleteInterface() { };

	// interface cloning
	virtual BaseInterface*	CloneInterface(void* remapDir = NULL) { return NULL; }
};

// BaseInterface server specializes InterfaceServer with an implementation
//   based on a Tab<> of interface pointers for storing interfaces, 
//   typically extension interfaces, and providing an interface iteration
//   protocol.
//   class IObject in the FnPub system specializes BaseInterfaceServer 
class BaseInterfaceServer : public InterfaceServer
{
protected:
	Tab<BaseInterface*> interfaces;

public:
	// interface serving, default implementation looks in interfaces table
	virtual BaseInterface* GetInterface(Interface_ID id) 
	{ 
		for (int i = 0; i < interfaces.Count(); i++)
			if (interfaces[i]->GetID() == id)
				return interfaces[i]->AcquireInterface();
		return NULL; 
	}
	// interface enumeration...
	virtual int NumInterfaces() const { return interfaces.Count(); }					
	virtual BaseInterface* GetInterfaceAt(int i) const { return interfaces[i]; }

	// default descructor, calls DeleteInterface() on all live entries in interfaces table
	~BaseInterfaceServer()
	{
		for (int i = 0; i < interfaces.Count(); i++)
			if (interfaces[i] != NULL)
				interfaces[i]->DeleteInterface();
	}
};

// InterfaceNotifyCallback base class, 
//   can be specialized by clients of an interface that controls its own lifetime
//   so that they can be notified when the interface is deleted or other changes occur.
//   registered with the interface via the lifetime control protocol 
class InterfaceNotifyCallback
{
public:
	// notify server is deleting  the interface
	virtual void InterfaceDeleted(BaseInterface* bi) { }

	// for furture notification extensions
	virtual BaseInterface* GetInterface(Interface_ID id) { return NULL; }
};

#endif
