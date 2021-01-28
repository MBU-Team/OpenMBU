//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"

void* dMemcpy(void *dst, const void *src, unsigned size)
{
   return memcpy(dst,src,size);
}


//--------------------------------------
void* dMemmove(void *dst, const void *src, unsigned size)
{
   return memmove(dst,src,size);
}

//--------------------------------------
void* dMemset(void *dst, S32 c, unsigned size)
{
   return memset(dst,c,size);
}

//--------------------------------------
S32 dMemcmp(const void *ptr1, const void *ptr2, unsigned len)
{
   return memcmp(ptr1, ptr2, len);
}

#ifdef new
#undef new
#endif

#if defined(TORQUE_COMPILER_MINGW)
#include <stdlib.h>
#endif

//--------------------------------------
void* FN_CDECL operator new(dsize_t, void* ptr)
{
   return (ptr);
}

void* dRealMalloc(dsize_t s)
{
   return malloc(s);
}

void dRealFree(void* p)
{
   free(p);
}

