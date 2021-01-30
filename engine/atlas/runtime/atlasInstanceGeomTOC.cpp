//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxDevice.h"
#include "core/frameAllocator.h"
#include "platform/profiler.h"

#include "atlas/runtime/atlasInstanceGeomTOC.h"
#include "atlas/runtime/atlasInstance2.h"
#include "atlas/runtime/atlasClipMap.h"
#include "atlas/resource/atlasConfigChunk.h"
#include "atlas/resource/atlasGeomCollision.h"
#include "atlas/runtime/atlasClipMapBatcher.h"

void AtlasInstanceGeomTOC::requestLoad(AtlasInstanceGeomStub* stub, U32 reason, F32 priority)
{
    // Skip load requests for leafs if we've already paged in all the leaves.
    if (mLeafCollisionMode && stub->level == mTreeDepth - 1)
        return;

    Parent::requestLoad(stub, reason, priority);
}

void AtlasInstanceGeomTOC::cancelLoadRequest(AtlasInstanceGeomStub* stub, U32 reason)
{
    // Skip load requests for leafs if we've already paged in all the leaves.
    if (mLeafCollisionMode && stub->level == mTreeDepth - 1)
        return;

    Parent::cancelLoadRequest(stub, reason);
}

bool AtlasInstanceGeomTOC::attemptSplit(StubType* s)
{
    // Try some early outs.

    // Already split.  Also our data & dependents' data is already
    // freshened, so no need to request it again.
    if (s->isSplit())
        return true;

    // Can't ever split.  No data to request.
    if (!s->hasChildren())
        return false;

    // So let's assume we can split it, and go through the possibilities.
    bool splitOK = true;

    // Do children have data? If not, request it.
    for (S32 i = 0; i < 4; i++)
    {
        StubType* c = s->mChildren[i];

        if (!getResourceStub(c)->hasResource())
        {
            requestLoad(c, AttemptSplitChildren, 1.f);
            splitOK = false;
        }
    }

    // Do the ancestor check, as we need to make sure the root is loaded.
    for (StubType* c = s->mParent; c != NULL; c = c->mParent)
    {
        if (!attemptSplit(c))
            splitOK = false;
    }

    // Check neighbors - are they all loaded within the threshold?
    for (S32 i = 0; i < 4; i++)
    {
        StubType* n = s->mNeighbors[i];

        const S32 MAXIMUM_ALLOWED_NEIGHBOR_DIFFERENCE = 2;

        for (S32 i = 0;
            n && i < MAXIMUM_ALLOWED_NEIGHBOR_DIFFERENCE;
            i++)
        {
            n = n->mParent;
        }

        if (n && !attemptSplit(n))
            splitOK = false;
    }

    // We also limit the split by the current morph.
    if (s->mLastMorph > 0.1)
    {
        // If everything else was a go, then accelerate the morph a bit. ;)
        if (splitOK)
            s->mLastMorph -= .1f;

        splitOK = false;
    }

    // And we're done with the attempt, so return our status.
    return splitOK;

}

void AtlasInstanceGeomTOC::doSplit(StubType* s)
{
    if (s->isSplit())
        return;

    if (s->hasChildren())
    {
        // Update children's LOD.
        for (S32 i = 0; i < 4; i++)
        {
            StubType* sChild = s->mChildren[i];
            const U16 lod = computeLod(sChild->mBounds);
            sChild->mLod = mClamp(lod, sChild->mLod & 0xFF00, sChild->mLod | 0xFF);
        }
    }

    // Make sure ancestors are split.
    if (s->mParent && !s->mParent->isSplit())
        doSplit(s->mParent);

    // Note that we ourselves are split, too.
    s->mSplit = true;
}

void AtlasInstanceGeomTOC::warmUpData(StubType* s, F32 priority)
{
    // Request our data to be loaded.
    requestLoad(s, WarmUp, priority);

    // Kill children unless we're likely to need them in the near future.
    if (s->hasChildren())
    {
        if (priority < 0.5)
        {
            // Dump children.
            for (S32 i = 0; i < 4; i++)
                cancelLoadRequest(s->mChildren[i], WarmUp);
        }
        else
        {
            // We're important, so keep children but dump grandchildren.
            for (S32 i = 0; i < 4; i++)
            {
                StubType* sChild = s->mChildren[i];
                if (sChild->hasChildren())
                {
                    for (S32 j = 0; j < 4; j++)
                        cancelLoadRequest(sChild->mChildren[j], WarmUp);
                }
            }
        }
    }
}

