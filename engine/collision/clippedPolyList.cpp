//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "math/mMath.h"
#include "console/console.h"
#include "collision/clippedPolyList.h"


bool ClippedPolyList::allowClipping = true;


//----------------------------------------------------------------------------

ClippedPolyList::ClippedPolyList()
{
    VECTOR_SET_ASSOCIATION(mPolyList);
    VECTOR_SET_ASSOCIATION(mVertexList);
    VECTOR_SET_ASSOCIATION(mIndexList);
    VECTOR_SET_ASSOCIATION(mPolyPlaneList);
    VECTOR_SET_ASSOCIATION(mPlaneList);

    mNormal.set(0, 0, 0);
    mIndexList.reserve(100);
}

ClippedPolyList::~ClippedPolyList()
{
}


//----------------------------------------------------------------------------

void ClippedPolyList::clear()
{
    // Only clears internal data
    mPolyList.clear();
    mVertexList.clear();
    mIndexList.clear();
    mPolyPlaneList.clear();
}

bool ClippedPolyList::isEmpty() const
{
    return mPolyList.size() == 0;
}


//----------------------------------------------------------------------------

U32 ClippedPolyList::addPoint(const Point3F& p)
{
    mVertexList.increment();
    Vertex& v = mVertexList.last();
    v.point.x = p.x * mScale.x;
    v.point.y = p.y * mScale.y;
    v.point.z = p.z * mScale.z;
    mMatrix.mulP(v.point);

    // Build the plane mask
    register U32      mask = 1;
    register S32      count = mPlaneList.size();
    register PlaneF* plane = mPlaneList.address();

    v.mask = 0;
    while (--count >= 0) {
        if (plane++->distToPlane(v.point) > 0)
            v.mask |= mask;
        mask <<= 1;
    }

    return mVertexList.size() - 1;
}


U32 ClippedPolyList::addPlane(const PlaneF& plane)
{
    mPolyPlaneList.increment();
    mPlaneTransformer.transform(plane, mPolyPlaneList.last());

    return mPolyPlaneList.size() - 1;
}


//----------------------------------------------------------------------------

void ClippedPolyList::begin(U32 material, U32 surfaceKey)
{
    mPolyList.increment();
    Poly& poly = mPolyList.last();
    poly.object = mCurrObject;
    poly.material = material;
    poly.vertexStart = mIndexList.size();
    poly.surfaceKey = surfaceKey;

    poly.polyFlags = 0;
    if (ClippedPolyList::allowClipping)
        poly.polyFlags = CLIPPEDPOLYLIST_FLAG_ALLOWCLIPPING;
}


//----------------------------------------------------------------------------

void ClippedPolyList::plane(U32 v1, U32 v2, U32 v3)
{
    mPolyList.last().plane.set(mVertexList[v1].point,
        mVertexList[v2].point, mVertexList[v3].point);
}

void ClippedPolyList::plane(const PlaneF& p)
{
    mPlaneTransformer.transform(p, mPolyList.last().plane);
}

void ClippedPolyList::plane(const U32 index)
{
    AssertFatal(index < mPolyPlaneList.size(), "Out of bounds index!");
    mPolyList.last().plane = mPolyPlaneList[index];
}

const PlaneF& ClippedPolyList::getIndexedPlane(const U32 index)
{
    AssertFatal(index < mPolyPlaneList.size(), "Out of bounds index!");
    return mPolyPlaneList[index];
}


//----------------------------------------------------------------------------

void ClippedPolyList::vertex(U32 vi)
{
    mIndexList.push_back(vi);
}


//----------------------------------------------------------------------------

