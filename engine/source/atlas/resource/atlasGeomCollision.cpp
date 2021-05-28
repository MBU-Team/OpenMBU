//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "atlas/resource/atlasGeomCollision.h"
#include "atlas/runtime/atlasInstance2.h"

bool AtlasGeomChunkTracer::castRayTriangle(Point3F orig, Point3F dir,
    Point3F vert0, Point3F vert1, Point3F vert2,
    F32& t, Point2F& bary)
{
    Point3F tvec, qvec;

    // Find vectors for two edges sharing vert0
    const Point3F edge1 = vert1 - vert0;
    const Point3F edge2 = vert2 - vert0;

    // Begin calculating determinant - also used to calculate U parameter.
    const Point3F pvec = mCross(dir, edge2);

    // If determinant is near zero, ray lies in plane of triangle.
    const F32 det = mDot(edge1, pvec);

    if (det > 0.00001)
    {
        // calculate distance from vert0 to ray origin
        tvec = orig - vert0;

        // calculate U parameter and test bounds
        bary.x = mDot(tvec, pvec); // bary.x is really bary.u...
        if (bary.x < 0.0 || bary.x > det)
            return false;

        // prepare to test V parameter
        qvec = mCross(tvec, edge1);

        // calculate V parameter and test bounds
        bary.y = mDot(dir, qvec); // bary.y is really bary.v
        if (bary.y < 0.0 || (bary.x + bary.y) > det)
            return false;

    }
    else if (det < -0.00001)
    {
        // calculate distance from vert0 to ray origin
        tvec = orig - vert0;

        // calculate U parameter and test bounds
        bary.x = mDot(tvec, pvec);
        if (bary.x > 0.0 || bary.x < det)
            return false;

        // prepare to test V parameter
        qvec = mCross(tvec, edge1);

        // calculate V parameter and test bounds
        bary.y = mDot(dir, qvec);
        if (bary.y > 0.0 || (bary.x + bary.y) < det)
            return false;
    }
    else
        return false;  // ray is parallel to the plane of the triangle.

    const F32 inv_det = 1.0 / det;

    // calculate t, ray intersects triangle
    t = mDot(edge2, qvec) * inv_det;
    bary *= inv_det;

    //AssertFatal((t >= 0.f && t <=1.f), "AtlasGeomTracer::castRayTriangle - invalid t!");

    // Hack, check the math here!
    return (t >= 0.f && t <= 1.f);
}

