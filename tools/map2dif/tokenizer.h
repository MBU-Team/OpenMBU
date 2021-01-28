//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

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

   char   mCurrTokenBuffer[MaxTokenSize + 1];
   bool   mTokenIsCurrent;

  public:
   Tokenizer();
   ~Tokenizer();
   bool openFile(const char* pFileName);

   bool        advanceToken(const bool crossLine, const bool assertAvailable = false);
   void        regressToken();
   bool        tokenAvailable();

   const char* getToken() const;
   bool tokenICmp(const char* pCmp) const;


   const char* getFileName() const     { return mFileName; }
   U32      getCurrentLine() const  { return mCurrLine; }
};


#endif //_TOKENIZER_H_
