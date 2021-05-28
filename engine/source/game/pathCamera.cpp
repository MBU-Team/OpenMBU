//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "math/mMath.h"
#include "math/mathIO.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "core/dnet.h"
#include "sim/pathManager.h"
#include "game/game.h"
#include "game/gameConnection.h"
#include "editor/editor.h"
#include "sim/processList.h"

#include "game/pathCamera.h"


//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(PathCameraData);

void PathCameraData::consoleInit()
{
}

void PathCameraData::initPersistFields()
{
    Parent::initPersistFields();
}

void PathCameraData::packData(BitStream* stream)
{
    Parent::packData(stream);
}

void PathCameraData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
}


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(PathCamera);

PathCamera::PathCamera()
{
    mNetFlags.clear(Ghostable);
    mTypeMask |= CameraObjectType;
    delta.time = 0;
    delta.timeVec = 0;

    mDataBlock = 0;
    mState = Forward;
    mNodeBase = 0;
    mNodeCount = 0;
    mPosition = 0;
    mTarget = 0;
    mTargetSet = false;

    MatrixF mat(1);
    mat.setPosition(Point3F(0, 0, 700));
    Parent::setTransform(mat);
}

PathCamera::~PathCamera()
{
}


//----------------------------------------------------------------------------

bool PathCamera::onAdd()
{
    if (!Parent::onAdd())
        return false;

    // Initialize from the current transform.
    if (!mNodeCount) {
        QuatF rot(getTransform());
        Point3F pos = getPosition();
        mSpline.removeAll();
        mSpline.push_back(new CameraSpline::Knot(pos, rot, 1,
            CameraSpline::Knot::NORMAL, CameraSpline::Knot::SPLINE));
        mNodeCount = 1;
    }

    //
    mObjBox.max = mObjScale;
    mObjBox.min = mObjScale;
    mObjBox.min.neg();
    resetWorldBox();

    if (mShapeInstance) {
        mNetFlags.set(Ghostable);
        setScopeAlways();
        addToScene();
    }
    else
        if (getContainer())
            getContainer()->addObject(this);

    return true;
}

void PathCamera::onRemove()
{
    if (getContainer())
        getContainer()->removeObject(this);

    if (mShapeInstance)
        removeFromScene();
    Parent::onRemove();
}

bool PathCamera::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<PathCameraData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    scriptOnNewDataBlock();
    return true;
}

//----------------------------------------------------------------------------

void PathCamera::onEditorEnable()
{
}

void PathCamera::onEditorDisable()
{
}


//----------------------------------------------------------------------------

void PathCamera::initPersistFields()
{
    Parent::initPersistFields();
}

void PathCamera::consoleInit()
{
}


//----------------------------------------------------------------------------

void PathCamera::processTick(const Move* move)
{
    // client and server
    Parent::processTick(move);

    // Move to new time
    advancePosition(TickMs);

    // Set new position
    MatrixF mat;
    interpolateMat(mPosition, &mat);
    Parent::setTransform(mat);
}

void PathCamera::interpolateTick(F32 dt)
{
    Parent::interpolateTick(dt);
    MatrixF mat;
    interpolateMat(delta.time + (delta.timeVec * dt), &mat);
    Parent::setRenderTransform(mat);
}

void PathCamera::interpolateMat(F32 pos, MatrixF* mat)
{
    CameraSpline::Knot knot;
    mSpline.value(pos - mNodeBase, &knot);
    knot.mRotation.setMatrix(mat);
    mat->setPosition(knot.mPosition);
}

void PathCamera::advancePosition(S32 ms)
{
    delta.timeVec = mPosition;

    // Advance according to current speed
    if (mState == Forward) {
        mPosition = mSpline.advanceTime(mPosition - mNodeBase, ms);
        if (mPosition > mNodeCount - 1)
            mPosition = mNodeCount - 1;
        mPosition += mNodeBase;
        if (mTargetSet && mPosition >= mTarget) {
            mTargetSet = false;
            mPosition = mTarget;
            mState = Stop;
        }
    }
    else
        if (mState == Backward) {
            mPosition = mSpline.advanceTime(mPosition - mNodeBase, -ms);
            if (mPosition < 0)
                mPosition = 0;
            mPosition += mNodeBase;
            if (mTargetSet && mPosition <= mTarget) {
                mTargetSet = false;
                mPosition = mTarget;
                mState = Stop;
            }
        }

    // Script callbacks
    if (int(mPosition) != int(delta.timeVec))
        onNode(int(mPosition));

    // Set frame interpolation
    delta.time = mPosition;
    delta.timeVec -= mPosition;
}


