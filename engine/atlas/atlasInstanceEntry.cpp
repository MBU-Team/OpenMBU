//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "materials/sceneData.h"
#include "util/frustrumCuller.h"
#include "atlas/atlasInstance.h"
#include "atlas/atlasInstanceEntry.h"

AtlasInstance* AtlasInstanceEntry::smCurrentInstance = NULL;

void AtlasInstanceEntry::setInstance(AtlasInstance* ci)
{
    smCurrentInstance = ci;
}

AtlasInstance* AtlasInstanceEntry::getInstance()
{
    return smCurrentInstance;
}

const bool AtlasInstanceEntry::isSplit() const
{
    // We have to both be split ourselves AND have some geometry. Textures
    // never go past geometry so we don't worry about that.
    return mSplit && mResourceEntry->hasResidentGeometryData();
}

void AtlasInstanceEntry::render(const S32 clipMasks)
{
    AtlasResource* tree = AtlasResource::getCurrentResource();
    AtlasResourceEntry* entry = mResourceEntry;

    const F32 level_factor = (1 << (tree->mTreeDepth - 1 - entry->mLevel));
    const F32 chunkSize = level_factor * tree->mBaseChunkSize;

    const Box3F& worldBounds = entry->mBounds;

    // Determine visibility...
    const S32 newMask = AtlasInstance::smCuller.testBoxVisibility(worldBounds, clipMasks, 0.f);

    if (newMask == -1)
    {
        // Not seen, return!
        return;
    }

    // We need center later.
    Point3F center;
    worldBounds.getCenter(&center);

    // Render our wireframe if appropriate.
    if (AtlasInstance::smShowChunkBounds)
    {
        Point3F extents(worldBounds.len_x(), worldBounds.len_y(), worldBounds.len_z());
        extents *= 0.5;

        // First is for LOD, second is for HEAT.
        //ColorI col(0xFF - (mLod & 0xFF), mLod & 0xFF, mLod >> 8)
        ColorI col(mClampF(mHeat * 255.f, 0, 255.f), mClampF(mHeat * 64.0f, 0, 255.f), mClampF(mHeat * 16.f, 0, 255.f));

        GFX->drawWireCube(extents, center, col);
    }

    // Bind texture if appropriate.
    if (entry->mTex.isValid())
    {
        if (isSplit() == false
            || !entry->mChildren[0]->hasResidentTextureData()
            || !entry->mChildren[1]->hasResidentTextureData()
            || !entry->mChildren[2]->hasResidentTextureData()
            || !entry->mChildren[3]->hasResidentTextureData())
        {
            AtlasResource::smBoundChunkLevel = entry->mLevel;
            AtlasResource::smBoundChunkPos = entry->mPos;

            GFX->setTexture(0, entry->mTex);
        }
    }

    // Render some geometry...
    if (isSplit() && hasChildren())
    {
        // We want to render from front to back so that we can maximize z-buffer
        // rejection... Can speed us up quite a bit.

        // So identify our quadrant and used a fixed render order lookup. Only
        // needs to happen in 2d, so this is VERY fast.
        U8 quadID = 0;
        if (AtlasResource::smCurObjCamPos.y > center.y)
            quadID |= 2;
        if (AtlasResource::smCurObjCamPos.x > center.x)
            quadID |= 1;

        // 01  |  11
        // 1   |   3
        // ----+----
        // 0   |   2
        // 00  |  10

        AssertFatal(quadID < 4, "AtlasResource::Entry::render - bad quadID");

        // This was originally based on science, but it's magical now.
        const static U8 renderLUT[4][4] = {
            /* 00 := */ {0, 1, 2, 3},
            /* 01 := */ {1, 3, 0, 2},
            /* 10 := */ {2, 3, 0, 1},
            /* 11 := */ {3, 1, 2, 0},
        };

        // Render non-textured first, then textured.
        for (S32 i = 0; i < 4; i++)
            if (mChildren[renderLUT[quadID][i]]->mResourceEntry->mTex.isNull())
                mChildren[renderLUT[quadID][i]]->render(newMask);

        for (S32 i = 0; i < 4; i++)
            if (mChildren[renderLUT[quadID][i]]->mResourceEntry->mTex.isValid())
                mChildren[renderLUT[quadID][i]]->render(newMask);
    }
    else
    {
        if (!entry->mGeom)
            return;

        // Set up our vertex shader constants.
        /* uniform float4   texGen       : register(C44),
        uniform float4   scaleOff        : register(C45),
        uniform float4   morphInfo       : register(C46) */
        // TC = (mVert[i].point.x + 1.0) / 2.0, (mVert[i].point.y + 1.0) / 2.0

        // Figure the target morph value...
        const F32 targetMorph = 1.f - (F32(mLod & 0xFF) / F32(0xFF));

        // Update mLastMorph to match...
        if (targetMorph > mLastMorph)
            mLastMorph = targetMorph;
        else
            mLastMorph -= getMin(F32(0.05), F32(mLastMorph - targetMorph));

        // We put our chunk offset as well as morph info in here, to minimize
        // our vertex const sets.
        Point4F scaleOff(
            entry->mPos.x * chunkSize + 0.5 * chunkSize,
            entry->mPos.y * chunkSize + 0.5 * chunkSize,
            0.5 * chunkSize,
            mLastMorph);

        // We have to calculate the texgen by taking the unit square texcoords
        // on the mesh and scaling and translating them based on where we are
        // relative to the chunk that bound its texture.
        //
        // So if we ARE the bound one, then we just say (0,0,1,1), ie, offset of
        // zero and scale of one on both axes.
        //
        // If we're a child then we have to be a bit more clever and compensate
        // for what quadrant/subquadran we're in, as well as scaling our range 
        // down to fit.
        //
        // So if we were in the lower right quadrant and our parent had bound
        // the geometry, then we're going to be scaled by 0.5 and offset by 0.5
        // in both axes.
        //
        // For each level lower than the bound chunk, we scale down by half.

        const U32 relativeLevelShift = BIT(entry->mLevel - AtlasResource::smBoundChunkLevel);
        F32 texChunkScale = 1.f / F32(relativeLevelShift);

        // And for offsets, we have to shift based on our relative size to the 
        // bound chunk and our position relative to him. So we basically convert
        // the bound chunk's position to be in our gridspace, then multiply the
        // position delta by 1.0/texChunkScale.

        Point2I boundLocalPos = AtlasResource::smBoundChunkPos;

        boundLocalPos.x *= relativeLevelShift;
        boundLocalPos.y *= relativeLevelShift;

        const Point2I deltaPos = entry->mPos - boundLocalPos;

        Point4F texGen(
            deltaPos.x * texChunkScale,
            deltaPos.y * texChunkScale,
            texChunkScale, texChunkScale);

        GFX->setVertexShaderConstF(44, &texGen[0], 4);
        GFX->setVertexShaderConstF(45, &scaleOff[0], 4);

        entry->mGeom->render();
    }
}

