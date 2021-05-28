//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"

void* dMemcpy(void* dst, const void* src, dsize_t size)
{
    return memcpy(dst, src, size);
}


//--------------------------------------
void* dMemmove(void* dst, const void* src, dsize_t size)
{
    return memmove(dst, src, size);
}

//--------------------------------------
void* dMemset(void* dst, S32 c, dsize_t size)
{
    return memset(dst, c, size);
}

//--------------------------------------
S32 dMemcmp(const void* ptr1, const void* ptr2, dsize_t len)
{
    return memcmp(ptr1, ptr2, len);
}

#if defined(TORQUE_COMPILER_MINGW)
#include <stdlib.h>
#endif

//--------------------------------------

void* dRealMalloc(dsize_t s)
{
    return malloc(s);
}

void dRealFree(void* p)
{
    free(p);
}

