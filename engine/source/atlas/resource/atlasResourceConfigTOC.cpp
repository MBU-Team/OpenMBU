//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "atlas/resource/atlasResourceConfigTOC.h"

void AtlasResourceConfigTOC::initialize(U32 maxConfigCount)
{
    // Allocate a TOC big enough for the max configs.

    // Things here may break since we can't use helpInitializeTOC(), so be
    // aware.

    // This is all we have to do... Hopefully. ;)
    mTreeDepth = 0;
    mStubCount = maxConfigCount;
    mStubs = new StubType[mStubCount];
}

void AtlasResourceConfigTOC::addConfig(const char* name, AtlasConfigChunk* acc)
{
    // Find the first unused stub and associate the chunk with that.
    for (S32 i = 0; i < mStubCount; i++)
    {
        // Unused == null name.
        if (mStubs[i].mName[0] == 0)
        {
            dStrncpy((char*)mStubs[i].mName, name, AtlasResourceConfigStub::MaxConfigChunkNameLength);
            instateNewChunk(mStubs + i, acc, true);
            return;
        }
    }

    Con::errorf("AtlasResourceConfigTOC::addConfig - could not add config due to full TOC!");
}

void AtlasResourceConfigTOC::load(bool forceReload/* =false */)
{
    // Don't reload if we don't have to.
    if (mAreConfigsLoaded && !forceReload)
        return;

    // Issue an immediateLoad on all our >0 length name chunks.
    for (S32 i = 0; i < mStubCount; i++)
        if (mStubs[i].mName[0] != 0)
            immediateLoad(mStubs + i, ConfigTOCLoad);

    mAreConfigsLoaded = true;
}

bool AtlasResourceConfigTOC::getConfig(const char* name, AtlasConfigChunk*& acc)
{
    // Let's just make sure we have data first. ;)
    load();

    // Look for matching stub, and fetch the chunk.
    for (S32 i = 0; i < mStubCount; i++)
    {
        if (!dStricmp(name, (char*)mStubs[i].mName))
        {
            acc = mStubs[i].mChunk;
            return true;
        }
    }

    Con::warnf("AtlasResourceConfigTOC::getConfig - no config chunk named '%s' found!", name);
    return false;
}