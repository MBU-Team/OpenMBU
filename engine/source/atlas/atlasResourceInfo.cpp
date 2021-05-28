//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "atlas/atlasResourceInfo.h"
#include "atlas/atlasCollision.h"
#include "atlas/atlasInstance.h"
#include "atlas/atlasInstanceEntry.h"
#include "util/quadTreeTracer.h"
#include "platform/profiler.h"
#include "sceneGraph/sceneGraph.h"
#include "util/safeDelete.h"

AtlasResourceInfo::AtlasResourceInfo()
    : mVert(), mPrim()
{
    mTriCount = 0;
    mRawVerts = NULL;
    mRawIndices = NULL;
    mColIndicesBuffer = NULL;
    mColIndicesOffsets = NULL;
    mColTree = NULL;
    mColGeom = NULL;
    mColIndices = NULL;
}

AtlasResourceInfo::~AtlasResourceInfo()
{
    // Clean everything up that we can.
    if ((mRawVerts) != 0) delete[] mRawVerts; (mRawVerts) = 0;
    if ((mColIndicesBuffer) != 0) delete[] mColIndicesBuffer; (mColIndicesBuffer) = 0;
    if ((mColIndicesOffsets) != 0) delete[] mColIndicesOffsets; (mColIndicesOffsets) = 0;
    if ((mColTree) != 0) delete[] mColTree; (mColTree) = 0;
    if ((mColGeom) != 0) delete[] mColGeom; (mColGeom) = 0;

    if (mRawIndices)
    {
        if ((mRawIndices) != 0) delete[](mRawIndices); (mRawIndices) = 0;;
        mColIndices = 0;
    }
    else
    {
        if ((mColIndices) != 0) delete[](mColIndices); (mColIndices) = 0;;
    }

    mConvexList.nukeList();

    // Clear our vert & prim references.
    mVert = NULL;
    mPrim = NULL;
}


