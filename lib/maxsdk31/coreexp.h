/**********************************************************************
 *<
	FILE: coreexp.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __COREEXPORT__H
#define __COREEXPORT__H

#ifdef BLD_CORE
#define CoreExport __declspec( dllexport )
#else
#define CoreExport __declspec( dllimport )
#endif

#endif // __COREEXPORT__H
