//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

bool Marble::moveCamera(Point3F, Point3F, Point3F&, U32, F32)
{
    // TODO: Implement moveCamera
    return false;
}

void Marble::processCameraMove(const Move*)
{
    // TODO: Implement processCameraMove
}

void Marble::startCenterCamera()
{
    // TODO: Implement startCenterCamera
}

bool Marble::isCameraClear(Point3F, Point3F)
{
    // TODO: Implement isCameraClear
    return false;
}

void Marble::getLookMatrix(MatrixF*)
{
    // TODO: Implement getLookMatrix
}

void Marble::cameraLookAtPt(const Point3F&)
{
    // TODO: Implement cameraLookAtPt
}

void Marble::resetPlatformsForCamera()
{
    // TODO: Implement resetPlatformsForCamera
}

void Marble::getOOBCamera(MatrixF*)
{
    // TODO: Implement getOOBCamera
}

void Marble::setPlatformsForCamera(const Point3F&, const Point3F&, const Point3F&)
{
    // TODO: Implement setPlatformsForCamera
}

void Marble::getCameraTransform(F32* pos, MatrixF* mat)
{
    // TODO: Implement getCameraTransform
    Parent::getCameraTransform(pos, mat);
}
