//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

static U32 sCollisionMask = StaticObjectType |
                            AtlasObjectType |
                            InteriorMapObjectType |
                            ShapeBaseObjectType |
                            PlayerObjectType |
                            VehicleBlockerObjectType;

static U32 sContactMask = StaticObjectType |
                          AtlasObjectType |
                          InteriorMapObjectType |
                          ShapeBaseObjectType |
                          PlayerObjectType |
                          VehicleBlockerObjectType;

bool gMarbleAxisSet = false;
Point3F gWorkGravityDir;
Point3F gMarbleSideDir;

Point3D Marble::getVelocityD() const
{
    return mVelocity;
}

void Marble::setVelocityD(const Point3D& vel)
{
    dMemcpy(mVelocity, vel, sizeof(mVelocity));
    mSinglePrecision.mVelocity = vel;

    setMaskBits(MoveMask);
}

void Marble::setVelocityRotD(const Point3D& rot)
{
    dMemcpy(mOmega, rot, sizeof(mOmega));

    setMaskBits(MoveMask);
}

void Marble::applyImpulse(const Point3F& pos, const Point3F& vec)
{
    // TODO: Implement applyImpulse
    Parent::applyImpulse(pos, vec);
}

void Marble::clearMarbleAxis()
{
    gMarbleAxisSet = false;
    mGravityFrame.mulP(Point3F(0.0f, 0.0f, -1.0f), &gWorkGravityDir);
}

void Marble::applyContactForces(const Move*, bool, Point3D&, const Point3D&, double, Point3D&, Point3D&, float&)
{
    // TODO: Implement applyContactForces
}

void Marble::getMarbleAxis(Point3D&, Point3D&, Point3D&)
{
    // TODO: Implement getMarbleAxis
}

const Point3F& Marble::getMotionDir()
{
    Point3D side;
    Point3D motion;
    Point3D up;
    Marble::getMarbleAxis(side, motion, up);

    return gMarbleMotionDir;
}

bool Marble::computeMoveForces(Point3D&, Point3D&, const Move*)
{
    // TODO: Implement computeMoveForces
    return false;
}

void Marble::velocityCancel(bool, bool, bool&, bool&, Vector<PathedInterior*>&)
{
    // TODO: Implement velocityCancel
}

Point3D Marble::getExternalForces(const Move*, double)
{
    // TODO: Implement getExternalForces
    return Point3D();
}

void Marble::advancePhysics(const Move*, U32)
{
    // TODO: Implement advancePhysics
}

ConsoleMethod(Marble, setVelocityRot, bool, 3, 3, "(vel)")
{
    Point3F rot;
    dSscanf(argv[2], "%f %f %f", &rot.x, &rot.y, &rot.z);
    object->setVelocityRotD(rot);

    return 1;
}