//----------------------------------------------------------------------------

void PathCamera::getCameraTransform(F32* pos, MatrixF* mat)
{
    // Overide the ShapeBase method to skip all the first/third person support.
    getRenderEyeTransform(mat);
}


//----------------------------------------------------------------------------

void PathCamera::setPosition(F32 pos)
{
    mPosition = mClampF(pos, mNodeBase, mNodeBase + mNodeCount - 1);
    MatrixF mat;
    interpolateMat(mPosition, &mat);
    Parent::setTransform(mat);
    setMaskBits(PositionMask);
}

void PathCamera::setTarget(F32 pos)
{
    mTarget = pos;
    mTargetSet = true;
    if (mTarget > mPosition)
        mState = Forward;
    else
        if (mTarget < mPosition)
            mState = Backward;
        else {
            mTargetSet = false;
            mState = Stop;
        }
    setMaskBits(TargetMask | StateMask);
}

void PathCamera::setState(State s)
{
    mState = s;
    setMaskBits(StateMask);
}


//-----------------------------------------------------------------------------

void PathCamera::reset(F32 speed)
{
    CameraSpline::Knot* knot = new CameraSpline::Knot;
    mSpline.value(mPosition - mNodeBase, knot);
    if (speed)
        knot->mSpeed = speed;
    mSpline.removeAll();
    mSpline.push_back(knot);

    mNodeBase = 0;
    mNodeCount = 1;
    mPosition = 0;
    mTargetSet = false;
    mState = Forward;
    setMaskBits(StateMask | PositionMask | WindowMask | TargetMask);
}

void PathCamera::pushBack(CameraSpline::Knot* knot)
{
    // Make room at the end
    if (mNodeCount == NodeWindow) {
        delete mSpline.remove(mSpline.getKnot(0));
        mNodeBase++;
    }
    else
        mNodeCount++;

    // Fill in the new node
    mSpline.push_back(knot);
    setMaskBits(WindowMask);

    // Make sure the position doesn't fall off
    if (mPosition < mNodeBase) {
        mPosition = mNodeBase;
        setMaskBits(PositionMask);
    }
}

void PathCamera::pushFront(CameraSpline::Knot* knot)
{
    // Make room at the front
    if (mNodeCount == NodeWindow)
        delete mSpline.remove(mSpline.getKnot(mNodeCount));
    else
        mNodeCount++;
    mNodeBase--;

    // Fill in the new node
    mSpline.push_front(knot);
    setMaskBits(WindowMask);

    // Make sure the position doesn't fall off
    if (mPosition > mNodeBase + (NodeWindow - 1)) {
        mPosition = mNodeBase + (NodeWindow - 1);
        setMaskBits(PositionMask);
    }
}

void PathCamera::popFront()
{
    if (mNodeCount < 2)
        return;

    // Remove the first node. Node base and position are unaffected.
    mNodeCount--;
    delete mSpline.remove(mSpline.getKnot(0));
}


//----------------------------------------------------------------------------

void PathCamera::onNode(S32 node)
{
    if (!isGhost())
        Con::executef(mDataBlock, 3, "onNode", scriptThis(), Con::getIntArg(node));
}


//----------------------------------------------------------------------------

void PathCamera::renderImage(SceneState* scene)
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
    Parent::renderImage(scene);
}



U32 PathCamera::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    Parent::packUpdate(con, mask, stream);

    if (stream->writeFlag(mask & StateMask))
        stream->writeInt(mState, StateBits);

    if (stream->writeFlag(mask & PositionMask))
        stream->write(mPosition);

    if (stream->writeFlag(mask & TargetMask))
        if (stream->writeFlag(mTargetSet))
            stream->write(mTarget);

    if (stream->writeFlag(mask & WindowMask)) {
        stream->write(mNodeBase);
        stream->write(mNodeCount);
        for (int i = 0; i < mNodeCount; i++) {
            CameraSpline::Knot* knot = mSpline.getKnot(i);
            mathWrite(*stream, knot->mPosition);
            mathWrite(*stream, knot->mRotation);
            stream->write(knot->mSpeed);
            stream->writeInt(knot->mType, CameraSpline::Knot::NUM_TYPE_BITS);
            stream->writeInt(knot->mPath, CameraSpline::Knot::NUM_PATH_BITS);
        }
    }

    // The rest of the data is part of the control object packet update.
    // If we're controlled by this client, we don't need to send it.
    if (stream->writeFlag(getControllingClient() == con && !(mask & InitialUpdateMask)))
        return 0;

    return 0;
}

