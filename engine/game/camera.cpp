//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "core/dnet.h"
#include "game/camera.h"
#include "game/gameConnection.h"
#include "math/mathIO.h"
#include "editor/editor.h"
#include "sim/processList.h"

#define MaxPitch 1.5706
#define CameraRadius	0.05;


//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(CameraData);

void CameraData::initPersistFields()
{
    Parent::initPersistFields();
}

void CameraData::packData(BitStream* stream)
{
    Parent::packData(stream);
}

void CameraData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
}


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(Camera);
F32 Camera::mMovementSpeed = 40;

Camera::Camera()
{
    mNetFlags.clear(Ghostable);
    mTypeMask |= CameraObjectType;
    delta.pos = Point3F(0, 0, 100);
    delta.rot = Point3F(0, 0, 0);
    delta.posVec = delta.rotVec = VectorF(0, 0, 0);
    mObjToWorld.setColumn(3, delta.pos);
    mRot = delta.rot;

    mMinOrbitDist = 0;
    mMaxOrbitDist = 0;
    mCurOrbitDist = 0;
    mOrbitObject = NULL;
    mPosition.set(0.f, 0.f, 0.f);
    mObservingClientObject = false;
    mode = 2;
}

Camera::~Camera()
{
}


//----------------------------------------------------------------------------

bool Camera::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mObjBox.max = mObjScale;
    mObjBox.min = mObjScale;
    mObjBox.min.neg();
    resetWorldBox();

    if (isClientObject() || gSPMode)
        getCurrentClientContainer()->addObject(this);
    else
        getCurrentServerContainer()->addObject(this);

    //  addToScene();
    return true;
}

void Camera::onEditorEnable()
{
    mNetFlags.set(Ghostable);
}

void Camera::onEditorDisable()
{
    mNetFlags.clear(Ghostable);
}

void Camera::onRemove()
{
    //   removeFromScene();
    if (getContainer())
        getContainer()->removeObject(this);

    Parent::onRemove();
}


//----------------------------------------------------------------------------
// check if the object needs to be observed through its own camera...
void Camera::getCameraTransform(F32* pos, MatrixF* mat)
{
    // The camera doesn't support a third person mode,
    // so we want to override the default ShapeBase behavior.
    ShapeBase* obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mOrbitObject));
    if (obj && static_cast<ShapeBaseData*>(obj->getDataBlock())->observeThroughObject)
        obj->getCameraTransform(pos, mat);
    else
        getEyeTransform(mat);
}

F32 Camera::getCameraFov()
{
    ShapeBase* obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mOrbitObject));
    if (obj && static_cast<ShapeBaseData*>(obj->getDataBlock())->observeThroughObject)
        return(obj->getCameraFov());
    else
        return(Parent::getCameraFov());
}

F32 Camera::getDefaultCameraFov()
{
    ShapeBase* obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mOrbitObject));
    if (obj && static_cast<ShapeBaseData*>(obj->getDataBlock())->observeThroughObject)
        return(obj->getDefaultCameraFov());
    else
        return(Parent::getDefaultCameraFov());
}

bool Camera::isValidCameraFov(F32 fov)
{
    ShapeBase* obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mOrbitObject));
    if (obj && static_cast<ShapeBaseData*>(obj->getDataBlock())->observeThroughObject)
        return(obj->isValidCameraFov(fov));
    else
        return(Parent::isValidCameraFov(fov));
}

void Camera::setCameraFov(F32 fov)
{
    ShapeBase* obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mOrbitObject));
    if (obj && static_cast<ShapeBaseData*>(obj->getDataBlock())->observeThroughObject)
        obj->setCameraFov(fov);
    else
        Parent::setCameraFov(fov);
}

