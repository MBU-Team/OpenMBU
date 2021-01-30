//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASRESOURCETEXTOC_H_
#define _ATLASRESOURCETEXTOC_H_

#include "atlas/core/atlasResourceTOC.h"
#include "atlas/resource/atlasTexChunk.h"

/// Atlas stub subclass for textures.
///
/// @ingroup AtlasResource
class AtlasResourceTexStub : public AtlasResourceStub<AtlasTexChunk>
{
public:
    typedef AtlasResourceStub<AtlasTexChunk> Parent;
    virtual void read(Stream* s, const U32 version)
    {
        Parent::read(s, version);
    }

    virtual void write(Stream* s)
    {
        Parent::write(s);
    }
};

/// Atlas TOC subclass for textures.
///
/// @ingroup AtlasResource
class AtlasResourceTexTOC : public AtlasResourceTOC<AtlasResourceTexStub>
{
public:
    typedef AtlasResourceTOC<AtlasResourceTexStub> Parent;

    void initializeTOC(U32 depth)
    {
        helpInitializeTOC(depth);
    }

    AtlasResourceTexTOC()
    {
        mIsQuadtree = true;
    }

    /// Return the size of texture tiles in pixels (on a side).
    ///
    /// If root chunk is loaded, then grab size from it, otherwise return
    /// an error...
    inline const S32 getTextureChunkSize()
    {
        StubType* s = getStub(0);
        if (s->mChunk)
            if (s->mChunk->bitmap)
                return s->mChunk->bitmap->getWidth();
            else if (s->mChunk->dds)
                return s->mChunk->dds->getWidth();

        // Return -1, we don't know yet!
        return -1;
    }
};

#endif