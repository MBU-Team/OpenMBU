//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "core/tVector.h"
#include "core/frameAllocator.h"
#include "math/mathIO.h"
#include "util/safeDelete.h"
#include "util/triBoxCheck.h"
#include "atlas/editor/atlasDiscreteMesh.h"
#include "atlas/resource/atlasGeomChunk.h"
#include "atlas/resource/atlasResourceGeomTOC.h"
#include "atlas/resource/atlasGeomCollision.h"
#include "collision/abstractPolyList.h"

#include "atlas/runtime/atlasInstance2.h"

//-----------------------------------------------------------------------------

U32 AtlasGeomChunk::smIndexBytesWritten = 0;
U32 AtlasGeomChunk::smVertexBytesWritten = 0;
U32 AtlasGeomChunk::smCollisionBytesWritten = 0;
U32 AtlasGeomChunk::smTotalBytesWritten = 0;
U32 AtlasGeomChunk::smChunksWritten = 0;

void AtlasGeomChunk::dumpIOStatistics()
{
    Con::printf("AtlasGeomChunk IO Report -------------------------------------------");
    Con::printf("   IO Breakdown:             Total            Avg        Pct");
    Con::printf("        index      bytes =   %8d       %8.1f     %3.2f",
        smIndexBytesWritten, F32(smIndexBytesWritten) / F32(smChunksWritten),
        F32(smIndexBytesWritten) / F32(smTotalBytesWritten) * 100.0);
    Con::printf("        vertex     bytes =   %8d       %8.1f     %3.2f",
        smVertexBytesWritten, F32(smVertexBytesWritten) / F32(smChunksWritten),
        F32(smVertexBytesWritten) / F32(smTotalBytesWritten) * 100.0);
    Con::printf("        collision  bytes =   %8d       %8.1f     %3.2f",
        smCollisionBytesWritten, F32(smCollisionBytesWritten) / F32(smChunksWritten),
        F32(smCollisionBytesWritten) / F32(smTotalBytesWritten) * 100.0);

    U32 remainingBytes = smTotalBytesWritten - (smIndexBytesWritten + smVertexBytesWritten + smCollisionBytesWritten);
    Con::printf("        other      bytes =   %8d       %8.1f     %3.2f",
        remainingBytes, F32(remainingBytes) / F32(smChunksWritten),
        F32(remainingBytes) / F32(smTotalBytesWritten) * 100.0);

    Con::printf("");
    Con::printf("   total bytes written = %8db, chunks written = %8.1d, avg. chunk size = %3.2fb",
        smTotalBytesWritten, smChunksWritten, F32(smTotalBytesWritten) / F32(smChunksWritten));

    // Clear our statistics.
    smIndexBytesWritten = 0;
    smVertexBytesWritten = 0;
    smCollisionBytesWritten = 0;
    smTotalBytesWritten = 0;
    smChunksWritten = 0;
}

ConsoleFunction(AtlasGeomChunk_dumpIOStatistics, void, 1, 1, "() - report IO statistics about Atlas geometry chunks.")
{
    AtlasGeomChunk::dumpIOStatistics();
}

//-----------------------------------------------------------------------------

AtlasGeomChunk::AtlasGeomChunk()
{
    mSkirtSkipCount = mIndexCount = mVertCount = 0;
    mIndex = NULL;
    mVert = NULL;

    mColTreeDepth = 4;
    mColTree = NULL;
    mColGeom = NULL;
    mColIndices = NULL;
    mColIndicesOffsets = NULL;
    mColIndicesBuffer = NULL;
    mColOffsetCount = mColIndexCount = 0;

    // Set a default invalid bounding box.
    mBounds.min.set(1, 1, 1);
    mBounds.max.set(-1, -1, -1);

    mTCBounds.point.set(0, 0);
    mTCBounds.extent.set(0, 0);
}

AtlasGeomChunk::~AtlasGeomChunk()
{
    SAFE_DELETE_ARRAY(mIndex);
    SAFE_DELETE_ARRAY(mVert);
    SAFE_DELETE_ARRAY(mColTree);
    SAFE_DELETE_ARRAY(mColIndicesOffsets);
    SAFE_DELETE_ARRAY(mColIndicesBuffer);
}

U32 AtlasGeomChunk::getHeadSentinel()
{
    return MAKEFOURCC('a', 'g', 'c', 'h');
}

U32 AtlasGeomChunk::getTailSentinel()
{
    return MAKEFOURCC('a', 'g', 'c', 't');
}

