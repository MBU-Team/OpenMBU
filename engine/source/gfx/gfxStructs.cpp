//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxDevice.h"

//-----------------------------------------------------------------------------
/*#ifdef TORQUE_DEBUG
GFXPrimitiveBuffer* GFXPrimitiveBuffer::smHead = NULL;
U32 GFXPrimitiveBuffer::smExtantPBCount = 0;

void GFXPrimitiveBuffer::dumpExtantPBs()
{
    if (!smExtantPBCount)
    {
        Con::printf("GFXPrimitiveBuffer::dumpExtantPBs - no extant PBs to dump. You are A-OK!");
        return;
    }

    Con::printf("GFXPrimitiveBuffer Usage Report - %d extant PBs", smExtantPBCount);
    Con::printf("---------------------------------------------------------------");
    Con::printf(" Addr  #idx #prims Profiler Path     RefCount");
    for (GFXPrimitiveBuffer* walk = smHead; walk; walk = walk->mDebugNext)
    {
        Con::printf(" %x  %6d %6d %s %d", walk, walk->mIndexCount, walk->mPrimitiveCount, walk->mDebugCreationPath, walk->mRefCount);
    }
    Con::printf("----- dump complete -------------------------------------------");
}

#endif*/

//-----------------------------------------------------------------------------
// GFXShaderFeatureData
//-----------------------------------------------------------------------------
GFXShaderFeatureData::GFXShaderFeatureData()
{
    dMemset(this, 0, sizeof(GFXShaderFeatureData));
}

//-----------------------------------------------------------------------------
U32 GFXShaderFeatureData::codify()
{
    U32 result = 0;
    U32 multiplier = 1;

    for (U32 i = 0; i < NumFeatures; i++)
    {
        result += features[i] * multiplier;
        multiplier <<= 1;
    }

    return result;
}

//-----------------------------------------------------------------------------
// Set
//-----------------------------------------------------------------------------
//void GFXPrimitiveBufferHandle::set(GFXDevice* theDevice, U32 indexCount, U32 primitiveCount, GFXBufferType bufferType)
//{
//    RefPtr<GFXPrimitiveBuffer>::operator=(theDevice->allocPrimitiveBuffer(indexCount, primitiveCount, bufferType));
//}

//------------------------------------------------------------------------

void GFXDebugMarker::enter()
{
    AssertWarn(!mActive, "Entered already-active debug marker!");
    GFX->enterDebugEvent(mColor, mName);
    mActive = true;
}

void GFXDebugMarker::leave()
{
    AssertWarn(mActive, "Left inactive debug marker!");
    GFX->leaveDebugEvent();
    mActive = false;
}

//-----------------------------------------------------------------------------

GFXVideoMode::GFXVideoMode()
{
    bitDepth = 32;
    fullScreen = false;
    borderless = false;
    refreshRate = 60;
    //wideScreen = false;
    resolution.set(800,600);
    antialiasLevel = 0;
}

void GFXVideoMode::parseFromString( const char *str )
{
    if(!str)
        return;

    // Copy the string, as dStrtok is destructive
    char *tempBuf = new char[dStrlen( str ) + 1];
    dStrcpy( tempBuf, str );

#define PARSE_ELEM(type, var, func, tokParam, sep) \
   if(const char *ptr = dStrtok( tokParam, sep)) \
   { type tmp = func(ptr); if(tmp > 0) var = tmp; }

    PARSE_ELEM(S32, resolution.x, dAtoi, tempBuf, " x\0")
    PARSE_ELEM(S32, resolution.y, dAtoi, NULL,    " x\0")
    PARSE_ELEM(S32, fullScreen,   dAtob, NULL,    " \0")
    PARSE_ELEM(S32, bitDepth,     dAtoi, NULL,    " \0")
    PARSE_ELEM(S32, refreshRate,  dAtoi, NULL,    " \0")
    PARSE_ELEM(S32, antialiasLevel, dAtoi, NULL,    " \0")

#undef PARSE_ELEM

    delete [] tempBuf;
}

const char * GFXVideoMode::toString()
{
    // TODO: Should we include borderless?
    return avar("%d %d %s %d %d %d", resolution.x, resolution.y, (fullScreen ? "true" : "false"), bitDepth,  refreshRate, antialiasLevel);
}