//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/stringStack.h"

void StringStack::getArgcArgv(StringTableEntry name, U32 *argc, const char ***in_argv)
{
   U32 startStack = mFrameOffsets[--mNumFrames] + 1;
   U32 argCount   = getMin(mStartStackSize - startStack, (U32)MaxArgs);

   *in_argv = mArgV;
   mArgV[0] = name;
   
   for(U32 i = 0; i < argCount; i++)
      mArgV[i+1] = mBuffer + mStartOffsets[startStack + i];
   argCount++;
   
   mStartStackSize = startStack - 1;
   *argc = argCount;

   mStart = mStartOffsets[mStartStackSize];
   mLen = 0;
}