//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxTarget.h"
#include "console/console.h"

GFXTextureObject *GFXTextureTarget::sDefaultDepthStencil = reinterpret_cast<GFXTextureObject *>( 0x1 );

void GFXTarget::describeSelf( char* buffer, U32 sizeOfBuffer )
{
   // We've got nothing
   buffer[0] = NULL;
}