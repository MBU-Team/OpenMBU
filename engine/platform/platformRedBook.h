//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMREDBOOK_H_
#define _PLATFORMREDBOOK_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

class RedBookDevice
{
   public:
      RedBookDevice();
      virtual ~RedBookDevice();

      bool     mAcquired;
      char *   mDeviceName;

      virtual bool open() = 0;
      virtual bool close() = 0;
      virtual bool play(U32) = 0;
      virtual bool stop() = 0;
      virtual bool getTrackCount(U32 *) = 0;
      virtual bool getVolume(F32 *) = 0;
      virtual bool setVolume(F32) = 0;
};

class RedBook
{
   private:
      static Vector<RedBookDevice *>   smDeviceList;
      static RedBookDevice *           smCurrentDevice;
      static char                      smLastError[];

   public:
      enum {
         PlayFinished = 0,
      };
      static void handleCallback(U32);

      static void init();
      static void destroy();

      static void installDevice(RedBookDevice *);
      static U32 getDeviceCount();
      static const char * getDeviceName(U32);
      static RedBookDevice * getCurrentDevice();

      static void setLastError(const char *);
      static const char * getLastError();

      static bool open(const char *);
      static bool open(RedBookDevice *);
      static bool close();
      static bool play(U32);
      static bool stop();
      static bool getTrackCount(U32 *);
      static bool getVolume(F32 *);
      static bool setVolume(F32);
};

//------------------------------------------------------------------------------

#endif
