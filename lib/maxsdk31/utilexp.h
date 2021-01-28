/**********************************************************************
 *<
	FILE: utilexp.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __UTILEXPORT__H
#define __UTILEXPORT__H

#ifdef BLD_UTIL
#define UtilExport __declspec( dllexport )
#else
#define UtilExport __declspec( dllimport )
#endif

#endif // __UTILEXPORT__H
