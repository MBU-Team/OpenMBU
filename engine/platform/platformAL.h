//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMAL_H_
#define _PLATFORMAL_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#include "al/altypes.h"
#include "al/alctypes.h"
#include "al/eaxtypes.h"

// declare externs of the AL fns here.
#define AL_FUNCTION(fn_return,fn_name,fn_args, fn_value) extern fn_return (FN_CDECL *fn_name)fn_args;
#include "al/al_func.h"
#include "al/alc_func.h"
#include "al/eax_func.h"
#undef AL_FUNCTION


namespace Audio
{

bool OpenALInit();
void OpenALShutdown();

bool OpenALDLLInit();
void OpenALDLLShutdown();

// special alx flags
#define AL_GAIN_LINEAR                  0xFF01

// helpers
F32 DBToLinear(F32 value);
F32 linearToDB(F32 value);

}  // end namespace Audio


#endif  // _H_PLATFORMAL_
