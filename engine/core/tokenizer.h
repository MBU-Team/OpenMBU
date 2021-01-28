//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#include "core/stream.h"

class SizedStream;

class Tokenizer
{
  public:
   enum {
      MaxTokenSize = 1023
   };

  private:
   char   mFileName[1024];

   char*  mpBuffer;
   U32 mBufferSize;

   U32 mCurrPos;
   U32 mCurrLine;
   U32 mPrevPos;
   U32 mStartPos;

   char   mCurrTokenBuffer[MaxTokenSize + 1];
   bool   mTokenIsCurrent;

  public:
   Tokenizer();
   ~Tokenizer();
   bool openFile(const char* pFileName);
   bool openFile(Stream* pStream);

   bool        advanceToken(const bool crossLine, const bool assertAvailable = false);
   bool        regressToken(const bool crossLine);
   bool        tokenAvailable();

   const char* getToken() const;
   bool tokenICmp(const char* pCmp) const;

   bool findToken(const char* pCmp);
   bool findToken(U32 start, const char* pCmp);

   const char* getFileName() const     { return mFileName; }
   U32      getCurrentLine() const  { return mCurrLine; }
   U32      getCurrentPos() const { return mCurrPos; }

   bool     setCurrentPos(U32 pos);

   bool     reset();

   bool     endOfFile();
};


#endif //_TOKENIZER_H_
