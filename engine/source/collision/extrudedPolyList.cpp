//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "math/mMath.h"
#include "console/console.h"
#include "collision/extrudedPolyList.h"
#include "collision/polyhedron.h"
#include "collision/collision.h"


const F32 sgFrontEpsilon = 0.01;

//----------------------------------------------------------------------------

// use table for U64 shifts of the form: 1 << N
// because compiler makes it a function call if done directly...
U32 U32leftShift[33] =
{
   U32(1) << 0,
   U32(1) << 1,
   U32(1) << 2,
   U32(1) << 3,
   U32(1) << 4,
   U32(1) << 5,
   U32(1) << 6,
   U32(1) << 7,
   U32(1) << 8,
   U32(1) << 9,

   U32(1) << 10,
   U32(1) << 11,
   U32(1) << 12,
   U32(1) << 13,
   U32(1) << 14,
   U32(1) << 15,
   U32(1) << 16,
   U32(1) << 17,
   U32(1) << 18,
   U32(1) << 19,

   U32(1) << 20,
   U32(1) << 21,
   U32(1) << 22,
   U32(1) << 23,
   U32(1) << 24,
   U32(1) << 25,
   U32(1) << 26,
   U32(1) << 27,
   U32(1) << 28,
   U32(1) << 29,

   U32(1) << 30,
   U32(1) << 31,
   U32(0) // one more for good measure
};

// Minimum distance from a face
F32 ExtrudedPolyList::FaceEpsilon = 0.01;

// Value used to compare collision times
F32 ExtrudedPolyList::EqualEpsilon = 0.0001;

ExtrudedPolyList::ExtrudedPolyList()
{
    VECTOR_SET_ASSOCIATION(mVertexList);
    VECTOR_SET_ASSOCIATION(mIndexList);
    VECTOR_SET_ASSOCIATION(mExtrudedList);
    VECTOR_SET_ASSOCIATION(mPlaneList);
    VECTOR_SET_ASSOCIATION(mPolyPlaneList);

    mVelocity.set(0, 0, 0);
    mIndexList.reserve(128);
    mVertexList.reserve(64);
    mPolyPlaneList.reserve(64);
    mPlaneList.reserve(64);
    mCollisionList = 0;
}

ExtrudedPolyList::~ExtrudedPolyList()
{
}


//----------------------------------------------------------------------------

bool ExtrudedPolyList::isEmpty() const
{
    return mCollisionList->count == 0;
}


//----------------------------------------------------------------------------

void ExtrudedPolyList::extrude(const Polyhedron& pt, const VectorF& vector)
{
    // Clear state
    mIndexList.clear();
    mVertexList.clear();
    mPlaneList.clear();
    mPolyPlaneList.clear();
    mFaceShift = 0;

    // Determine which faces will be extruded.
    mExtrudedList.setSize(pt.planeList.size());
    for (U32 f = 0; f < pt.planeList.size(); f++) {
        const PlaneF& face = pt.planeList[f];
        ExtrudedFace& eface = mExtrudedList[f];
        F32 dot = mDot(face, vector);
        eface.active = dot > EqualEpsilon;
        if (eface.active) {
            eface.faceShift = FaceEpsilon / dot;
            eface.maxDistance = dot;
            eface.plane = face;
            eface.planeMask = U32leftShift[mPlaneList.size()]; // U64(1) << mPlaneList.size();

            // Add the face as a plane to clip against.
            mPlaneList.increment(2);
            PlaneF* plane = mPlaneList.end() - 2;
            plane[0] = plane[1] = face;
            plane[0].invert();
        }
    }

    // Produce extruded planes for bounding and internal edges
    for (U32 e = 0; e < pt.edgeList.size(); e++) {
        Polyhedron::Edge const& edge = pt.edgeList[e];
        ExtrudedFace& ef1 = mExtrudedList[edge.face[0]];
        ExtrudedFace& ef2 = mExtrudedList[edge.face[1]];
        if (ef1.active || ef2.active) {

            // Assumes that the edge points are clockwise
            // for face[0].
            const Point3F& p1 = pt.pointList[edge.vertex[1]];
            const Point3F& p2 = pt.pointList[edge.vertex[0]];
            Point3F p3 = p2 + vector;

            mPlaneList.increment(2);
            PlaneF* plane = mPlaneList.end() - 2;
            plane[0].set(p3, p2, p1);
            plane[1] = plane[0];
            plane[1].invert();

            U32 pmask = U32leftShift[mPlaneList.size() - 2]; // U64(1) << (mPlaneList.size()-2)
            ef1.planeMask |= pmask;
            ef2.planeMask |= pmask << 1;
        }
    }
}