void AtlasResourceInfo::read(Stream* s, AtlasResource* tree)
{
    PROFILE_START(ChunkGeomInfo_read);

    //  Store our AtlasResource for future reference.
    mData = tree;

    // Read a sentinel.
    U32 sent = 0;
    s->read(&sent);
    AssertISV(sent == 0xbeef1234, "AtlasResourceInfo::read - bad sentinel.");

    // Read the vertex buffer.

    // How many verts in our buffer?
    s->read(&mRawVertexCount);

    // Prepare raw buffer.
    mRawVerts = new GFXAtlasVert[mRawVertexCount];

    for (U32 i = 0; i < mRawVertexCount; i++)
    {
        // First read the raw data...
        S16 x, y, z, morph;

        // Z is vertical in Torque, not Y+
        s->read(&x);
        s->read(&y);
        s->read(&z);
        s->read(&morph);

        // Now stuff into the VB... (try to scale it a bit to be more useful)
        mRawVerts[i].point.set(
            F32(x) / F32(1 << 14),
            F32(y) / F32(1 << 14),
            F32(z) * tree->mVerticalScale
        );

        // Store morph...
        mRawVerts[i].morphCoord.set(F32(morph) * tree->mVerticalScale, 0, 0);

        // And do a simple linear texgen here for now.
        mRawVerts[i].texCoord.set(F32(x) / F32(1 << 14) * 0.5 + 0.5,
            F32(y) / F32(1 << 14) * 0.5 + 0.5);
    }

    // Load the index buffer.
    s->read(&mRawIndexCount);
    AssertISV(mRawIndexCount, "AtlasResourceInfo::read - no indices in file!");

    mRawIndices = new U16[mRawIndexCount];
    for (U32 i = 0; i < mRawIndexCount; i++)
        s->read(mRawIndices + i);

    // Load the real triangle count.
    s->read(&mTriCount);

    // Load our collision info as well.
    U8 colDataFlag;
    s->read(&colDataFlag);

    AssertISV(colDataFlag == 0 || colDataFlag == 1,
        "AtlasResourceInfo::read - bad colDataFlag!");

    if (!colDataFlag)
    {
        // Null out the collision pointers.
        mColIndices = NULL;
        mColTree = NULL;
        mColIndicesBuffer = NULL;
        mColIndicesOffsets = NULL;

        // Check the postscript.
        U32 post = 0;
        s->read(&post);
        AssertISV(post == 0xb1e2e3f4,
            "AtlasResourceInfo::read - missing postscript!");

        PROFILE_END();
        return;
    }

    // Just use the indices directly. We'll be aware of this in prepare and not
    // delete the memory.
    mColIndices = mRawIndices;

    // We have to copy the points though.
    mColGeom = new Point3F[mRawVertexCount];

    for (U32 i = 0; i < mRawVertexCount; i++)
    {
        // These need to be scaled to be in a unit square (0..1), due to craziness in the
        // castRay code.

        //scaleOff(mPos.x * chunkSize + 0.5 * chunkSize, mPos.y * chunkSize + 0.5 * chunkSize, 0.5 * chunkSize);
        // Thus if we don't care about position (mPos = { 0, 0 }) and have a unit chunkSize, we get...
        //scaleOff( 0  + 0.5, 0 + 0.5, 0.5 );
        // Scale equation is (from our shader):
        //realPos.x = IN.position.x * scaleOff.z + scaleOff.x;
        //
        // Z is left untouched, of course. The quadtree stuff only operates
        // on the XY plane.

        mColGeom[i].x = mRawVerts[i].point.x * 0.5 + 0.5;
        mColGeom[i].y = mRawVerts[i].point.y * 0.5 + 0.5;
        mColGeom[i].z = mRawVerts[i].point.z;

        AssertFatal(mColGeom[i].x >= 0 && mColGeom[i].x <= 1.f,
            "AtlasResourceInfo::read - out of bound collision vertex.x!");
        AssertFatal(mColGeom[i].y >= 0 && mColGeom[i].y <= 1.f,
            "AtlasResourceInfo::read - out of bound collision vertex.y!");
    }

    // Load the QT
    U32 nodeCount = QuadTreeTracer::getNodeCount(tree->mColTreeDepth);
    mColTree = new ColNode[nodeCount];

    for (S32 i = 0; i < nodeCount; i++)
        mColTree[i].read(s);

    // Read the sentinel
    U32 sent2 = 0;
    s->read(&sent2);
    AssertISV(sent2 == U32(0xb33fd34d),
        "AtlasResourceInfo::read - invalid sent2");

    // Load the bin offsets and the triangle buffer.
    U32 binCount = tree->mColGridSize * tree->mColGridSize;
    mColIndicesOffsets = new U16[binCount];
    for (S32 i = 0; i < binCount; i++)
        s->read(&mColIndicesOffsets[i]);

    U32 bufferSize = 0;
    s->read(&bufferSize);
    mColIndicesBuffer = new U16[bufferSize];
    for (S32 i = 0; i < bufferSize; i++)
        s->read(&mColIndicesBuffer[i]);

    // Check the postscript.
    U32 post = 0;
    s->read(&post);
    AssertISV(post == 0xb1e2e3f4,
        "AtlasResourceInfo::read - missing postscript!");

    // Yay, all done!
    PROFILE_END();
}

void AtlasResourceInfo::prepare()
{
    // BJGTODO - fix this hack!
    bool dedicated = Con::getBoolVariable("$Server::Dedicated");
    if (!dedicated)
    {
        // Deal with index buffer.
        mPrim.set(GFX, mRawIndexCount, 0, GFXBufferTypeStatic);
        U16* idx;
        mPrim.lock(&idx);
        dMemcpy(idx, mRawIndices, sizeof(U16) * mRawIndexCount);
        mPrim.unlock();
    }

    // Only blast if we're not using it for collision indices.
    if (mColIndicesBuffer == NULL)
    {
        delete[] mRawIndices;
        mRawIndices = NULL;
    }

    // Deal with vertex buffer.
    if (!dedicated)
    {
        mVert.set(GFX, mRawVertexCount, GFXBufferTypeStatic);
        mVert.lock();
        dMemcpy(&mVert[0], mRawVerts, sizeof(mRawVerts[0]) * mRawVertexCount);
        mVert.unlock();
    }

    // We always kill the loaded verts.
    delete[] mRawVerts;
    mRawVerts = NULL;
}

void AtlasResourceInfo::render()
{
    PROFILE_START(Atlas_renderGeom);

    PROFILE_START(Atlas_setBuffers);
    GFX->setVertexBuffer(mVert);
    GFX->setPrimitiveBuffer(mPrim);
    PROFILE_END();

    PROFILE_START(Atlas_renderDIP);
    GFX->drawIndexedPrimitive(GFXTriangleList, 0, mVert->mNumVerts, 0, mPrim->mIndexCount / 3);
    PROFILE_END();

    PROFILE_END();
}

