//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

#include "math/mathUtils.h"

//----------------------------------------------------------------------------

static Box3F sgLastCollisionBox;
static U32 sgLastCollisionMask;
static bool sgResetFindObjects;
static U32 sgCountCalls;

void Marble::clearObjectsAndPolys()
{
    // TODO: Implement clearObjectsAndPolys
}

bool Marble::pointWithinPoly(const ConcretePolyList::Poly&, const Point3F&)
{
    // TODO: Implement pointWithinPoly
    return false;
}

bool Marble::pointWithinPolyZ(const ConcretePolyList::Poly&, const Point3F&, const Point3F&)
{
    // TODO: Implement pointWithinPolyZ
    return false;
}

void Marble::findObjectsAndPolys(U32 collisionMask, const Box3F& testBox, bool testPIs)
{
    // TODO: Implement findObjectsAndPolys
}

bool Marble::testMove(Point3D velocity, Point3D& position, F64& deltaT, F64 radius, U32 collisionMask, bool testPIs)
{
    // TODO: Implement testMove

    return false;
}

void Marble::findContacts(U32, const Point3D*, const F32*)
{
    // TODO: Implement findContacts
}

void Marble::computeFirstPlatformIntersect(F64&, Vector<PathedInterior*>&)
{
    // TODO: Implement computeFirstPlatformIntersect
}

void Marble::resetObjectsAndPolys(U32 collisionMask, const Box3F& testBox)
{
    sgLastCollisionBox.min.set(0, 0, 0);
    sgLastCollisionBox.max.set(0, 0, 0);

    sgResetFindObjects = 1;
    sgCountCalls = 0;

    if (!smPathItrVec.size())
        findObjectsAndPolys(collisionMask, testBox, 0);
}
