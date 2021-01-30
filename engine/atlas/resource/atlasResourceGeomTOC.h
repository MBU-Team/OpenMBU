//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _ATLASRESOURCEGEOMTOC_H_
#define _ATLASRESOURCEGEOMTOC_H_

#include "atlas/core/atlasResourceTOC.h"
#include "atlas/resource/atlasGeomChunk.h"

class RayInfo;
class Convex;
class AbstractPolyList;

/// @defgroup AtlasResource Atlas Resource System
///
/// Although the core background loading system is implemented in AtlasCore, 
/// the specific data types that are loaded, the rules for regenerating them,
/// and so forth all live in this group.
///
/// @ingroup Atlas

/// Atlas resource stub subclass for geometry.
///
/// @ingroup AtlasResource
class AtlasResourceGeomStub : public AtlasResourceStub<AtlasGeomChunk>
{
public:
    AtlasResourceGeomStub()
    {
        mBounds.min.set(-100, -100, -100);
        mBounds.max.set(100, 100, 100);
    }

    /// Geometry bounds.
    Box3F mBounds;

    typedef AtlasResourceStub<AtlasGeomChunk> Parent;
    virtual void read(Stream* s, const U32 version);
    virtual void write(Stream* s);
};

/// Atlas resource TOC subclass for geometry.
///
/// @ingroup AtlasResource
class AtlasResourceGeomTOC : public AtlasResourceTOC<AtlasResourceGeomStub>
{
public:

    typedef AtlasResourceTOC<AtlasResourceGeomStub> Parent;

    U32 mGoalBatchSize;
    F32 mDistance_LODMax;

    void initializeTOC(U32 depth)
    {
        helpInitializeTOC(depth);
    }

    AtlasResourceGeomTOC();
    ~AtlasResourceGeomTOC();

    virtual bool write(Stream* s);
    virtual bool read(Stream* s);

    /// Override so we can update stub bounds when a new chunk is stored.
    virtual void instateLoadedChunk(StubType* stub, ChunkType* newChunk);
};

/// Allow traversal over atlas geometry using UV coordinates. Performs
/// normal interpolation.
class AtlasSurfaceQueryHelper
{
public:
    AtlasGeomChunk* mChunk;

    /// Texture bounds of the chunk.
    RectF mTexBounds;

    /// Number of bins in the grid on each axis.
    U32 mGridCount;

    /// List of lists of triangles in bins. Indexed by mTriangleOffsets.
    ///
    /// Lists are 0 terminated.
    Vector<U16> mTriangleBuffer;

    /// For each bin, index into mTriangleBuffers for that bin's triangles.
    Vector<U16> mTriangleOffsets;

    /// Last triangle we hit. Used to optimize queries.
    S32 mLastHitTriangle;

    /// List of vertex normals, used for interpolation.
    Vector<Point3F> mVertexNormals;

    AtlasSurfaceQueryHelper()
    {
        mGridCount = 8;
        mTexBounds.point.set(0, 0);
        mTexBounds.extent.set(0, 0);
        mLastHitTriangle = -1;
    }

    ~AtlasSurfaceQueryHelper()
    {
    }

