//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



#include "platformX86UNIX/platformX86UNIX.h"
#include <stdlib.h>

void* dMemcpy(void *dst, const void *src, dsize_t size)
{
   return memcpy(dst,src,size);
}


//--------------------------------------
void* dMemmove(void *dst, const void *src, dsize_t size)
{
   return memmove(dst,src,size);
}

//--------------------------------------
void* dMemset(void *dst, S32 c, dsize_t size)
{
   return memset(dst,c,size);
}

//--------------------------------------
S32 dMemcmp(const void *ptr1, const void *ptr2, dsize_t len)
{
   return memcmp(ptr1, ptr2, len);
}

#ifdef new
#undef new
#endif

//--------------------------------------
//void* FN_CDECL operator new(dsize_t, void* ptr) noexcept
//{
//   return (ptr);
//}

void* dRealMalloc(dsize_t s)
{
   return malloc(s);
}

void dRealFree(void* p)
{
   free(p);
}

