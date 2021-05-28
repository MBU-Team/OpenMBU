/**********************************************************************
 *<
	FILE: Max.h

	DESCRIPTION: Main include file for MAX plug-ins.

	CREATED BY: Rolf Berteig

	HISTORY: Created 10/01/95

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
       

#ifndef STRICT
#define STRICT
#endif

// System includes
#include <strbasic.h>
#include <windows.h>
#include <windowsx.h>
//#include <ctl3d.h>
#include <commctrl.h>

#include "udmIA64.h"
// WIN64 Cleanup: Shuler
// only contains datatypes to complete the Unified Data Model
// Remove this once MSVC70 is out.

// Some standard library includes
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// Defines which version of MAX to build
#include "buildver.h"

// Defines basic MAX types
#include "maxtypes.h"
#include "trig.h"

// Support libraries
#include "utillib.h"
#include "geomlib.h"
#include "gfxlib.h"
#include "meshlib.h"
#include "patchlib.h"
#include "mnmath.h"

// Core include files
#include "coreexp.h"
#include "winutil.h"
#include "custcont.h"
#include "mouseman.h"
#include "plugin.h"
#include "units.h"
#include "stack.h"   			
#include "interval.h"
#include "hold.h"	
#include "channels.h"
#include "SvCore.h"
#include "animtbl.h"
#include "ref.h"
#include "inode.h"
#include "control.h"
#include "object.h"
#include "objmode.h"
#include "soundobj.h"
#include "iparamb.h"
#include "triobj.h"
#include "patchobj.h"
#include "polyobj.h"
#include "cmdmode.h"
#include "appio.h"
#include "lockid.h"
#include "excllist.h"

// interfaces into MAX executable
#include "maxapi.h"
#include "ioapi.h"
#include "impapi.h"
#include "impexp.h"
#include "iColorMan.h"