void AtlasGeomChunk::generate(AtlasChunk* children[4])
{
    Vector<AtlasDiscreteMesh*> meshes;

    const F32 texOffset[4][2] =
    {
       { 0, 0},
       { 0, 1},
       { 1, 1},
       { 1, 0},
    };

    // Grab all the meshes.
    for (S32 i = 0; i < 4; i++)
    {
        if (children[i])
        {
            meshes.push_back(((AtlasGeomChunk*)children[i])->copyToDiscreteMesh());
            meshes.last()->transformTexCoords(Point2F(0.5, texOffset[i][0] * 0.5), Point2F(0.5, texOffset[i][1] * 0.5));
            /*Box3F b = meshes.last()->calcBounds();

            Con::printf("       - mesh bounds = (%f, %f, %f), (%f, %f, %f)",
               b.min.x, b.min.y, b.min.z,
               b.max.x, b.max.y, b.max.z); */
        }
    }

    AtlasDiscreteMesh bigAdm;
    bigAdm.combine(meshes);

    // Clean up meshes...
    while (meshes.size())
    {
        delete meshes.last();
        meshes.pop_back();
    }

    // Weld & decimate.
    bigAdm.weld(0.01, F32_MAX);
    AtlasDiscreteMesh* smallAdm = bigAdm.decimate(((AtlasResourceGeomTOC*)mOwningTOC)->mGoalBatchSize);

    // And copy it into ourselves.
    copyFromDiscreteMesh(smallAdm);

    Box3F b = smallAdm->calcBounds();

    // Make sure that our new bounds include all the child bounds. This will
    // help prevent culling/LOD strangeness.
    for (S32 i = 0; i < 4; i++)
    {
        Box3F childBounds = ((AtlasGeomChunk*)children[i])->getBounds();

        AssertFatal(childBounds.min.x >= -100.f, "AtlasGeomChunk::generate - bad child bounds! (1)");
        AssertFatal(childBounds.min.y >= -100.f, "AtlasGeomChunk::generate - bad child bounds! (2)");
        AssertFatal(childBounds.min.z >= -100.f, "AtlasGeomChunk::generate - bad child bounds! (3)");

        b.min.setMin(childBounds.min);
        b.max.setMax(childBounds.max);

        AssertFatal(b.min.x >= -100.f, "AtlasGeomChunk::generate - bad parent bounds! (1)");
        AssertFatal(b.min.y >= -100.f, "AtlasGeomChunk::generate - bad parent bounds! (2)");
        AssertFatal(b.min.z >= -100.f, "AtlasGeomChunk::generate - bad parent bounds! (3)");
    }

    // Update our bounds.
    mBounds = b;

    /*Con::printf("       - FINAL mesh bounds = (%f, %f, %f), (%f, %f, %f)",
       mBounds.min.x, mBounds.min.y, mBounds.min.z,
       mBounds.max.x, mBounds.max.y, mBounds.max.z); */

       // Clean up.
    delete smallAdm;

    // All done!
}

void AtlasGeomChunk::copyFromDiscreteMesh(AtlasDiscreteMesh* adm)
{
    // Assume we've got no data.
    AssertISV(adm, "AtlasGeomChunk::copyFromDiscreteMesh - must pass a mesh to copy!");
    AssertISV(mIndexCount == 0, "AtlasGeomChunk::copyFromDiscreteMesh - already have index count!");
    AssertISV(mIndex == 0, "AtlasGeomChunk::copyFromDiscreteMesh - already have index data!");
    AssertISV(mVertCount == 0, "AtlasGeomChunk::copyFromDiscreteMesh - already have vertex count!");
    AssertISV(mVert == 0, "AtlasGeomChunk::copyFromDiscreteMesh - already have vertex data!");

    // Now, we copy the indices directly...
    mIndexCount = adm->mIndexCount;
    mIndex = new U16[adm->mIndexCount];
    dMemcpy(mIndex, adm->mIndex, sizeof(U16) * adm->mIndexCount);

    // And swizzle the vertex data.
    mVertCount = adm->mVertexCount;
    mVert = new GFXAtlasVert2[mVertCount];

    for (S32 i = 0; i < mVertCount; i++)
    {
        GFXAtlasVert2& v = mVert[i];

        v.point = adm->mPos[i];
        v.texCoord = adm->mTex[i];
        v.normal = adm->mNormal[i];

        if (adm->mHasMorphData)
        {
            v.texMorphOffset = adm->mTexMorphOffset[i];
            v.pointMorphOffset = adm->mPosMorphOffset[i];
        }
        else
        {
            v.texMorphOffset.set(0, 0);
            v.pointMorphOffset.set(0, 0, 0);
        }
    }

    // Update our bounds.
    mBounds = adm->calcBounds();

    // And our collision, if we had it to begin with.
    if (mColTree)
        generateCollision();

    // All done!
}

AtlasDiscreteMesh* AtlasGeomChunk::copyToDiscreteMesh()
{
    AtlasDiscreteMesh* adm = new AtlasDiscreteMesh;

    // Copy indices.
    adm->mIndexCount = mIndexCount;
    adm->mIndex = new U16[mIndexCount];
    dMemcpy(adm->mIndex, mIndex, sizeof(U16) * mIndexCount);

    // Prep vertices...
    adm->mVertexCount = mVertCount;
    adm->mPos = new Point3F[mVertCount];
    adm->mNormal = new Point3F[mVertCount];
    adm->mTex = new Point2F[mVertCount];
    adm->mHasMorphData = false;

    // And swizzle vertices out...
    for (S32 i = 0; i < mVertCount; i++)
    {
        adm->mPos[i] = mVert[i].point;
        adm->mTex[i] = mVert[i].texCoord;
        adm->mNormal[i] = mVert[i].normal;
    }

    // All done.
    return adm;
}

