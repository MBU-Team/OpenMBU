//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FILTERSTREAM_H_
#define _FILTERSTREAM_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STREAM_H_
#include "core/stream.h"
#endif

class FilterStream : public Stream
{
  public:
   virtual ~FilterStream();

   virtual bool    attachStream(Stream* io_pSlaveStream) = 0;
   virtual void    detachStream()                        = 0;
   virtual Stream* getStream()                           = 0;

   // Mandatory overrides.  By default, these are simply passed to
   //  whatever is returned from getStream();
  protected:
   bool _read(const U32 in_numBytes,  void* out_pBuffer);
   bool _write(const U32 in_numBytes, const void* in_pBuffer);
  public:
   bool hasCapability(const Capability) const;

   U32  getPosition() const;
   bool setPosition(const U32 in_newPosition);
   U32  getStreamSize();
};

#endif //_FILTERSTREAM_H_
