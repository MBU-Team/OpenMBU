//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/vehicles/vehicleBlocker.h"
#include "core/bitStream.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "math/mathIO.h"
#include "console/consoleTypes.h"

IMPLEMENT_CO_NETOBJECT_V1(VehicleBlocker);


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
VehicleBlocker::VehicleBlocker()
{
    mNetFlags.set(Ghostable | ScopeAlways);

    mTypeMask = VehicleBlockerObjectType;

    mConvexList = new Convex;
}

VehicleBlocker::~VehicleBlocker()
{
    delete mConvexList;
    mConvexList = NULL;
}

//--------------------------------------------------------------------------
void VehicleBlocker::initPersistFields()
{
    Parent::initPersistFields();
    addField("dimensions", TypePoint3F, Offset(mDimensions, VehicleBlocker));
}

//--------------------------------------------------------------------------
bool VehicleBlocker::onAdd()
{
    if (!Parent::onAdd())
        return false;

    mObjBox.min.set(-mDimensions.x, -mDimensions.y, 0);
    mObjBox.max.set(mDimensions.x, mDimensions.y, mDimensions.z);
    resetWorldBox();
    setRenderTransform(mObjToWorld);

    addToScene();

    return true;
}


void VehicleBlocker::onRemove()
{
    mConvexList->nukeList();
    removeFromScene();

    Parent::onRemove();
}


U32 VehicleBlocker::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    mathWrite(*stream, getTransform());
    mathWrite(*stream, getScale());
    mathWrite(*stream, mDimensions);

    return retMask;
}


void VehicleBlocker::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    MatrixF mat;
    Point3F scale;
    Box3F objBox;
    mathRead(*stream, &mat);
    mathRead(*stream, &scale);
    mathRead(*stream, &mDimensions);
    mObjBox.min.set(-mDimensions.x, -mDimensions.y, 0);
    mObjBox.max.set(mDimensions.x, mDimensions.y, mDimensions.z);
    setScale(scale);
    setTransform(mat);
}


void VehicleBlocker::buildConvex(const Box3F& box, Convex* convex)
{
    // These should really come out of a pool
    mConvexList->collectGarbage();

    if (box.isOverlapped(getWorldBox()) == false)
        return;

    // Just return a box convex for the entire shape...
    Convex* cc = 0;
    CollisionWorkingList& wl = convex->getWorkingList();
    for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
    {
        if (itr->mConvex->getType() == BoxConvexType &&
            itr->mConvex->getObject() == this) {
            cc = itr->mConvex;
            break;
        }
    }
    if (cc)
        return;

    // Create a new convex.
    BoxConvex* cp = new BoxConvex;
    mConvexList->registerObject(cp);
    convex->addToWorkingList(cp);
    cp->init(this);

    mObjBox.getCenter(&cp->mCenter);
    cp->mSize.x = mObjBox.len_x() / 2.0f;
    cp->mSize.y = mObjBox.len_y() / 2.0f;
    cp->mSize.z = mObjBox.len_z() / 2.0f;
}

