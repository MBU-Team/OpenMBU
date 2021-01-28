//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _RESIZESTREAM_H_
#define _RESIZESTREAM_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif

class ResizeFilterStream : public FilterStream
{
   typedef FilterStream Parent;

   Stream* m_pStream;
   U32     m_startOffset;
   U32     m_streamLen;
   U32     m_currOffset;

  public:
   ResizeFilterStream();
   ~ResizeFilterStream();

   bool    attachStream(Stream* io_pSlaveStream);
   void    detachStream();
   Stream* getStream();

   bool setStreamOffset(const U32 in_startOffset,
                        const U32 in_streamLen);

   // Mandatory overrides.
  protected:
   bool _read(const U32 in_numBytes,  void* out_pBuffer);
  public:
   U32  getPosition() const;
   bool setPosition(const U32 in_newPosition);

   U32  getStreamSize();
};

#endif //_RESIZESTREAM_H_
