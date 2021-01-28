/**********************************************************************
 *<
	FILE: IRefHierarchy.h

	DESCRIPTION: Interface for accessing reference hierarchy related info

	CREATED BY: Attila Szabo

	HISTORY: Created Nov/27/00

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/
#ifndef __IREFHIERARCHY__H
#define __IREFHIERARCHY__H

#include "iFnPub.h"

// This interface is supposed to group reference hierarchy related methods
class IRefHierarchy : public FPStaticInterface 
{
	public:
	
		// function IDs 
		enum 
		{ 
			fnIdIsRefTargetInstanced,
		};

		// This method can be used to find out if an object is instanced 
		// (instanced pipeline). 
		// It Checks if the Derived Object is instanced. If it is, the pipeline 
		// part below and including that derived object is instanced.
		// If the argument is NULL, returns FALSE
		virtual BOOL IsRefTargetInstanced( ReferenceTarget* refTarget ) = 0;
};


#define REFHIERARCHY_INTERFACE Interface_ID(0x296e2793, 0x247d12e4)
inline IRefHierarchy* GetRefHierarchyInterface() 
{ 
	return static_cast<IRefHierarchy*>(GetCOREInterface(REFHIERARCHY_INTERFACE)); 
}


#endif // __IREFHIERARCHY__H