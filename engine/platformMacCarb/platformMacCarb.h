//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMMACCARB_H_
#define _PLATFORMMACCARB_H_

// NOTE: Placing system headers before Torque's platform.h will work around the Torque-Redefines-New problems.
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
//#include <QD.h>
#include "platform/platform.h"


class MacCarbPlatState
{
public:
   GDHandle      hDisplay;
   WindowPtr     appWindow;
   
   char          appWindowTitle[256];
   bool          quit;
   
   void *        ctx; // was an AGLContext -- but didn't want to inc AGL.h and all...

   S32           desktopBitsPixel;
   S32           desktopWidth;
   S32           desktopHeight;
   U32           currentTime;
   
   U32           osVersion;
   bool          osX;
   
   TSMDocumentID tsmDoc;
   bool        tsmActive;
   
   MacCarbPlatState();
};

extern MacCarbPlatState platState;

extern bool GL_EXT_Init();


extern WindowPtr CreateOpenGLWindow( GDHandle hDevice, U32 width, U32 height, bool fullScreen );
extern WindowPtr CreateCurtain( GDHandle hDevice, U32 width, U32 height );

U32 GetMilliseconds();

U8* str2p(const char *str);
U8* str2p(const char *str, U8 *dst_p);

char* p2str(U8 *p);
char* p2str(U8 *p, char *dst_str);

U8 TranslateOSKeyCode(U8 vcode);

// earlier versions of OSX don't have these convinience macros, so manually stick them here.
#ifndef IntToFixed
#define IntToFixed(a) 	((Fixed)(a) <<16)
#define FixedToInt(a)	((short)(((Fixed)(a) + fixed1/2) >> 16))
#endif

#endif //_PLATFORMMACCARB_H_

