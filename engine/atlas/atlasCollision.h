//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#ifndef _ATLASCOLLISION_
#define _ATLASCOLLISION_

#include "atlas/atlasResource.h"
#include "atlas/atlasResourceEntry.h"
#include "atlas/atlasHeightfield.h"
#include "util/quadTreeTracer.h"

#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif

class AtlasGeomTracer : public QuadTreeTracer
{
protected:

    const AtlasResourceEntry* mEntry;
    AtlasResourceInfo* mInfo;
    AtlasResource* mData;

    Point3F mScale;
    Point3F mRayDelta;

    const AtlasResourceInfo::ColNode* findSquare(const U32& level, const Point2I& pos) const
    {
        const U32 idx = getNodeIndex((mTreeDepth - 1) - level, Point2I(pos.x >> level, pos.y >> level));
        AssertFatal(idx < getNodeCount(mTreeDepth), "AtlasGeomTracer::findSquare - invalid chunk index calculated!");
        return mInfo->mColTree + idx;
    }

    virtual const F32 getSquareMin(const U32& level, const Point2I& pos) const
    {
        return findSquare(level, pos)->min * mData->mVerticalScale;
    }

    virtual const F32 getSquareMax(const U32& level, const Point2I& pos) const
    {
        return findSquare(level, pos)->max * mData->mVerticalScale;
    }

    virtual bool castLeafRay(const Point2I pos, const Point3F& start, const Point3F& end, const F32& startT, const F32& endT, RayInfo* info);

    bool castRayTriangle(Point3F orig, Point3F dir, Point3F vert0, Point3F vert1, Point3F vert2, F32& t, Point2F& bary);
public:

    AtlasGeomTracer(AtlasResource* aData, AtlasResourceInfo* aGeom, const AtlasResourceEntry* aEntry)
        : QuadTreeTracer(aData->mColTreeDepth)
    {
        mData = aData;
        mInfo = aGeom;
        mEntry = aEntry;

        //  Since our points go from -0.5 to 0.5, we have to remember to scale...
        Point3F size = mEntry->mBounds.max - mEntry->mBounds.min;
        mScale = size / 2;
        mScale.z = 1;
    }

    bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);

};

/// Performs castRays for a AtlasResource. Ultimately calls into AtlasGeomTracer.
class AtlasResourceTracer : public QuadTreeTracer
{
protected:
    AtlasResource* mData;

    const AtlasResourceEntry* findSquare(const U32& level, const Point2I& pos) const
    {
        const U32 idx = getNodeIndex((mTreeDepth - 1) - level, Point2I(pos.y >> level, pos.x >> level));
        AssertFatal(idx < mData->mEntryCount, "AtlasResourceTracer::findSquare - invalid chunk index calculated!");
        return mData->mChunkTable[idx];
    }

    virtual const F32 getSquareMin(const U32& level, const Point2I& pos) const
    {
        return findSquare(level, pos)->mBounds.min.z;
    }

    virtual const F32 getSquareMax(const U32& level, const Point2I& pos) const
    {
        return findSquare(level, pos)->mBounds.max.z;
    }

    virtual bool castLeafRay(const Point2I pos, const Point3F& start, const Point3F& end, const F32& startT, const F32& endT, RayInfo* info);
public:

    AtlasResourceTracer(AtlasResource* aData)
        : QuadTreeTracer(aData->mTreeDepth)
    {
        mData = aData;
    }
};


class AtlasChunkConvex : public Convex
{
    friend class AtlasResourceInfo;

    AtlasResourceInfo* geom;
    U16     offset;
    Point3F point[4]; // Forth point turns us into a goofy little tetrahedron.
    VectorF normal;
    Box3F   box;

public:
    AtlasChunkConvex()
    {
        mType = AtlasChunkConvexType;
    }

    AtlasChunkConvex(const AtlasChunkConvex& cv)
    {
        mType = AtlasChunkConvexType;

        // Do a partial copy.
        mObject = cv.mObject;
        point[0] = cv.point[0];
        point[1] = cv.point[1];
        point[2] = cv.point[2];
        point[3] = cv.point[3];
        normal = cv.normal;
        box = cv.box;
        offset = cv.offset;
    }

    Box3F getBoundingBox() const;
    Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;
    Point3F support(const VectorF& v) const;
    void getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf);
    void getPolyList(AbstractPolyList* list);
};

#endif