void AtlasGeomChunk::write(Stream* s)
{
    U32 startPos = s->getPosition();

    Parent::write(s);

    // Serialize our bounds.
    mathWrite(*s, mBounds);

    U32 idxStartPos = s->getPosition();

    // Write our indices.
    s->write(mIndexCount);
    s->write(mSkirtSkipCount);
    for (S32 i = 0; i < mIndexCount; i++)
        s->write(mIndex[i]);

    U32 idxEndPos = s->getPosition();

    // Naive vertex write.
    s->write(mVertCount);
    for (S32 i = 0; i < mVertCount; i++)
    {
        const GFXAtlasVert2& v = mVert[i];

        s->write(v.point.x);
        s->write(v.point.y);
        s->write(v.point.z);

        s->write(v.texCoord.x);
        s->write(v.texCoord.y);

        s->write(v.texMorphOffset.x);
        s->write(v.texMorphOffset.y);

        s->write(v.pointMorphOffset.x);
        s->write(v.pointMorphOffset.y);
        s->write(v.pointMorphOffset.z);

        s->write(v.normal.x);
        s->write(v.normal.y);
        s->write(v.normal.z);
    }

    U32 vertEndPos = s->getPosition();

    // Add a note indicating whether we're going to have any collision.

    // Now, if needed, write some collision info.
    if (mColTree)
    {
        s->write(U8(0xDF));

        // Write the collision tree.
        s->write(mColTreeDepth);
        const U32 nodeCount = AtlasTOC::getNodeCount(mColTreeDepth);
        for (U32 i = 0; i < nodeCount; i++)
            mColTree[i].write(s);

        // And the indices/offset buffers.
        s->write(mColOffsetCount);
        for (U32 i = 0; i < mColOffsetCount; i++)
            s->write(mColIndicesOffsets[i]);

        s->write(mColIndexCount);
        for (U32 i = 0; i < mColIndexCount; i++)
            s->write(mColIndicesBuffer[i]);
    }
    else
    {
        s->write(U8(0x0F));
    }

    // Write a post-collision sentinel.
    s->write(U32(0xcefedeef));

    U32 colEndPos = s->getPosition();

    // Ok, we're all done. While we're here, though, update our IO statistics.
    smCollisionBytesWritten += colEndPos - vertEndPos;
    smTotalBytesWritten += colEndPos;
    smVertexBytesWritten += vertEndPos - idxEndPos;
    smIndexBytesWritten += idxEndPos - idxStartPos;
    smChunksWritten++;
}

void AtlasGeomChunk::read(Stream* s)
{
    Parent::read(s);

    if (((AtlasResourceGeomTOC*)mOwningTOC)->getFormatVersion() < 120)
    {

        // Also, a naive read.
        s->read(&mIndexCount);
        s->read(&mSkirtSkipCount);

        mIndex = new U16[mIndexCount];
        for (S32 i = 0; i < mIndexCount; i++)
            s->read(&mIndex[i]);

        s->read(&mVertCount);

        mVert = new GFXAtlasVert2[mVertCount];
        for (S32 i = 0; i < mVertCount; i++)
        {
            GFXAtlasVert2& v = mVert[i];

            s->read(&v.point.x);
            s->read(&v.point.y);
            s->read(&v.point.z);

            s->read(&v.texCoord.x);
            s->read(&v.texCoord.y);

            s->read(&v.pointMorphOffset.x);
            s->read(&v.pointMorphOffset.y);
            s->read(&v.pointMorphOffset.z);

            s->read(&v.texMorphOffset.x);
            s->read(&v.texMorphOffset.y);

            v.normal.set(0, 0, 1);
        }

        if (((AtlasResourceGeomTOC*)mOwningTOC)->getFormatVersion() > 100)
        {
            mathRead(*s, &mBounds);
        }
        else
        {
            // Calculate bounds based on the actual values here. This is gross but
            // not really on the critical path, so who cares... right? ;)
            AtlasDiscreteMesh* adm = copyToDiscreteMesh();
            mBounds = adm->calcBounds();
            delete adm;
        }

        return;
    }

    // New v.120 read code.

    // Read our bounds.
    mathRead(*s, &mBounds);

    // Read our indices.
    s->read(&mIndexCount);
    s->read(&mSkirtSkipCount);

    mIndex = new U16[mIndexCount];
    for (S32 i = 0; i < mIndexCount; i++)
        s->read(mIndex + i);

    Point2F minTC, maxTC;

    // And our verts.
    s->read(&mVertCount);
    mVert = new GFXAtlasVert2[mVertCount];

    if (((AtlasResourceGeomTOC*)mOwningTOC)->getFormatVersion() < 130)
    {
        // Deal with odd-ordered vertex data.
        for (S32 i = 0; i < mVertCount; i++)
        {
            GFXAtlasVert2& v = mVert[i];

            s->read(&v.point.x);
            s->read(&v.point.y);
            s->read(&v.point.z);

            s->read(&v.texCoord.x);
            s->read(&v.texCoord.y);

            s->read(&v.pointMorphOffset.x);
            s->read(&v.pointMorphOffset.y);
            s->read(&v.pointMorphOffset.z);

            s->read(&v.texMorphOffset.x);
            s->read(&v.texMorphOffset.y);

            v.normal.set(0, 0, 1);

            // Set up initial values for TC bounds if needed.
            if (i == 0)
                minTC = maxTC = v.texCoord;

            // Ok, update min/max.
            minTC.setMin(v.texCoord);
            minTC.setMin(v.texCoord + v.texMorphOffset);
            maxTC.setMax(v.texCoord);
            maxTC.setMax(v.texCoord + v.texMorphOffset);
        }
    }
    else if (((AtlasResourceGeomTOC*)mOwningTOC)->getFormatVersion() < 140)
    {
        // We can directly load everything.
        for (S32 i = 0; i < mVertCount; i++)
        {
            GFXAtlasVert2& v = mVert[i];

            s->read(&v.point.x);
            s->read(&v.point.y);
            s->read(&v.point.z);

            s->read(&v.texCoord.x);
            s->read(&v.texCoord.y);

            s->read(&v.texMorphOffset.x);
            s->read(&v.texMorphOffset.y);

            s->read(&v.pointMorphOffset.x);
            s->read(&v.pointMorphOffset.y);
            s->read(&v.pointMorphOffset.z);

            v.normal.set(0, 0, 1);

            // Set up initial values for TC bounds if needed.
            if (i == 0)
                minTC = maxTC = v.texCoord;

            // Ok, update min/max.
            minTC.setMin(v.texCoord);
            minTC.setMin(v.texCoord + v.texMorphOffset);
            maxTC.setMax(v.texCoord);
            maxTC.setMax(v.texCoord + v.texMorphOffset);
        }
    }
    else
    {
        // We can directly load everything.
        for (S32 i = 0; i < mVertCount; i++)
        {
            GFXAtlasVert2& v = mVert[i];

            s->read(&v.point.x);
            s->read(&v.point.y);
            s->read(&v.point.z);

            s->read(&v.texCoord.x);
            s->read(&v.texCoord.y);

            s->read(&v.texMorphOffset.x);
            s->read(&v.texMorphOffset.y);

            s->read(&v.pointMorphOffset.x);
            s->read(&v.pointMorphOffset.y);
            s->read(&v.pointMorphOffset.z);

            s->read(&v.normal.x);
            s->read(&v.normal.y);
            s->read(&v.normal.z);

            // Set up initial values for TC bounds if needed.
            if (i == 0)
                minTC = maxTC = v.texCoord;

            // Ok, update min/max.
            minTC.setMin(v.texCoord);
            minTC.setMin(v.texCoord + v.texMorphOffset);
            maxTC.setMax(v.texCoord);
            maxTC.setMax(v.texCoord + v.texMorphOffset);
        }
    }

    // Store the TC bounds.
    mTCBounds.point = minTC;
    mTCBounds.extent = maxTC - minTC;

    // Will we have collision?
    U8 colFlag = 0;
    s->read(&colFlag);

    AssertFatal(colFlag == U8(0xDF) || colFlag == U8(0x0F), "AtlasGeomChunk::read - invalid colFlag!");

    // If there is collision data, read it.
    if (colFlag == 0xDF)
    {
        // The collision tree.
        s->read(&mColTreeDepth);
        const U32 nodeCount = AtlasTOC::getNodeCount(mColTreeDepth);
        mColTree = new ColNode[nodeCount];
        for (U32 i = 0; i < nodeCount; i++)
            mColTree[i].read(s);

        // And the indices/offset buffers.
        s->read(&mColOffsetCount);
        mColIndicesOffsets = new U16[mColOffsetCount];
        for (U32 i = 0; i < mColOffsetCount; i++)
            s->read(mColIndicesOffsets + i);

        s->read(&mColIndexCount);
        mColIndicesBuffer = new U16[mColIndexCount];
        for (U32 i = 0; i < mColIndexCount; i++)
            s->read(mColIndicesBuffer + i);
    }
    else if (colFlag != 0x0f)
    {
        Con::errorf("AtlasGeomChunk::read - unknown collision flag!");
    }

    U32 postColSentinel = 0;
    s->read(&postColSentinel);
    AssertISV(postColSentinel == 0xcefedeef, "AtlasGeomChunk::read - invalid post collision sentinel!");
}

