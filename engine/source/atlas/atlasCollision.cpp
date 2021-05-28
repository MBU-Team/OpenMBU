//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
// Some portions of Atlas code derived from Thatcher Ulrich's original
// ChunkLOD implementation.
//-----------------------------------------------------------------------------

#include "atlas/atlasCollision.h"
#include "atlas/atlasInstance.h"
#include "atlas/atlasInstanceEntry.h"
#include "atlas/atlasResource.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "materials/sceneData.h"
#include "platform/profiler.h"
#include "util/frustrumCuller.h"
#include "util/quadTreeTracer.h"

bool AtlasGeomTracer::castRayTriangle(Point3F orig, Point3F dir,
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

bool AtlasGeomTracer::castLeafRay(const Point2I pos, const Point3F& start, const Point3F& end, const F32& startT, const F32& endT, RayInfo* info)
{
    if (AtlasInstance::smCollideGeomBoxes)
    {
        const F32 invSize = 1.f / F32(BIT(mTreeDepth - 1));

        // This is a bit of a hack. But good for testing.
        // Do collision against a box and return the result...
        F32 t; Point3F n;
        Box3F box;

        box.min.set(Point3F(F32(pos.x) * invSize, F32(pos.y) * invSize, getSquareMin(0, pos)));
        box.max.set(Point3F(F32(pos.x + 1) * invSize, F32(pos.y + 1) * invSize, getSquareMax(0, pos)));

        //Con::printf("   checking at xy = {%f, %f}->{%f, %f}, [%d, %d]", start.x, start.y, end.x, end.y, pos.x, pos.y);

        if (box.collideLine(start, end, &t, &n) && t >= startT && t <= endT)
        {
            info->object = NULL;
            info->t = t;
            info->normal = n;
            return true;
        }

        return false;
    }
    else
    {
        // Get the triangle list...
        U16* triOffset = mInfo->mColIndicesBuffer + mInfo->mColIndicesOffsets[pos.x * mData->mColGridSize + pos.y];

        // Store best hit results...
        bool gotHit = false;
        F32 bestT = F32_MAX;
        U16 bestTri, offset;

        while ((offset = *triOffset) != 0xFFFF)
        {
            // Advance to the next triangle..
            triOffset++;

            // Get each triangle, and do a raycast against it.
            Point3F a, b, c;

            a = mInfo->mColGeom[mInfo->mColIndices[offset + 0]];
            b = mInfo->mColGeom[mInfo->mColIndices[offset + 1]];
            c = mInfo->mColGeom[mInfo->mColIndices[offset + 2]];

            /*Con::printf("  o testing triangle %d ({%f,%f,%f},{%f,%f,%f},{%f,%f,%f})",
               offset, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z); */

            F32     localT;
            Point2F localBary;

            // Do the cast, using our conveniently precalculated ray delta...
            if (castRayTriangle(start, mRayDelta, a, b, c, localT, localBary))
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

                    gotHit = true;
                }
            }
            else
            {
                //Con::printf(" - didn't hit triangle %d at  t=%f", offset, localT);
            }
        }

        // Fill in extra info for the hit.
        if (gotHit)
        {
            // Calculate the normal, we skip that for the initial check.
            Point3F norm;

            // Hi norm!
            Point3F a, b, c;

            a = mInfo->mColGeom[mInfo->mColIndices[bestTri + 0]];
            b = mInfo->mColGeom[mInfo->mColIndices[bestTri + 1]];
            c = mInfo->mColGeom[mInfo->mColIndices[bestTri + 2]];

            // Space a/b/c out appropriately, lest the normal be messed up.
            a *= mScale;
            b *= mScale;
            c *= mScale;

            // Store everything relevant into the info structure.
            info->t = bestT;

            Point3F e0 = b - a;
            Point3F e1 = c - a;

            info->normal = mCross(e1, e0); //PlaneF(a,b,c);
            info->normal.normalize();

            AssertFatal(info->normal.z > 0, "Bad normal!");

            info->object = AtlasInstanceEntry::getInstance();

            // Return true, we hit something!
            return true;
        }

        // Nothing doing, fail.
        return false;
    }
}

bool AtlasGeomTracer::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
    // Don't collide if nothing's there...
    if (!mInfo->mColTree)
        return false;

    // Transform start and end into chunkspace. But this is already done in
    // AtlasResourceTracer::castLeafRay.

    // Cache the delta of the ray, to save us some per-tri vector math later on...
    mRayDelta = end - start;

    return QuadTreeTracer::castRay(start, end, info);
}

//------------------------------------------------------------------------------

