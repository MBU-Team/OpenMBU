//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/consoleTypes.h"
#include "console/console.h"
#include "game/collisionTest.h"

static F32 BoxSize = 2;

bool CollisionTest::testPolytope = false;
bool CollisionTest::testClippedPolyList = false;
bool CollisionTest::testDepthSortList = false;
bool CollisionTest::testExtrudedPolyList = false;
bool CollisionTest::depthSort = false;
bool CollisionTest::depthRender = false;
bool CollisionTest::renderAlways = false;


//----------------------------------------------------------------------------

CollisionTest::CollisionTest()
{
}

CollisionTest::~CollisionTest()
{
}

void CollisionTest::consoleInit()
{
    Con::addVariable("Collision::boxSize", TypeF32, &BoxSize);

    Con::addVariable("Collision::testPolytope", TypeBool, &testPolytope);
    Con::addVariable("Collision::testClippedPolyList", TypeBool, &testClippedPolyList);
    Con::addVariable("Collision::testExtrudedPolyList", TypeBool, &testExtrudedPolyList);
    Con::addVariable("Collision::testDepthSortList", TypeBool, &testDepthSortList);

    Con::addVariable("Collision::depthSort", TypeBool, &depthSort);
    Con::addVariable("Collision::depthRender", TypeBool, &depthRender);
    Con::addVariable("Collision::renderAlways", TypeBool, &renderAlways);
}

void CollisionTest::collide(const MatrixF& transform)
{
    //
    Point3F pos;
    transform.getColumn(3, &pos);
    boundingBox.min = pos - Point3F(BoxSize, BoxSize, BoxSize);
    boundingBox.max = pos + Point3F(BoxSize, BoxSize, BoxSize);
    boundingSphere.center = pos;
    boundingSphere.radius = BoxSize * 1.5;

    if (testPolytope) {
        MatrixF imat(true);
        volume.buildBox(imat, boundingBox);
        tree.clear();
    }

    if (testClippedPolyList) {
        polyList.clear();
        polyList.mPlaneList.clear();
        polyList.mNormal.set(0, 0, 0);

        // Planes bounding the square.
        polyList.mPlaneList.setSize(6);
        polyList.mPlaneList[0].set(boundingBox.min, VectorF(-1, 0, 0));
        polyList.mPlaneList[1].set(boundingBox.max, VectorF(0, 1, 0));
        polyList.mPlaneList[2].set(boundingBox.max, VectorF(1, 0, 0));
        polyList.mPlaneList[3].set(boundingBox.min, VectorF(0, -1, 0));
        polyList.mPlaneList[4].set(boundingBox.min, VectorF(0, 0, -1));
        polyList.mPlaneList[5].set(boundingBox.max, VectorF(0, 0, 1));
    }

    if (testDepthSortList) {
        depthSortList.clear();
        mDepthSortExtent.set(5, 20, 5); // hard-code for now
        MatrixF mat = transform;
        mat.inverse(); // we want world to camera (or whatever transform represents)
        depthSortList.set(mat, mDepthSortExtent);

        // we use a different box and sphere...
        // our box starts at the camera and goes forward mDepthSortExtent.y
        // with width and height of mDepthSortExtent.x and mDepthSortExtent.z
        Point3F x, y, z, p;
        transform.getColumn(0, &x);
        transform.getColumn(1, &y);
        transform.getColumn(2, &z);
        transform.getColumn(3, &p);
        x *= 0.5f * mDepthSortExtent.x;
        y *= mDepthSortExtent.y;
        z *= 0.5f * mDepthSortExtent.z;
        Point3F boxMin = p;
        Point3F boxMax = p;
        boxMin.setMin(p - x - z);
        boxMin.setMin(p - x + z);
        boxMin.setMin(p + x - z);
        boxMin.setMin(p + x + z);
        boxMin.setMin(p - x - z + y);
        boxMin.setMin(p - x + z + y);
        boxMin.setMin(p + x - z + y);
        boxMin.setMin(p + x + z + y);

        boxMax.setMax(p - x - z);
        boxMax.setMax(p - x + z);
        boxMax.setMax(p + x - z);
        boxMax.setMax(p + x + z);
        boxMax.setMax(p - x - z + y);
        boxMax.setMax(p - x + z + y);
        boxMax.setMax(p + x - z + y);
        boxMax.setMax(p + x + z + y);

        Point3F boxCenter = boxMin + boxMax;
        boxCenter *= 0.5f;
        F32 boxRadius = (boxMax - boxMin).len();

        mDepthBox.min = boxMin;
        mDepthBox.max = boxMax;
        mDepthSphere.center = boxCenter;
        mDepthSphere.radius = boxRadius;
    }

    if (testExtrudedPolyList) {
        MatrixF imat(1);
        polyhedron.buildBox(imat, boundingBox);
        VectorF v1(0, 3, 0);
        transform.mulV(v1, &extrudeVector);
        extrudedList.extrude(polyhedron, extrudeVector);
        extrudedList.setVelocity(extrudeVector);
        extrudedList.setCollisionList(&collisionList);

        Point3F p1 = pos + extrudeVector;
        boundingBox.min = boundingBox.max = pos;
        boundingBox.min.setMin(p1);
        boundingBox.max.setMax(p1);
        boundingBox.min -= Point3F(BoxSize, BoxSize, BoxSize);
        boundingBox.max += Point3F(BoxSize, BoxSize, BoxSize);
        boundingSphere.radius += extrudeVector.len();
    }

    if (testPolytope || testClippedPolyList || testExtrudedPolyList || testDepthSortList) {
        testPos = boundingSphere.center;
        getCurrentClientContainer()->findObjects(0xFFFFFFFF, CollisionTest::callback, this);
    }

    if (testExtrudedPolyList) {
        extrudedList.adjustCollisionTime();
    }
}

