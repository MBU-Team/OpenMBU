//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mMath.h"
#include "console/console.h"
#include "collision/concretePolyList.h"


//----------------------------------------------------------------------------

ConcretePolyList::ConcretePolyList()
{
    VECTOR_SET_ASSOCIATION(mPolyList);
    VECTOR_SET_ASSOCIATION(mVertexList);
    VECTOR_SET_ASSOCIATION(mIndexList);
    VECTOR_SET_ASSOCIATION(mPolyPlaneList);

    mIndexList.reserve(100);
}

ConcretePolyList::~ConcretePolyList()
{

}


//----------------------------------------------------------------------------
void ConcretePolyList::clear()
{
    // Only clears internal data
    mPolyList.clear();
    mVertexList.clear();
    mIndexList.clear();
    mPolyPlaneList.clear();
}

//----------------------------------------------------------------------------

U32 ConcretePolyList::addPoint(const Point3F& p)
{
    mVertexList.increment();
    Point3F& v = mVertexList.last();
    v.x = p.x * mScale.x;
    v.y = p.y * mScale.y;
    v.z = p.z * mScale.z;
    mMatrix.mulP(v);

    return mVertexList.size() - 1;
}


U32 ConcretePolyList::addPlane(const PlaneF& plane)
{
    mPolyPlaneList.increment();
    mPlaneTransformer.transform(plane, mPolyPlaneList.last());

    return mPolyPlaneList.size() - 1;
}


//----------------------------------------------------------------------------

void ConcretePolyList::begin(U32 material, U32 surfaceKey)
{
    mPolyList.increment();
    Poly& poly = mPolyList.last();
    poly.object = mCurrObject;
    poly.material = material;
    poly.vertexStart = mIndexList.size();
    poly.surfaceKey = surfaceKey;
}


//----------------------------------------------------------------------------

void ConcretePolyList::plane(U32 v1, U32 v2, U32 v3)
{
    mPolyList.last().plane.set(mVertexList[v1],
        mVertexList[v2], mVertexList[v3]);
}

void ConcretePolyList::plane(const PlaneF& p)
{
    mPlaneTransformer.transform(p, mPolyList.last().plane);
}

void ConcretePolyList::plane(const U32 index)
{
    AssertFatal(index < mPolyPlaneList.size(), "Out of bounds index!");
    mPolyList.last().plane = mPolyPlaneList[index];
}

const PlaneF& ConcretePolyList::getIndexedPlane(const U32 index)
{
    AssertFatal(index < mPolyPlaneList.size(), "Out of bounds index!");
    return mPolyPlaneList[index];
}


//----------------------------------------------------------------------------

void ConcretePolyList::vertex(U32 vi)
{
    mIndexList.push_back(vi);
}


//----------------------------------------------------------------------------

bool ConcretePolyList::isEmpty() const
{
    return false;
}

void ConcretePolyList::end()
{
    Poly& poly = mPolyList.last();
    poly.vertexCount = mIndexList.size() - poly.vertexStart;
}


void ConcretePolyList::render()
{
    /*
       glVertexPointer(3,GL_FLOAT,sizeof(Point3F),mVertexList.address());
       glEnableClientState(GL_VERTEX_ARRAY);
       glColor4f(1,0,0,0.25);
       glEnable(GL_BLEND);
       glDisable(GL_CULL_FACE);
       glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

       Poly * p;
       for (p = mPolyList.begin(); p < mPolyList.end(); p++) {
          if(p->material != 0xFFFFFFFF)
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
