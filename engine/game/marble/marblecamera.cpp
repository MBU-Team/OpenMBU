//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

bool Marble::moveCamera(Point3F, Point3F, Point3F&, U32, F32)
{
    // TODO: Implement
    return false;
}

void Marble::processCameraMove(const Move*)
{
    
}

void Marble::startCenterCamera()
{
    
}

bool Marble::isCameraClear(Point3F, Point3F)
{
    // TODO: Implement
    return false;
}

void Marble::getLookMatrix(MatrixF*)
{
    
}

void Marble::cameraLookAtPt(const Point3F&)
{
    
}

void Marble::resetPlatformsForCamera()
{
    
}

void Marble::getOOBCamera(MatrixF*)
{
    
}

void Marble::setPlatformsForCamera(const Point3F&, const Point3F&, const Point3F&)
{
    
}

void Marble::getCameraTransform(F32* pos, MatrixF* mat)
{
    Parent::getCameraTransform(pos, mat);
}