AtlasGeomChunk* AtlasGeomChunk::generateCopy(S32 reformat)
{
    AtlasGeomChunk* agc = new AtlasGeomChunk();

    agc->mIndexCount = mIndexCount;
    agc->mIndex = new U16[mIndexCount];
    dMemcpy(agc->mIndex, mIndex, sizeof(U16) * mIndexCount);

    agc->mVertCount = mVertCount;
    agc->mVert = new GFXAtlasVert2[mVertCount];
    dMemcpy(agc->mVert, mVert, sizeof(GFXAtlasVert2) * mVertCount);

    agc->mSkirtSkipCount = mSkirtSkipCount;
    agc->mBounds = mBounds;

    // Copy our collision info if any...
    if (mColTree)
    {
        // The collision tree.
        agc->mColTreeDepth = mColTreeDepth;
        const U32 nodeCount = AtlasTOC::getNodeCount(mColTreeDepth);
        agc->mColTree = new ColNode[nodeCount];
        dMemcpy(agc->mColTree, mColTree, sizeof(ColNode) * nodeCount);

        // And the indices/offset buffers.
        agc->mColOffsetCount = mColOffsetCount;
        agc->mColIndicesOffsets = new U16[mColOffsetCount];
        dMemcpy(agc->mColIndicesOffsets, mColIndicesOffsets, sizeof(U16) * mColOffsetCount);

        agc->mColIndexCount = mColIndexCount;
        agc->mColIndicesBuffer = new U16[mColIndexCount];
        dMemcpy(agc->mColIndicesBuffer, mColIndicesBuffer, sizeof(U16) * mColIndexCount);
    }

    return agc;
}

//-----------------------------------------------------------------------------

void AtlasGeomChunk::process()
{
    // Prepare our buffers for rendering.
    if (!GFXDevice::devicePresent())
        return;

    // Geometry...
    mVB.set(GFX, mVertCount, GFXBufferTypeStatic);
    GFXAtlasVert2* v = mVB.lock();
    dMemcpy(v, mVert, sizeof(GFXAtlasVert2) * mVertCount);
    mVB.unlock();

    // Indices...
    mPB.set(GFX, mIndexCount, 0, GFXBufferTypeStatic);
    U16* idx;
    mPB.lock(&idx);
    dMemcpy(idx, mIndex, sizeof(U16) * mIndexCount);
    mPB.unlock();

    // All done!
}