void PathCamera::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    // StateMask
    if (stream->readFlag())
        mState = stream->readInt(StateBits);

    // PositionMask
    if (stream->readFlag()) {
        stream->read(&mPosition);
        delta.time = mPosition;
        delta.timeVec = 0;
    }

    // TargetMask
    if (stream->readFlag())
        if (mTargetSet = stream->readFlag())
            stream->read(&mTarget);

    // WindowMask
    if (stream->readFlag()) {
        mSpline.removeAll();
        stream->read(&mNodeBase);
        stream->read(&mNodeCount);
        for (int i = 0; i < mNodeCount; i++) {
            CameraSpline::Knot* knot = new CameraSpline::Knot();
            mathRead(*stream, &knot->mPosition);
            mathRead(*stream, &knot->mRotation);
            stream->read(&knot->mSpeed);
            knot->mType = (CameraSpline::Knot::Type)stream->readInt(CameraSpline::Knot::NUM_TYPE_BITS);
            knot->mPath = (CameraSpline::Knot::Path)stream->readInt(CameraSpline::Knot::NUM_PATH_BITS);
            mSpline.push_back(knot);
        }
    }

    // Controlled by the client?
    if (stream->readFlag())
        return;

}


//-----------------------------------------------------------------------------
// Console access methods
//-----------------------------------------------------------------------------

ConsoleMethod(PathCamera, setPosition, void, 3, 3, "PathCamera.setPosition(pos);")
{
    object->setPosition(dAtof(argv[2]));
}

ConsoleMethod(PathCamera, setTarget, void, 3, 3, "PathCamera.setTarget(pos);")
{
    object->setTarget(dAtof(argv[2]));
}

ConsoleMethod(PathCamera, setState, void, 3, 3, "PathCamera.setState({forward,backward,stop});")
{
    if (!dStricmp(argv[2], "forward"))
        object->setState(PathCamera::Forward);
    else
        if (!dStricmp(argv[2], "backward"))
            object->setState(PathCamera::Backward);
        else
            object->setState(PathCamera::Stop);
}

ConsoleMethod(PathCamera, reset, void, 2, 3, "PathCamera.reset(speed=0);")
{
    object->reset((argc >= 3) ? dAtof(argv[2]) : 1);
}


static CameraSpline::Knot::Type resolveKnotType(const char* arg)
{
    if (dStricmp(arg, "Position Only") == 0)
        return CameraSpline::Knot::POSITION_ONLY;
    if (dStricmp(arg, "Kink") == 0)
        return CameraSpline::Knot::KINK;
    return CameraSpline::Knot::NORMAL;
}

static CameraSpline::Knot::Path resolveKnotPath(const char* arg)
{
    if (!dStricmp(arg, "Linear"))
        return CameraSpline::Knot::LINEAR;
    return CameraSpline::Knot::SPLINE;
}

ConsoleMethod(PathCamera, pushBack, void, 6, 6, "PathCamera.pushBack(transform,speed,type,path);")
{
    Point3F pos;
    AngAxisF aa(Point3F(0, 0, 0), 0);
    dSscanf(argv[2], "%g %g %g %g %g %g %g", &pos.x, &pos.y, &pos.z, &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);
    QuatF rot(aa);
    object->pushBack(new CameraSpline::Knot(pos, rot, dAtof(argv[3]), resolveKnotType(argv[4]), resolveKnotPath(argv[5])));
}

ConsoleMethod(PathCamera, pushFront, void, 6, 6, "PathCamera.pushFront(transform,speed,type,path);")
{
    Point3F pos;
    AngAxisF aa(Point3F(0, 0, 0), 0);
    dSscanf(argv[2], "%g %g %g %g %g %g %g", &pos.x, &pos.y, &pos.z, &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);
    QuatF rot(aa);
    object->pushFront(new CameraSpline::Knot(pos, rot, dAtof(argv[3]), resolveKnotType(argv[4]), resolveKnotPath(argv[5])));
}

ConsoleMethod(PathCamera, popFront, void, 2, 2, "PathCamera.popFront();")
{
    object->popFront();
}



