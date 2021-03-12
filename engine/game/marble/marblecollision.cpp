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

bool Marble::pointWithinPoly(const ConcretePolyList::Poly& poly, const Point3F& point)
{
    // TODO: Implement pointWithinPoly
    return false;
}

bool Marble::pointWithinPolyZ(const ConcretePolyList::Poly& poly, const Point3F& point, const Point3F& upDir)
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

    // TEMP: Until properly implemented
    position += velocity * deltaT;

    return false;
}

void Marble::findContacts(U32 contactMask, const Point3D* inPos, const F32* inRad)
{
    static Vector<Marble::MaterialCollision> materialCollisions;
    materialCollisions.clear();

    F32 rad;
    Box3F objBox;
    if (inRad != NULL && inPos != NULL)
    {
        rad = *inRad;

        objBox.min = Point3F(-rad, -rad, -rad);
        objBox.max = Point3F(rad, rad, rad);
    } else
    {
        // If either is null, both should be null
        inRad = NULL;
        inPos = NULL;
    }

    const Point3D* pos = inPos;
    if (inPos == NULL)
        pos = &mPosition;

    if (inRad == NULL)
    {
        rad = mRadius;
        objBox = mObjBox;
    } else
    {
      rad = *inRad;  
    }
    
    if (contactMask != 0)
    {
        Box3F extrudedMarble;
        extrudedMarble.min = objBox.min + *pos - 0.00009999999747378752f;
        extrudedMarble.max = objBox.max + *pos + 0.00009999999747378752f;

        findObjectsAndPolys(contactMask, extrudedMarble, true);
    }

    if ((contactMask & PlayerObjectType) != 0)
    {
        for (S32 i = 0; i < marbles.size(); i++)
        {
            Marble* otherMarble = marbles[i];

            // TODO: Finish Implementing findContacts
        }
    }
    
    F32 incRad = rad + 0.00009999999747378752f;

    for (S32 i = 0; i < polyList.mPolyList.size(); i++)
    {
        ConcretePolyList::Poly poly = polyList.mPolyList[i];

        // TODO: Finish Implementing findContacts
    }
}

void Marble::computeFirstPlatformIntersect(F64& dt, Vector<PathedInterior*>& pitrVec)
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
