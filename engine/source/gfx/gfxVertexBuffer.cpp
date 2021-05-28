//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "platform/types.h"
#include "gfxVertexBuffer.h"
#include "gfx/gfxDevice.h"

//-----------------------------------------------------------------------------
// Set
//-----------------------------------------------------------------------------
void GFXVertexBufferHandleBase::set(GFXDevice* theDevice, U32 numVerts, U32 flags, U32 vertexSize, GFXBufferType type)
{
    RefPtr<GFXVertexBuffer>::operator=(theDevice->allocVertexBuffer(numVerts, flags, vertexSize, type));
}