bool AtlasGeomChunkTracer::castLeafRay(const Point2I pos, const Point3F& start,
    const Point3F& end, const F32& startT,
    const F32& endT, RayInfo* info)
{
    if (AtlasInstance2::smRayCollisionDebugLevel == AtlasInstance2::RayCollisionDebugToColTree)
    {
        const F32 invSize = 1.f / F32(BIT(mTreeDepth - 1));

        // This is a bit of a hack. But good for testing.
        // Do collision against the collision tree leaf bounding box and return the result...
        F32 t; Point3F n;
        Box3F box;

        box.min.set(Point3F(F32(pos.x) * invSize, F32(pos.y) * invSize, getSquareMin(0, pos)));
        box.max.set(Point3F(F32(pos.x + 1) * invSize, F32(pos.y + 1) * invSize, getSquareMax(0, pos)));

        //Con::printf("   checking at xy = {%f, %f}->{%f, %f}, [%d, %d]", start.x, start.y, end.x, end.y, pos.x, pos.y);

        if (box.collideLine(start, end, &t, &n) && t >= startT && t <= endT)
        {
            info->t = t;
            info->normal = n;
            return true;
        }

        return false;
    }
    else
    {
        // Get the triangle list...
        U16* triOffset = mChunk->mColIndicesBuffer +
            mChunk->mColIndicesOffsets[pos.x * BIT(mChunk->mColTreeDepth - 1) + pos.y];

        // Store best hit results...
        bool gotHit = false;
        F32 bestT = F32_MAX;
        U16 bestTri, offset;
        Point2F bestBary;

        while ((offset = *triOffset) != 0xFFFF)
        {
            // Advance to the next triangle..
            triOffset++;

            // Get each triangle, and do a raycast against it.
            Point3F a, b, c;

            AssertFatal(offset < mChunk->mIndexCount,
                "AtlasGeomTracer2::castLeafRay - offset past end of index list.");

            a = mChunk->mVert[mChunk->mIndex[offset + 0]].point;
            b = mChunk->mVert[mChunk->mIndex[offset + 1]].point;
            c = mChunk->mVert[mChunk->mIndex[offset + 2]].point;

            /*Con::printf("  o testing triangle %d ({%f,%f,%f},{%f,%f,%f},{%f,%f,%f})",
                                 offset, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z); */

            F32     localT;
            Point2F localBary;

            // Do the cast, using our conveniently precalculated ray delta...
            if (castRayTriangle(mRayStart, mRayDelta, a, b, c, localT, localBary))
            {
                //Con::printf(" - hit triangle %d at  t=%f", offset, localT);

                // The ray intersected, but we have to make sure we hit actually on
                // the line segment. (ie, a ray isn't a ray, Ray.)

                // BJGTODO - This should prevent some nasty edge cases, but we
                //           seem to be calculating the wrong start and end T's.
                //           So I've disabled this for now but it will cause
                //           problems later!
                //if(localT < startT || localT > endT)
                //   continue;

                // It really, really hit, wow!
                if (localT < bestT)
                {
                    // And it hit before anything else we've seen.
                    bestTri = offset;
                    bestT = localT;
                    bestBary = localBary;

                    gotHit = true;
                }
            }
            else
            {
                //Con::printf(" - didn't hit triangle %d at  t=%f", offset, localT);
            }
        }

        // Fill in extra info for the hit.
        if (!gotHit)
            return false;

        // Calculate the normal, we skip that for the initial check.
        Point3F norm; // Hi norm!

        const Point3F& a = mChunk->mVert[mChunk->mIndex[bestTri + 0]].point;
        const Point3F& b = mChunk->mVert[mChunk->mIndex[bestTri + 1]].point;
        const Point3F& c = mChunk->mVert[mChunk->mIndex[bestTri + 2]].point;

        const Point2F& aTC = mChunk->mVert[mChunk->mIndex[bestTri + 0]].texCoord;
        const Point2F& bTC = mChunk->mVert[mChunk->mIndex[bestTri + 1]].texCoord;
        const Point2F& cTC = mChunk->mVert[mChunk->mIndex[bestTri + 2]].texCoord;

        // Store everything relevant into the info structure.
        info->t = bestT;

        const Point3F e0 = b - a;
        const Point3F e1 = c - a;

        info->normal = mCross(e1, e0);
        info->normal.normalize();

        // Calculate and store the texture coords.
        const Point2F e0TC = bTC - aTC;
        const Point2F e1TC = cTC - aTC;
        info->texCoord = e0TC * bestBary.x + e1TC * bestBary.y + aTC;

        // Return true, we hit something!
        return true;
    }
}

bool AtlasGeomChunkTracer::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
    // Don't collide if nothing's there...
    if (!mChunk->mColTree)
        return false;

    // Do our tracing math in 0..1 space, but still need filespace coordinates
    // as that's how our geometry is currently stored. So let's store off the
    // values pass, then convert them to chunkspace (0..1) so we can process
    // them normally.

    // Cache the delta of the ray, to save us some per-tri vector math later on...
    mRayDelta = end - start;
    mRayStart = start;
    mRayEnd = end;

    // Figure scale and offset to get to chunkspace from filespace.
    mScale.set(1.0 / mChunk->mBounds.len_x(), 1.0 / mChunk->mBounds.len_y(), 1.0);
    mOffset = -mChunk->mBounds.min;
    mOffset.z = 0;

    // Now, scale down to chunkspace, and cast the ray!
    Point3F adjStart = (start + mOffset);
    adjStart.convolve(mScale);

    Point3F adjEnd = (end + mOffset);
    adjEnd.convolve(mScale);

    return QuadTreeTracer::castRay(adjStart, adjEnd, info);
}

//------------------------------------------------------------------------------

Box3F AtlasConvex::getBoundingBox() const
{
    // Transform and return the box in world space.
    Box3F worldBox = box;
    mObject->getTransform().mul(worldBox);
    return worldBox;
}

Box3F AtlasConvex::getBoundingBox(const MatrixF&, const Point3F&) const
{
    // Function should not be called....
    AssertISV(false, "AtlasChunkConvex::getBoundingBox(m,p) - Don't call me! -- BJG");
    return box;
}

