//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLASINSTANCETEXTOC_H_
#define _ATLASINSTANCETEXTOC_H_

#include "atlas/runtime/atlasInstanceTOC.h"
#include "atlas/resource/atlasResourceTexTOC.h"

/// @ingroup AtlasRuntime
class AtlasInstanceTexStub : public AtlasInstanceStub<AtlasTexChunk, AtlasInstanceTexStub>
{
public:
};

/// @ingroup AtlasRuntime
class AtlasInstanceTexTOC : public AtlasInstanceTOC<AtlasInstanceTexStub, AtlasResourceTexTOC>
{
    S32 mTextureChunkSize;

public:

    AtlasInstanceTexTOC()
    {
        mTextureChunkSize = -1;
        mIsQuadtree = true;
    }

    inline const S32 getTextureChunkSize()
    {
        if (mTextureChunkSize == -1)
            mTextureChunkSize = getResourceTOC()->getTextureChunkSize();

        return mTextureChunkSize;
    }
};

#endif