bool AtlasInstanceEntry::canSplit()
{
    // Already split.  Also our data & dependents' data is already
    // freshened, so no need to request it again.
    if (isSplit())
        return true;

    // Can't ever split.  No data to request.
    if (!hasChildren())
        return false;

    AtlasResourceEntry* entry = mResourceEntry;
    AtlasResource* tree = AtlasResource::getCurrentResource();
    AtlasInstance* instance = AtlasInstanceEntry::getInstance();

    // Ok until proven guilty.
    bool splitOK = true;

    // Make sure children have data.
    for (S32 i = 0; i < 4; i++)
    {
        AtlasInstanceEntry* c = mChildren[i];
        if (!c->mResourceEntry->hasResidentGeometryData())
        {
            tree->requestGeomLoad(c->mResourceEntry, instance,
                1.0f, AtlasResource::CanSplitChildPreload);

            if (!c->mGeometryReference)
            {
                c->mResourceEntry->incRef(AtlasResourceEntry::LoadTypeGeometry);
                c->mGeometryReference = true;
            }

            splitOK = false;
        }
    }

    // Make sure ancestors have data.
    for (AtlasInstanceEntry* c = mParent; c != NULL; c = c->mParent)
    {
        if (!c->canSplit())
            splitOK = false;
    }

    // Make sure neighbors have data at a close-enough level in the tree.
    for (S32 i = 0; i < 4; i++)
    {
        AtlasInstanceEntry* n = mNeighbor[i];

        // allow up to two levels of difference between chunk neighbors.
        const S32 MAXIMUM_ALLOWED_NEIGHBOR_DIFFERENCE = 2;

        for (S32 count = 0;
            n && count < MAXIMUM_ALLOWED_NEIGHBOR_DIFFERENCE;
            count++)
        {
            n = n->mParent;
        }

        if (n && !n->canSplit())
            splitOK = false;
    }

    // We can't split unless we're within 0.1 of being morphed out!
    if (mLastMorph > 0.1)
    {
        //Con::printf("Waiting for morph, %f", mLastMorph);

        // But, we can fudge a bit if we're all the way ready otherwise.
        if (splitOK)
            mLastMorph -= 0.2f;

        splitOK = false;
    }

    // Ok, all done, return results.
    return splitOK;
}

void AtlasInstanceEntry::requestUnloadGeometry(AtlasResource::DeleteReason reason)
{
    AtlasResourceEntry* entry = mResourceEntry;
    AtlasInstance* instance = AtlasInstanceEntry::getInstance();

    if (mGeometryReference)
    {
        // Put descendants in the queue first, so they get
        // unloaded first.
        if (hasChildren())
            for (int i = 0; i < 4; i++)
                mChildren[i]->requestUnloadGeometry(reason);

        // Blast our geometry.
        entry->decRef(AtlasResourceEntry::LoadTypeGeometry, instance);
        mGeometryReference = false;
    }
}