void AtlasGeomChunk::render()
{
    GFX->setVertexBuffer(mVB);
    GFX->setPrimitiveBuffer(mPB);
    GFX->drawIndexedPrimitive(GFXTriangleList, 0, mVertCount, 0, mIndexCount / 3);
}

Box3F AtlasGeomChunk::getBounds()
{
    return mBounds;
}

void AtlasGeomChunk::generateCollision()
{
    // Clean everything up, in case this is a regen situation.
    SAFE_DELETE_ARRAY(mColTree);
    SAFE_DELETE_ARRAY(mColIndicesBuffer);
    SAFE_DELETE_ARRAY(mColIndicesOffsets);

    // We basically generate a hierarchy of bounding volumes and generate a list
    // for each bin of what triangles intersect it.

    // First, do the binning. This is a bit gross but, hey, what can you do...
    const U32 gridSize = BIT(mColTreeDepth - 1);
    const U32 gridCount = gridSize * gridSize;

    Vector<U16>* bins = new Vector<U16>[gridCount];

    // We currently do not compress this stuff.

    // Track the min/max for the bins.
    Vector<F32> binsMax(gridCount); binsMax.setSize(gridCount);
    Vector<F32> binsMin(gridCount); binsMin.setSize(gridCount);

    // Clear bins.
    for (S32 i = 0; i < gridCount; i++)
    {
        binsMax[i] = mBounds.min.z - 1.f;
        binsMin[i] = mBounds.max.z + 1.f;
    }

    // Get the size of bins (we step in x/y, not in Z).
    Point3F binSize(mBounds.len_x() / F32(gridSize), mBounds.len_y() / F32(gridSize), mBounds.len_z());

    for (S32 i = 0; i < gridSize; i++)
    {
        for (S32 j = 0; j < gridSize; j++)
        {
            //Con::printf("--------- bin (%d, %d) ---------", i, j);

            // Figure the bounds for this bin...
            Box3F binBox;

            binBox.min.x = mBounds.min.x + binSize.x * i;
            binBox.min.y = mBounds.min.y + binSize.y * j;
            binBox.min.z = mBounds.min.z - 1.f;

            binBox.max.x = mBounds.min.x + binSize.x * (i + 1);
            binBox.max.y = mBounds.min.y + binSize.y * (j + 1);
            binBox.max.z = mBounds.max.z + 1.f;

            Vector<U16>& binList = bins[i * gridSize + j];

            F32& binMin = binsMin[i * gridSize + j];
            F32& binMax = binsMax[i * gridSize + j];

            // Now, consider all the triangles in the mesh. Note: we assume a trilist.
            for (S32 v = 0; v < (mIndexCount - mSkirtSkipCount) - 1; v += 3)
            {
                // Reject anything degenerate...
                if (mIndex[v + 0] == mIndex[v + 1] || mIndex[v + 1] == mIndex[v + 2] || mIndex[v + 2] == mIndex[v + 0])
                    continue;

                // Get the verts.
                const GFXAtlasVert2& a = mVert[mIndex[v + 0]];
                const GFXAtlasVert2& b = mVert[mIndex[v + 1]];
                const GFXAtlasVert2& c = mVert[mIndex[v + 2]];


                // Otherwise, we're good, so consider it for the current bin.
                const Point3F& aPos = a.point;
                const Point3F& bPos = b.point;
                const Point3F& cPos = c.point;

                if (triBoxOverlap(binBox, aPos, bPos, cPos))
                {
                    // Spam the console.
                    /*Con::printf("   tri %d; box = (%f,%f,%f) (%f,%f,%f)",
                                   v,
                                   binBox.min.x, binBox.min.y, binBox.min.z,
                                   binBox.max.x, binBox.max.y, binBox.max.z);

                    Con::printf("          a = (%f,%f,%f)", aPos.x, aPos.y, aPos.z);
                    Con::printf("          b = (%f,%f,%f)", bPos.x, bPos.y, bPos.z);
                    Con::printf("          c = (%f,%f,%f)", cPos.x, cPos.y, cPos.z);  */

                    // Got a hit, add it to the list!
                    binList.push_back(v);

                    // Update the Z min/max info. This will be TOO BIG if we have a
                    // very large triangle! An optimal implementation will do a clip,
                    // then update the bin. This is probably ok for the moment, though.
                    if (a.point.z > binMax) binMax = a.point.z;
                    if (b.point.z > binMax) binMax = b.point.z;
                    if (c.point.z > binMax) binMax = c.point.z;

                    if (a.point.z < binMin) binMin = a.point.z;
                    if (b.point.z < binMin) binMin = b.point.z;
                    if (c.point.z < binMin) binMin = c.point.z;
                }
            }

            // Ok, we're all set for this bin...
            AssertFatal(binMin <= binMax,
                "AtlasGeomChunk::generateCollision - empty bin, crap!");
        }
    }

    // Next, generate the quadtree.
    const U32 nodeCount = AtlasTOC::getNodeCount(mColTreeDepth);
    mColTree = new ColNode[nodeCount];

    // For thoroughness, we wipe the colnode.
    for (S32 i = 0; i < nodeCount; i++)
    {
        mColTree[i].min = -12341.0;
        mColTree[i].max = -12342.0;
    }

    // We have to recursively generate this from the bins on up. First we copy 
    // the bins from earlier, then we do our recursomatic thingummy. (It's 
    // actually not recursive.)
    for (S32 i = 0; i < gridSize; i++)
    {
        for (S32 j = 0; j < gridSize; j++)
        {
            const U32 qtIdx = AtlasTOC::getNodeIndex(mColTreeDepth - 1, Point2I(i, j));

            mColTree[qtIdx].min = binsMin[i * gridSize + j];
            mColTree[qtIdx].max = binsMax[i * gridSize + j];

            AssertFatal(mColTree[qtIdx].min <= mColTree[qtIdx].max,
                "AtlasGeomChunk::generateCollision - bad child quadtree node min/max! (negative a)");

        }
    }

    // Alright, now we go up the bins, generating from the four children of each,
    // till we hit the root.

    // For each empty level from bottom to top...
    for (S32 depth = mColTreeDepth - 2; depth >= 0; depth--)
    {
        // For each square...
        for (S32 i = 0; i < BIT(depth); i++)
            for (S32 j = 0; j < BIT(depth); j++)
            {
                const U32 curIdx = AtlasTOC::getNodeIndex(depth, Point2I(i, j));

                ColNode& cn = mColTree[curIdx];
                cn.min = mBounds.max.z + 100.f;
                cn.max = mBounds.min.z - 100.f;

                // For each of this square's 4 children...
                for (S32 subI = 0; subI < 2; subI++)
                {
                    for (S32 subJ = 0; subJ < 2; subJ++)
                    {
                        const U32 subIdx =
                            AtlasTOC::getNodeIndex(depth + 1, Point2I(i * 2 + subI, j * 2 + subJ));

                        ColNode& subCn = mColTree[subIdx];

                        // As is the child.
                        AssertFatal(subCn.min <= subCn.max,
                            "AtlasGeomChunk::generateCollision - bad child quadtree node min/max! (a)");

                        // Update the min and max of the parent.
                        if (subCn.min < cn.min) cn.min = subCn.min;
                        if (subCn.max > cn.max) cn.max = subCn.max;

                        // Make sure we actually contain the child.
                        AssertFatal(subCn.min >= cn.min,
                            "AtlasGeomChunk::generateCollision - bad quadtree child min during coltree generation!");
                        AssertFatal(subCn.max <= cn.max,
                            "AtlasGeomChunk::generateCollision - bad quadtree child max during coltree generation!");

                        // And that the parent is still valid.
                        AssertFatal(cn.min <= cn.max,
                            "AtlasGeomChunk::generateCollision - bad parent quadtree node min/max!");

                        // As is the child.
                        AssertFatal(subCn.min <= subCn.max,
                            "AtlasGeomChunk::generateCollision - bad child quadtree node min/max! (b)");

                    }
                }
            }
    }

    // Wasn't that fun? Now we have a ready-to-go quadtree.

    // We have to generate...
    // ... the list of triangle offsets for each bin. (Done above!)
    // ... the triangle buffer which stores the offsets for each bin.
    AtlasChunkGeomCollisionBufferGenerator ctbg(gridSize);

    for (S32 i = 0; i < gridSize; i++)
        for (S32 j = 0; j < gridSize; j++)
            ctbg.insertBinList(Point2I(i, j), bins[i * gridSize + j]);

    ctbg.store(mColIndexCount, mColIndicesBuffer, mColOffsetCount, mColIndicesOffsets);

    // And delete our lists, we're done with them.
    delete[] bins;
}

