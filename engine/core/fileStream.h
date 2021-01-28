//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FILESTREAM_H_
#define _FILESTREAM_H_

#ifndef _FILEIO_H_
#include "core/fileio.h"
#endif
#ifndef _STREAM_H_
#include "core/stream.h"
#endif

class FileStream : public Stream
{
public:
   enum AccessMode
   {
      Read = File::Read,
      Write = File::Write,
      ReadWrite = File::ReadWrite,
      WriteAppend = File::WriteAppend
   };
   enum
   {
      BUFFER_SIZE = 8 * 1024,         // this can be changed to anything appropriate
      BUFFER_INVALID = 0xffffffff      // file offsets must all be less than this
   };

private:
   File mFile;                         // file being streamed
   U32  mStreamCaps;                   // dependent on access mode
   U8   mBuffer[BUFFER_SIZE];
   U32  mBuffHead;                     // first valid position of buffer (from start-of-file)
   U32  mBuffPos;                      // next read or write will occur here
   U32  mBuffTail;                     // last valid position in buffer (inclusive)
   bool mDirty;                        // whether buffer has been written to
   bool mEOF;                          // whether disk reads have reached the end-of-file

   FileStream(const FileStream &i_fileStrm);             // disable copy constructor
   FileStream& operator=(const FileStream &i_fileStrm);  // disable assignment operator

public:
   FileStream();                       // default constructor
   virtual ~FileStream();              // destructor

   // mandatory methods from Stream base class...
   virtual bool hasCapability(const Capability i_cap) const;

   virtual U32  getPosition() const;
   virtual bool setPosition(const U32 i_newPosition);
   virtual U32  getStreamSize();

   // additional methods needed for a file stream...
   bool open(const char *i_pFilename, AccessMode i_openMode);
   void close();

   bool flush();

protected:
   // more mandatory methods from Stream base class...
   virtual bool _read(const U32 i_numBytes, void *o_pBuffer);
   virtual bool _write(const U32 i_numBytes, const void* i_pBuffer);

   void init();
   bool fillBuffer(const U32 i_startPosition);
   void clearBuffer();
   static void calcBlockHead(const U32 i_position, U32 *o_blockHead);
   static void calcBlockBounds(const U32 i_position, U32 *o_blockHead, U32 *o_blockTail);
   void setStatus();
};

#endif // _FILE_STREAM_H