void AtlasInstanceEntry::warmUpData(F32 priority)
{
    AtlasResource* tree = AtlasResource::getCurrentResource();
    AtlasResourceEntry* entry = mResourceEntry;
    AtlasInstance* instance = AtlasInstanceEntry::getInstance();

    // Make sure we have data!
    if (!mGeometryReference)
    {
        tree->requestGeomLoad(entry, instance, priority, AtlasResource::WarmUpPreload);
        entry->incRef(AtlasResourceEntry::LoadTypeGeometry);
        mGeometryReference = true;
    }

    // And blast children, unless we're Important, in which case, kill
    // the grandkids.
    if (hasChildren())
    {
        // Are we unimportant?
        if (priority < 0.5f)
        {
            // Dump our child nodes' data.
            for (S32 i = 0; i < 4; i++)
                mChildren[i]->requestUnloadGeometry(AtlasResource::DeleteWarmupUnimportant);
        }
        else
        {
            // Fairly high priority; leave our children
            // loaded, but dump our grandchildren's data.
            for (int i = 0; i < 4; i++)
            {
                AtlasInstanceEntry* c = mChildren[i];

                if (c->hasChildren())
                {
                    for (S32 j = 0; j < 4; j++)
                        c->mChildren[j]->requestUnloadGeometry(AtlasResource::DeleteWarmupImportant);
                }
            }
        }
    }
}

void AtlasInstanceEntry::clear()
{
    // As long as we're tracking heat we have to do the whole tree. The
    // performance hit of doing the full tree is not big.
    /*   if(isSplit())
    { */

    mSplit = false;

    mHeat = getMax(0.f, getMin(mHeat, 10.f) - .01f);

    if (hasChildren())
        for (S32 i = 0; i < 4; i++)
            mChildren[i]->clear();
    //}
}

void AtlasInstanceEntry::doSplit()
{
    AtlasResource* tree = AtlasResource::getCurrentResource();
    AtlasResourceEntry* entry = mResourceEntry;

    if (!mSplit)
    {
        AssertFatal(canSplit(),
            "AtlasResource::Entry::doSplit() - Can't split!");
        AssertFatal(mGeometryReference,
            "AtlasResource::Entry::doSplit() - No resident data!");

        if (hasChildren())
        {
            // Make sure children have a valid lod value.
            for (S32 i = 0; i < 4; i++)
            {
                AtlasInstanceEntry* c = mChildren[i];
                U16 lod = smCurrentInstance->computeLod(c->mResourceEntry->mBounds, AtlasResource::smCurObjCamPos);
                c->mLod = mClamp(lod, c->mLod & 0xFF00, c->mLod | 0x0FF);
            }
        }

        // make sure ancestors are split...
        for (AtlasInstanceEntry* p = mParent; p && p->isSplit() == false; p = p->mParent)
            p->doSplit();

        mSplit = true;

    }
}

void AtlasInstanceEntry::updateLOD()
{
    AtlasResource* tree = AtlasResource::getCurrentResource();
    AtlasResourceEntry* entry = mResourceEntry;

    // First, do the geometry LOD calculations.
    U16 desiredLOD = smCurrentInstance->computeLod(entry->mBounds, AtlasResource::smCurObjCamPos);

    AssertFatal(mLod >> 8 == entry->mLevel,
        "AtlasResource::Entry::updateLOD - Invalid LOD!");

    // Are we trying to be a split node or not?
    if (hasChildren()
        && desiredLOD > (mLod | 0x0FF)
        && canSplit())
    {
        // Need to split
        doSplit();

        // And update our kids.
        for (S32 i = 0; i < 4; i++)
            mChildren[i]->updateLOD();
    }
    else
    {
        // We're good... this chunk can represent its region within the max error tolerance.
        if ((mLod & 0xFF00) == 0)
        {
            // Root chunk -- make sure we have valid morph value.
            mLod = mClamp(desiredLOD, mLod & 0xFF00, mLod | 0x0FF);
            AssertFatal(mLod >> 8 == entry->mLevel,
                "AtlasResource::Entry::updateLOD - Invalid LOD!");
        }

        // Otherwise, let's unload our kids.
        if (hasChildren())
        {
            F32 priority = 0.5;

            if (desiredLOD > (mLod & 0xFF00))
                priority = F32(mLod & 0xFF) / 255.0f;

            if (priority < 0.5f)
            {
                for (S32 i = 0; i < 4; i++)
                    mChildren[i]->requestUnloadGeometry(AtlasResource::DeleteUpdateLod);
            }
            else
            {
                for (int i = 0; i < 4; i++)
                    mChildren[i]->warmUpData(priority);
            }
        }
    }
}