//-----------------------------------------------------------------------------

bool AtlasGeomChunk::buildCollisionInfo(const Box3F& box, Convex* c,
    AbstractPolyList* poly, AtlasInstance2* object)
{
    AssertFatal(mColTree, "AtlasResourceInfo::buildCollisionInfo - no collision tree!");

    // We want to only return a given triangle once so let's allocate a temp
    // array to store if we've returned that triangle or not. We'll also
    // track each vert to make sure we're only giving out the minimum amount
    // of vertex data (where appropriate).
    const U32 triangleCount = mIndexCount / 3;
    FrameTemp<U8> triangleMarkers(triangleCount);
    dMemset(triangleMarkers, 0, sizeof(U8) * triangleCount);

    // We don't use this for convexes, so if it's a convex then just make it
    // of length 1 - this is cheap as we're getting it out of the frame alloc.
    FrameTemp<U16> vertMarkers(c ? 1 : mVertCount);
    dMemset(vertMarkers, 0xFF, sizeof(U16) * mVertCount);


    // Okies, we've got a box in coltree space (ie, 0..1 along our
    // internal space) so let's figure which bins we're gonna hit,
    // do a quick culling pass, and then on with our lives...

    // We return this later on so we can tell if we got anything...
    bool gotData = false;

    // Figure the range of chunks this covers...
    const U32 colGridSize = BIT(mColTreeDepth - 1);
    const Point2F bucketSize(
        mBounds.len_x() / F32(colGridSize),
        mBounds.len_y() / F32(colGridSize));

    S32 xStart = mFloor((box.min.x - mBounds.min.x) / bucketSize.x);
    S32 yStart = mFloor((box.min.y - mBounds.min.y) / bucketSize.y);
    S32 xEnd = mCeil((box.max.x - mBounds.min.x) / bucketSize.x);
    S32 yEnd = mCeil((box.max.y - mBounds.min.y) / bucketSize.y);

    // Clamp only to chunks that exist!
    xStart = mClamp(xStart, 0, colGridSize);
    yStart = mClamp(yStart, 0, colGridSize);
    xEnd = mClamp(xEnd, 0, colGridSize);
    yEnd = mClamp(yEnd, 0, colGridSize);

    const F32 zScale = 1.f; //AtlasResource::getCurrentResource()->mVerticalScale;

    if (c)
        mConvexList.collectGarbage();

    // Ok, iterate over this range, check the bins, and start making convexes.
    for (S32 i = xStart; i < xEnd; i++)
    {
        for (S32 j = yStart; j < yEnd; j++)
        {
            const Point2I pos(i, j);
            const U32 nodeID = AtlasTOC::getNodeIndex(mColTreeDepth - 1, pos);

            // Get the bin...
            ColNode* e = &mColTree[nodeID];

            // Check the bin Z-range...
            if (
                (box.max.z > (e->max * zScale) && box.min.z > (e->max * zScale))
                ||
                (box.max.z < (e->min * zScale) && box.min.z < (e->min * zScale))
                )
                continue;

            // Ok, do what we gotta do for this bin...

            if (c)
            {
                // Do a convex!

                // Get the triangle list...
                U16 offset;
                U16* triOffset = mColIndicesBuffer + mColIndicesOffsets[pos.x * colGridSize + pos.y];

                while ((offset = *triOffset) != 0xFFFF)
                {
                    // Advance to the next triangle..
                    triOffset++;

                    // Did we emit this triangle already?
                    if (triangleMarkers[offset / 3])
                        continue;
                    triangleMarkers[offset / 3] = 1; // Mark we emitted it.

                    // Register the triangle as a convex, and note we have data.
                    registerConvex(offset, c, object, box);
                    gotData = true;
                }
            }

            if (poly)
            {
                // There's a polylist, let's stick our polygons in the list...
                // Someday we may want to do some boxTri checks, for the moment
                // let's be lazy muffins and dump it right in.

                // Get the triangle list...
                U16 offset;
                U16* triOffset = mColIndicesBuffer + mColIndicesOffsets[pos.x * colGridSize + pos.y];

                while ((offset = *triOffset) != 0xFFFF)
                {
                    // Advance to the next triangle..
                    triOffset++;

                    // Did we emit this triangle already?
                    if (triangleMarkers[offset / 3])
                        continue;
                    triangleMarkers[offset / 3] = 1; // Mark we emitted it.

                    // Get each triangle, and dump it into the polylist.
                    Point3F a, b, c;
                    const U16 idxA = mIndex[offset + 0];
                    const U16 idxB = mIndex[offset + 1];
                    const U16 idxC = mIndex[offset + 2];

                    a = mVert[idxA].point;
                    b = mVert[idxB].point;
                    c = mVert[idxC].point;

                    // Make sure it's really in the box.
                    // Check against the query box before registering. This can significantly
                    // help performance in over-large bin situations. If you have small bins
                    // it may just be an overhead.
                    if (!triBoxOverlap(box, a, b, c))
                        continue;

                    // Add points if we've not already inserted it to the polylist.

                    // Given we're already terminating our triangle lists with 0xFFF,
                    // it's a safe sentinel value to use. (Some artist will run into
                    // this someday and be angry they're shorted a vert.)

                    S32 v[3];

                    if (vertMarkers[idxA] == 0xFFFF)
                        vertMarkers[idxA] = v[0] = poly->addPoint(a);
                    else
                        v[0] = vertMarkers[idxA];

                    if (vertMarkers[idxB] == 0xFFFF)
                        vertMarkers[idxB] = v[1] = poly->addPoint(b);
                    else
                        v[1] = vertMarkers[idxB];

                    if (vertMarkers[idxC] == 0xFFFF)
                        vertMarkers[idxC] = v[2] = poly->addPoint(c);
                    else
                        v[2] = vertMarkers[idxC];

                    poly->begin(0, offset);
                    poly->vertex(v[0]);
                    poly->vertex(v[1]);
                    poly->vertex(v[2]);
                    poly->plane(v[0], v[1], v[2]);
                    poly->end();

                    gotData = true;
                }
            }
        }
    }

    return gotData;
}