//----------------------------------------------------------------------------
void Camera::processTick(const Move* move)
{
    Parent::processTick(move);
    Point3F vec, pos;
    if (move) {
        // If using editor then force camera into fly mode
        if (gEditingMission && mode != FlyMode)
            setFlyMode();

        // Update orientation
        delta.rotVec = mRot;
        mObjToWorld.getColumn(3, &delta.posVec);

        mRot.x += move->pitch;
        if (mRot.x > MaxPitch)
            mRot.x = MaxPitch;
        else if (mRot.x < -MaxPitch)
            mRot.x = -MaxPitch;

        mRot.z += move->yaw;

        if (mode == OrbitObjectMode || mode == OrbitPointMode)
        {
            if (mode == OrbitObjectMode && bool(mOrbitObject)) {
                // If this is a shapebase, use its render eye transform
                // to avoid jittering.
                GameBase* castObj = mOrbitObject;
                ShapeBase* shape = dynamic_cast<ShapeBase*>(castObj);
                if (shape != NULL) {
                    MatrixF ret;
                    shape->getRenderEyeTransform(&ret);
                    mPosition = ret.getPosition();
                }
                else {
                    // Hopefully this is a static object that doesn't move,
                    // because the worldbox doesn't get updated between ticks.
                    mOrbitObject->getWorldBox().getCenter(&mPosition);
                }
            }
            setPosition(mPosition, mRot);
            validateEyePoint(1.0f, &mObjToWorld);
            pos = mPosition;
        }
        else
        {
            // Update pos
            bool faster = move->trigger[0] || move->trigger[1];
            F32 scale = mMovementSpeed * (faster + 1);

            mObjToWorld.getColumn(3, &pos);
            mObjToWorld.getColumn(0, &vec);
            pos += vec * move->x * TickSec * scale;
            mObjToWorld.getColumn(1, &vec);
            pos += vec * move->y * TickSec * scale;
            mObjToWorld.getColumn(2, &vec);
            pos += vec * move->z * TickSec * scale;
            setPosition(pos, mRot);
        }

        // If on the client, calc delta for backstepping
        if (isClientObject() || gSPMode) {
            delta.pos = pos;
            delta.rot = mRot;
            delta.posVec = delta.posVec - delta.pos;
            delta.rotVec = delta.rotVec - delta.rot;
        }
        setMaskBits(MoveMask);
    }

    if (getControllingClient() && mContainer)
        updateContainer();
}

void Camera::onDeleteNotify(SimObject* obj)
{
    Parent::onDeleteNotify(obj);
    if (obj == (SimObject*)mOrbitObject)
    {
        mOrbitObject = NULL;

        if (mode == OrbitObjectMode)
            mode = OrbitPointMode;
    }
}

void Camera::interpolateTick(F32 dt)
{
    Parent::interpolateTick(dt);
#ifndef MB_ULTRA_PREVIEWS
    Point3F rot = delta.rot + delta.rotVec * dt;

    if (mode == OrbitObjectMode || mode == OrbitPointMode)
    {
        if (mode == OrbitObjectMode && bool(mOrbitObject))
        {
            // If this is a shapebase, use its render eye transform
            // to avoid jittering.
            GameBase* castObj = mOrbitObject;
            ShapeBase* shape = dynamic_cast<ShapeBase*>(castObj);
            if (shape != NULL) {
                MatrixF ret;
                shape->getRenderEyeTransform(&ret);
                mPosition = ret.getPosition();
            }
            else {
                // Hopefully this is a static object that doesn't move,
                // because the worldbox doesn't get updated between ticks.
                mOrbitObject->getWorldBox().getCenter(&mPosition);
            }
        }
        setPosition(mPosition, rot);
        validateEyePoint(1.0f, &mObjToWorld);
    }
    else {
        Point3F pos = delta.pos + delta.posVec * dt;
        setPosition(pos, rot);
    }
#endif
}

void Camera::setPosition(const Point3F& pos, const Point3F& rot)
{
    MatrixF xRot, zRot;
    xRot.set(EulerF(rot.x, 0, 0));
    zRot.set(EulerF(0, 0, rot.z));
    MatrixF temp;
    temp.mul(zRot, xRot);
    temp.setColumn(3, pos);
    Parent::setTransform(temp);
    mRot = rot;
}


