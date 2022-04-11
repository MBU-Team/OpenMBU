//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXSHADER_H_
#define _GFXSHADER_H_

#include "platform/types.h"
#include "gfx/gfxResource.h"

//**************************************************************************
// Shader
//**************************************************************************
class GFXShader : public GFXResource
{
public:
    U32 mVertexFlags;
    F32 mPixVersion;

    virtual ~GFXShader() {};

    virtual void process() {};

    // GFXResource interface
    virtual void describeSelf(char* buffer, U32 sizeOfBuffer)
    {
        // We got nothing
        buffer[0] = NULL;
    }
};

#endif // GFXSHADER
