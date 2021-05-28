//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#ifndef _X86UNIXOGLVIDEO_H_
#define _X86UNIXOGLVIDEO_H_

#ifndef _PLATFORMVIDEO_H_
#include "platform/platformVideo.h"
#endif

class OpenGLDevice : public DisplayDevice
{
      static bool smCanSwitchBitDepth;

      bool mRestoreGamma;
      U16  mOriginalRamp[256*3];

      void addResolution(S32 width, S32 height, bool check=true);

   public:
      OpenGLDevice();
      virtual ~OpenGLDevice();

      void initDevice();
      bool activate( U32 width, U32 height, U32 bpp, bool fullScreen );
      void shutdown();
      void destroy();
      bool setScreenMode( U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt = false, bool repaint = true );
      void swapBuffers();
      const char* getDriverInfo();
      bool getGammaCorrection(F32 &g);
      bool setGammaCorrection(F32 g);
      bool setVerticalSync( bool on );
      void loadResolutions();

      static DisplayDevice* create();
};

#endif // _H_X86UNIXOGLVIDEO
