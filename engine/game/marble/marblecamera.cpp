//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"

//----------------------------------------------------------------------------

static U32 sCameraCollisionMask = 0x4008; // TODO: Figure out ObjectType mask

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
    prevMouseX = dt * prevMouseX + mMouseX * oneMinusDelta;
    MatrixF xRot;
    m_matF_set_euler(Point3F(oneMinusDelta * mMouseY + delta.prevMouseY + dt, 0.0f, 0.0f), xRot);
    MatrixF zRot;
    m_matF_set_euler(Point3F(0.0f, 0.0f, prevMouseX), zRot);

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

    Point3F camForwardDir(camMat[1], camMat[5], camMat[9]);
    Point3F gravityRenderDir = getGravityRenderDir();
    Point3F invGravityRenderDir(-gravityRenderDir.z, -gravityRenderDir.y, -gravityRenderDir.x);
    Point3F preStartPos(invGravityRenderDir.z, invGravityRenderDir.y, invGravityRenderDir.x);
    Point3F camUpDir(invGravityRenderDir.z, invGravityRenderDir.y, invGravityRenderDir.x);
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

        invGravityRenderDir.z = mEffect.lastCamFocus.x * 0.9750000238418579;
        invGravityRenderDir.y = mEffect.lastCamFocus.y * 0.9750000238418579;
        invGravityRenderDir.x = mEffect.lastCamFocus.z * 0.9750000238418579;

        position.x += invGravityRenderDir.z;
        position.y += invGravityRenderDir.y;
        position.z += invGravityRenderDir.x;
    }

    double radius = mRadius + 0.25;
    mEffect.lastCamFocus = position;
    invGravityRenderDir.z = camUpDir.x * radius;
    invGravityRenderDir.y = camUpDir.y * radius;
    invGravityRenderDir.x = camUpDir.z * radius;
    startCam - Point3F(invGravityRenderDir.z + position.x, invGravityRenderDir.y + position.y, invGravityRenderDir.x + position.z);
    float camDist = mDataBlock->cameraDistance;

    Point3F endPos(startCam.x - camForwardDir.x * camDist, startCam.y - camForwardDir.y * camDist, startCam.z - camForwardDir.z * camDist);
    if (!Marble::smEndPad.isNull() && mMode & StoppingMode)
    {
        float effectTime;
        if (mEffect.effectTime >= 2.0f)
            effectTime = 1.0f;
        else
            effectTime = mEffect.effectTime * 0.5f;

        effectTime *= 0.5f * mDataBlock->cameraDistance;

        endPos.x -= effectTime * camForwardDir.x;
        endPos.y -= effectTime * camForwardDir.y;
        endPos.z -= effectTime * camForwardDir.z;
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
    if (mCameraInit && isCameraClear(mLastCamPos, endPos) && !mContainer->castRay(mLastCamPos, endPos, sCameraCollisionMask, &coll))
    {
        position = endPos;
    }
    else
    {
        preStartPos = startCam;
        if (!isCameraClear(position, startCam))
            preStartPos = position;

        moveCamera(preStartPos, endPos, position, 4, 1.0f);
    }

    resetPlatformsForCamera();

    mLastCamPos = position;
    mOOBCamPos = position;
    mCameraInit = true;

    camForwardDir = preStartPos;
    m_point3F_normalize(camForwardDir);

    Point3F camSideDir;
    mCross(camForwardDir, camUpDir, camSideDir);
    m_point3F_normalize(camSideDir);

    mCross(camSideDir, camForwardDir, camUpDir);
    m_point3F_normalize(camUpDir);

    mat->identity();
    mat->setColumn(0, camSideDir);
    mat->setColumn(1, camForwardDir);
    mat->setColumn(2, camUpDir);
    mat->setColumn(3, position);
}