void AtlasGeomChunk::registerConvex(const U16 offset, Convex* convex,
    AtlasInstance2* object, const Box3F& queryBox)
{
    // Get the triangle information...
    Point3F a, b, c;

    a = mVert[mIndex[offset + 0]].point;
    b = mVert[mIndex[offset + 1]].point;
    c = mVert[mIndex[offset + 2]].point;

    // Check against the query box before registering. This can significantly
    // help performance in over-large bin situations. If you have small bins
    // it may just be an overhead.
    if (!triBoxOverlap(queryBox, a, b, c))
        return;

    // First, check our active convexes for a potential match (and clean things
    // up, too.)
    Convex* cc = NULL;

    // See if the square already exists as part of the working set.
    CollisionWorkingList& wl = convex->getWorkingList();
    for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
    {
        if (itr->mConvex->getType() != AtlasConvexType)
            continue;

        AtlasConvex* chunkc = static_cast<AtlasConvex*>(itr->mConvex);

        if (chunkc->geom != this)
            continue;

        if (chunkc->offset != offset)
            continue;

        // A match! Don't need to add it.
        return;
    }

    // Check for degenerates - this should never fire, is generally indicative
    // of corrupt collision data.
    /*if(mIndex[offset+0] == mIndex[offset+1] ||
       mIndex[offset+1] == mIndex[offset+2] ||
       mIndex[offset+2] == mIndex[offset+0])
    {
       Con::warnf("AtlasGeomChunk::registerConvex - found a degenerate, skipping!");
       return;
    }*/

    // Set up the convex...
    AtlasConvex* cp = new AtlasConvex();

    mConvexList.registerObject(cp);
    convex->addToWorkingList(cp);

    cp->geom = this;
    cp->offset = offset;
    cp->mObject = object;

    cp->normal = PlaneF(a, b, c);
    cp->normal.normalize();

    cp->point[0] = a;
    cp->point[1] = b;
    cp->point[2] = c;
    cp->point[3] = (a + b + c) / 3.f - (2.f * cp->normal);

    // Update the bounding box.
    Box3F& bounds = cp->box;
    bounds.min.set(F32_MAX, F32_MAX, F32_MAX);
    bounds.max.set(-F32_MAX, -F32_MAX, -F32_MAX);

    bounds.min.setMin(a);
    bounds.min.setMin(b);
    bounds.min.setMin(c);
    bounds.min.setMin(cp->point[3]);

    bounds.max.setMax(a);
    bounds.max.setMax(b);
    bounds.max.setMax(c);
    bounds.max.setMax(cp->point[3]);
}