void ClippedPolyList::end()
{
    Poly& poly = mPolyList.last();

    // Anything facing away from the mNormal is rejected
    if (mDot(poly.plane, mNormal) > 0) {
        mIndexList.setSize(poly.vertexStart);
        mPolyList.decrement();
        return;
    }

    // Build initial inside/outside plane masks
    U32 indexStart = poly.vertexStart;
    U32 vertexCount = mIndexList.size() - indexStart;

    U32 frontMask = 0, backMask = 0;
    U32 i;
    for (i = indexStart; i < mIndexList.size(); i++) {
        U32 mask = mVertexList[mIndexList[i]].mask;
        frontMask |= mask;
        backMask |= ~mask;
    }

    // Trivial accept if all the vertices are on the backsides of
    // all the planes.
    if (!frontMask) {
        poly.vertexCount = vertexCount;
        return;
    }

    // Trivial reject if any plane not crossed has all it's points
    // on the front.
    U32 crossMask = frontMask & backMask;
    if (~crossMask & frontMask) {
        mIndexList.setSize(poly.vertexStart);
        mPolyList.decrement();
        return;
    }

    // Need to do some clipping
    for (U32 p = 0; p < mPlaneList.size(); p++) {
        U32 pmask = 1 << p;
        // Only test against this plane if we have something
        // on both sides
        if (crossMask & pmask) {
            U32 indexEnd = mIndexList.size();
            U32 i1 = indexEnd - 1;
            U32 mask1 = mVertexList[mIndexList[i1]].mask;

            for (U32 i2 = indexStart; i2 < indexEnd; i2++) {
                U32 mask2 = mVertexList[mIndexList[i2]].mask;
                if ((mask1 ^ mask2) & pmask) {
                    //
                    mVertexList.increment();
                    VectorF& v1 = mVertexList[mIndexList[i1]].point;
                    VectorF& v2 = mVertexList[mIndexList[i2]].point;
                    VectorF vv = v2 - v1;
                    F32 t = -mPlaneList[p].distToPlane(v1) / mDot(mPlaneList[p], vv);

                    mIndexList.push_back(mVertexList.size() - 1);
                    Vertex& iv = mVertexList.last();
                    iv.point.x = v1.x + vv.x * t;
                    iv.point.y = v1.y + vv.y * t;
                    iv.point.z = v1.z + vv.z * t;
                    iv.mask = 0;

                    // Test against the remaining planes
                    for (U32 i = p + 1; i < mPlaneList.size(); i++)
                        if (mPlaneList[i].distToPlane(iv.point) > 0) {
                            iv.mask = 1 << i;
                            break;
                        }
                }
                if (!(mask2 & pmask)) {
                    U32 index = mIndexList[i2];
                    mIndexList.push_back(index);
                }
                mask1 = mask2;
                i1 = i2;
            }

            // Check for degenerate
            indexStart = indexEnd;
            if (mIndexList.size() - indexStart < 3) {
                mIndexList.setSize(poly.vertexStart);
                mPolyList.decrement();
                return;
            }
        }
    }

    // Emit what's left and compress the index list.
    poly.vertexCount = mIndexList.size() - indexStart;
    memcpy(&mIndexList[poly.vertexStart],
        &mIndexList[indexStart], poly.vertexCount);
    mIndexList.setSize(poly.vertexStart + poly.vertexCount);
}


//----------------------------------------------------------------------------

void ClippedPolyList::memcpy(U32* dst, U32* src, U32 size)
{
    U32* end = src + size;
    while (src != end)
        *dst++ = *src++;
}


//----------------------------------------------------------------------------

void ClippedPolyList::render()
{
    /*
       glVertexPointer(3,GL_FLOAT,sizeof(Vertex),mVertexList.address());
       glEnableClientState(GL_VERTEX_ARRAY);
       glColor4f(1,0,0,0.25);
       glEnable(GL_BLEND);
       glDisable(GL_CULL_FACE);
       glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

       Poly * p;
       for (p = mPolyList.begin(); p < mPolyList.end(); p++) {
          glDrawElements(GL_POLYGON,p->vertexCount,
             GL_UNSIGNED_INT,&mIndexList[p->vertexStart]);
       }

       glColor3f(0.6,0.6,0.6);
       glDisable(GL_BLEND);
       for (p = mPolyList.begin(); p < mPolyList.end(); p++) {
          glDrawElements(GL_LINE_LOOP,p->vertexCount,
             GL_UNSIGNED_INT,&mIndexList[p->vertexStart]);
       }

       glDisableClientState(GL_VERTEX_ARRAY);
    */
}
