//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ATLASGEOMCOLLISION_H_
#define _ATLASGEOMCOLLISION_H_

#include "atlas/runtime/atlasInstanceGeomTOC.h"
#include "util/quadTreeTracer.h"
#include "collision/convex.h"

/// Trace rays against an AtlasGeomChunk.
///
/// @ingroup AtlasResource
class AtlasGeomChunkTracer : public QuadTreeTracer
{
protected:

    AtlasGeomChunk* mChunk;
    Point3F mRayDelta, mRayStart, mRayEnd, mScale, mOffset;

    const AtlasGeomChunk::ColNode* findSquare(const U32& level, const Point2I& pos) const
    {
        const U32 idx = getNodeIndex((mTreeDepth - 1) - level, Point2I(pos.x >> level, pos.y >> level));
        AssertFatal(idx < getNodeCount(mTreeDepth), "AtlasGeomTracer::findSquare - invalid chunk index calculated!");
        return mChunk->mColTree + idx;
    }

    virtual const F32 getSquareMin(const U32& level, const Point2I& pos) const
    {
        return findSquare(level, pos)->min;
    }

    virtual const F32 getSquareMax(const U32& level, const Point2I& pos) const
    {
        return findSquare(level, pos)->max;
    }

    virtual bool castLeafRay(const Point2I pos, const Point3F& start, const Point3F& end, const F32& startT, const F32& endT, RayInfo* info);

    bool castRayTriangle(Point3F orig, Point3F dir, Point3F vert0, Point3F vert1, Point3F vert2, F32& t, Point2F& bary);
public:

    AtlasGeomChunkTracer(AtlasGeomChunk* chunk)
        : QuadTreeTracer(chunk->mColTreeDepth)
    {
        AssertISV(chunk, "AtlasGeomChunkTracer::ctor - need a loaded chunk to work!");

        mChunk = chunk;
    }

    /// Perform a raycast against the quadtree. This expects positions in
    /// FILE space, not CHUNK space.
    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
};

/// Implementation of a triangle-based convex for Atlas collision. 
///
/// @ingroup AtlasResource
class AtlasConvex : public Convex
{
    friend class AtlasGeomChunk;

    /// The chunk that created us.
    AtlasGeomChunk* geom;

    /// Offset into that chunk's VB.
    U16     offset;

    Point3F point[4]; // Fourth point turns us into a goofy little tetrahedron.
    VectorF normal;

    /// Our bounding box.
    Box3F   box;

public:
    AtlasConvex()
    {
        mType = AtlasConvexType;
    }

    AtlasConvex(const AtlasConvex& cv)
    {
        mType = AtlasConvexType;

        // Do a partial copy.
        mObject = cv.mObject;
        point[0] = cv.point[0];
        point[1] = cv.point[1];
        point[2] = cv.point[2];
        point[3] = cv.point[3];
        normal = cv.normal;
        box = cv.box;
        offset = cv.offset;
        geom = cv.geom;
    }

    Box3F getBoundingBox() const;
    Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;
    Point3F support(const VectorF& v) const;
    void getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf);
    void getPolyList(AbstractPolyList* list);
};

#endif