//----------------------------------------------------------------------------

void ExtrudedPolyList::setCollisionList(CollisionList* info)
{
    mCollisionList = info;
    mCollisionList->count = 0;
    mCollisionList->t = 2;
}


//----------------------------------------------------------------------------

void ExtrudedPolyList::adjustCollisionTime()
{
    // Called after all the polys have been added.
    // There was a reason for doing it here instead of subtracting
    // the face Epsilon (faceShift) when the closest point is calculated,
    // but I can't remember what it is...
    if (mCollisionList->count) {
        mCollisionList->t -= mFaceShift;
        if (mCollisionList->t > 1)
            mCollisionList->t = 1;
        else
            if (mCollisionList->t < 0)
                mCollisionList->t = 0;
    }
}


//----------------------------------------------------------------------------

U32 ExtrudedPolyList::addPoint(const Point3F& p)
{
    mVertexList.increment();
    Vertex& v = mVertexList.last();

    v.point.x = p.x * mScale.x;
    v.point.y = p.y * mScale.y;
    v.point.z = p.z * mScale.z;
    mMatrix.mulP(v.point);

    // Build the plane mask, planes come in pairs
    v.mask = 0;
    for (U32 i = 0; i < mPlaneList.size(); i += 2)
    {
        F32 dist = mPlaneList[i].distToPlane(v.point);
        if (dist >= sgFrontEpsilon)
            v.mask |= U32leftShift[i];   // U64(1) << i;
        else
            v.mask |= U32leftShift[i + 1]; // U64(2) << i;
    }
    return mVertexList.size() - 1;
}


U32 ExtrudedPolyList::addPlane(const PlaneF& plane)
{
    mPolyPlaneList.increment();
    mPlaneTransformer.transform(plane, mPolyPlaneList.last());

    return mPolyPlaneList.size() - 1;
}


//----------------------------------------------------------------------------

void ExtrudedPolyList::begin(U32 material, U32 /*surfaceKey*/)
{
    mPoly.object = mCurrObject;
    mPoly.material = material;
    mIndexList.clear();
}

void ExtrudedPolyList::plane(U32 v1, U32 v2, U32 v3)
{
    mPoly.plane.set(mVertexList[v1].point,
        mVertexList[v2].point,
        mVertexList[v3].point);
}

void ExtrudedPolyList::plane(const PlaneF& p)
{
    mPlaneTransformer.transform(p, mPoly.plane);
}

void ExtrudedPolyList::plane(const U32 index)
{
    AssertFatal(index < mPolyPlaneList.size(), "Out of bounds index!");
    mPoly.plane = mPolyPlaneList[index];
}

const PlaneF& ExtrudedPolyList::getIndexedPlane(const U32 index)
{
    AssertFatal(index < mPolyPlaneList.size(), "Out of bounds index!");
    return mPolyPlaneList[index];
}


void ExtrudedPolyList::vertex(U32 vi)
{
    mIndexList.push_back(vi);
}

void ExtrudedPolyList::end()
{
    // Anything facing away from the mVelocity is rejected
    if (mCollisionList->count >= CollisionList::MaxCollisions ||
        mDot(mPoly.plane, mNormalVelocity) > 0)
        return;

    // Test the mPoly against the planes each extruded face.
    U32 cFaceCount = 0;
    ExtrudedFace* cFace[30];
    bool          cEdgeColl[30];
    ExtrudedFace* face = mExtrudedList.begin();
    ExtrudedFace* end = mExtrudedList.end();
    for (; face != end; face++)
    {
        if (face->active && (face->faceDot = -mDot(face->plane, mPoly.plane)) > 0)
        {
            if (testPoly(*face)) {
                cFace[cFaceCount] = face;
                cEdgeColl[cFaceCount++] = false;
            }
        }
    }
    if (!cFaceCount)
    {
        face = mExtrudedList.begin();
        end = mExtrudedList.end();
        for (; face != end; face++)
        {
            if (face->active && (face->faceDot = -mDot(face->plane, mPoly.plane)) <= 0)
            {
                if (testPoly(*face)) {
                    cFace[cFaceCount] = face;
                    cEdgeColl[cFaceCount++] = true;
                }
            }
        }
    }
    if (!cFaceCount)
        return;

    // Pick the best collision face
    face = cFace[0];
    bool edge = cEdgeColl[0];
    for (U32 f = 1; f < cFaceCount; f++)
    {
        if (cFace[f]->faceDot > face->faceDot)
        {
            face = cFace[f];
            edge = cEdgeColl[f];
        }
    }
    // Add it to the collision list if it's better and/or equal
    // to our current best.
    if (face->time <= mCollisionList->t + EqualEpsilon) {
        if (face->time < mCollisionList->t - EqualEpsilon) {
            mFaceShift = face->faceShift;
            mCollisionList->t = face->time;
            mCollisionList->count = 0;
            mCollisionList->maxHeight = face->height;
        }
        else {
            if (face->height > mCollisionList->maxHeight)
                mCollisionList->maxHeight = face->height;
            if (face->faceShift > mFaceShift)
                mFaceShift = face->faceShift;
        }
        Collision& collision = mCollisionList->collision[mCollisionList->count++];
        collision.point = face->point;
        collision.faceDot = face->faceDot;
        collision.face = face - mExtrudedList.begin();
        collision.object = mPoly.object;
        if (edge == false)
            collision.normal = mPoly.plane;
        else
            collision.normal = -face->plane;
        collision.material = mPoly.material;
    }
}