//----------------------------------------------------------------------------

void Camera::writePacketData(GameConnection* connection, BitStream* bstream)
{
    // Update client regardless of status flags.
    Parent::writePacketData(connection, bstream);

    Point3F pos;
    mObjToWorld.getColumn(3, &pos);
    bstream->setCompressionPoint(pos);
    mathWrite(*bstream, pos);
    bstream->write(mRot.x);
    bstream->write(mRot.z);

    U32 writeMode = mode;
    Point3F writePos = mPosition;
    S32 gIndex = -1;
    if (mode == OrbitObjectMode)
    {
        gIndex = bool(mOrbitObject) ? connection->getGhostIndex(mOrbitObject) : -1;
        if (gIndex == -1)
        {
            writeMode = OrbitPointMode;
            mOrbitObject->getWorldBox().getCenter(&writePos);
        }
    }
    bstream->writeRangedU32(writeMode, CameraFirstMode, CameraLastMode);

    if (writeMode == OrbitObjectMode || writeMode == OrbitPointMode)
    {
        bstream->write(mMinOrbitDist);
        bstream->write(mMaxOrbitDist);
        bstream->write(mCurOrbitDist);
        if (writeMode == OrbitObjectMode)
        {
            bstream->writeFlag(mObservingClientObject);
            bstream->writeInt(gIndex, NetConnection::GhostIdBitSize);
        }
        if (writeMode == OrbitPointMode)
            bstream->writeCompressedPoint(writePos);
    }
}

void Camera::readPacketData(GameConnection* connection, BitStream* bstream)
{
    Parent::readPacketData(connection, bstream);
    Point3F pos, rot;
    mathRead(*bstream, &pos);
    bstream->setCompressionPoint(pos);
    bstream->read(&rot.x);
    bstream->read(&rot.z);

    GameBase* obj = 0;
    mode = bstream->readRangedU32(CameraFirstMode, CameraLastMode);
    mObservingClientObject = false;
    if (mode == OrbitObjectMode || mode == OrbitPointMode) {
        bstream->read(&mMinOrbitDist);
        bstream->read(&mMaxOrbitDist);
        bstream->read(&mCurOrbitDist);

        if (mode == OrbitObjectMode)
        {
            mObservingClientObject = bstream->readFlag();
            S32 gIndex = bstream->readInt(NetConnection::GhostIdBitSize);
            obj = static_cast<GameBase*>(connection->resolveGhost(gIndex));
        }
        if (mode == OrbitPointMode)
            bstream->readCompressedPoint(&mPosition);
    }
    if (obj != (GameBase*)mOrbitObject) {
        if (mOrbitObject) {
            clearProcessAfter();
            clearNotify(mOrbitObject);
        }
        mOrbitObject = obj;
        if (mOrbitObject) {
            processAfter(mOrbitObject);
            deleteNotify(mOrbitObject);
        }
    }

    setPosition(pos, rot);
    delta.pos = pos;
    delta.rot = rot;
    delta.rotVec.set(0, 0, 0);
    delta.posVec.set(0, 0, 0);
}

U32 Camera::packUpdate(NetConnection* con, U32 mask, BitStream* bstream)
{
    Parent::packUpdate(con, mask, bstream);

    // The rest of the data is part of the control object packet update.
    // If we're controlled by this client, we don't need to send it.
    if (bstream->writeFlag(getControllingClient() == con && !(mask & InitialUpdateMask)))
        return 0;

    if (bstream->writeFlag(mask & MoveMask)) {
        Point3F pos;
        mObjToWorld.getColumn(3, &pos);
        bstream->write(pos.x);
        bstream->write(pos.y);
        bstream->write(pos.z);
        bstream->write(mRot.x);
        bstream->write(mRot.z);
    }

    return 0;
}

