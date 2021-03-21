//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _LIGHTINGSYSTEM_SHADOWBASE_H_
#define _LIGHTINGSYSTEM_SHADOWBASE_H_

#include "platform/platform.h"

class ShadowBase
{
public:
    virtual ~ShadowBase() {}
    virtual bool shouldRender(F32 camDist) = 0;
    virtual void render(F32 camDist) = 0;
    virtual U32 getLastRenderTime() = 0;
};

#endif