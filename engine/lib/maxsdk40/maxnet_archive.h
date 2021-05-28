//-----------------------------------------------------------------------------
// ---------------------------
// File ....: maxnet_archive.h
// ---------------------------
// Author...: Gus J Grubba
// Date ....: February 2000
// O.S. ....: Windows 2000
//
// History .: Feb, 15 2000 - Created
//
// 3D Studio Max Network Rendering - Archival (The "*.maz" file)
// 
//-----------------------------------------------------------------------------

#ifndef _MAXNET_ARCHIVE_H_
#define _MAXNET_ARCHIVE_H_

//-----------------------------------------------
//-- Archives

#define NET_ARCHIVE_SIG			0x6612FE10
#define NET_ARCHIVE_EXT			_T(".maz")
#define NET_ARCHIVE_MAX_NAME	128

typedef struct tagNET_ARCHIVE_HEADER {
	DWORD	sig;
	int		count;
	char	reserved[64];
} NET_ARCHIVE_HEADER;

typedef struct tagNET_ARCHIVE_LIST {
	char	name[NET_ARCHIVE_MAX_NAME];
	DWORD	comp;
	DWORD	size,orig_size;
} NET_ARCHIVE_LIST;

#endif

//-- EOF: maxnet_archive.h ----------------------------------------------------

