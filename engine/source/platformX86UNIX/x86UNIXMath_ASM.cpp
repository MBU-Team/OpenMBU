//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mMath.h"

static S32 m_mulDivS32_ASM(S32 a, S32 b, S32 c)
{  // a * b / c
   S32 r;

   __asm__ __volatile__(
      "imul  %2\n"
      "idiv  %3\n"
      : "=a" (r) : "a" (a) , "b" (b) , "c" (c)
      );
   return r;
}


static U32 m_mulDivU32_ASM(S32 a, S32 b, U32 c)
{  // a * b / c
   S32 r;
   __asm__ __volatile__(
      "mov   $0, %%edx\n"
      "mul   %2\n"
      "div   %3\n"
      : "=a" (r) : "a" (a) , "b" (b) , "c" (c)
      );
   return r;
}

//------------------------------------------------------------------------------
void mInstallLibrary_ASM()
{
   m_mulDivS32              = m_mulDivS32_ASM;
   m_mulDivU32              = m_mulDivU32_ASM;
}
