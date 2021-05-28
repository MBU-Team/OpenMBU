//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASRESOURCECONFIGTOC_H_
#define _ATLASRESOURCECONFIGTOC_H_

#include "atlas/core/atlasResourceTOC.h"
#include "atlas/resource/atlasConfigChunk.h"

/// Atlas resource stub subclass for configuration chunks.
///
/// @ingroup AtlasResource
class AtlasResourceConfigStub : public AtlasResourceStub<AtlasConfigChunk>
{
public:

    typedef AtlasResourceStub<AtlasConfigChunk> Parent;

    AtlasResourceConfigStub()
    {
        // Null our string.
        dMemset(mName, 0, sizeof(mName[0]) * MaxConfigChunkNameLength);
    }

    enum
    {
        MaxConfigChunkNameLength = 128,
    };

    U8 mName[MaxConfigChunkNameLength];

    virtual void write(Stream* s)
    {
        Parent::write(s);
        s->write(MaxConfigChunkNameLength, mName);
    }

    virtual void read(Stream* s, const U32 version)
    {
        Parent::read(s, version);
        s->read(MaxConfigChunkNameLength, mName);
    }
};

/// Atlas resource TOC subclass for configuration data.
///
/// We abuse some of the semantics around TOCs for this class; we need to be
/// able to store arbitrary configuration data in the atlas file (for instance,
/// to give texture schema information), but the header area is of fixed size.
///
/// This makes supporting, say, variable numbers of source textures difficult.
///
/// So we use a config TOC to store an arbitrary number of configuration
/// chunks. It's not a quadtree, but most of the resource TOC logic is not
/// predicated on quadtrees anyway, so we should be OK.
///
/// Note that we inherit privately, so that you have to use our config
/// API.
///
/// @ingroup AtlasResource
class AtlasResourceConfigTOC : public AtlasResourceTOC<AtlasResourceConfigStub>
{
    bool mAreConfigsLoaded;

public:
    typedef AtlasResourceTOC<AtlasResourceConfigStub> Parent;

    AtlasResourceConfigTOC()
    {
        mAreConfigsLoaded = false;

        // Make it puke if people try to access this TOC as a quadtree - it's not.
        mIsQuadtree = false;
    }

    /// Since we're dealing with small datasets (ie, config data), we don't want
    /// the normal paging semantics. Therefore, call load() to get all config
    /// data into memory and ready for access.
    ///
    /// This is a convenience method (unless you specify a forced reload), since
    /// the other methods in this class will request data be loaded if it's not
    /// already.
    void load(bool forceReload = false);

    void initialize(U32 maxConfigCount);

    bool getConfig(const char* name, AtlasConfigChunk*& acc);
    void addConfig(const char* name, AtlasConfigChunk* acc);

    AtlasResourceTOC<AtlasResourceConfigStub>* castToResourceTOC()
    {
        return this;
    }

};

#endif
