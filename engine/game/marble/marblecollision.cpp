//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

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

void Marble::findObjectsAndPolys(U32, const Box3F&, bool)
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

void Marble::resetObjectsAndPolys(U32, const Box3F&)
{
    // TODO: Implement resetObjectsAndPolys
}
