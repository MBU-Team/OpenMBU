//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ZIPSUBSTREAM_H_
#define _ZIPSUBSTREAM_H_

//Includes
#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif

struct z_stream_s;

class ZipSubRStream : public FilterStream
{
   typedef FilterStream Parent;
   static const U32 csm_streamCaps;
   static const U32 csm_inputBufferSize;

   Stream* m_pStream;
   U32     m_uncompressedSize;
   U32     m_currentPosition;
   bool     m_EOS;

   z_stream_s*  m_pZipStream;
   U8*          m_pInputBuffer;

   U32          m_originalSlavePosition;

   U32 fillBuffer(const U32 in_attemptSize);

  public:
   ZipSubRStream();
   virtual ~ZipSubRStream();

   // Overrides of NFilterStream
  public:
   bool    attachStream(Stream* io_pSlaveStream);
   void    detachStream();
   Stream* getStream();

   void setUncompressedSize(const U32);

   // Mandatory overrides.  By default, these are simply passed to
   //  whatever is returned from getStream();
  protected:
   bool _read(const U32 in_numBytes,  void* out_pBuffer);
  public:
   bool hasCapability(const Capability) const;

   U32  getPosition() const;
   bool setPosition(const U32 in_newPosition);

   U32  getStreamSize();
};

class ZipSubWStream : public FilterStream
{
   typedef FilterStream Parent;
   static const U32 csm_streamCaps;
   static const U32 csm_bufferSize;

   Stream*      m_pStream;
   z_stream_s*  m_pZipStream;

   U32          m_currPosition;  // Indicates number of _uncompressed_ bytes written

   U8*          m_pOutputBuffer;
   U8*          m_pInputBuffer;

  public:
   ZipSubWStream();
   virtual ~ZipSubWStream();

   // Overrides of NFilterStream
  public:
   bool    attachStream(Stream* io_pSlaveStream);
   void    detachStream();
   Stream* getStream();

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

#endif //_ZIPSUBSTREAM_H_
