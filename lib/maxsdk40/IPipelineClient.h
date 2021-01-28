 /**********************************************************************
 
	FILE: IPipelineClient.h

	DESCRIPTION:  Geometry pipeline client API

	CREATED BY: Attila Szabo, Discreet

	HISTORY: [attilas|19.09.2000]


 *>	Copyright (c) 1998-2000, All Rights Reserved.
 **********************************************************************/

#ifndef __IPIPELINECLIENT__H
#define __IPIPELINECLIENT__H

#include "baseinterface.h"

// GUID that identifies this ifc (interface)
#define PIPELINECLIENT_INTERFACE Interface_ID(0x62383d51, 0x2d0f7d6a)

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// This interface should be implemented by objects that flow up the 
// geometry pipeline and have data members that belong to the pipeline
// channels (geometry, topology, texmap, etc)
// ... in other words by objects that are part of the data flow evaluation 
// of the Max stack. 
//________________________________________________________________________
class IPipelineClient : public BaseInterface
{
	public:
		// --- IPipelineClient methods
		virtual void	ShallowCopy( IPipelineClient* from, ULONG_PTR channels ) = 0;
		virtual void	DeepCopy( IPipelineClient* from, ULONG_PTR channels ) = 0;
		virtual void	NewAndCopyChannels( ULONG_PTR channels ) = 0;
		virtual void	FreeChannels( ULONG_PTR channels, int zeroOthers = 1 ) = 0;
		virtual void	ZeroChannels( ULONG_PTR channels ) = 0;
		virtual void	AppendAllChannels( IPipelineClient* from ) = 0;

		// --- from BaseInterface
		virtual Interface_ID	GetID() { return PIPELINECLIENT_INTERFACE; }
};

#endif 