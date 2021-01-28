//-----------------------------------------------------------------------------
// -------------------
// File ....: Client.h
// -------------------
// Author...: Gus J Grubba
// Date ....: November 1995
// O.S. ....: Windows NT 3.51
//
// History .: Nov, 18 1995 - Created
//
// 3D Studio Max Network Rendering 
// 
// Client
//
//-----------------------------------------------------------------------------

#ifndef _CLIENTINCLUDE_
#define _CLIENTINCLUDE_

#ifndef  NETCEXPORT
#define  NETCEXPORT __declspec( dllimport )
#endif

#include "Max.h"
#include <maxnet_archive.h>
#include <maxnet_job.h>

//-----------------------------------------------------------------------------
//-- Interface Class
     
class ClientInterface {
	public:
		virtual HINSTANCE	HostInst       		() = 0;
		virtual HWND		HostWnd          	() = 0;
		virtual TCHAR*		GetDir           	(int i) = 0;
		virtual TCHAR*		GetAppDir        	() = 0;
		virtual TCHAR*		GetMaxFileName   	() = 0;
		virtual bool		SaveMaxFile			(TCHAR *file, TCHAR *archivename, bool archive, DWORD *filesize)=0;
};

NETCEXPORT bool clAssignJob(ClientInterface* ci, Job* nj, CJobText* jobtext);


//-----------------------------------------------------------------------------
//-- Helper functions, for submitting jobs with MaxNet
     
#define INETCREATEHELPERS_INTERFACE_ID Interface_ID(0x1d65311, 0x7e6d8b)
class INetCreateHelpers : public FPStaticInterface {
public:
	virtual void NetCreateJob( Job& nj, CJobText& jobText )=0;
	virtual bool NetCreateArchive( Job& nj, TCHAR* archivename )=0;
};

inline INetCreateHelpers* GetINetCreateHelpers()
	{ return (INetCreateHelpers*)GetCOREInterface(INETCREATEHELPERS_INTERFACE_ID);}

#endif

//-- EOF: Client.h ------------------------------------------------------------