void AtlasInstanceGeomTOC::processLOD(SceneState* state)
{
    PROFILE_START(AtlasInstanceGeomTOC_processLOD);

    // Setup the frustrum culler.
    mCuller.init(state);

    // Now, let's walk the quadtree. 

    // We need a stack...
    FrameAllocatorMarker fm;
    StubType** stack = (StubType**)fm.alloc(sizeof(StubType*) * (mTreeDepth * 4 + 1));
    U32 stackDepth = 0;

    // Push the root stub into the stack.
    stack[stackDepth++] = getStub(0, Point2I(0, 0));

    while (stackDepth)
    {
        // Pop top item from the stack.
        StubType* s = stack[--stackDepth];

        // First off, calculate the desired LOD for this dude.
        const U16 desiredLod = computeLod(s->mBounds);

        // Now, do we need to split?
        if (s->hasChildren()
            && desiredLod > (s->mLod | 0xFF)
            && attemptSplit(s))
        {
            // Alright, make sure we're split!
            doSplit(s);

            // And push our kids.
            for (S32 i = 0; i < 4; i++)
                stack[stackDepth++] = s->mChildren[i];
        }
        else
        {
            // Getting here indicates that we're at sufficient LOD. But we
            // have to update our morph & maybe note some load requests.

            // Since the root chunk doesn't have a parent, we have to specially
            // update its LOD so it can morph properly.
            if ((s->mLod & 0xFF00) == 0)
            {
                // Make sure we've got data for the root, otherwise we'll stall.
                if (!getResourceStub(s)->hasResource())
                    requestLoad(s, WarmUp, 1.0f);

                s->mLod = mClamp(desiredLod, s->mLod & 0xFF00, s->mLod | 0x00FF);
                AssertFatal((s->mLod >> 8) == s->level, "invalid LOD/level!");
            }

            // And, let's do some child-updating.
            if (s->hasChildren())
            {
                F32 priority = 0.5;

                if (desiredLod > (s->mLod & 0xFF00))
                    priority = F32(s->mLod & 0xFF) / F32(0xFF);

                if (priority < 0.5f)
                {
                    // Unload our children if we're low priority.
                    for (S32 i = 0; i < 4; i++)
                        cancelLoadRequest(s->mChildren[i], LowPriority);
                }
                else
                {
                    // We're relatively high priority, so get our children
                    // ready to go.
                    for (S32 i = 0; i < 4; i++)
                        warmUpData(s->mChildren[i], priority);
                }
            }
        }
    }

    PROFILE_END();
}

void AtlasInstanceGeomTOC::renderBounds(const S32 debugMode)
{
    PROFILE_START(AtlasInstanceGeomTOC_renderBounds);

    // We need a stack...
    FrameAllocatorMarker fm;
    StackNode* stack = (StackNode*)fm.alloc(sizeof(StackNode) * (mTreeDepth * 4 + 1));
    U32 stackDepth = 0;

    // Push the root stub into the stack.
    stack[stackDepth].stub = getStub(0, Point2I(0, 0));
    stack[stackDepth].clipMask = FrustrumCuller::AllClipPlanes;
    stackDepth++;

    while (stackDepth)
    {
        // Pop top item from the stack.
        const StackNode snode = stack[--stackDepth];
        StubType* s = snode.stub;

        // Do our clipping.
        const S32 newMask = mCuller.testBoxVisibility(s->mBounds, snode.clipMask, 1.f);

        if (newMask == -1)
        {
            // Not seen, so skip this one.
            continue;
        }

        PROFILE_START(AIGTC_drawWireCube);
        Point3F center;
        s->mBounds.getCenter(&center);

        ColorI boxColor;
        AtlasResourceGeomStub* args = getResourceStub(s);

        switch (debugMode)
        {
        case AtlasInstance2::ChunkBoundsDebugHeat:
            boxColor.set(args->mHeat * 100.0, args->mHeat * 10.0, args->mHeat * 1.0);
            break;

        case AtlasInstance2::ChunkBoundsDebugLODThreshold:
            boxColor.set(0, 0, (s->mLod & 0xFF));
            break;

        case AtlasInstance2::ChunkBoundsDebugLODHolistic:
            boxColor.set((s->mLod >> 8) * (256.0 / 8.0), 0, (s->mLod & 0xFF));
            break;

        case AtlasInstance2::ChunkBoundsDebugMorph:
            boxColor.set((s->mLod & 0xFF), s->mLastMorph * 255.0, 0);
            break;

        default:
            AssertISV(false, "AtlasInstanceGeomTOC::renderBounds - unknown debug mode!");
            break;
        }


        GFX->drawWireCube(
            Point3F(s->mBounds.len_x() / 2.0, s->mBounds.len_y() / 2.0, s->mBounds.len_z() / 2.0),
            center, boxColor);
        PROFILE_END();

        // If I'm not maxed out on LOD, don't draw me.
        if (!s->isSplit())
            continue;

        // Recurse.
        if (s->hasChildren())
        {
            for (S32 i = 0; i < 4; i++)
            {
                stack[stackDepth].stub = s->mChildren[i];
                stack[stackDepth].clipMask = newMask;
                stackDepth++;
            }
        }
    }

    PROFILE_END();
}