void Camera::unpackUpdate(NetConnection* con, BitStream* bstream)
{
    Parent::unpackUpdate(con, bstream);

    // controlled by the client?
    if (bstream->readFlag())
        return;

    if (bstream->readFlag()) {
        Point3F pos, rot;
        bstream->read(&pos.x);
        bstream->read(&pos.y);
        bstream->read(&pos.z);
        bstream->read(&rot.x);
        bstream->read(&rot.z);
        setPosition(pos, rot);

        // New delta for client side interpolation
        delta.pos = pos;
        delta.rot = rot;
        delta.posVec = delta.rotVec = VectorF(0, 0, 0);
    }
}


//----------------------------------------------------------------------------

void Camera::initPersistFields()
{
    Parent::initPersistFields();
}

void Camera::consoleInit()
{
    Con::addVariable("Camera::movementSpeed", TypeF32, &mMovementSpeed);
}

Point3F& Camera::getPosition()
{
    static Point3F position;
    mObjToWorld.getColumn(3, &position);
    return position;
}

ConsoleMethod(Camera, getPosition, const char*, 2, 2, "()"
    "Get the position of the camera.\n\n"
    "@returns A string of form \"x y z\".")
{
    static char buffer[100];

    Point3F& pos = object->getPosition();
    dSprintf(buffer, sizeof(buffer), "%g %g %g", pos.x, pos.y, pos.z);
    return buffer;
}

ConsoleMethod(Camera, setOrbitMode, void, 7, 8, "(GameBase orbitObject, transform mat, float minDistance,"
    " float maxDistance, float curDistance, bool ownClientObject)"
    "Set the camera to orbit around some given object.\n\n"
    "@param   orbitObject  Object we want to orbit.\n"
    "@param   mat          A set of fields: posX posY posZ aaX aaY aaZ aaTheta\n"
    "@param   minDistance  Minimum distance to keep from object.\n"
    "@param   maxDistance  Maximum distance to keep from object.\n"
    "@param   curDistance  Distance to set initially from object.\n"
    "@param   ownClientObj Are we observing an object owned by us?")
{
    Point3F pos;
    AngAxisF aa;
    F32 minDis, maxDis, curDis;

    GameBase* orbitObject = NULL;
    if (Sim::findObject(argv[2], orbitObject) == false)
    {
        Con::warnf("Cannot orbit non-existing object.");
        object->setFlyMode();
        return;
    }

    dSscanf(argv[3], "%g %g %g %g %g %g %g",
        &pos.x, &pos.y, &pos.z, &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);
    minDis = dAtof(argv[4]);
    maxDis = dAtof(argv[5]);
    curDis = dAtof(argv[6]);

    object->setOrbitMode(orbitObject, pos, aa, minDis, maxDis, curDis, (argc == 8) ? dAtob(argv[7]) : false);
}

ConsoleMethod(Camera, setFlyMode, void, 2, 2, "()"
    "Set the camera to be able to fly freely.")
{
    object->setFlyMode();
}

//----------------------------------------------------------------------------

