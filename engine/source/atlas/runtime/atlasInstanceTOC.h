//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLASINSTANCETOC_H_
#define _ATLASINSTANCETOC_H_

#include "atlas/core/atlasBaseTOC.h"

template<class ChunkType, class thisT>
class AtlasInstanceStub : public AtlasBaseStub
{
public:
    typedef ChunkType ChunkType;

    AtlasInstanceStub()
    {
        mLod = 0;
        mSplit = false;
        mParent = NULL;

        for (S32 i = 0; i < 4; i++)
            mNeighbors[i] = mChildren[i] = NULL;
    }

    U16 mLod;
    bool mSplit;

    thisT* mParent;
    thisT* mNeighbors[4];
    thisT* mChildren[4];

    inline const bool hasChildren() const
    {
        return mChildren[0];
    }

    inline const bool isSplit() const
    {
        return mSplit;
    }
};

/// Atlas instance TOC base class.
///
/// Provides most functionality for tracking an instance of an Atlas TOC.
///
/// Allows you to bind to an existing AtlasResourceTOC, make load requests,
/// and perform other operations on the instance dataset.
///
/// @ingroup AtlasRuntime
template<class StubType, class ResourceTOCType>
class AtlasInstanceTOC : public AtlasBaseTOC<StubType>
{
protected:
    ResourceTOCType* mResourceTOC;

public:
    typedef StubType StubType;
    typedef ResourceTOCType ResourceTOCType;

    ~AtlasInstanceTOC()
    {
        cancelAllLoadRequests(InstanceShutdown);
    }

    /// Callback to allow subclasses to update stub data when we initialize.
    virtual void updateBounds(StubType* stub, U32 i, S32 x, S32 y) {};

    ResourceTOCType* getResourceTOC()
    {
        return mResourceTOC;
    }

    /// Initialize our TOC.
    virtual void initializeTOC(ResourceTOCType* resourceToc)
    {
        // Initialize the easy stuff.
        mResourceTOC = resourceToc;
        mFile = mResourceTOC->getAtlasFile();
        helpInitializeTOC(mResourceTOC->getTreeDepth());

        // Ok, now we have to stitch up all our connections.
        for (S32 i = mTreeDepth - 1; i >= 0; i--)
        {
            for (S32 x = 0; x < BIT(i); x++)
            {
                for (S32 y = 0; y < BIT(i); y++)
                {
                    StubType* s = getStub(i, Point2I(x, y));

                    // Set LOD.
                    s->mLod = i << 8;

                    // Do the parent pointer.
                    if (i > 0)
                        s->mParent = getStub(i - 1, Point2I(x / 2, y / 2));

                    // Do child pointers, if children there be.
                    if (i < mTreeDepth - 1)
                    {
                        s->mChildren[0] = getStub(i + 1, Point2I(x * 2 + 0, y * 2 + 0));
                        s->mChildren[1] = getStub(i + 1, Point2I(x * 2 + 1, y * 2 + 0));
                        s->mChildren[2] = getStub(i + 1, Point2I(x * 2 + 1, y * 2 + 1));
                        s->mChildren[3] = getStub(i + 1, Point2I(x * 2 + 0, y * 2 + 1));
                    }
                    else
                    {
                        // Terminate the tree plz.
                        for (S32 i = 0; i < 4; i++)
                            s->mChildren[i] = NULL;
                    }

                    // Do neighbor pointers.
                    dMemset(s->mNeighbors, 0, sizeof(s->mNeighbors[0]) * 4);

                    if (x > 0)
                        s->mNeighbors[0] = getStub(i, Point2I(x - 1, y));
                    if (y < BIT(i) - 1)
                        s->mNeighbors[1] = getStub(i, Point2I(x, y + 1));
                    if (x < BIT(i) - 1)
                        s->mNeighbors[2] = getStub(i, Point2I(x + 1, y));
                    if (y > 0)
                        s->mNeighbors[3] = getStub(i, Point2I(x, y - 1));

                    // Potentially update the bounds.
                    updateBounds(s, i, x, y);
                }
            }
        }

        // And we have a stitched up instance tree!
    }

    /// Given a pointer to a stub from this TOC, convert to a stub in the
    /// corresponding resource TOC.
    inline typename ResourceTOCType::StubType* getResourceStub(StubType* stub)
    {
        return mResourceTOC->getStub((stub - mStubs));
    }

    /// Given a pointer to a stub from the resource TOC, convert back to a stub
    /// in this TOC.
    inline StubType* getInstanceStub(typename ResourceTOCType::StubType* stub)
    {
        return mStubs + (stub - mResourceTOC->mStubs);
    }

    virtual void requestLoad(StubType* stub, U32 reason, F32 priority)
    {
        typename ResourceTOCType::StubType* s = getResourceStub(stub);
        s->mRequests.request(this, priority, reason);

        // Notify the resource TOC so it can queue/unqueue it for load.
        mResourceTOC->requestLoad(s, reason, priority);
    }

    virtual void cancelLoadRequest(StubType* stub, U32 reason)
    {
        typename ResourceTOCType::StubType* s = getResourceStub(stub);
        s->mRequests.cancel(this, reason);

        // Notify the resource TOC so it can queue/unqueue it for load.
        mResourceTOC->cancelLoadRequest(s, reason);
    }

    /// Purge all data.
    virtual void cancelAllLoadRequests(U32 reason)
    {
        for (S32 i = 0; i < mStubCount; i++)
            cancelLoadRequest(mStubs + i, reason);
    }

    /// Helper function; returns true iff all the children of this stub are
    /// populated with chunks. If this is a leaf, we return false.
    bool areChildrenLoaded(StubType* stub)
    {
        if (!stub->hasChildren())
            return false;

        for (S32 i = 0; i < 4; i++)
            if (!getResourceStub(stub->mChildren[i])->mChunk)
                return false;

        return true;
    }
};

#endif