//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mMath.h"
#include "console/console.h"
#include "collision/optimizedPolyList.h"
#include "core/color.h"


//----------------------------------------------------------------------------

OptimizedPolyList::OptimizedPolyList()
{
    VECTOR_SET_ASSOCIATION(mPolyList);
    VECTOR_SET_ASSOCIATION(mVertexList);
    VECTOR_SET_ASSOCIATION(mIndexList);
    VECTOR_SET_ASSOCIATION(mPlaneList);
    VECTOR_SET_ASSOCIATION(mEdgeList);

    mIndexList.reserve(100);
}

OptimizedPolyList::~OptimizedPolyList()
{
    mPolyList.clear();
    mVertexList.clear();
    mIndexList.clear();
    mPlaneList.clear();
    mEdgeList.clear();
}


//----------------------------------------------------------------------------
void OptimizedPolyList::clear()
{
    // Only clears internal data
    mPolyList.clear();
    mVertexList.clear();
    mIndexList.clear();
    mPlaneList.clear();
    mEdgeList.clear();
    mPolyPlaneList.clear();
}

//----------------------------------------------------------------------------

U32 OptimizedPolyList::addPoint(const Point3F& p)
{
    Point3F v;
    v.x = p.x * mScale.x;
    v.y = p.y * mScale.y;
    v.z = p.z * mScale.z;
    mMatrix.mulP(v);

    for (U32 i = 0; i < mVertexList.size(); i++)
    {
        if (isEqual(v, mVertexList[i]))
            return i;
    }

    // If we make it to here then we didn't find the vertex
    mVertexList.push_back(v);

    return mVertexList.size() - 1;
}


U32 OptimizedPolyList::addPlane(const PlaneF& plane)
{
    PlaneF pln;
    mPlaneTransformer.transform(plane, pln);

    for (U32 i = 0; i < mPlaneList.size(); i++)
    {
        // The PlaneF == operator uses the Point3F == operator and
        // doesn't check the d
        if (isEqual(pln, mPlaneList[i]) &&
            mFabs(pln.d - mPlaneList[i].d) < DEV)
            return i;
    }

    // If we made it here then we didin't find the vertex
    mPlaneList.push_back(pln);

    return mPlaneList.size() - 1;
}


//----------------------------------------------------------------------------

void OptimizedPolyList::begin(U32 material, U32 surfaceKey)
{
    mPolyList.increment();
    Poly& poly = mPolyList.last();
    poly.object = mCurrObject;
    poly.material = material;
    poly.vertexStart = mIndexList.size();
    poly.surfaceKey = surfaceKey;
}


//----------------------------------------------------------------------------

void OptimizedPolyList::plane(U32 v1, U32 v2, U32 v3)
{
    AssertFatal(v1 < mVertexList.size() && v2 < mVertexList.size() && v3 < mVertexList.size(),
        "OptimizedPolyList::plane(): Vertex indices are larger than vertex list size");

    mPolyList.last().plane = addPlane(PlaneF(mVertexList[v1], mVertexList[v2], mVertexList[v3]));
}

void OptimizedPolyList::plane(const PlaneF& p)
{
    mPolyList.last().plane = addPlane(p);
}

void OptimizedPolyList::plane(const U32 index)
{
    AssertFatal(index < mPlaneList.size(), "Out of bounds index!");
    mPolyList.last().plane = index;
}

const PlaneF& OptimizedPolyList::getIndexedPlane(const U32 index)
{
    AssertFatal(index < mPlaneList.size(), "Out of bounds index!");
    return mPlaneList[index];
}


//----------------------------------------------------------------------------

void OptimizedPolyList::vertex(U32 vi)
{
    mIndexList.push_back(vi);
}


//----------------------------------------------------------------------------

bool OptimizedPolyList::isEmpty() const
{
    return !mPolyList.size();
}

void OptimizedPolyList::end()
{
    Poly& poly = mPolyList.last();
    poly.vertexCount = mIndexList.size() - poly.vertexStart;
}


