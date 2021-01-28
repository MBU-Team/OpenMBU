//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FILEOBJECT_H_
#define _FILEOBJECT_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _FILESTREAM_H_
#include "core/fileStream.h"
#endif

class FileObject : public SimObject
{
   typedef SimObject Parent;
   U8 *mFileBuffer;
   U32 mBufferSize;
   U32 mCurPos;
   FileStream stream;
public:
   FileObject();
   ~FileObject();

   bool openForWrite(const char *fileName, const bool append = false);
   bool openForRead(const char *fileName);
   bool readMemory(const char *fileName);
   const U8 *readLine();
   bool isEOF();
   void writeLine(const U8 *line);
   void close();

   DECLARE_CONOBJECT(FileObject);
};

#endif