bool AtlasResourceTracer::castLeafRay(const Point2I pos, const Point3F& start, const Point3F& end, const F32& startT, const F32& endT, RayInfo* info)
{
    const F32 invSize = 1.f / F32(BIT(mTreeDepth - 1));

    // Alright, we got to a leaf, so let's call its AtlasGeomTracer!
    const AtlasResourceEntry* ent = findSquare(0, pos);

    // This is a bit of a hack.
    // Do collision against a box and return the result...
    F32 t; Point3F n;
    Box3F box;

    if (AtlasInstance::smCollideChunkBoxes)
    {
        box.min.set(Point3F(F32(pos.x) * invSize, F32(pos.y) * invSize, ent->mBounds.min.z));
        box.max.set(Point3F(F32(pos.x + 1) * invSize, F32(pos.y + 1) * invSize, ent->mBounds.max.z));

        if (box.collideLine(start, end, &t, &n))
        {
            info->object = NULL;
            info->t = t;
            info->normal = n;
            return true;
        }

        return false;
    }
    else
    {

        if (ent->mGeom)
        {
            AtlasGeomTracer cgt(mData, ent->mGeom, ent); // This is really lame, we should
            // not be allocating on the fly.

            // The AtlasGeomTracer is in 0..1 space, so we have to translate and
            // scale the ray so that we end up intersecting the right ray against the
            // right spaces.

            // Say our chunk exists in the rectangle defined by Point2Fs min and max.
            // That means the ray must first have min subtracted from it. Then we
            // scale down by the size of the chunk (ie, max-min).

            // Also, we're already in 0..1 space for the whole Atlas, so we
            // actually have to upscale so that a chunk is a unit.

            const F32 objScale = F32(BIT(mTreeDepth - 1)) * mData->mBaseChunkSize;

            Point3F fixedStart = start;
            fixedStart.x *= objScale;
            fixedStart.y *= objScale;

            Point3F fixedEnd = end;
            fixedEnd.x *= objScale;
            fixedEnd.y *= objScale;

            // Now we just have to shift down by the chunk offset.
            Point3F shift(ent->mBounds.min.x, ent->mBounds.min.y, 0);
            fixedStart -= shift;
            fixedEnd -= shift;

            // And scale down so we fit in a unit square.
            fixedStart.x /= ent->mBounds.max.x - ent->mBounds.min.x;
            fixedStart.y /= ent->mBounds.max.y - ent->mBounds.min.y;
            fixedEnd.x /= ent->mBounds.max.x - ent->mBounds.min.x;
            fixedEnd.y /= ent->mBounds.max.y - ent->mBounds.min.y;

            // All done, fire it off!
            return cgt.castRay(fixedStart, fixedEnd, info);
        }

        return false;
    }
}

//------------------------------------------------------------------------------

Box3F AtlasChunkConvex::getBoundingBox() const
{
    // Transform and return the box in world space.
    Box3F worldBox = box;
    mObject->getTransform().mul(worldBox);
    return worldBox;
}

Box3F AtlasChunkConvex::getBoundingBox(const MatrixF&, const Point3F&) const
{
    // Function should not be called....
    AssertISV(false, "AtlasChunkConvex::getBoundingBox(m,p) - Don't call me! -- BJG");
    return box;
}

Point3F AtlasChunkConvex::support(const VectorF& v) const
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

void AtlasChunkConvex::getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf)
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


void AtlasChunkConvex::getPolyList(AbstractPolyList* list)
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

//------------------------------------------------------------------------------

bool AtlasResource::buildCollisionInfo(const Box3F& box, Convex* c, AbstractPolyList* poly)
{
    bool gotData = false;

    // Figure the range of chunks this covers...
    S32 xStart = mFloor(box.min.x / mBaseChunkSize);
    S32 yStart = mFloor(box.min.y / mBaseChunkSize);
    S32 xEnd = mCeil(box.max.x / mBaseChunkSize);
    S32 yEnd = mCeil(box.max.y / mBaseChunkSize);

    // Clamp only to chunks that exist!
    xStart = mClamp(xStart, 0, BIT(mTreeDepth - 1));
    yStart = mClamp(yStart, 0, BIT(mTreeDepth - 1));
    xEnd = mClamp(xEnd, 0, BIT(mTreeDepth - 1));
    yEnd = mClamp(yEnd, 0, BIT(mTreeDepth - 1));

    // Ok, iterate over this range and let each chunk register any convexes.
    for (S32 i = xStart; i < xEnd; i++)
    {
        for (S32 j = yStart; j < yEnd; j++)
        {
            const Point2I pos(i, j);
            const U32 nodeID = QuadTreeTracer::getNodeIndex(mTreeDepth - 1, Point2I(pos.y, pos.x));

            // Get the chunk...
            AtlasResourceEntry* e = mChunkTable[nodeID];
            AssertFatal(e->mGeom && e->mGeom->mColTree,
                "AtlasResource::buildCollisionInfo - no col tree on leaf node?!?!");

            // Check the bounding box Z-range...
            if (
                (box.max.z > e->mBounds.max.z && box.min.z > e->mBounds.max.z)
                ||
                (box.max.z < e->mBounds.min.z && box.min.z < e->mBounds.min.z)
                )
                continue;

            // Move the box into chunkspace...
            Box3F localBox = box;

            localBox.min.x /= mBaseChunkSize;
            localBox.min.y /= mBaseChunkSize;
            localBox.max.x /= mBaseChunkSize;
            localBox.max.y /= mBaseChunkSize;

            localBox.min.x -= F32(pos.x);
            localBox.min.y -= F32(pos.y);
            localBox.max.x -= F32(pos.x);
            localBox.max.y -= F32(pos.y);

            // Let the ChunkGeomInfo deal with collision...
            if (e->hasResidentGeometryData())
            {
                // Figure the local matrix of this chunk..
                MatrixF chunkMat(1);

                // It goes, chunk->object.
                chunkMat.scale(VectorF(mBaseChunkSize, mBaseChunkSize, 1.f));
                chunkMat.setPosition(Point3F(
                    F32(pos.x) * mBaseChunkSize,
                    F32(pos.y) * mBaseChunkSize,
                    0));

                // Do the deed!
                AtlasResource::setCurrentResource(this);
                gotData |= e->mGeom->buildCollisionInfo(&chunkMat, localBox, c, poly);
                AtlasResource::setCurrentResource(NULL);
            }
        }
    }

    return gotData;
}
