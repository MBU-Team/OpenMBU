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

    bool moveSuccess = false;
    U32 i = 0;
    for (bool flag = timeStep > 0.0f; flag && i < maxIterations; flag = totalTime > 0.0f)
    {
        if (testMove(velocity, position, time, 0.25, sCameraCollisionMask, true))
        {
            moveSuccess = true;

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

    if (totalTime >= 0.000001 || (maxIterations <= 1 && moveSuccess))
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
    // TODO: Cleanup this mess of decompiled code
    double v1;
    float v3;
    float valuea;

    if (value >= 0.0)
        v1 = 1.0;
    else
        v1 = -1.0;
    v3 = v1;
    valuea = fabsf(value);
    return (F32)(mPow(valuea, 3.2f) * v3);
}

F32 rescaleDeadZone(F32 value, F32 deadZone)
{
    // TODO: Cleanup this mess of decompiled code
    double v2;
    double v3;
    double result;
    float valuea;
    float valueb;

    v2 = value;
    v3 = deadZone;
    if (deadZone >= (double)value)
    {
        if (-v3 <= v2)
        {
            result = 0.0;
        }
        else
        {
            valueb = (v2 + v3) / (1.0 - v3);
            result = valueb;
        }
    }
    else
    {
        valuea = (v2 - v3) / (1.0 - v3);
        result = valuea;
    }
    return result;
}

F32 computePitchSpeedFromDelta(F32 delta)
{
    // TODO: Cleanup this mess of decompiled code
    double v1;

    if (delta < 0.3141592700403172)
    {
        v1 = 0.31415927;
    LABEL_5:
        delta = v1;
        return (float)(delta * 4.0);
    }
    if (delta > 1.570796326794897)
    {
        v1 = 1.5707964;
        goto LABEL_5;
    }
    return (float)(delta * 4.0);
}

void Marble::processCameraMove(const Move* move)
{
    delta.prevMouseX = mMouseX;
    delta.prevMouseY = mMouseY;

    Point2F value(move->yaw / M_PI_F, move->pitch / M_PI_F);

    pushToSquare(value);

    // ----------------------------------------------------------------------------
    // TODO: Cleanup this mess of decompiled code
    // ----------------------------------------------------------------------------

    double v4;
    double v5;
    double v6;
    bool v7;
    double v8;
    double v9;
    double v10;
    double v11;
    double v12;
    double v13;
    double v14;
    double v16;
    float deadZone;
    double v20;
    float delta_new;
    float movePitchc;
    float movePitch;
    float movePitchd;
    float movePitche;
    float movePitcha;
    float movePitchf;
    float movePitchg;
    float movePitchh;
    float movePitchi;
    float movePitchj;
    float movePitchk;
    float movePitchb;

    deadZone = rescaleDeadZone(value.x, 0.25);
    movePitch = applyNonlinearScale2(deadZone);
    *((float*)&v20 + 1) = fabsf(movePitch);
    delta_new = this->mLastYaw;
    v20 = *((float*)&v20 + 1);
    v4 = fabsf(delta_new);
    if (v4 > v20)
        goto LABEL_2;
    if (this->mLastYaw < 0.0 && movePitch > 0.0 || this->mLastYaw > 0.0 && movePitch < 0.0)
        this->mLastYaw = 0.0;
    if (fabsf(movePitch) > 0.2)
    {
        *((float*)&v20 + 1) = this->mLastYaw;
        if (fabsf(*((float*)&v20 + 1)) < 0.2)
        {
            if (movePitch >= 0.0)
                v6 = 0.2;
            else
                v6 = -0.2;
            this->mLastYaw = v6;
        }
    }
    delta_new = movePitch - this->mLastYaw;
    if (fabsf(delta_new) <= 0.0544000044465065)
    {
    LABEL_2:
        v5 = movePitch;
    }
    else if (delta_new >= 0.0)
    {
        v5 = this->mLastYaw + 0.0544000044465065;
    }
    else
    {
        v5 = this->mLastYaw - 0.0544000044465065;
    }
    v7 = (this->mMode & CameraHoverMode) == 0;
    this->mLastYaw = v5;
    delta_new = this->mLastYaw * 0.7 * 6.283185307179586 * 0.03200000151991844;
    if (!v7)
    {
        delta_new = 0.02;
        goto LABEL_31;
    }
    if (this->mCenteringCamera)
    {
        *((float*)&v20 + 1) = fabsf(delta_new);
        movePitchd = fabsf(this->mRadsLeftToCenter);
        movePitche = *((float*)&v20 + 1) - movePitchd;
        movePitcha = fabsf(movePitche);
        if (movePitcha >= 0.15)
        {
            movePitchf = this->mRadsLeftToCenter / this->mRadsStartingToCenter;
            v8 = sinf(movePitchf) * 0.15;
        }
        else
        {
            if (movePitcha < 0.05)
            {
                this->mCenteringCamera = 0;
            LABEL_27:
                if (this->mRadsLeftToCenter <= (double)delta_new)
                {
                    delta_new = delta_new - movePitcha;
                    v9 = this->mRadsLeftToCenter + movePitcha;
                }
                else
                {
                    delta_new = movePitcha + delta_new;
                    v9 = this->mRadsLeftToCenter - movePitcha;
                }
                this->mRadsLeftToCenter = v9;
                goto LABEL_31;
            }
            v8 = 0.050000001;
        }
        movePitcha = v8;
        goto LABEL_27;
    }
LABEL_31:
    if ((this->mMode & CameraHoverMode) == 0)
    {
        if (move->deviceIsKeyboardMouse)
        {
            v10 = this->mMouseY + value.y;
            goto LABEL_48;
        }
        movePitchi = rescaleDeadZone(value.y, 0.69999999);
        v12 = movePitchi;
        if (movePitchi <= 0.0)
            v13 = 0.4 - v12 * -0.75;
        else
            v13 = v12 * 1.1 + 0.4;
        movePitchj = v13;
        *((float*)&v20 + 1) = movePitchj - this->mMouseY;
        movePitchk = fabsf(*((float*)&v20 + 1));
        movePitchb = computePitchSpeedFromDelta(movePitchk) * 0.03200000151991844 * 0.8;
        v14 = *((float*)&v20 + 1);
        if (*((float*)&v20 + 1) <= 0.0)
        {
            *((float*)&v20 + 1) = -v14;
            if (*((float*)&v20 + 1) < (double)movePitchb)
                movePitchb = *((float*)&v20 + 1);
            v14 = -movePitchb;
        }
        else if (movePitchb <= v14)
        {
        LABEL_46:
            v10 = this->mMouseY + movePitchb;
            goto LABEL_48;
        }
        movePitchb = v14;
        goto LABEL_46;
    }
    if (this->mMouseY < 0.69999999)
    {
        movePitchg = 0.699999988079071 - this->mMouseY;
        v10 = getMin(0.050000001f, movePitchg) + this->mMouseY;
    LABEL_48:
        this->mMouseY = v10;
        goto LABEL_49;
    }
    if (this->mMouseY > 0.69999999)
    {
        v20 = this->mMouseY;
        movePitchh = v20 - 0.699999988079071;
        v11 = getMin(0.050000001f, movePitchh);
        v10 = v20 - v11;
        goto LABEL_48;
    }
LABEL_49:

    // ----------------------------------------------------------------------------
    // End of Uncleaned Decompiled Code
    // ----------------------------------------------------------------------------

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

    start.z = 0.25f;

    Point3D ep = end;
    findContacts(sCameraCollisionMask, &ep, &start.z);

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

void Marble::setPlatformsForCamera(const Point3F& marblePos, const Point3F& startCam, const Point3F& endCam)
{
    smPathItrVec.clear();

    Box3F camBox = mObjBox;
    camBox.min = marblePos + camBox.min;
    camBox.max = marblePos + camBox.max;

    camBox.min.setMin(startCam - 0.25f);
    camBox.max.setMax(startCam + 0.25f);
    camBox.min.setMin(endCam - 0.25f);
    camBox.max.setMax(endCam + 0.25f);
    
    camBox.min -= 0.25f;
    camBox.max += 0.25f;

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