void AtlasInstanceGeomTOC::renderGeometry(AtlasClipMapBatcher* cmb)
{
    AssertFatal(cmb, "AtlasInstanceGeomTOC::renderGeometry - I need an AtlasClipMapBatcher!");
    PROFILE_START(AtlasInstanceGeomTOC_renderGeometry);

    // We need a stack...
    FrameAllocatorMarker fm;
    StackNode* stack = (StackNode*)fm.alloc(sizeof(StackNode) * (mTreeDepth * 4 + 1));
    U32 stackDepth = 0;

    // Push the root stub into the stack.
    stack[stackDepth].stub = getStub(0, Point2I(0, 0));
    stack[stackDepth].clipMask = FrustrumCuller::AllClipPlanes;
    stackDepth++;

    while (stackDepth)
    {
        // Pop top item from the stack.
        const StackNode snode = stack[--stackDepth];
        StubType* s = snode.stub;

        // Do our clipping.
        const S32 newMask = mCuller.testBoxVisibility(s->mBounds, snode.clipMask, 1.f);

        if (newMask == -1)
        {
            // Not seen, so skip this one.
            continue;
        }

        // Draw ourselves. :)
        if (getResourceStub(s)->mChunk && !s->isSplit())
        {
            PROFILE_START(AIGTC_renderGeometry_GFXcalls);
            // Figure the target morph value...
            const F32 targetMorph = 1.f - (F32(s->mLod & 0xFF) / F32(0xFF));

            // Update mLastMorph to match...
            if (targetMorph > s->mLastMorph)
                s->mLastMorph = targetMorph;
            else
                s->mLastMorph -= getMin(F32(0.05), F32(s->mLastMorph - targetMorph));

            cmb->queue(mCuller.mCamPos, getResourceStub(s), s->mLastMorph);

            PROFILE_END();
        }

        // If I'm not maxed out on LOD, don't draw me.
        if (!s->isSplit())
            continue;

        // Recurse.
        if (s->hasChildren())
        {
            for (S32 i = 0; i < 4; i++)
            {
                stack[stackDepth].stub = s->mChildren[i];
                stack[stackDepth].clipMask = newMask;
                stackDepth++;
            }
        }
    }
    PROFILE_END();
}

void AtlasInstanceGeomTOC::clear(const F32 deltaT)
{
#if 0
    // We need a stack...
    FrameAllocatorMarker fm;
    StubType** stack = (StubType**)fm.alloc(sizeof(StubType*) * (mTreeDepth * 4 + 1));
    U32 stackDepth = 0;

    // Push the root stub into the stack.
    stack[stackDepth++] = getStub(0, Point2I(0, 0));

    while (stackDepth)
    {
        // Pop top item from the stack.
        StubType* s = stack[--stackDepth];

        s->mSplit = false;

        // Recurse.
        if (s->hasChildren())
        {
            for (S32 i = 0; i < 4; i++)
                stack[stackDepth++] = s->mChildren[i];
        }
    }
#else
    // Given we have a big array and have to visit everywhere, let's just
    // run through the whole thing.
    for (S32 i = 0; i < mStubCount; i++)
        mStubs[i].mSplit = false;
#endif
}

//-----------------------------------------------------------------------------