//----------------------------------------------------------------------------

bool ExtrudedPolyList::testPoly(ExtrudedFace& face)
{
    // Build intial inside/outside plane masks
    U32 indexStart = 0;
    U32 indexEnd = mIndexList.size();
    U32 oVertexSize = mVertexList.size();
    U32 oIndexSize = mIndexList.size();

    U32 frontMask = 0, backMask = 0;
    for (U32 i = indexStart; i < indexEnd; i++) {
        U32 mask = mVertexList[mIndexList[i]].mask & face.planeMask;
        frontMask |= mask;
        backMask |= ~mask;
    }

    // Clip the mPoly against the planes that bound the face...
    // Trivial accept if all the vertices are on the backsides of
    // all the planes.
    if (frontMask) {
        // Trivial reject if any plane not crossed has all it's points
        // on the front.
        U32 crossMask = frontMask & backMask;
        if (~crossMask & frontMask)
            return false;

        // Need to do some clipping
        for (U32 p = 0; p < mPlaneList.size(); p++) {
            U32 pmask = U32leftShift[p]; // U64(1) << p;
            U32 newStart = mIndexList.size();

            // Only test against this plane if we have something
            // on both sides
            if (face.planeMask & crossMask & pmask) {
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
                        for (U32 i = p + 1; i < mPlaneList.size(); i++) {
                            U32 mask = U32leftShift[i]; // U64(1) << i;
                            if (face.planeMask & mask &&
                                mPlaneList[i].distToPlane(iv.point) > 0) {
                                iv.mask = mask;
                                break;
                            }
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
                indexStart = newStart;
                indexEnd = mIndexList.size();
                if (mIndexList.size() - indexStart < 3) {
                    mVertexList.setSize(oVertexSize);
                    mIndexList.setSize(oIndexSize);
                    return false;
                }
            }
        }
    }

    // Find closest point on the mPoly
    Point3F bp;
    F32 bd = 1E30;
    F32 height = -1E30;
    for (U32 b = indexStart; b < indexEnd; b++) {
        Vertex& vertex = mVertexList[mIndexList[b]];
        F32 dist = face.plane.distToPlane(vertex.point);
        if (dist <= bd) {
            bd = (dist < 0) ? 0 : dist;
            bp = vertex.point;
        }
        // Since we don't clip against the back plane, we'll
        // only include vertex heights that are within range.
        if (vertex.point.z > height && dist < face.maxDistance)
            height = vertex.point.z;
    }

    // Remove temporary data
    mVertexList.setSize(oVertexSize);
    mIndexList.setSize(oIndexSize);

    // Add it to the collision list if it's better and/or equal
    // to our current best.
    if (bd < face.maxDistance + FaceEpsilon) {
        face.time = bd / face.maxDistance;
        face.height = height;
        face.point = bp;
        return true;
    }
    return false;
}


//----------------------------------------------------------------------------

void ExtrudedPolyList::render()
{
    /*
       if (!mCollisionList)
          return;

       glBegin(GL_LINES);
       glColor3f(1,1,0);

       for (U32 d = 0; d < mCollisionList->count; d++) {
          Collision& face = mCollisionList->collision[d];
          Point3F ep = face.point;
          ep += face.normal;
          glVertex3fv(face.point);
          glVertex3fv(ep);
       }

       glEnd();
    */
}

//--------------------------------------------------------------------------
void ExtrudedPolyList::setVelocity(const VectorF& velocity)
{
    mVelocity = velocity;
    if (velocity.isZero() == false)
    {
        mNormalVelocity = velocity;
        mNormalVelocity.normalize();
        setInterestNormal(mNormalVelocity);
    }
    else
    {
        mNormalVelocity.set(0, 0, 0);
        clearInterestNormal();
    }
}


