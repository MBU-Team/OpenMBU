//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MACCARBOGLVIDEO_H_
#define _MACCARBOGLVIDEO_H_

#ifndef _PLATFORMVIDEO_H_
#include "Platform/platformVideo.h"
#endif

class OpenGLDevice : public DisplayDevice
{
      bool mCanChangeGamma;
      bool mRestoreGamma;
      U16  mOriginalRamp[256*3];
   
   public:
      OpenGLDevice();

      bool enumDisplayModes(GDHandle hDevice);

      void initDevice();
      bool activate( U32 width, U32 height, U32 bpp, bool fullScreen );

      bool GLCleanupHelper();
      void shutdown();
      void destroy();
      
      bool setScreenMode( U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt = false, bool repaint = true );
      void swapBuffers();
      
      const char* getDriverInfo();
      bool getGammaCorrection(F32 &g);
      bool setGammaCorrection(F32 g);
      bool setVerticalSync( bool on );

      static DisplayDevice* create();
};

#endif // _MACCARBOGLVIDEO_H_