bool AtlasResourceInfo::buildCollisionInfo(const MatrixF* localMat, const Box3F& box, Convex* c, AbstractPolyList* poly)
{
    AssertFatal(mColTree, "AtlasResourceInfo::buildCollisionInfo - no collision tree!");
    AssertFatal(localMat,
        "AtlasResourceInfo::buildCollisionInfo - No local matrix for convex build!");

    // Okies, we've got a box in coltree space (ie, 0..1 along our
    // internal space) so let's figure which bins we're gonna hit,
    // do a quick culling pass, and then on with our lives...

    // We return this later on so we can tell if we got anything...
    bool gotData = false;

    // Figure the range of chunks this covers...
    const F32 invSize = F32(AtlasResource::getCurrentResource()->mColGridSize);
    S32 xStart = mFloor(box.min.x * invSize);
    S32 yStart = mFloor(box.min.y * invSize);
    S32 xEnd = mCeil(box.max.x * invSize);
    S32 yEnd = mCeil(box.max.y * invSize);

    // Clamp only to chunks that exist!
    xStart = mClamp(xStart, 0, AtlasResource::getCurrentResource()->mColGridSize);
    yStart = mClamp(yStart, 0, AtlasResource::getCurrentResource()->mColGridSize);
    xEnd = mClamp(xEnd, 0, AtlasResource::getCurrentResource()->mColGridSize);
    yEnd = mClamp(yEnd, 0, AtlasResource::getCurrentResource()->mColGridSize);

    const F32 zScale = AtlasResource::getCurrentResource()->mVerticalScale;

    // Ok, iterate over this range, check the bins, and start making ChunkConvexes.
    for (S32 i = xStart; i < xEnd; i++)
    {
        for (S32 j = yStart; j < yEnd; j++)
        {
            const Point2I pos(i, j);
            const U32 nodeID = QuadTreeTracer::getNodeIndex(AtlasResource::getCurrentResource()->mColTreeDepth - 1, pos);

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
                U16* triOffset = mColIndicesBuffer + mColIndicesOffsets[pos.x * mData->mColGridSize + pos.y];

                while ((offset = *triOffset) != 0xFFFF)
                {
                    // Advance to the next triangle..
                    triOffset++;

                    // Register the triangle as a convex, and note we have data.
                    registerConvex(localMat, offset, c);
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
                U16* triOffset = mColIndicesBuffer + mColIndicesOffsets[pos.x * mData->mColGridSize + pos.y];

                while ((offset = *triOffset) != 0xFFFF)
                {
                    // Advance to the next triangle..
                    triOffset++;

                    // Get each triangle, and dump it into the polylist.
                    Point3F a, b, c;

                    a = mColGeom[mColIndices[offset + 0]];
                    b = mColGeom[mColIndices[offset + 1]];
                    c = mColGeom[mColIndices[offset + 2]];

                    // Get the points back into object space.
                    localMat->mulP(a);
                    localMat->mulP(b);
                    localMat->mulP(c);

                    S32 v[3];

                    // BJGTODO - optimize this someday? (ie, reuse indices...)
                    v[0] = poly->addPoint(a);
                    v[1] = poly->addPoint(b);
                    v[2] = poly->addPoint(c);

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

void AtlasResourceInfo::registerConvex(const MatrixF* localMat, const U16 offset, Convex* convex)
{
    // First, check our active convexes for a potential match (and clean things
    // up, too.)
    mConvexList.collectGarbage();

    Convex* cc = NULL;

    // See if the square already exists as part of the working set.
    CollisionWorkingList& wl = convex->getWorkingList();
    for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
    {
        if (itr->mConvex->getType() != AtlasChunkConvexType)
            continue;

        AtlasChunkConvex* chunkc = static_cast<AtlasChunkConvex*>(itr->mConvex);

        if (chunkc->geom != this)
            continue;

        if (chunkc->offset != offset)
            continue;

        // A match! Don't need to add it.
        return;
    }

    // Get the triangle information...
    Point3F a, b, c;

    a = mColGeom[mColIndices[offset + 0]];
    b = mColGeom[mColIndices[offset + 1]];
    c = mColGeom[mColIndices[offset + 2]];

    // Get the points back into object space.
    localMat->mulP(a);
    localMat->mulP(b);
    localMat->mulP(c);

    // Set up the convex...
    AtlasChunkConvex* cp = new AtlasChunkConvex();

    mConvexList.registerObject(cp);
    convex->addToWorkingList(cp);

    cp->geom = this;
    cp->offset = offset;
    cp->mObject = AtlasInstanceEntry::getInstance();

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