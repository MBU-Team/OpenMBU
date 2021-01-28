//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mMath.h"

#if defined(TORQUE_SUPPORTS_VC_INLINE_X86_ASM)
static S32 m_mulDivS32_ASM(S32 a, S32 b, S32 c)
{  // a * b / c
   S32 r;
   _asm
   {
      mov   eax, a
      imul  b
      idiv  c
      mov   r, eax
   }
   return r;
}


static U32 m_mulDivU32_ASM(S32 a, S32 b, U32 c)
{  // a * b / c
   S32 r;
   _asm
   {
      mov   eax, a
      mov   edx, 0
      mul   b
      div   c
      mov   r, eax
   }
   return r;
}
#endif

//------------------------------------------------------------------------------
void mInstallLibrary_ASM()
{
#if defined(TORQUE_SUPPORTS_VC_INLINE_X86_ASM)
   m_mulDivS32              = m_mulDivS32_ASM;
   m_mulDivU32              = m_mulDivU32_ASM;
#endif
}


