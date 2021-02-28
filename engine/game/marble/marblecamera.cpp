//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"
#include "game/gameProcess.h"

//----------------------------------------------------------------------------

static U32 sCameraCollisionMask = InteriorObjectType | StaticShapeObjectType;

bool Marble::moveCamera(Point3F start, Point3F end, Point3F& result, U32 maxIterations, F32 timeStep)
{
    disableCollision();

    Point3D position = start;
    Point3D velocity = end - start;

    F64 time = timeStep;
    F64 totalTime = timeStep;

    bool someBool = false;
    U32 i = 0;
    for (bool flag = timeStep > 0.0f; flag && i < maxIterations; flag = totalTime > 0.0f)
    {
        if (testMove(velocity, position, time, 0.25, sCameraCollisionMask, true))
        {
            someBool = true;

            F64 normals = mLastContact.normal.x * velocity.x
                      + mLastContact.normal.y * velocity.y
                      + mLastContact.normal.z * velocity.z;
            
            velocity -= mLastContact.normal * normals;
        }
        ++i;

        totalTime -= time;
        time = totalTime;
        if (velocity.len() < 0.001)
            break;
    }

    enableCollision();

    result = position;

    if (totalTime >= 0.000001 || (maxIterations <= 1 && someBool))
        return false;

    return true;
}

void pushToSquare(Point2F& dir)
{
    F32 x = fabsf(dir.x);
    F32 y = fabsf(dir.y);

    F32 len = dir.len() * 1.25f;
    if (len > 1.0f)
        len = 1.0f;

    F64 max;
    if (y >= x)
        max = y;
    else
        max = x;

    if (max < 0.0099999998)
        max = 0.0099999998;

    dir *= len / max;
}

void Marble::processCameraMove(const Move* move)
{
    delta.prevMouseX = mMouseX;
    delta.prevMouseY = mMouseY;

    Point2F value(move->yaw / M_PI_F, move->pitch / M_PI_F);

    pushToSquare(value);

    // TODO: Implement processCameraMove
    
    float delta = 0; // Temp

    // TODO: Implement processCameraMove

    float finalYaw;
    if (!move->deviceIsKeyboardMouse || (mMode & CameraHoverMode) != 0)
        finalYaw = delta;
    else
        finalYaw = value.x;

    this->mMouseX += finalYaw;
    this->mMouseY = mClampF(mMouseY, -0.34999999, 1.5);
    if (mMouseX > 6.283185307179586f)
        mMouseX -= 6.283185307179586f;
    if (mMouseX < 0.0f)
        mMouseX += 6.283185307179586f;
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

    if (prevMouseX > M_PI_F && mMouseX < M_PI_F && prevMouseX - mMouseX > M_PI_F)
        prevMouseX -= M_2PI;
    else if (mMouseX > M_PI_F && prevMouseX < M_PI_F && mMouseX - prevMouseX > M_PI_F)
        prevMouseX += M_2PI;

    double dt = gClientProcessList.getLastDelta();
    double oneMinusDelta = 1.0 - dt;

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
    if (!Marble::smEndPad.isNull() && (mMode & StoppingMode) != 0)
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
    if (!Marble::smEndPad.isNull() && (mMode & StoppingMode) != 0)
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

ConsoleMethod(Marble, cameraLookAtPt, void, 3, 3, "(point)")
{
    Point3F pt;
    dSscanf(argv[2], "%f %f %f", &pt.x, &pt.y, &pt.z);
    object->cameraLookAtPt(pt);
}
