//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/fileObject.h"

IMPLEMENT_CONOBJECT(FileObject);

bool FileObject::isEOF()
{
   return mCurPos == mBufferSize;
}

FileObject::FileObject()
{
   mFileBuffer = NULL;
   mBufferSize = 0;
   mCurPos = 0;
}

FileObject::~FileObject()
{
   dFree(mFileBuffer);
}

void FileObject::close()
{
   stream.close();
   dFree(mFileBuffer);
   mFileBuffer = NULL;
   mBufferSize = mCurPos = 0;
}

bool FileObject::openForWrite(const char *fileName, const bool append)
{
   char buffer[1024];
   Con::expandScriptFilename( buffer, sizeof( buffer ), fileName );

   close();

   if ( !append )
      return( ResourceManager->openFileForWrite(stream, buffer) );

   // Use the WriteAppend flag so it doesn't clobber the existing file:
   if ( !ResourceManager->openFileForWrite(stream, buffer, File::WriteAppend) )
      return( false );

   stream.setPosition( stream.getStreamSize() );
   return( true );
}

bool FileObject::openForRead(const char* /*fileName*/)
{
   AssertFatal(false, "Error, not yet implemented!");
   return false;
}

bool FileObject::readMemory(const char *fileName)
{
   char buffer[1024];
   Con::expandScriptFilename( buffer, sizeof( buffer ), fileName );

   close();
   Stream *s = ResourceManager->openStream(buffer);
   if(!s)
      return false;
   mBufferSize = ResourceManager->getSize(buffer);
   mFileBuffer = (U8 *) dMalloc(mBufferSize + 1);
   mFileBuffer[mBufferSize] = 0;
   s->read(mBufferSize, mFileBuffer);
   ResourceManager->closeStream(s);
   mCurPos = 0;

   return true;
}

const U8 *FileObject::readLine()
{
   if(!mFileBuffer)
      return (U8 *) "";

   U32 tokPos = mCurPos;

   for(;;)
   {
      if(mCurPos == mBufferSize)
         break;

      if(mFileBuffer[mCurPos] == '\r')
      {
         mFileBuffer[mCurPos++] = 0;
         if(mFileBuffer[mCurPos] == '\n')
            mCurPos++;
         break;
      }

      if(mFileBuffer[mCurPos] == '\n')
      {
         mFileBuffer[mCurPos++] = 0;
         break;
      }

      mCurPos++;
   }

   return mFileBuffer + tokPos;
}

void FileObject::writeLine(const U8 *line)
{
   stream.write(dStrlen((const char *) line), line);
   stream.write(2, "\r\n");
}

ConsoleMethod( FileObject, openForRead, bool, 3, 3, "(string filename)")
{
   return object->readMemory(argv[2]);
}

ConsoleMethod( FileObject, openForWrite, bool, 3, 3, "(string filename)")
{
   return object->openForWrite(argv[2]);
}

ConsoleMethod( FileObject, openForAppend, bool, 3, 3, "(string filename)")
{
   return object->openForWrite(argv[2], true);
}

ConsoleMethod( FileObject, isEOF, bool, 2, 2, "Are we at the end of the file?")
{
   return object->isEOF();
}

ConsoleMethod( FileObject, readLine, const char*, 2, 2, "Read a line from the file.")
{
   return (const char *) object->readLine();
}

ConsoleMethod( FileObject, writeLine, void, 3, 3, "(string text)"
              "Write a line to the file, if it was opened for writing.")
{
   object->writeLine((const U8 *) argv[2]);
}

ConsoleMethod( FileObject, close, void, 2, 2, "Close the file.")
{
   object->close();
}
