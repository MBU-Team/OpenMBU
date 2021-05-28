 /**********************************************************************
 
	FILE: IFaceDataMgr.h

	DESCRIPTION:  Face-Data management API

	CREATED BY: Attila Szabo, Discreet

	HISTORY: [attilas|30.8.2000]


 *>	Copyright (c) 1998-2000, All Rights Reserved.
 **********************************************************************/

#ifndef __IFACEDATAMGR__H
#define __IFACEDATAMGR__H

#include "idatachannel.h"
#include "baseinterface.h"

// GUID that identifies this ifc (interface)
#define FACEDATAMGR_INTERFACE Interface_ID(0x1b454148, 0x6a066927)

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Interface for managing face-data channels. 
// Objects that want to have face-data channels should implement this ifc
//
// If this interface needs to b changed, a new one should be derived from
// it and changed (IFaceDataMgr2) and made sure objects that support 
// face-data implement both old and new interfaces
//________________________________________________________________________
class IFaceDataMgr : public BaseInterface
{
	public:
		//
		// Modifiers and procedural objects should call these methods 
		// to add\remove\retrieve a face-data channel on an object (mesh,patch,poly)
		//

		// Returns the number of face-data-channels
		virtual ULONG		NumFaceDataChans( ) const = 0;

		// Retrieves a face-data-channel 
		virtual IFaceDataChannel* GetFaceDataChan( const Class_ID& ID ) const = 0;

		// Adds a face-data-channel to the object. Returns TRUE on success
		virtual BOOL		AddFaceDataChan( IFaceDataChannel* pChan ) = 0;

		// Removes a face-data-channel from the object. Returns TRUE on success
		virtual BOOL		RemoveFaceDataChan( const Class_ID& ID ) = 0;

		//
		// The "system" (Max) should call these methods to manage the
		// face-data channels when the object flows up the stack
		//

		// Appends a face-data-channel to the object. Returns TRUE on success
		virtual BOOL		AppendFaceDataChan( const IFaceDataChannel* pChan ) = 0;

		// Adds or appends face-data channels from the from object, to this object
		// If the channel already exists on this object, it's appended otherwise 
		// gets added
		virtual BOOL		CopyFaceDataChans( const IFaceDataMgr* pFrom ) = 0;

		// Deletes all face-data-channels from this object
		virtual void		RemoveAllFaceDataChans() = 0;

		// Mechanism for executing an operation for all face-data-channels on this object:
		// For all face-data-channels calls IFaceDataEnumCallBack::proc() with 
		// a pointer to that face-data- channel and a context data
		// Returns FALSE if the call back returns FALSE for any of the face-data-channels
		virtual BOOL EnumFaceDataChans( IFaceDataChannelsEnumCallBack& cb, void* pContext ) const = 0; 

		// Allow persistance of info kept in object implementing this interface
		virtual IOResult Save(ISave* isave) = 0;
		virtual IOResult Load(ILoad* iload) = 0;

		// --- from GenericInterface
		virtual Interface_ID	GetID() { return FACEDATAMGR_INTERFACE; }

};

#endif 