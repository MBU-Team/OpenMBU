//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "atlas/resource/atlasResourceGeomTOC.h"
#include "atlas/resource/atlasGeomCollision.h"
#include "core/frameAllocator.h"
#include "math/mathIO.h"

void AtlasResourceGeomStub::read(Stream* s, const U32 version)
{
    Parent::read(s, version);

    // Will there be a bounding box or just min/max?
    if (version < 101)
    {
        // Nuts, no box.
        F32 tmp;
        s->read(&tmp);
        s->read(&tmp);

        // Upconvert? No, not yet.

        Con::errorf("AtlasResourceGeomStub::read - pre v101 file format, no bounding box data present!");
    }
    else
    {
        // Ok, load that box.
        mathRead(*s, &mBounds);

        // Check sentinel.
        U8 sentinel;
        s->read(&sentinel);

        AssertISV(sentinel == 0xd0, "AtlasResourceGeomStub::read - did not encounter valid sentinel.")
    }
}

void AtlasResourceGeomStub::write(Stream* s)
{
    Parent::write(s);
    mathWrite(*s, mBounds);

    // Add a sentinel
    s->write(U8(0xd0));
}


//-------------------------------------------------------------------------

AtlasResourceGeomTOC::AtlasResourceGeomTOC()
{
    mGoalBatchSize = 1500;
    mDistance_LODMax = 50.f;
    mIsQuadtree = true;
}

AtlasResourceGeomTOC::~AtlasResourceGeomTOC()
{
}

bool AtlasResourceGeomTOC::write(Stream* s)
{
    s->write(mGoalBatchSize);

    return Parent::write(s);
}

bool AtlasResourceGeomTOC::read(Stream* s)
{
    s->read(&mGoalBatchSize);

    return Parent::read(s);
}

void AtlasResourceGeomTOC::instateLoadedChunk(StubType* stub, ChunkType* newChunk)
{
    // Got a new chunk, so let's update the stub's bounding box.
    stub->mBounds = newChunk->getBounds();

    // And pass control on up.
    Parent::instateLoadedChunk(stub, newChunk);
}