//----------------------------------------------------------------------------

void AtlasGeomChunk::calculatePoints(const Point3F& camPos, Point3F& outNearPos,
    Point3F& outFarPos, Point2F& outNearTC, Point2F& outFarTC)
{
    // The camera position is relative to our bounding box, so in filespace.
    const Point3F& min = mBounds.min;
    const Point3F& max = mBounds.max;
    Point3F center;
    mBounds.getCenter(&center);

    if (camPos.x <= min.x) { outNearPos.x = min.x; outFarPos.x = max.x; }
    else if (camPos.x > max.x) { outNearPos.x = max.x; outFarPos.x = min.x; }
    else { outNearPos.x = camPos.x; outFarPos.x = (camPos.x > center.x ? min.x : max.x); }

    if (camPos.y <= min.y) { outNearPos.y = min.y; outFarPos.y = max.y; }
    else if (camPos.y > max.y) { outNearPos.y = max.y; outFarPos.y = min.y; }
    else { outNearPos.y = camPos.y; outFarPos.y = (camPos.y > center.y ? min.y : max.y); }

    if (camPos.z <= min.z) { outNearPos.z = min.z; outFarPos.z = max.z; }
    else if (camPos.z > max.z) { outNearPos.z = max.z; outFarPos.z = min.z; }
    else { outNearPos.z = camPos.z; outFarPos.z = (camPos.z > center.z ? min.z : max.z); }

    // This is a hack but since we only care about delta at this point might be
    // ok. Fix me later -- BJG
    outNearTC = mTCBounds.point;
    outFarTC = mTCBounds.point + mTCBounds.extent;
}

//-----------------------------------------------------------------------------

AtlasChunkGeomCollisionBufferGenerator::AtlasChunkGeomCollisionBufferGenerator(U32 gridSize)
{
    mGridSize = gridSize;

    // Allocate space for the bins.
    mBinOffsets.setSize(gridSize * gridSize);

    // Assume we've got approx. 3 triangles per bin and prep some space.
    mTriangles.reserve(3 * mBinOffsets.size());
}

void AtlasChunkGeomCollisionBufferGenerator::clear()
{
    mTriangles.clear();
    mBinOffsets.clear();
}

void AtlasChunkGeomCollisionBufferGenerator::insertBinList(Point2I bin, Vector<U16>& binList)
{
    // Stick the list on the end.
    mBinOffsets[bin.x * mGridSize + bin.y] = mTriangles.size();

    mTriangles.reserve(binList.size() + mTriangles.size() + 1);

    for (S32 i = 0; i < binList.size(); i++)
        mTriangles.push_back(binList[i]);
    mTriangles.push_back(0xFFFF); // Terminate the list.
}

void AtlasChunkGeomCollisionBufferGenerator::store(U32& indexCount, U16*& indices, U32& offsetCount, U16*& offsets)
{
    // Note our average bin size.
    U32 total = 0, min = U32_MAX, max = 0, count = 0;
    for (S32 i = 0; i < mBinOffsets.size(); i++)
    {
        count++;
        U32 thisBin = 0, curPos = mBinOffsets[i];
        while (mTriangles[curPos++] != 0xFFFF)
            thisBin++;

        min = getMin(min, thisBin);
        max = getMax(max, thisBin);
        total += thisBin;
    }

    /*Con::printf("AtlasChunkGeomCollisionBufferGenerator::store - bin size avg %f min %d max %d count %d",
       F32(total) / F32(count), min, max, total);*/

       // Clean up any existing data.
    SAFE_DELETE_ARRAY(indices);
    SAFE_DELETE_ARRAY(offsets);

    // Fill out offset data.
    offsetCount = mBinOffsets.size();
    offsets = new U16[offsetCount];
    dMemcpy(offsets, mBinOffsets.address(), offsetCount * sizeof(U16));

    // Fill out index data.
    indexCount = mTriangles.size();
    indices = new U16[indexCount];
    dMemcpy(indices, mTriangles.address(), indexCount * sizeof(U16));
}

