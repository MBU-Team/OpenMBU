//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "atlas/atlasResourceEntry.h"
#include "atlas/atlasResource.h"
#include "console/consoleTypes.h"
#include "platform/profiler.h"

//------------------------------------------------------------------------
// AtlasResourceEntry
//------------------------------------------------------------------------

FreeListChunker<AtlasResourceEntry::LoadRequest> AtlasResourceEntry::smLoadRequestChunker;

AtlasResourceEntry::LoadRequest* AtlasResourceEntry::allocRequest()
{
    return smLoadRequestChunker.alloc();
}

void AtlasResourceEntry::freeRequest(AtlasResourceEntry::LoadRequest* lr)
{
    smLoadRequestChunker.free(lr);
}

void AtlasResourceEntry::incRef(LoadType type)
{
    mRefCount[type]++;
}

void AtlasResourceEntry::decRef(LoadType type, AtlasInstance* who)
{
    AssertFatal(mRefCount[type] > 0, "AtlasResourceEntry::decRef - bottomed out!");

    mRefCount[type]--;

    if (!mRefCount[type])
        doUnload(type);

    // Clean any load requests from this fellow out of our list.
    LoadRequest** walk = &mRequestListHead;

    while (*walk)
    {
        if ((*walk)->mRequester == who && (*walk)->mType == type)
        {
            LoadRequest* cre = *walk;
            *walk = (*walk)->mNext;
            freeRequest(cre);

            return;
        }

        walk = &((*walk)->mNext);
    }
}

void AtlasResourceEntry::read(Stream* s, U32 recursion_count)
{
    AtlasResource* tree = AtlasResource::getCurrentResource();

    // Do some generic initialization.
    mGeom = NULL;

    // Read our sentinel.
    U32 sentinel;
    s->read(&sentinel);

    AssertISV(sentinel == 0xDEADBEEF, "Bad sentinel!");

    // Read our label.
    S32 label;
    s->read(&label);

    AssertISV(label <= tree->mEntryCount,
        avar("AtlasResourceEntry::read - "
            "Encountered out of range chunk label (label=%d, recurse=%d)",
            label, recursion_count)
    );

    AssertFatal(tree->mChunkTable[label] == NULL,
        avar("AtlasResourceEntry::read - "
            "Collision in chunk table (label=%d, recurse=%d)",
            label, recursion_count)
    );

    // Register ourselves in the table.
    tree->mChunkTable[label] = this;

    // Read neighbor links.
    for (S32 i = 0; i < 4; i++)
        s->read(&mNeighbor[i].mLabel);

    // Read our chunk address.
    U8 lvlByte; S16 tmp16;
    s->read(&lvlByte);  mLevel = lvlByte;
    s->read(&tmp16); mPos.x = tmp16;
    s->read(&tmp16); mPos.y = tmp16;

    // And our vertical bounds...
    s->read(&mMin);
    s->read(&mMax);

    // The location of our geometry info...
    U32 o;
    s->read(&o); mDataFilePosition = o;

    // Store the bounding box info.
    const F32 EXTRA_BOX_SIZE = 1e-3f; // this is to make chunks overlap by about a millimeter, to avoid cracks.
    F32 level_factor = (1 << (tree->mTreeDepth - 1 - mLevel));
    F32 chunkSize = level_factor * tree->mBaseChunkSize;

    mBounds.min.x = mPos.x * chunkSize - EXTRA_BOX_SIZE;
    mBounds.min.y = mPos.y * chunkSize - EXTRA_BOX_SIZE;
    mBounds.min.z = mMin * tree->mVerticalScale;

    mBounds.max.x = (mPos.x + 1) * chunkSize + EXTRA_BOX_SIZE;
    mBounds.max.y = (mPos.y + 1) * chunkSize + EXTRA_BOX_SIZE;
    mBounds.max.z = mMax * tree->mVerticalScale;

    // Recurse and read children.
    if (recursion_count)
    {
        // Terminate children if we're a branch.
        for (S32 i = 0; i < 4; i++)
        {
            mChildren[i] = tree->allocChunk(mLod + 0x0100, this);
            mChildren[i]->read(s, recursion_count - 1);
        }
    }
    else
    {
        // Terminate children if we're a leaf.
        for (S32 i = 0; i < 4; i++)
            mChildren[i] = NULL;
    }
}

void AtlasResourceEntry::lookupNeighbors()
{
    AtlasResource* tree = AtlasResource::getCurrentResource();

    // Stitch up the neighbor references.
    for (S32 i = 0; i < 4; i++)
    {
        // Let's be really paranoid and deal with invalid references!
        if (mNeighbor[i].mLabel < -1 || mNeighbor[i].mLabel >= tree->mEntryCount)
        {
            //AssertWarn(false, "AtlasResourceEntry::lookupNeighbors - bad neighbor reference!");
            mNeighbor[i].mLabel = -1;
        }

        if (mNeighbor[i].mLabel == -1)
            mNeighbor[i].mEntry = NULL;
        else
            mNeighbor[i].mEntry = tree->mChunkTable[mNeighbor[i].mLabel];
    }

    // Recurse!
    if (hasChildren())
    {
        for (S32 i = 0; i < 4; i++)
            mChildren[i]->lookupNeighbors();
    }
}

void AtlasResourceEntry::doUnload(LoadType type)
{

    // Assert disabled due to purge() causing force-unloads.
    //   AssertFatal(mRefCount[type] == 0, 
    //      "AtlasResourceEntry::doUnload - warning, have a ref count but are unloading anyway!");

    // Release our geometry and texture info.
    switch (type)
    {
    case LoadTypeGeometry:
        SAFE_DELETE(mGeom);
        break;

    case LoadTypeTexture:
        mTex = NULL;
        break;

    default:
        AssertISV(false, "AtlasResourceEntry::doUnload - unknown type!");
        break;
    }
}

void AtlasResourceEntry::requestLoad(LoadType type, AtlasInstance* who, F32 priority, AtlasResource::LoadReason why)
{
    // Check for a matching request...
    LoadRequest* walk = mRequestListHead;

    while (walk)
    {
        if (walk->mRequester == who && walk->mType == type)
        {
            // If so, update it.
            walk->mPriority = priority;
            walk->mReason = why;
            return;
        }

        walk = walk->mNext;
    }

    // If not, allocate it.
    walk = allocRequest();

    walk->mNext = mRequestListHead;
    walk->mPriority = priority;
    walk->mRequester = who;
    walk->mReason = why;
    walk->mType = type;

    mRequestListHead = walk;
}

F32 AtlasResourceEntry::getLoadPriority(LoadType type)
{
    F32 max = 0.f;

    LoadRequest* walk = mRequestListHead;

    while (walk)
    {
        if (walk->mType == type)
            max = getMax(max, walk->mPriority);

        walk = walk->mNext;
    }

    return max;
}

void AtlasResourceEntry::purge()
{
    doUnload(LoadTypeGeometry);
    doUnload(LoadTypeTexture);
}