void Camera::renderImage(SceneState*)
{
    if (gEditingMission)
    {
        /*
              glPushMatrix();
              dglMultMatrix(&mObjToWorld);
              glScalef(mObjScale.x,mObjScale.y,mObjScale.z);
              wireCube(Point3F(1, 1, 1),Point3F(0,0,0));
              glPopMatrix();
        */
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//    NEW Observer Code
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void Camera::setFlyMode()
{
    mode = FlyMode;

    if (bool(mOrbitObject)) {
        clearProcessAfter();
        clearNotify(mOrbitObject);
    }
    mOrbitObject = NULL;
}

void Camera::setOrbitMode(GameBase* obj, Point3F& pos, AngAxisF& rot, F32 minDist, F32 maxDist, F32 curDist, bool ownClientObject)
{
    mObservingClientObject = ownClientObject;

    rot;
    if (bool(mOrbitObject)) {
        clearProcessAfter();
        clearNotify(mOrbitObject);
    }
    mOrbitObject = obj;
    if (bool(mOrbitObject))
    {
        processAfter(mOrbitObject);
        deleteNotify(mOrbitObject);
        mOrbitObject->getWorldBox().getCenter(&mPosition);
        mode = OrbitObjectMode;
    }
    else
    {
        mode = OrbitPointMode;
        mPosition = pos;
    }

    QuatF q(rot);
    MatrixF tempMat(true);
    q.setMatrix(&tempMat);
    Point3F dir;
    tempMat.getColumn(1, &dir);

    setPosition(mPosition, dir);

    mMinOrbitDist = minDist;
    mMaxOrbitDist = maxDist;
    mCurOrbitDist = curDist;
}


void Camera::validateEyePoint(F32 pos, MatrixF* mat)
{
    if (pos != 0) {
        // Use the eye transform to orient the camera
        Point3F dir;
        mat->getColumn(1, &dir);
        pos *= mMaxOrbitDist - mMinOrbitDist;
        // Use the camera node's pos.
        Point3F startPos;
        Point3F endPos;
        mObjToWorld.getColumn(3, &startPos);

        // Make sure we don't extend the camera into anything solid
        if (mOrbitObject)
            mOrbitObject->disableCollision();
        disableCollision();
        RayInfo collision;
        U32 mask = AtlasObjectType |
            TerrainObjectType |
            InteriorObjectType |
            WaterObjectType |
            StaticShapeObjectType |
            PlayerObjectType |
            ItemObjectType |
            VehicleObjectType;

        Container* pContainer = isServerObject() ? getCurrentServerContainer() : getCurrentClientContainer();
        if (!pContainer->castRay(startPos, startPos - dir * 2.5 * pos, mask, &collision))
            endPos = startPos - dir * pos;
        else
        {
            float dot = mDot(dir, collision.normal);
            if (dot > 0.01)
            {
                float colDist = mDot(startPos - collision.point, dir) - (1 / dot) * CameraRadius;
                if (colDist > pos)
                    colDist = pos;
                if (colDist < 0)
                    colDist = 0;
                endPos = startPos - dir * colDist;
            }
            else
                endPos = startPos - dir * pos;
        }
        mat->setColumn(3, endPos);
        enableCollision();
        if (mOrbitObject)
            mOrbitObject->enableCollision();
    }
}

void Camera::setPosition(const Point3F& pos, const Point3F& rot, MatrixF* mat)
{
    MatrixF xRot, zRot;
    xRot.set(EulerF(rot.x, 0, 0));
    zRot.set(EulerF(0, 0, rot.z));
    mat->mul(zRot, xRot);
    mat->setColumn(3, pos);
    mRot = rot;
}

void Camera::setTransform(const MatrixF& mat)
{
    // This method should never be called on the client.

    // This currently converts all rotation in the mat into
    // rotations around the z and x axis.
    Point3F pos, vec;
    mat.getColumn(1, &vec);
    mat.getColumn(3, &pos);
    Point3F rot(-mAtan(vec.z, mSqrt(vec.x * vec.x + vec.y * vec.y)), 0, -mAtan(-vec.x, vec.y));
    setPosition(pos, rot);
}

F32 Camera::getDamageFlash() const
{
    if (mode == OrbitObjectMode && isServerObject() && bool(mOrbitObject))
    {
        const GameBase* castObj = mOrbitObject;
        const ShapeBase* psb = dynamic_cast<const ShapeBase*>(castObj);
        if (psb)
            return psb->getDamageFlash();
    }

    return mDamageFlash;
}

F32 Camera::getWhiteOut() const
{
    if (mode == OrbitObjectMode && isServerObject() && bool(mOrbitObject))
    {
        const GameBase* castObj = mOrbitObject;
        const ShapeBase* psb = dynamic_cast<const ShapeBase*>(castObj);
        if (psb)
            return psb->getWhiteOut();
    }

    return mWhiteOut;
}

