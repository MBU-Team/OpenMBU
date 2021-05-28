/**********************************************************************
 *<
	FILE: pbbitmap.h

	DESCRIPTION: BMM Bitmap parameter wrapper for the ParamBlock2 system

	CREATED BY: John Wainwright

	HISTORY: created 4/27/00

 *>	Copyright (c) Autodesk 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __PBBITMAP__
#define __PBBITMAP__

#include <commdlg.h>
#include <vfw.h>
#include "bmmlib.h"

// a wrapper for bitmap/bitmapinfo pairs for holding bitmaps in a ParamBlock
class PBBitmap 
{
public:
	BitmapInfo	bi;
	Bitmap		*bm;
	PB2Export   PBBitmap(BitmapInfo	&bi);
				PBBitmap() { bm = NULL; }
	PB2Export  ~PBBitmap();

	PB2Export void		Load();
	PB2Export PBBitmap*	Clone();
};

#endif


