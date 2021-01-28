//-----------------------------------------------------------------------------
// -------------------
// File ....: maxnet.h
// -------------------
// Author...: Gus J Grubba
// Date ....: February 2000
//
// Descr....: 3D Studio MAX Network Interface
//
// History .: Feb, 07 2000 - Started
//            
//-----------------------------------------------------------------------------

#ifndef _MAXNET_H_
#define _MAXNET_H_

#ifdef WIN32
#ifndef MAXNETEXPORT
#define MAXNETEXPORT __declspec( dllimport )
#endif
#else
#define MAXNETEXPORT
#endif

//-----------------------------------------------------------------------------
//-- MaxNet Errors

typedef enum {
	MAXNET_ERR_NONE = 0,
	MAXNET_ERR_CANCEL,
	MAXNET_ERR_NOMEMORY,
	MAXNET_ERR_FILEIO,
	MAXNET_ERR_BADARGUMENT,
	MAXNET_ERR_NOTCONNECTED,
	MAXNET_ERR_NOTREADY,
	MAXNET_ERR_IOERROR,
	MAXNET_ERR_CMDERROR,
	MAXNET_ERR_HOSTNOTFOUND,
	MAXNET_ERR_BADSOCKETVERSION,
	MAXNET_ERR_WOULDBLOCK,
	MAXNET_ERR_SOCKETLIMIT,
	MAXNET_ERR_CONNECTIONREFUSED,
	MAXNET_ERR_ACCESSDENIED,
	MAXNET_ERR_TIMEOUT,
	MAXNET_ERR_BADADDRESS,
	MAXNET_ERR_HOSTUNREACH,
	MAXNET_ERR_UNKNOWN
} maxnet_error_t;

//-----------------------------------------------------------------------------
//-- MaxNet Class (Exception Handler)
//

class MAXNETEXPORT MaxNet {
	protected:
		int	gerror;
		maxnet_error_t	error;
		maxnet_error_t	TranslateError	(int err);
	public:
						MaxNet			();
		maxnet_error_t	GetError		();
		int				GetGError		();
		const TCHAR*	GetErrorText	();
};

#include <maxnet_platform.h>
#include <maxnet_job.h>
#include <maxnet_archive.h>
#include <maxnet_manager.h>
#include <maxnet_low.h>
#include <maxnet_file.h>

//-----------------------------------------------------------------------------
//-- Interface

MAXNETEXPORT MaxNetManager*	CreateManager			( );
MAXNETEXPORT void			DestroyManager			(MaxNetManager* mgr);

//-- Utilities

//-- Initializes a "Job" structure
MAXNETEXPORT bool			jobReadMAXProperties	(char* max_filename, Job* job, CJobText& jobText);
//-- Reads Render Data from a *.max file and fills in a Job structure
MAXNETEXPORT void			jobSetJobDefaults		(Job* job);

MAXNETEXPORT void			NumberedFilename		(TCHAR* infile, TCHAR* outfile, int number);
MAXNETEXPORT bool			IsMacNull				(BYTE *addr);
MAXNETEXPORT bool			GetMacAddress			(BYTE* addr);
MAXNETEXPORT bool			MatchMacAddress			(BYTE* addr1, BYTE* addr2);
MAXNETEXPORT void			Mac2String				(BYTE* addr, TCHAR* string );
MAXNETEXPORT void			Mac2StringCondensed		(BYTE* addr, TCHAR* string );
MAXNETEXPORT void			StringCondensed2Mac		(TCHAR* string, BYTE* addr);
MAXNETEXPORT void			InitConfigurationInfo	(ConfigurationBlock &cb, TCHAR workdisk = 0);
MAXNETEXPORT bool			MatchServers			(HSERVER srv1, HSERVER srv2);
MAXNETEXPORT bool			Maz						(TCHAR* archivename, TCHAR* file_list, DWORD* filesize = 0);
MAXNETEXPORT bool			UnMaz					(TCHAR* archivename, TCHAR* output_path);

//-- Localization Resources for Manager and Server

MAXNETEXPORT TCHAR*			ResString				(int id, TCHAR* buffer = 0);

#endif

//-- EOF: maxnet.h ------------------------------------------------------------