    void generateLookupTables()
    {
        // Walk all of the bins and suck triangles into them. Not very
        // efficient but could be worse.
        Point2F binSize;
        binSize.x = mTexBounds.extent.x / F32(mGridCount);
        binSize.y = mTexBounds.extent.y / F32(mGridCount);

        // Initialize our arrays.
        mTriangleOffsets.setSize(mGridCount * mGridCount);
        mTriangleBuffer.reserve(mTriangleOffsets.size() * 8);
        mTriangleBuffer.clear();
        mLastHitTriangle = -1;

        for (S32 x = 0; x < mGridCount; x++)
        {
            for (S32 y = 0; y < mGridCount; y++)
            {
                // Generate the bin TC bounds.
                RectF binBounds;
                binBounds.point.x = F32(x) * binSize.x;
                binBounds.point.y = F32(y) * binSize.y;
                binBounds.extent = binSize;

                // Note bin start.
                mTriangleOffsets[x * mGridCount + y] = mTriangleBuffer.size();

                //Con::printf("for bin %dx%d", x, y);

                // Now, find all triangles that overlap this TC range.
                for (S32 i = 0; i < mChunk->mIndexCount - 2; i += 3)
                {
                    // Snag texcoords!
                    const Point2F& a = mChunk->mVert[mChunk->mIndex[i + 0]].texCoord;
                    const Point2F& b = mChunk->mVert[mChunk->mIndex[i + 1]].texCoord;
                    const Point2F& c = mChunk->mVert[mChunk->mIndex[i + 2]].texCoord;

                    // If the TCs are a degenerate quad, skip it...
                    if (a == b || b == c || c == a)
                        continue;

                    // Do overlap test.
                    if (!binBounds.intersectTriangle(a, b, c))
                        continue;

                    // We overlap, so add to list.
                    mTriangleBuffer.push_back(i);
                    //Con::printf("     idx %d", i);
                }

                // Finally, null-terminate the list.
                mTriangleBuffer.push_back(0xFFFF);
            }
        }

        // Generate face normals.
        Vector<Point3F> faceNormals;
        faceNormals.reserve(mChunk->mIndexCount / 3);

        for (S32 i = 0; i < mChunk->mIndexCount - 2; i += 3)
        {
            // Snag verts!
            const GFXAtlasVert2& a = mChunk->mVert[mChunk->mIndex[i + 0]];
            const GFXAtlasVert2& b = mChunk->mVert[mChunk->mIndex[i + 1]];
            const GFXAtlasVert2& c = mChunk->mVert[mChunk->mIndex[i + 2]];

            Point3F e1 = b.point - a.point;
            Point3F e2 = c.point - a.point;
            faceNormals.increment();

            e1.normalize();
            e2.normalize();
            mCross(e2, e1, &faceNormals.last());
        }

        // Now - for every vert, generate a normal by averaging the faces it
        // is present on.
        mVertexNormals.setSize(mChunk->mVertCount);

        Vector<U16> adjacentFaces(128);

        for (S32 i = 0; i < mChunk->mVertCount; i++)
        {
            // Find adjacent faces - not fast but we can optimize later.
            adjacentFaces.clear();
            for (S32 j = 0; j < mChunk->mIndexCount - 2; j += 3)
            {
                // If it's degenerate UV wise, skip it.
                const GFXAtlasVert2& a = mChunk->mVert[mChunk->mIndex[j + 0]];
                const GFXAtlasVert2& b = mChunk->mVert[mChunk->mIndex[j + 1]];
                const GFXAtlasVert2& c = mChunk->mVert[mChunk->mIndex[j + 2]];

                if (a.texCoord == b.texCoord || b.texCoord == c.texCoord || c.texCoord == a.texCoord)
                    continue;

                if (mChunk->mIndex[j + 0] == i || mChunk->mIndex[j + 1] == i || mChunk->mIndex[j + 2] == i)
                    adjacentFaces.push_back(j / 3);
            }

            if (!adjacentFaces.size())
                continue;

            // Ok, now accumulate the normals.
            Point3F normAcc(0, 0, 0);
            //AssertFatal(adjacentFaces.size(), "AtlasLightingQueryHelper::generateLookupTables - orphan vertex!");
            for (S32 j = 0; j < adjacentFaces.size(); j++)
                normAcc = normAcc + faceNormals[adjacentFaces[j]];

            normAcc /= F32(adjacentFaces.size());

            // Normalize the result.
            normAcc.normalize();

            // Store!
            mVertexNormals[i] = normAcc;
        }
    }

    bool pointInTri(const Point2F& pt, const Point2F& a, const Point2F& b,
        const Point2F& c, Point2F& baryCoords)
    {
        const Point2F e1 = b - a;
        const Point2F e2 = c - a;
        const Point2F h = pt - a;

        baryCoords.x = (e2.x * h.y - e2.y * h.x) / (e2.x * e1.y - e2.y * e1.x);
        baryCoords.y = (h.x * e1.y - h.y * e1.x) / (e2.x * e1.y - e2.y * e1.x);

        if (baryCoords.x < 0.f)
            return false;

        if (baryCoords.y < 0.f)
            return false;

        if ((baryCoords.x + baryCoords.y) > 1.f)
            return false;

        // Ok we got a hit!
        return true;
    }

