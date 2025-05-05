//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "marble.h"
#include "game/gameProcess.h"
#include "game/fx/cameraFXMgr.h"

//----------------------------------------------------------------------------

static U32 sCameraCollisionMask = InteriorObjectType | StaticShapeObjectType | StaticTSObjectType;

#define RADIUS_FOR_CAMERA_MBG 0.09f
#define RADIUS_FOR_CAMERA_MBU 0.25f
#define RADIUS_FOR_CAMERA (mSize < 1.5f ? RADIUS_FOR_CAMERA_MBG : RADIUS_FOR_CAMERA_MBU)

bool Marble::moveCamera(Point3F start, Point3F end, Point3F& result, U32 maxIterations, F32 timeStep)
{
    disableCollision();

    Point3D position = start;
    Point3D velocity = end - start;

    F64 time = timeStep;
    F64 totalTime = timeStep;

    bool hitSomething = false;
    U32 i = 0;
    for (bool flag = timeStep > 0.0f; flag && i < maxIterations; flag = totalTime > 0.0f)
    {
        if (testMove(velocity, position, time, RADIUS_FOR_CAMERA, sCameraCollisionMask, true))
        {
            hitSomething = true;
            
            velocity -= mLastContact.normal * mDot(mLastContact.normal, velocity);
        }

        ++i;

        totalTime -= time;
        time = totalTime;
        if (velocity.len() < 0.001)
            break;
    }

    enableCollision();

    result = position;

    if (totalTime >= 0.000001 || (maxIterations <= 1 && hitSomething))
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

F32 applyNonlinearScale2(F32 value)
{
    float sign = value >= 0.0 ? 1.0 : -1.0;
    float absval = fabsf(value);
    return (mPow(absval, 3.2f) * sign);
}

F32 rescaleDeadZone(F32 value, F32 deadZone)
{
    if (deadZone >= value)
    {
        if (-deadZone <= value)
            return 0.0;
        else
            return (value + deadZone) / (1.0 - deadZone);
    }
    else
        return (value - deadZone) / (1.0 - deadZone);
}

F32 computePitchSpeedFromDelta(F32 delta)
{
    return mClampF(delta, 0.3141592700403172, 1.570796326794897) * 4;
}

void Marble::processCameraMove(const Move* move)
{
    delta.prevMouseX = mMouseX;
    delta.prevMouseY = mMouseY;

    Point2F value(move->yaw / M_PI_F, move->pitch / M_PI_F);

    pushToSquare(value);

    F32 horizontalDeadZone = move->horizontalDeadZone;
    F32 verticalDeadZone = move->verticalDeadZone;
    F32 cameraAccelSpeed = move->cameraAccelSpeed;
    F32 cameraSensitivityHorizontal = move->cameraSensitivityHorizontal;
    F32 cameraSensitivityVertical = move->cameraSensitivityVertical;

    //F32 horizontalDeadZone = 0.25f;
    //F32 verticalDeadZone = 0.7f;
    //F32 cameraAccelSpeed = 0.05f;
    //F32 cameraSensitivityHorizontal = 0.7f;
    //F32 cameraSensitivityVertical = 0.8f;

    // Old values:
    //F32 verticalDeadZone = 0.69999999;
    //F32 cameraAccelSpeed = 0.0544000044465065;

    float moveYaw = applyNonlinearScale2(rescaleDeadZone(value.x, horizontalDeadZone));
    if (fabsf(this->mLastYaw) > fabsf(moveYaw))
        this->mLastYaw = moveYaw;
    else
    {
        if (this->mLastYaw < 0.0 && moveYaw > 0.0 || this->mLastYaw > 0.0 && moveYaw < 0.0)
            this->mLastYaw = 0.0;
        if (fabsf(moveYaw) > 0.2)
        {
            if (fabsf(this->mLastYaw) < 0.2)
            {
                if (moveYaw >= 0.0)
                    this->mLastYaw = 0.2;
                else
                    this->mLastYaw = -0.2;
            }
        }
        float deltaYaw = moveYaw - this->mLastYaw;
        if (fabsf(deltaYaw) <= cameraAccelSpeed)
            this->mLastYaw = moveYaw;
        else if (deltaYaw >= 0.0)
            this->mLastYaw += cameraAccelSpeed;
        else
            this->mLastYaw -= cameraAccelSpeed;
    }
    float delta_new = this->mLastYaw * cameraSensitivityHorizontal * M_2PI_F * TickSec;
    if ((this->mMode & CameraHoverMode) == 0)
    {
        if (this->mCenteringCamera)
        {
            float yawDiff = fabsf(fabsf(delta_new) - fabsf(this->mRadsLeftToCenter));
            if (yawDiff >= 0.15)
                yawDiff = sinf(this->mRadsLeftToCenter / this->mRadsStartingToCenter) * 0.15f;
            else
            {
                if (yawDiff >= 0.05)
                    yawDiff = 0.05;
                else
                    this->mCenteringCamera = false;
            }
   
            if (this->mRadsLeftToCenter <= (double)delta_new)
            {
                delta_new = delta_new - yawDiff;
                this->mRadsLeftToCenter += yawDiff;
            }
            else
            {
                delta_new = yawDiff + delta_new;
                this->mRadsLeftToCenter -= yawDiff;
            }
        }

        if (move->deviceIsKeyboardMouse)
        {
            this->mMouseY += value.y;
        }
        else
        {
            float rescaledY = rescaleDeadZone(value.y, verticalDeadZone);
            if (move->autoCenterCamera)
            {
                if (rescaledY <= 0.0)
                    rescaledY = 0.4f - rescaledY * -0.75f;
                else
                    rescaledY = rescaledY * 1.1f + 0.4f;
                float movePitchDelta = (rescaledY - this->mMouseY);
                float movePitchSpeed = computePitchSpeedFromDelta(fabsf(movePitchDelta)) * TickSec * cameraSensitivityVertical;
                if (movePitchDelta <= 0.0)
                {
                    movePitchDelta = -movePitchDelta;
                    if (movePitchDelta < movePitchSpeed)
                        movePitchSpeed = movePitchDelta;
                    movePitchDelta = -movePitchSpeed;
                    movePitchSpeed = movePitchDelta;
                } else if (movePitchSpeed > movePitchDelta)
                {
                    movePitchSpeed = movePitchDelta;
                }
                this->mMouseY += movePitchSpeed;
            }
            else
            {
                this->mMouseY += rescaledY * TickSec * (cameraSensitivityVertical * 2.0f);
            }
        }
    }
    else 
    {
        delta_new = 0.02;
        if (this->mMouseY < 0.69999999)
        {
            this->mMouseY = getMin(0.050000001f, (float)(0.699999988079071 - this->mMouseY)) + this->mMouseY;
        }
        else if (this->mMouseY > 0.69999999)
        {
            this->mMouseY = this->mMouseY - getMin(0.050000001f, (float)(this->mMouseY - 0.699999988079071));
        }
    }

    float finalYaw;
    if (!move->deviceIsKeyboardMouse || (mMode & CameraHoverMode) != 0)
        finalYaw = delta_new;
    else
        finalYaw = value.x;

    this->mMouseX += finalYaw;
    this->mMouseY = mClampF(mMouseY, -0.34999999, 1.5);
    if (mMouseX > M_2PI_F)
        mMouseX -= M_2PI_F;
    if (mMouseX < 0.0f)
        mMouseX += M_2PI_F;
}

void Marble::startCenterCamera()
{
    if (mVelocity.y * mVelocity.y + mVelocity.x * mVelocity.x + mVelocity.z * mVelocity.z >= 9.0)
    {
        Point3F motionDir = getMotionDir();

        mRadsLeftToCenter = mAtan(mVelocity.x, mVelocity.y) - mAtan(motionDir.x, motionDir.y);

        if (fabsf(mRadsLeftToCenter) >= 0.5235987755982988f)
        {
            if (mRadsLeftToCenter <= M_PI_F)
            {
                if (mRadsLeftToCenter < 0.0f && mRadsLeftToCenter < -M_PI_F)
                    mRadsLeftToCenter = mRadsLeftToCenter + M_2PI_F;
            }
            else
                mRadsLeftToCenter = mRadsLeftToCenter - M_2PI_F;
            mCenteringCamera = 1;
            mRadsStartingToCenter = mRadsLeftToCenter;
        }
    }
}

bool Marble::isCameraClear(Point3F start, Point3F end)
{
    if (!moveCamera(start, end, start, 1, 1.0f))
        return false;
       
    F32 radius = RADIUS_FOR_CAMERA;
    Point3D ep = end;
    findContacts(sCameraCollisionMask, &ep, &radius);

    return mContacts.empty();
}

void Marble::getLookMatrix(MatrixF* camMat)
{
    float prevMouseX = delta.prevMouseX;

    if (prevMouseX > M_PI_F && mMouseX < M_PI_F && prevMouseX - mMouseX > M_PI_F)
        prevMouseX -= M_2PI;
    else if (mMouseX > M_PI_F && prevMouseX < M_PI_F && mMouseX - prevMouseX > M_PI_F)
        prevMouseX += M_2PI;

#ifdef MB_ULTRA_PREVIEWS
    double dt = getCurrentClientProcessList()->getLastDelta();
#else
    double dt = gClientProcessList.getLastDelta();
#endif
    double oneMinusDelta = 1.0 - dt;

    MatrixF xRot;
    m_matF_set_euler(Point3F(oneMinusDelta * mMouseY + delta.prevMouseY * dt, 0.0f, 0.0f), xRot);
    MatrixF zRot;
    m_matF_set_euler(Point3F(0.0f, 0.0f, oneMinusDelta * mMouseX + prevMouseX * dt), zRot);

    mGravityRenderFrame.setMatrix(camMat);
    camMat->mul(zRot);
    camMat->mul(xRot);
}

void Marble::cameraLookAtPt(const Point3F& pt)
{
    Point3F forward;
    Point3F vec;

    Point3F rPos = getRenderPosition();
    Point3F delta = pt - rPos;

    MatrixF grav;
    mGravityFrame.setMatrix(&grav);
    vec.x = grav[0];
    vec.y = grav[4];
    vec.z = grav[8];

    forward.x = grav[1];
    forward.y = grav[5];
    forward.z = grav[9];

    Point2F deltaRot(mDot(delta, vec), mDot(delta, forward));

    if (deltaRot.len() >= 0.0001)
        this->mMouseX = mAtan(deltaRot.x, deltaRot.y);
}

void Marble::resetPlatformsForCamera()
{
#ifdef MB_ULTRA_PREVIEWS
    float backDelta = getCurrentClientProcessList()->getLastDelta();
#else
    float backDelta = gClientProcessList.getLastDelta();
#endif

    for (S32 i = 0; i < smPathItrVec.size(); i++)
    {
        PathedInterior* pathedInterior = smPathItrVec[i];

        pathedInterior->popTickState();
        pathedInterior->interpolateTick(backDelta);
    }
}

void Marble::getOOBCamera(MatrixF* mat)
{
    MatrixF camMat;
    getLookMatrix(&camMat);
    Point3F camUpDir(camMat[2], camMat[6], camMat[10]);
    Point3F matPos(mRenderObjToWorld[3], mRenderObjToWorld[7], mRenderObjToWorld[11]);
#ifdef MBXP_DYNAMIC_CAMERA
    matPos += mCameraPosition;
#endif
    float radius = mRadius + RADIUS_FOR_CAMERA;
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

void Marble::setPlatformsForCamera(const Point3F& marblePos, const Point3F& startCam, const Point3F& endCam)
{
    smPathItrVec.clear();

    Box3F camBox = mObjBox;
    camBox.min = marblePos + camBox.min;
    camBox.max = marblePos + camBox.max;

    camBox.min.setMin(startCam - RADIUS_FOR_CAMERA);
    camBox.max.setMax(startCam + RADIUS_FOR_CAMERA);
    camBox.min.setMin(endCam - RADIUS_FOR_CAMERA);
    camBox.max.setMax(endCam + RADIUS_FOR_CAMERA);
    
    camBox.min -= RADIUS_FOR_CAMERA;
    camBox.max += RADIUS_FOR_CAMERA;

#ifdef MB_ULTRA_PREVIEWS
    F32 delta = getCurrentClientProcessList()->getLastDelta();
#else
    F32 delta = gClientProcessList.getLastDelta();
#endif

    for (PathedInterior* i = PathedInterior::getPathedInteriors(this); ; i = i->getNext())
    {
        if (!i)
            break;
        Box3F in_rOverlap = i->getExtrudedBox();
        if (camBox.isOverlapped(in_rOverlap))
        {
            i->pushTickState();
            i->interpolateTick(delta);
            i->setTransform(i->getRenderTransform());
            smPathItrVec.push_back(i);
        }
    }
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

#ifndef MBG_PHYSICS
    if (!Marble::smEndPad.isNull() && (mMode & StoppingMode) != 0)
    {
        MatrixF padMat = Marble::smEndPad->getTransform();
        Point3F offset;
        padMat.getColumn(3, &position);
        padMat.getColumn(2, &offset);

        position += offset;
     
        position *= 0.02500000037252903;
        position += mEffect.lastCamFocus * 0.9750000238418579;
    }
#endif

    F64 verticalOffset = mRadius + RADIUS_FOR_CAMERA;
    mEffect.lastCamFocus = position;

    Point3F startCam = camUpDir * verticalOffset + position;
    
    Point3F endPos = startCam - forwardDir * mDataBlock->cameraDistance;
    if (!Marble::smEndPad.isNull() && (mMode & StoppingMode) != 0)
    {
        F32 effectTime;
        if (mEffect.effectTime >= 2.0f)
            effectTime = 1.0f;
        else
            effectTime = mEffect.effectTime * 0.5f;

        effectTime *= 0.5f * mDataBlock->cameraDistance;

        endPos -= forwardDir * effectTime;
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
        Point3F preStartPos = startCam;
        if (!isCameraClear(position, startCam))
            preStartPos = position;

        moveCamera(preStartPos, endPos, position, 4, 1.0f);
    }

    resetPlatformsForCamera();

#ifdef MBXP_DYNAMIC_CAMERA
    mLastCamPos = position + mCameraPosition;
    mOOBCamPos = position + mCameraPosition;
#else
    mLastCamPos = position;
    mOOBCamPos = position;
#endif
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
#ifdef MBXP_DYNAMIC_CAMERA
    mat->setColumn(3, mCameraPosition + position);
#else
    mat->setColumn(3, position);
#endif

#ifdef MBXP_CAMERA_SHAKE
    MatrixF camFX = gCamFXMgr.getTrans();
    if (!camFX.isIdentity())
    {
        mat->mul(camFX);
    }
#endif
}

ConsoleMethod(Marble, cameraLookAtPt, void, 3, 3, "(point)")
{
    Point3F pt;
    dSscanf(argv[2], "%f %f %f", &pt.x, &pt.y, &pt.z);
    object->cameraLookAtPt(pt);
}