void OptimizedPolyList::render()
{
    /*
       if (mPolyList.size() == 0 || mVertexList.size() == 0)
          return;

       //glVertexPointer(3, GL_FLOAT,sizeof(Point3F), mVertexList.address());
       //glEnableClientState(GL_VERTEX_ARRAY);

       //Poly * p;
       //for (p = mPolyList.begin(); p < mPolyList.end(); p++)
       //   glDrawElements(GL_POLYGON,p->vertexCount, GL_UNSIGNED_INT, &mIndexList[p->vertexStart]);

       //glDisableClientState(GL_VERTEX_ARRAY);

       // If you need normals
       for (U32 i = 0; i < mPolyList.size(); i++)
       {
          if (mPolyList[i].vertexCount < 3)
             continue;

          ColorF color(gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f), gRandGen.randF(0.0f, 1.0f));
          glColor3f(color.red, color.green, color.blue);
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);

          glBegin(GL_TRIANGLES);
             for (U32 j = 1; j < mPolyList[i].vertexCount - 1; j++)
             {
                U32 i0 = mIndexList[mPolyList[i].vertexStart];
                U32 i1 = mIndexList[mPolyList[i].vertexStart + j];
                U32 i2 = mIndexList[mPolyList[i].vertexStart + j + 1];

                Point3F p0 = mVertexList[i0];
                Point3F p1 = mVertexList[i1];
                Point3F p2 = mVertexList[i2];

                PlaneF normal = mPlaneList[mPolyList[i].plane];
                glNormal3f(normal.x, normal.y, normal.z);

                glVertex3f(p0.x, p0.y, p0.z);
                glVertex3f(p1.x, p1.y, p1.z);
                glVertex3f(p2.x, p2.y, p2.z);
             }
          glEnd();
       }
    */
}

void OptimizedPolyList::copyPolyToList(OptimizedPolyList* target, U32 pdx) const
{
    target->mPolyList.increment();
    OptimizedPolyList::Poly& tpoly = target->mPolyList.last();

    tpoly.material = mPolyList[pdx].material;
    tpoly.object = mPolyList[pdx].object;
    tpoly.surfaceKey = mPolyList[pdx].surfaceKey;
    tpoly.vertexCount = mPolyList[pdx].vertexCount;

    PlaneF pln = mPlaneList[mPolyList[pdx].plane];
    tpoly.plane = target->addPlane(pln);

    tpoly.vertexStart = target->mIndexList.size();

    for (U32 i = 0; i < mPolyList[pdx].vertexCount; i++)
    {
        U32 idx = mIndexList[mPolyList[pdx].vertexStart + i];
        Point3F pt = mVertexList[idx];

        target->mIndexList.increment();
        target->mIndexList.last() = target->addPoint(pt);
    }
}

void OptimizedPolyList::copyPolyToList(OptimizedPolyList* target, const FullPoly& poly) const
{
    target->mPolyList.increment();
    OptimizedPolyList::Poly& tpoly = target->mPolyList.last();

    tpoly.material = poly.material;
    tpoly.object = poly.object;
    tpoly.surfaceKey = poly.surfaceKey;
    tpoly.vertexCount = poly.indexes.size();

    PlaneF pln = poly.plane;
    tpoly.plane = target->addPlane(pln);

    tpoly.vertexStart = target->mIndexList.size();

    for (U32 i = 0; i < poly.indexes.size(); i++)
    {
        Point3F pt = poly.vertexes[poly.indexes[i]];

        target->mIndexList.increment();
        target->mIndexList.last() = target->addPoint(pt);
    }
}

OptimizedPolyList::FullPoly OptimizedPolyList::getFullPoly(U32 pdx)
{
    FullPoly ret;

    OptimizedPolyList::Poly& poly = mPolyList[pdx];

    ret.material = poly.material;
    ret.object = poly.object;
    ret.surfaceKey = poly.surfaceKey;

    for (U32 i = 0; i < poly.vertexCount; i++)
    {
        U32 idx = mIndexList[poly.vertexStart + i];
        Point3F& pt = mVertexList[idx];

        S32 pdx = -1;

        for (U32 j = 0; j < ret.vertexes.size(); j++)
        {
            if (pt == ret.vertexes[j])
            {
                pdx = j;
                break;
            }
        }

        if (pdx == -1)
        {
            pdx = ret.vertexes.size();
            ret.vertexes.push_back(pt);
        }

        ret.indexes.push_back(pdx);
    }

    return ret;
}