bool AtlasInstanceGeomTOC::castRay(const Point3F& start, const Point3F& end, RayInfo* info, bool emptyCollide)
{
    // For the geom TOC, in order to deal with chunks correctly we have to
    // do a recursive AABB check. This can be optimized.
    // We basically descend to leaf nodes (if you want to support arbitrary 
    // collision nodes, rather than leaf nodes, the change should happen in
    // this and buildCollisionInfo) and have the relevant chunks resolve the query.

    /// Ray status.
    bool retValue = false;
    F32 minT = 2.0;
    Point3F n;
    Point2F tc(0.5, 0.5);

    // We need a stack...
    FrameAllocatorMarker fm;
    StubType** stack = (StubType**)fm.alloc(sizeof(StubType*) * (mTreeDepth * 4 + 1));
    U32 stackDepth = 0;

    // Push the root stub into the stack.
    stack[stackDepth++] = getStub(0, Point2I(0, 0));

    while (stackDepth)
    {
        // Pop top item from the stack.
        StubType* s = stack[--stackDepth];

        // If we don't overlap, skip it. NOTE: collideLine seems to not want to detect
        // fully contained lines.
        if (!s->mBounds.collideLine(start, end))
            continue;

        // Recurse?
        if (s->hasChildren())
        {
            for (S32 i = 0; i < 4; i++)
            {
                // When this goes off it may indicate a BV tree that does not
                // properly contain child nodes (ie, the parent bounds
                // do not contain their children fully). Make sure your importer
                // is ENFORCING this EXPLICITLY.
                AssertFatal(s->mBounds.isContained(s->mChildren[i]->mBounds),
                    "AtlasInstanceGeomTOC::castRay - child not contained by parent!");
                stack[stackDepth++] = s->mChildren[i];
            }

            continue;
        }

        // Got a leaf, and we intersect, so let's pass the collision query
        // onto it.

        // Debug mode?
        if (AtlasInstance2::smRayCollisionDebugLevel == AtlasInstance2::RayCollisionDebugToChunkBox)
        {
            F32 newT;
            Point3F newN;

            // Skip out if we don't hit, otherwise get the t and n
            if (s->mBounds.collideLine(start, end, &newT, &newN))
            {
                if (newT < minT)
                {
                    minT = newT;
                    n = newN;
                }
                retValue = true;
            }
        }

        // No debug, do the actual query.
        AtlasGeomChunkTracer agct(getResourceStub(s)->mChunk);
        RayInfo ri;
        if (agct.castRay(start, end, &ri))
        {
            if (ri.t < minT)
            {
                minT = ri.t;
                n = ri.normal;
                tc = ri.texCoord;
            }
            retValue = true;
        }
    }

    if (retValue)
    {
        info->t = minT;
        info->normal = n;
        info->texCoord = tc;
    }

    return retValue;
}

bool AtlasInstanceGeomTOC::buildCollisionInfo(const Box3F& box, Convex* c,
    AbstractPolyList* poly, AtlasInstance2* object)
{
    // We basically descend to leaf nodes (if you want to support arbitrary 
    // collision nodes, rather than leaf nodes, the change should happen in
    // this and castRay) and have the relevant chunks resolve the query.

    bool retValue = false;

    // We need a stack...
    FrameAllocatorMarker fm;
    StubType** stack = (StubType**)fm.alloc(sizeof(StubType*) * (mTreeDepth * 4 + 1));
    U32 stackDepth = 0;

    // Push the root stub into the stack.
    stack[stackDepth++] = getStub(0, Point2I(0, 0));

    while (stackDepth)
    {
        // Pop top item from the stack.
        StubType* s = stack[--stackDepth];

        // If we don't overlap, skip it.
        if (!box.isOverlapped(s->mBounds))
            continue;

        // Recurse?
        if (s->hasChildren())
        {
            for (S32 i = 0; i < 4; i++)
                stack[stackDepth++] = s->mChildren[i];

            continue;
        }

        // Got a leaf, let's pass control on to it...
        AtlasResourceGeomStub* rs = getResourceStub(s);
        AssertFatal(rs->mChunk, "AtlasResourceGeomTOC::buildCollisionInfo - chunk not present for collision query!");
        if (rs->mChunk)
        {
            retValue |= rs->mChunk->buildCollisionInfo(box, c, poly, object);
        }
        else
        {
            Con::errorf("AtlasResourceGeomTOC::buildCollisionInfo - chunk not present for collision query (%d,%d @ %d)!", s->pos.x, s->pos.y, s->level);
        }
    }

    return retValue;
}

void AtlasInstanceGeomTOC::loadCollisionLeafChunks()
{
    AssertISV(!mLeafCollisionMode, "AtlasInstanceGeomTOC::loadCollisionLeafChunks - already in collision mode!");

    Con::printf("AtlasInstanceGeomTOC::loadCollisionLeafChunks - Loading leaf collision chunks...");

    for (S32 x = 0; x < BIT(mTreeDepth - 1); x++)
    {
        for (S32 y = 0; y < BIT(mTreeDepth - 1); y++)
        {
            AtlasInstanceGeomStub* aigs = getStub(mTreeDepth - 1, Point2I(x, y));
            AtlasResourceGeomStub* args = getResourceStub(aigs);

            if (!args->hasResource())
            {
                // Immediately load and process.
                mResourceTOC->immediateLoad(args, CollisionPreload);

                // Note the request so that we won't unload later.
                requestLoad(aigs, CollisionPreload, 1.0);
            }
        }
    }

    mLeafCollisionMode = true;

    Con::printf("AtlasInstanceGeomTOC::loadCollisionLeafChunks - DONE.");
}