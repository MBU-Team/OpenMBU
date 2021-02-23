//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

static U32 sCameraCollisionMask = 0x4008; // TODO: Figure out ObjectType mask

bool Marble::moveCamera(Point3F start, Point3F end, Point3F& result, U32 maxIterations, F32 timeStep)
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

bool Marble::isCameraClear(Point3F start, Point3F end)
{
    if (!moveCamera(start, end, start, 1, 1.0f))
        return false;

    Point3D ep = end;
    findContacts(sCameraCollisionMask, &ep, &start.z);

    return mContacts.size() == 0;
}

void Marble::getLookMatrix(MatrixF* camMat)
{
    float prevMouseX = delta.prevMouseX;
    double _prevMouseX = prevMouseX;
    double newMouseX;
    if (prevMouseX > M_PI_F && mMouseX < M_PI_F && _prevMouseX - mMouseX > M_PI_F)
    {
        newMouseX = _prevMouseX - (M_PI * 2);
    LABEL_5:
        prevMouseX = newMouseX;
        goto LABEL_10;
    }

    if (mMouseX > M_PI_F && _prevMouseX < M_PI_F && mMouseX - _prevMouseX > M_PI_F)
    {
        newMouseX = _prevMouseX + (M_PI * 2);
        goto LABEL_5;
    }
LABEL_10:
    double dt = gClientProcessList.getLastDelta();
    double oneMinusDelta = 1.0f - dt;

    MatrixF xRot;
    m_matF_set_euler(Point3F(oneMinusDelta * mMouseY + delta.prevMouseY * dt, 0.0f, 0.0f), xRot);
    MatrixF zRot;
    m_matF_set_euler(Point3F(0.0f, 0.0f, oneMinusDelta * mMouseX + prevMouseX * dt), zRot);

    mGravityRenderFrame.setMatrix(camMat);
    camMat->mul(zRot);
    camMat->mul(xRot);
}

void Marble::cameraLookAtPt(const Point3F&)
{
    // TODO: Implement cameraLookAtPt
}

void Marble::resetPlatformsForCamera()
{
    // TODO: Implement resetPlatformsForCamera
}

void Marble::getOOBCamera(MatrixF* mat)
{
    MatrixF camMat;
    getLookMatrix(&camMat);
    Point3F camUpDir(camMat[2], camMat[6], camMat[10]);
    Point3F matPos(mRenderObjToWorld[3], mRenderObjToWorld[7], mRenderObjToWorld[11]);
    float radius = mRadius + 0.25f;
    Point3F scale(camMat[2] * radius, camMat[6] * radius, camMat[10] * radius);
    matPos += scale;
    Point3F camForwardDir = matPos - mOOBCamPos;
    m_point3F_normalize(camForwardDir);
    Point3F camSideDir;
    mCross(camForwardDir, camUpDir, &camSideDir);
    mCross(camSideDir, camForwardDir, &camUpDir);
    m_point3F_normalize(camSideDir);
    m_point3F_normalize(camUpDir);

    mat->identity();
    mat->setColumn(0, camSideDir);
    mat->setColumn(1, camForwardDir);
    mat->setColumn(2, camUpDir);
    mat->setColumn(3, mOOBCamPos);
}

void Marble::setPlatformsForCamera(const Point3F&, const Point3F&, const Point3F&)
{
    // TODO: Implement setPlatformsForCamera
}

void Marble::getCameraTransform(F32* pos, MatrixF* mat)
{
    if (mOOB)
    {
        getOOBCamera(mat);
        return;
    }

    MatrixF camMat;
    getLookMatrix(&camMat);

    Point3F forwardDir(camMat[1], camMat[5], camMat[9]);
    Point3F camUpDir = -getGravityRenderDir();
    Point3F position(mRenderObjToWorld[3], mRenderObjToWorld[7], mRenderObjToWorld[11]);

    Point3F startCam;
    if (!Marble::smEndPad.isNull() && mMode & StoppingMode)
    {
        MatrixF padMat = Marble::smEndPad->getTransform();
        position.x = padMat[3];
        position.y = padMat[7];
        position.z = padMat[11];
        
        startCam.x = padMat[2];
        startCam.y = padMat[6];
        startCam.z = padMat[10];

        position += startCam;
        position *= 0.02500000037252903;

        position += mEffect.lastCamFocus * 0.9750000238418579;
    }

    double radius = mRadius + 0.25;
    mEffect.lastCamFocus = position;

    startCam = camUpDir * radius + position;

    float camDist = mDataBlock->cameraDistance;

    Point3F endPos(startCam.x - forwardDir.x * camDist, startCam.y - forwardDir.y * camDist, startCam.z - forwardDir.z * camDist);
    if (!Marble::smEndPad.isNull() && mMode & StoppingMode)
    {
        float effectTime;
        if (mEffect.effectTime >= 2.0f)
            effectTime = 1.0f;
        else
            effectTime = mEffect.effectTime * 0.5f;

        effectTime *= 0.5f * mDataBlock->cameraDistance;

        endPos.x -= effectTime * forwardDir.x;
        endPos.y -= effectTime * forwardDir.y;
        endPos.z -= effectTime * forwardDir.z;
    }

    setPlatformsForCamera(position, startCam, endPos);

    Box3F testBox;
    if (mCameraInit)
        testBox.max = mLastCamPos;
    else
        testBox.max = endPos;
    testBox.min = testBox.max;
    testBox.min.setMin(endPos);
    testBox.min.setMin(position);
    testBox.max.setMax(endPos);
    testBox.max.setMax(position);
    resetObjectsAndPolys(sCameraCollisionMask, testBox);
    RayInfo coll;
    position = endPos;
    if (mCameraInit && isCameraClear(mLastCamPos, endPos) && !mContainer->castRay(mLastCamPos, endPos, sCameraCollisionMask, &coll))
    {
        position = endPos;
    }
    else
    {
        Point3F preStartPos = startCam;
        if (!isCameraClear(position, startCam))
            preStartPos = position;

        moveCamera(preStartPos, endPos, position, 4, 1.0f);
    }

    resetPlatformsForCamera();

    mLastCamPos = position;
    mOOBCamPos = position;
    mCameraInit = true;

    Point3F camForwardDir = startCam - position;
    m_point3F_normalize(camForwardDir);

    Point3F camSideDir;
    mCross(camForwardDir, camUpDir, &camSideDir);
    m_point3F_normalize(camSideDir);

    mCross(camSideDir, camForwardDir, &camUpDir);
    m_point3F_normalize(camUpDir);

    mat->identity();
    mat->setColumn(0, camSideDir);
    mat->setColumn(1, camForwardDir);
    mat->setColumn(2, camUpDir);
    mat->setColumn(3, position);
}
