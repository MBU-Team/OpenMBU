//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

Point3D Marble::getVelocityD() const
{
    // TODO: Implement
    return Point3D();
}

void Marble::setVelocityD(const Point3D&)
{
    
}

void Marble::setVelocityRotD(const Point3D&)
{
    
}

void Marble::applyImpulse(const Point3F& pos, const Point3F& vec)
{
    Parent::applyImpulse(pos, vec);
}

void Marble::clearMarbleAxis()
{
    
}

void Marble::applyContactForces(const Move*, bool, Point3D&, const Point3D&, double, Point3D&, Point3D&, float&)
{
    
}

void Marble::getMarbleAxis(Point3D&, Point3D&, Point3D&)
{
    
}

const Point3F& Marble::getMotionDir()
{
    // TODO: Implement
    return Point3F();
}

bool Marble::computeMoveForces(Point3D&, Point3D&, const Move*)
{
    // TODO: Implement
    return false;
}

void Marble::velocityCancel(bool, bool, bool&, bool&, Vector<PathedInterior*>&)
{
    
}

Point3D Marble::getExternalForces(const Move*, double)
{
    // TODO: Implement
    return Point3D();
}

void Marble::advancePhysics(const Move*, U32)
{
    
}