void AtlasInstanceEntry::updateTexture()
{
    AtlasResource* tree = AtlasResource::getCurrentResource();
    AtlasResourceEntry* entry = mResourceEntry;
    AtlasInstance* instance = AtlasInstanceEntry::getInstance();

    AssertFatal(tree->mTQTFile,
        "ChunkEntry::updateTexture - no TQT file loaded!");

    // Don't go past what we've got texture data for.
    if (entry->mLevel >= tree->mTQTFile->mTreeDepth)
        return;

    S32 desiredTexLevel = instance->computeTextureLod(entry->mBounds, AtlasResource::smCurObjCamPos);

    if (entry->hasResidentTextureData())
    {
        AssertFatal(mParent == NULL || mParent->mResourceEntry->hasResidentTextureData(),
            "AtlasResource::Entry::updateTexture() - Parent is missing texture!");

        // Decide if we should release our texture.
        if (!entry->hasResidentGeometryData()
            || desiredTexLevel < entry->mLevel)
        {
            // Release our texture, and the texture of any
            // descendants.  Really should go into a cache
            // or something, in case we want to revive it
            // soon.
            if (!entry->mGeom)
                requestUnloadTextures(AtlasResource::DeleteTextureNoGeom);
            else if (desiredTexLevel < entry->mLevel)
                requestUnloadTextures(AtlasResource::DeleteTextureLevelTooHigh);
            else
                AssertISV(false, "AtlasInstanceEntry::updateTexture - Bad choice!");
        }
        else
        {
            // Keep status quo for this node, and recurse to children.
            if (hasChildren())
            {
                for (S32 i = 0; i < 4; i++)
                    mChildren[i]->updateTexture();
            }
        }
    }
    else
    {
        // Decide if we should load our texture.
        if (desiredTexLevel >= entry->mLevel
            && entry->hasResidentGeometryData())
        {
            // Yes, we would like to load.
            tree->requestTexLoad(entry, instance, 1.f, AtlasResource::UpdateTextureLoad);

            if (!mTextureReference)
            {
                entry->incRef(AtlasResourceEntry::LoadTypeTexture);
                mTextureReference = true;
            }
        }
        else
        {
            // No need to load anything, or to check children.
        }
    }
}

void AtlasInstanceEntry::requestUnloadTextures(AtlasResource::DeleteReason reason)
{
    AtlasResource* resource = AtlasResource::getCurrentResource();
    AtlasResourceEntry* entry = mResourceEntry;
    AtlasInstance* instance = AtlasInstanceEntry::getInstance();

    // Don't go past what we've got texture data for.
    if (entry->mLevel >= resource->mTQTFile->mTreeDepth)
        return;

    if (entry->hasResidentTextureData())
    {
        // Put descendants in the queue first, so they get
        // unloaded first.
        if (hasChildren())
        {
            for (int i = 0; i < 4; i++)
                mChildren[i]->requestUnloadTextures(reason);
        }

        if (mTextureReference)
        {
            entry->decRef(AtlasResourceEntry::LoadTypeTexture, instance);
            mTextureReference = false;
        }
    }
}

void AtlasInstanceEntry::init(AtlasResourceEntry* cre)
{
    AtlasInstance* instance = AtlasInstanceEntry::getInstance();
    AtlasResource* resource = AtlasResource::getCurrentResource();
    mResourceEntry = cre;

    // Mirror neighbor, parent, child linkages. This involves some slightly
    // scary pointer arithmetic, proceed with caution!

    for (S32 i = 0; i < 4; i++)
    {
        if (cre->mNeighbor[i].mEntry)
            mNeighbor[i] = instance->mChunks + (U32)(cre->mNeighbor[i].mEntry - resource->mChunks);
        else
            mNeighbor[i] = NULL;

        if (cre->mChildren[i])
            mChildren[i] = instance->mChunks + (U32)(cre->mChildren[i] - resource->mChunks);
        else
            mChildren[i] = NULL;
    }

    if (cre->mParent)
        mParent = instance->mChunks + (U32)(cre->mParent - resource->mChunks);
    else
        mParent = NULL;

    // Do some sanity checking...
    for (S32 i = 0; i < 4; i++)
    {
        AssertFatal(mNeighbor[i] != this, "AtlasInstanceEntry::init - self-reference on neighbor!");
        AssertFatal(mChildren[i] != this, "AtlasInstanceEntry::init - self-reference on child!");
    }

    AssertFatal(mParent != this, "AtlasInstanceEntry::init - self-reference on parent!");

    // Ok, now copy over miscellanous other info.
    mLod = cre->mLod;
    mSplit = false;
    mLabel = cre->mLabel;
    mHeat = 0.f;

    mTextureReference = false;
    mGeometryReference = false;
}