void CollisionTest::callback(SceneObject* obj, void* thisPtr)
{
    CollisionTest* ptr = reinterpret_cast<CollisionTest*>(thisPtr);

    if (testPolytope) {
        if (BSPNode* root = obj->buildCollisionBSP(&ptr->tree, ptr->boundingBox, ptr->boundingSphere))
            ptr->volume.intersect(obj, root);
    }
    if (testClippedPolyList) {
        obj->buildPolyList(&ptr->polyList, ptr->boundingBox, ptr->boundingSphere);
    }
    if (testExtrudedPolyList) {
        obj->buildPolyList(&ptr->extrudedList, ptr->boundingBox, ptr->boundingSphere);
    }
    if (testDepthSortList) {
        obj->buildPolyList(&ptr->depthSortList, ptr->mDepthBox, ptr->mDepthSphere);
    }
}

extern void wireCube(F32 size, Point3F pos);

void CollisionTest::render()
{
    /*
       bool collision = false;
       if (testPolytope || renderAlways) {
          if (volume.didIntersect())
             volume.render();
       }
       if (testClippedPolyList || renderAlways) {
          if (polyList.mPolyList.size())
             collision = true;
          polyList.render();
       }
       if (testExtrudedPolyList || renderAlways) {
          if (collisionList.count)
             collision = true;
          extrudedList.render();
          glPushAttrib(GL_DEPTH_BUFFER_BIT);
          glEnable(GL_DEPTH_TEST);
          polyhedron.render(extrudeVector,collisionList.t);
          glPopAttrib();
       }
       if (testDepthSortList || renderAlways) {
          if (depthSort && testDepthSortList)
             depthSortList.sort();
          // should we write depth values of polys...
          // if polys are correctly sorted then writing depth values
          // should result in no overlap of polys when looking down
          // from camera...otoh, if polys are out of order, we should
          // see overlap
          DepthSortList::renderWithDepth = depthRender;
          depthSortList.render();
       }

       if (collision || renderAlways)
         wireCube(BoxSize,testPos);
    */
}