Point3F AtlasConvex::support(const VectorF& v) const
{
    // Calculate support vector...
    F32 bestDot = mDot(point[0], v);
    const Point3F* bestP = &point[0];
    for (S32 i = 1; i < 4; i++)
    {
        F32 newD = mDot(point[i], v);
        if (newD > bestDot)
        {
            bestDot = newD;
            bestP = &point[i];
        }
    }

    return *bestP;
}

void AtlasConvex::getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf)
{
    U32 i;
    cf->material = 0;
    cf->object = mObject;

    // For a tetrahedron this is pretty easy.

    //    points...
    S32 firstVert = cf->mVertexList.size();
    cf->mVertexList.increment(); mat.mulP(point[0], &cf->mVertexList.last());
    cf->mVertexList.increment(); mat.mulP(point[1], &cf->mVertexList.last());
    cf->mVertexList.increment(); mat.mulP(point[2], &cf->mVertexList.last());
    cf->mVertexList.increment(); mat.mulP(point[3], &cf->mVertexList.last());

    //    edges...
    cf->mEdgeList.increment();
    cf->mEdgeList.last().vertex[0] = firstVert + 0;
    cf->mEdgeList.last().vertex[1] = firstVert + 1;

    cf->mEdgeList.increment();
    cf->mEdgeList.last().vertex[0] = firstVert + 1;
    cf->mEdgeList.last().vertex[1] = firstVert + 2;

    cf->mEdgeList.increment();
    cf->mEdgeList.last().vertex[0] = firstVert + 2;
    cf->mEdgeList.last().vertex[1] = firstVert + 0;

    cf->mEdgeList.increment();
    cf->mEdgeList.last().vertex[0] = firstVert + 3;
    cf->mEdgeList.last().vertex[1] = firstVert + 0;

    cf->mEdgeList.increment();
    cf->mEdgeList.last().vertex[0] = firstVert + 3;
    cf->mEdgeList.last().vertex[1] = firstVert + 1;

    cf->mEdgeList.increment();
    cf->mEdgeList.last().vertex[0] = firstVert + 3;
    cf->mEdgeList.last().vertex[1] = firstVert + 2;

    //    triangles...
    cf->mFaceList.increment();
    cf->mFaceList.last().normal;
    mat.mulV(normal, &cf->mFaceList.last().normal);
    cf->mFaceList.last().vertex[0] = firstVert + 0;
    cf->mFaceList.last().vertex[1] = firstVert + 1;
    cf->mFaceList.last().vertex[2] = firstVert + 2;

    cf->mFaceList.increment();
    mat.mulV(PlaneF(point[0], point[1], point[3]), &cf->mFaceList.last().normal);
    cf->mFaceList.last().vertex[0] = firstVert + 3;
    cf->mFaceList.last().vertex[1] = firstVert + 0;
    cf->mFaceList.last().vertex[2] = firstVert + 1;

    cf->mFaceList.increment();
    mat.mulV(PlaneF(point[1], point[2], point[3]), &cf->mFaceList.last().normal);
    cf->mFaceList.last().vertex[0] = firstVert + 3;
    cf->mFaceList.last().vertex[1] = firstVert + 1;
    cf->mFaceList.last().vertex[2] = firstVert + 2;

    cf->mFaceList.increment();
    mat.mulV(PlaneF(point[2], point[0], point[3]), &cf->mFaceList.last().normal);
    cf->mFaceList.last().vertex[0] = firstVert + 3;
    cf->mFaceList.last().vertex[1] = firstVert + 0;
    cf->mFaceList.last().vertex[2] = firstVert + 2;

    // All done!
}


void AtlasConvex::getPolyList(AbstractPolyList* list)
{
    list->setTransform(&mObject->getTransform(), mObject->getScale());
    list->setObject(mObject);

    S32 v[3];

    v[0] = list->addPoint(point[0]);
    v[1] = list->addPoint(point[1]);
    v[2] = list->addPoint(point[2]);

    list->begin(0, offset);
    list->vertex(v[0]);
    list->vertex(v[1]);
    list->vertex(v[2]);
    list->plane(v[0], v[1], v[2]);
    list->end();
}