    /// Perform a lookup in UV space. Takes a UV parameter, and returns the
    /// index of the triangle it's on, the position in 3space at which that UV
    /// is located, and a smoothed normal for that location.
    bool lookup(const Point2F& inUv, U16& outTriIdx, Point3F& outPos, Point3F& outNorm)
    {
        // First, see if we are within the last hit triangle - if so we can
        // save some work!
        if (mLastHitTriangle != -1)
        {
            // Is the UV within the texcoords for this triangle?
            const GFXAtlasVert2& a = mChunk->mVert[mChunk->mIndex[mLastHitTriangle + 0]];
            const GFXAtlasVert2& b = mChunk->mVert[mChunk->mIndex[mLastHitTriangle + 1]];
            const GFXAtlasVert2& c = mChunk->mVert[mChunk->mIndex[mLastHitTriangle + 2]];

            Point2F bary;
            if (pointInTri(inUv, a.texCoord, b.texCoord, c.texCoord, bary))
            {
                // Interpolate position...
                Point3F posEdge1 = b.point - a.point;
                Point3F posEdge2 = c.point - a.point;
                outPos = a.point + posEdge1 * bary.x + posEdge2 * bary.y;

                // Interpolate normal...
                const Point3F& nA = mVertexNormals[mChunk->mIndex[mLastHitTriangle + 0]];
                const Point3F& nB = mVertexNormals[mChunk->mIndex[mLastHitTriangle + 1]];
                const Point3F& nC = mVertexNormals[mChunk->mIndex[mLastHitTriangle + 2]];
                outNorm = nA + (nB - nA) * bary.x + (nC - nA) * bary.y;

                // Note the triangle we hit!
                outTriIdx = mLastHitTriangle;
                return true;
            }
        }

        // Otherwise, search the bin.
        Point2F binSize;
        binSize.x = mTexBounds.extent.x / F32(mGridCount);
        binSize.y = mTexBounds.extent.y / F32(mGridCount);

        const Point2F localUV = inUv - mTexBounds.point;
        S32 binX = mFloor(localUV.x / binSize.x);
        S32 binY = mFloor(localUV.y / binSize.y);

        // Get the bin list.
        U32 index = binX * mGridCount + binY;
        AssertFatal((index < mTriangleOffsets.size()), "Invalid uv coords!");
        S32 binOffset = mTriangleOffsets[index];

        // Walk it.
        U16 curIdx = 0;
        while ((curIdx = mTriangleBuffer[binOffset++]) != 0xFFFF)
        {
            // Is the UV within the texcoords for this triangle?
            AssertFatal(binOffset < mTriangleBuffer.size(),
                "AtlasLightingQueryHelper::lookup - out of rang binOffset!");
            AssertFatal(curIdx + 2 < mChunk->mIndexCount,
                "AtlasLightingQueryHelper::lookup - out of range index!");

            const GFXAtlasVert2& a = mChunk->mVert[mChunk->mIndex[curIdx + 0]];
            const GFXAtlasVert2& b = mChunk->mVert[mChunk->mIndex[curIdx + 1]];
            const GFXAtlasVert2& c = mChunk->mVert[mChunk->mIndex[curIdx + 2]];

            Point2F bary;
            if (pointInTri(inUv, a.texCoord, b.texCoord, c.texCoord, bary))
            {
                // Interpolate position...
                Point3F posEdge1 = b.point - a.point;
                Point3F posEdge2 = c.point - a.point;
                outPos = a.point + posEdge1 * bary.x + posEdge2 * bary.y;

                // Interpolate normal...
                const Point3F& nA = mVertexNormals[mChunk->mIndex[curIdx + 0]];
                const Point3F& nB = mVertexNormals[mChunk->mIndex[curIdx + 1]];
                const Point3F& nC = mVertexNormals[mChunk->mIndex[curIdx + 2]];
                outNorm = nA + (nB - nA) * bary.x + (nC - nA) * bary.y;

                // Normalize for good measure!
                outNorm.normalize();

                // Note the triangle we hit!
                mLastHitTriangle = outTriIdx = curIdx;
                return true;
            }
        }

        return false;
    }
};

#endif
