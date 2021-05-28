 /**********************************************************************
 
	FILE: IMeshFaceDataMgrmpl.h

	DESCRIPTION:  Face-Data management API implementation

	CREATED BY: Attila Szabo, Discreet

	HISTORY: [attilas|30.8.2000]


 *>	Copyright (c) 1998-2000, All Rights Reserved.
 **********************************************************************/

#ifndef __IFACEDATAMGRIMPL__H
#define __IFACEDATAMGRIMPL__H

#include "ifacedatamgr.h"
#pragma warning (disable: 4786)
#include <map>
#include "export.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Face-data management implementation
//________________________________________________________________________
class IFaceDataMgrImpl : public IFaceDataMgr
{
	public:
		typedef std::map<Class_ID, IFaceDataChannel*>	FaceDataChannels;	
		typedef FaceDataChannels::iterator FaceDataChanIt;
		typedef FaceDataChannels::const_iterator FaceDataChanConstIt;

		// --- from IFaceDataMgr
		DllExport virtual ULONG		NumFaceDataChans( ) const;
		DllExport virtual IFaceDataChannel* GetFaceDataChan( const Class_ID& ID ) const;
		DllExport virtual BOOL		AddFaceDataChan( IFaceDataChannel* pChan );
		DllExport virtual BOOL		RemoveFaceDataChan( const Class_ID& ID );

		DllExport virtual BOOL		AppendFaceDataChan( const IFaceDataChannel* pChan );
		DllExport virtual BOOL		CopyFaceDataChans( const IFaceDataMgr* pFrom );
		DllExport virtual void		RemoveAllFaceDataChans();
		DllExport virtual BOOL		EnumFaceDataChans( IFaceDataChannelsEnumCallBack& cb, void* pContext ) const; 

		// Allow persistance of info kept in object implementing this interface
		virtual IOResult Save(ISave* isave) { return IO_OK; };
		virtual IOResult Load(ILoad* iload) { return IO_OK; };

		// --- from GenericInterface
		DllExport virtual BaseInterface*  CloneInterface(void* remapDir = NULL);

		// --- our own methods
		DllExport IFaceDataMgrImpl( );
		DllExport virtual ~IFaceDataMgrImpl( );

		// --- typedefs
		class CopyFaceDataCB : public IFaceDataChannelsEnumCallBack
		{
			public:
				DllExport virtual BOOL Proc( IFaceDataChannel* pChan, void* pContext );
		};
		class DeleteFaceDataSetCB : public IFaceDataChannelsEnumCallBack
		{
			public:
				DllExport virtual BOOL Proc( IFaceDataChannel* pChan, void* pContext );
		};

	protected: 
		FaceDataChannels	fDataChans;
};


#endif 