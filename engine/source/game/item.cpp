//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/bitStream.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "game/item.h"
#include "sim/processList.h"

#include "tsStatic.h"
#include "collision/boxConvex.h"
#include "game/shadow.h"
#include "collision/earlyOutPolyList.h"
#include "collision/extrudedPolyList.h"
#include "math/mathIO.h"

//----------------------------------------------------------------------------

const F32 sRotationSpeed = 6.0;        // Secs/Rotation
const F32 sAtRestVelocity = 0.15;      // Min speed after collision
const S32 sCollisionTimeout = 15;      // Timout value in ticks

// Client prediction
static F32 sMinWarpTicks = 0.5;        // Fraction of tick at which instant warp occures
static S32 sMaxWarpTicks = 3;          // Max warp duration in ticks

F32 Item::mGravity = -20;

const U32 sClientCollisionMask = (AtlasObjectType | TerrainObjectType |
    InteriorObjectType | StaticShapeObjectType |
    VehicleObjectType | PlayerObjectType |
    StaticTSObjectType);

const U32 sServerCollisionMask = (sClientCollisionMask |
    TriggerObjectType);

const S32 Item::csmAtRestTimer = 64;

static const U32 sgAllowedDynamicTypes = DamagableItemObjectType;

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(ItemData);

ItemData::ItemData()
{
    shadowEnable = false;
    shadowCanMove = true;
    shadowCanAnimate = true;


    friction = 0;
    elasticity = 0;

    sticky = false;
    gravityMod = 1.0;
    maxVelocity = -1;

    density = 2;
    drag = 0.5;

    genericShadowLevel = Item_GenericShadowLevel;
    noShadowLevel = Item_NoShadowLevel;
    dynamicTypeField = 0;
    pickUpName = StringTable->insert("an item");

    lightOnlyStatic = false;
    lightType = Item::NoLight;
    lightColor.set(1.f, 1.f, 1.f, 1.f);
    lightTime = 1000;
    lightRadius = 10.f;

#ifdef MARBLE_BLAST
    powerupId = 0;
#endif

#ifdef MB_ULTRA
    buddyShapeName = NULL;
    buddySequence = NULL;
    addToHUDRadar = false;
    scaleFactor = 1.0f;
#endif
}

static EnumTable::Enums itemLightEnum[] =
{
   { Item::NoLight,           "NoLight" },
   { Item::ConstantLight,     "ConstantLight" },
   { Item::PulsingLight,      "PulsingLight" }
};
static EnumTable gItemLightTypeTable(Item::NumLightTypes, &itemLightEnum[0]);

void ItemData::initPersistFields()
{
    Parent::initPersistFields();

    addField("pickUpName", TypeString, Offset(pickUpName, ItemData));

    addField("friction", TypeF32, Offset(friction, ItemData));
    addField("elasticity", TypeF32, Offset(elasticity, ItemData));
    addField("sticky", TypeBool, Offset(sticky, ItemData));
    addField("gravityMod", TypeF32, Offset(gravityMod, ItemData));
    addField("maxVelocity", TypeF32, Offset(maxVelocity, ItemData));
    addField("dynamicType", TypeS32, Offset(dynamicTypeField, ItemData));

#ifdef MB_ULTRA
    addField("addToHUDRadar", TypeBool, Offset(addToHUDRadar, ItemData));
#endif

    addField("lightType", TypeEnum, Offset(lightType, ItemData), 1, &gItemLightTypeTable);
    addField("lightColor", TypeColorF, Offset(lightColor, ItemData));
    addField("lightTime", TypeS32, Offset(lightTime, ItemData));
    addField("lightRadius", TypeF32, Offset(lightRadius, ItemData));
    addField("lightOnlyStatic", TypeBool, Offset(lightOnlyStatic, ItemData));

#ifdef MB_ULTRA
    addField("scaleFactor", TypeF32, Offset(scaleFactor, ItemData));
#endif

#ifdef MARBLE_BLAST
    addField("powerupId", TypeS32, Offset(powerupId, ItemData));
#endif

#ifdef MB_ULTRA
    addField("buddyShapeName", TypeString, Offset(buddyShapeName, ItemData));
    addField("buddySequence", TypeString, Offset(buddySequence, ItemData));
#endif
}

void ItemData::packData(BitStream* stream)
{
    Parent::packData(stream);
    stream->writeFloat(friction, 10);
    stream->writeFloat(elasticity, 10);
    stream->writeFlag(sticky);
    if (stream->writeFlag(gravityMod != 1.0))
        stream->writeFloat(gravityMod, 10);
    if (stream->writeFlag(maxVelocity != -1))
        stream->write(maxVelocity);

    if (stream->writeFlag(lightType != Item::NoLight))
    {
        AssertFatal(Item::NumLightTypes < (1 << 2), "ItemData: light type needs more bits");
        stream->writeInt(lightType, 2);
        stream->writeFloat(lightColor.red, 7);
        stream->writeFloat(lightColor.green, 7);
        stream->writeFloat(lightColor.blue, 7);
        stream->writeFloat(lightColor.alpha, 7);
        stream->write(lightTime);
        stream->write(lightRadius);
        stream->writeFlag(lightOnlyStatic);
    }
}

void ItemData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
    friction = stream->readFloat(10);
    elasticity = stream->readFloat(10);
    sticky = stream->readFlag();
    if (stream->readFlag())
        gravityMod = stream->readFloat(10);
    else
        gravityMod = 1.0;

    if (stream->readFlag())
        stream->read(&maxVelocity);
    else
        maxVelocity = -1;

    if (stream->readFlag())
    {
        lightType = stream->readInt(2);
        lightColor.red = stream->readFloat(7);
        lightColor.green = stream->readFloat(7);
        lightColor.blue = stream->readFloat(7);
        lightColor.alpha = stream->readFloat(7);
        stream->read(&lightTime);
        stream->read(&lightRadius);
        lightOnlyStatic = stream->readFlag();
    }
    else
        lightType = Item::NoLight;
}


//----------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(Item);

Item::Item()
{
#ifdef MB_ULTRA
    mTypeMask |= ItemObjectType | GameBaseHiFiObjectType;
    mNetFlags.set(HiFiPassive);
    setModStaticFields(true);
#else
    mTypeMask |= ItemObjectType;
#endif
    mDataBlock = 0;
    mCollideable = false;
    mStatic = false;
    mRotate = false;
    mRotate2 = false;
    mVelocity = VectorF(0, 0, 0);
    mAtRest = true;
    mAtRestCounter = 0;
    mInLiquid = false;
    delta.warpTicks = 0;
    delta.dt = 1;
    mCollisionObject = 0;
    mCollisionTimeout = 0;
    //   mGenerateShadow = true;

#ifdef MARBLE_BLAST
    mPermanent = false;
    mHiddenTimer = 0;
#endif

#ifdef MB_ULTRA
    mBuddy = NULL;
    mBuddyOn = false;
#endif

    mConvex.init(this);
    mWorkingQueryBox.min.set(-1e9, -1e9, -1e9);
    mWorkingQueryBox.max.set(-1e9, -1e9, -1e9);
}

Item::~Item()
{
}


//----------------------------------------------------------------------------

bool Item::onAdd()
{
    if (!Parent::onAdd() || !mDataBlock)
        return false;

    mTypeMask |= (mDataBlock->dynamicTypeField & sgAllowedDynamicTypes);

    if (mStatic)
        mAtRest = true;
    mObjToWorld.getColumn(3, &delta.pos);

    // Setup the box for our convex object...
    mObjBox.getCenter(&mConvex.mCenter);
    mConvex.mSize.x = mObjBox.len_x() / 2.0;
    mConvex.mSize.y = mObjBox.len_y() / 2.0;
    mConvex.mSize.z = mObjBox.len_z() / 2.0;
    mWorkingQueryBox.min.set(-1e9, -1e9, -1e9);
    mWorkingQueryBox.max.set(-1e9, -1e9, -1e9);

    addToScene();

    if (isServerObject())
    {
        scriptOnAdd();
    }
    else if (mDataBlock->lightType != NoLight)
    {
        Sim::getLightSet()->addObject(this);
        mDropTime = Sim::getCurrentTime();
    }

#ifdef MB_ULTRA
    if (mDataBlock->addToHUDRadar && isGhost())
        Con::executef(this, 1, "onAddHUDRadarItem");
    if (isGhost())
        createBuddy();
#endif

    return true;
}

bool Item::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<ItemData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    scriptOnNewDataBlock();

#ifdef MB_ULTRA
    if (isGhost())
        createBuddy();
#endif

    return true;
}

void Item::onRemove()
{
    mWorkingQueryBox.min.set(-1e9, -1e9, -1e9);
    mWorkingQueryBox.max.set(-1e9, -1e9, -1e9);

#ifdef MB_ULTRA
    if (mDataBlock && mDataBlock->addToHUDRadar && isGhost())
        Con::executef(this, 1, "onRemoveHUDRadarItem");
    destroyBuddy();
#endif

    scriptOnRemove();
    removeFromScene();
    Parent::onRemove();
}

void Item::onDeleteNotify(SimObject* obj)
{
    if (obj == mCollisionObject) {
        mCollisionObject = 0;
        mCollisionTimeout = 0;
    }
}

// Lighting: -----------------------------------------------------------------

void Item::registerLights(LightManager* lightManager, bool lightingScene)
{
    if (lightingScene)
        return;

    if (mDataBlock->lightOnlyStatic && !mStatic)
        return;

    F32 intensity;
    switch (mDataBlock->lightType)
    {
    case ConstantLight:
        intensity = mFadeVal;
        break;

    case PulsingLight:
    {
        F32 delta = Sim::getCurrentTime() - mDropTime;
        intensity = 0.5f + 0.5f * mSin(M_PI * delta / F32(mDataBlock->lightTime));
        intensity = 0.15f + intensity * 0.85f;
        intensity *= mFadeVal;  // fade out light on flags
        break;
    }

    default:
        return;
    }

    mLight.mColor = mDataBlock->lightColor * intensity;
    mLight.mColor.clamp();
    mLight.mType = LightInfo::Point;
    mLight.mRadius = mDataBlock->lightRadius;

    mLight.mPos = getBoxCenter();

    // this is not a flash effect like projectiles,
    // explosions, and weapon fire, so we need a
    // better quality type, but still keep it fast...
    mLight.sgSupportedFeatures = LightInfo::sgNoSpecCube;

    lightManager->sgRegisterGlobalLight(&mLight, this, false);
}


//----------------------------------------------------------------------------

Point3F Item::getVelocity() const
{
    return mVelocity;
}

void Item::setVelocity(const VectorF& vel)
{
    mVelocity = vel;
    setMaskBits(PositionMask);
    mAtRest = false;
    mAtRestCounter = 0;
}

void Item::applyImpulse(const Point3F&, const VectorF& vec)
{
    // Items ignore angular velocity
    VectorF vel;
    vel.x = vec.x / mDataBlock->mass;
    vel.y = vec.y / mDataBlock->mass;
    vel.z = vec.z / mDataBlock->mass;
    setVelocity(vel);
}

void Item::setCollisionTimeout(ShapeBase* obj)
{
    if (mCollisionObject)
        clearNotify(mCollisionObject);
    deleteNotify(obj);
    mCollisionObject = obj;
    mCollisionTimeout = sCollisionTimeout;
    setMaskBits(ThrowSrcMask);
}


//----------------------------------------------------------------------------

void Item::processTick(const Move* move)
{
    Parent::processTick(move);

    //
    if (mCollisionObject && !--mCollisionTimeout)
        mCollisionObject = 0;

    // Warp to catch up to server
    if (delta.warpTicks > 0) {
        delta.warpTicks--;

        // Set new pos.
        MatrixF mat = mObjToWorld;
        mat.getColumn(3, &delta.pos);
        delta.pos += delta.warpOffset;
        mat.setColumn(3, delta.pos);
        Parent::setTransform(mat);

        // Backstepping
        delta.posVec.x = -delta.warpOffset.x;
        delta.posVec.y = -delta.warpOffset.y;
        delta.posVec.z = -delta.warpOffset.z;
    }
    else {
        if (isServerObject() && mAtRest && (mStatic == false && mDataBlock->sticky == false))
        {
            if (++mAtRestCounter > csmAtRestTimer)
            {
                mAtRest = false;
                mAtRestCounter = 0;
            }
        }

        if (!mStatic && !mAtRest && isHidden() == false)
        {
            updateVelocity(TickSec);
            updateWorkingCollisionSet(isGhost() || gSPMode ? sClientCollisionMask : sServerCollisionMask, TickSec);
            updatePos(isGhost() || gSPMode ? sClientCollisionMask : sServerCollisionMask, TickSec);
        }
        else {
            // Need to clear out last updatePos or warp interpolation
            delta.posVec.set(0, 0, 0);
            delta.pos = getPosition();
        }
    }

#ifdef MB_ULTRA
    if (isGhost())
    {
        if (mHiddenTimer)
        {
            mHiddenTimer--;
            if (!mHiddenTimer)
                setHidden(false);
        }
    }
#endif
}

void Item::interpolateTick(F32 dt)
{
    Parent::interpolateTick(dt);

    // Client side interpolation
    Point3F pos = delta.pos + delta.posVec * dt;
    MatrixF mat = mRenderObjToWorld;
    mat.setColumn(3, pos);
    setRenderTransform(mat);
    delta.dt = dt;
}


//----------------------------------------------------------------------------

void Item::setTransform(const MatrixF& mat)
{
#ifdef MARBLE_BLAST
    Parent::setTransform(mat);
#else
    Point3F pos;
    mat.getColumn(3, &pos);
    MatrixF tmat;
    if (!mRotate) {
        // Forces all rotation to be around the z axis
        VectorF vec;
        mat.getColumn(1, &vec);
        tmat.set(EulerF(0, 0, -mAtan(-vec.x, vec.y)));
    }
    else
        tmat.identity();
    tmat.setColumn(3, pos);
    Parent::setTransform(tmat);
#endif

    if (!mStatic)
    {
        mAtRest = false;
        mAtRestCounter = 0;
    }
    setMaskBits(RotationMask | PositionMask | NoWarpMask);
}


//----------------------------------------------------------------------------
void Item::updateWorkingCollisionSet(const U32 mask, const F32 dt)
{
    // It is assumed that we will never accelerate more than 10 m/s for gravity...
    //
    Point3F scaledVelocity = mVelocity * dt;
    F32 len = scaledVelocity.len();
    F32 newLen = len + (10 * dt);

    // Check to see if it is actually necessary to construct the new working list,
    //  or if we can use the cached version from the last query.  We use the x
    //  component of the min member of the mWorkingQueryBox, which is lame, but
    //  it works ok.
    bool updateSet = false;

    Box3F convexBox = mConvex.getBoundingBox(getTransform(), getScale());
    F32 l = (newLen * 1.1) + 0.1;  // from Convex::updateWorkingList
    convexBox.min -= Point3F(l, l, l);
    convexBox.max += Point3F(l, l, l);

    // Check containment
    {
        if (mWorkingQueryBox.min.x != -1e9)
        {
            if (mWorkingQueryBox.isContained(convexBox) == false)
            {
                // Needed region is outside the cached region.  Update it.
                updateSet = true;
            }
            else
            {
                // We can leave it alone, we're still inside the cached region
            }
        }
        else
        {
            // Must update
            updateSet = true;
        }
    }

    // Actually perform the query, if necessary
    if (updateSet == true)
    {
        mWorkingQueryBox = convexBox;
        mWorkingQueryBox.min -= Point3F(2 * l, 2 * l, 2 * l);
        mWorkingQueryBox.max += Point3F(2 * l, 2 * l, 2 * l);

        disableCollision();
        if (mCollisionObject)
            mCollisionObject->disableCollision();

        mConvex.updateWorkingList(mWorkingQueryBox, mask);

        if (mCollisionObject)
            mCollisionObject->enableCollision();
        enableCollision();
    }
}

void Item::updateVelocity(const F32 dt)
{
    // Acceleration due to gravity
    mVelocity.z += (mGravity * mDataBlock->gravityMod) * dt;
    F32 len;
    if (mDataBlock->maxVelocity > 0 && (len = mVelocity.len()) > (mDataBlock->maxVelocity * 1.05)) {
        Point3F excess = mVelocity * (1.0 - (mDataBlock->maxVelocity / len));
        excess *= 0.1;
        mVelocity -= excess;
    }

    // Container buoyancy & drag
    mVelocity.z -= mBuoyancy * (mGravity * mDataBlock->gravityMod * mGravityMod) * dt;
    mVelocity -= mVelocity * mDrag * dt;
}


void Item::updatePos(const U32 /*mask*/, const F32 dt)
{
    // Try and move
    Point3F pos;
    mObjToWorld.getColumn(3, &pos);
    delta.posVec = pos;

    bool contact = false;
    bool nonStatic = false;
    bool stickyNotify = false;
    CollisionList collisionList;
    F32 time = dt;

    static Polyhedron sBoxPolyhedron;
    static ExtrudedPolyList sExtrudedPolyList;
    static EarlyOutPolyList sEarlyOutPolyList;
    MatrixF collisionMatrix(true);
    Point3F end = pos + mVelocity * time;
    U32 mask = isServerObject() ? sServerCollisionMask : sClientCollisionMask;

    // Part of our speed problem here is that we don't track contact surfaces, like we do
    //  with the player.  In order to handle the most common and performance impacting
    //  instance of this problem, we'll use a ray cast to detect any contact surfaces below
    //  us.  This won't be perfect, but it only needs to catch a few of these to make a
    //  big difference.  We'll cast from the top center of the bounding box at the tick's
    //  beginning to the bottom center of the box at the end.
    Point3F startCast((mObjBox.min.x + mObjBox.max.x) * 0.5,
        (mObjBox.min.y + mObjBox.max.y) * 0.5,
        mObjBox.max.z);
    Point3F endCast((mObjBox.min.x + mObjBox.max.x) * 0.5,
        (mObjBox.min.y + mObjBox.max.y) * 0.5,
        mObjBox.min.z);
    collisionMatrix.setColumn(3, pos);
    collisionMatrix.mulP(startCast);
    collisionMatrix.setColumn(3, end);
    collisionMatrix.mulP(endCast);
    RayInfo rinfo;
    bool doToughCollision = true;
    if (mCollisionObject)
        mCollisionObject->disableCollision();
    if (getContainer()->castRay(startCast, endCast, mask, &rinfo))
    {
        F32 bd = -mDot(mVelocity, rinfo.normal);

        if (bd >= 0.0)
        {
            // Contact!
            if (mDataBlock->sticky && rinfo.object->getType() & (STATIC_COLLISION_MASK)) {
                mVelocity.set(0, 0, 0);
                mAtRest = true;
                mAtRestCounter = 0;
                stickyNotify = true;
                mStickyCollisionPos = rinfo.point;
                mStickyCollisionNormal = rinfo.normal;
                doToughCollision = false;;
            }
            else {
                // Subtract out velocity into surface and friction
                VectorF fv = mVelocity + rinfo.normal * bd;
                F32 fvl = fv.len();
                if (fvl) {
                    F32 ff = bd * mDataBlock->friction;
                    if (ff < fvl) {
                        fv *= ff / fvl;
                        fvl = ff;
                    }
                }
                bd *= 1 + mDataBlock->elasticity;
                VectorF dv = rinfo.normal * (bd + 0.002);
                mVelocity += dv;
                mVelocity -= fv;

                // Keep track of what we hit
                contact = true;
                U32 typeMask = rinfo.object->getTypeMask();
                if (!(typeMask & StaticObjectType))
                    nonStatic = true;
                if (isServerObject() && (typeMask & ShapeBaseObjectType)) {
                    ShapeBase* col = static_cast<ShapeBase*>(rinfo.object);
                    queueCollision(col, mVelocity - col->getVelocity());
                }
            }
        }
    }
    if (mCollisionObject)
        mCollisionObject->enableCollision();

    if (doToughCollision)
    {
        U32 count;
        for (count = 0; count < 3; count++)
        {
            // Build list from convex states here...
            end = pos + mVelocity * time;


            collisionMatrix.setColumn(3, end);
            Box3F wBox = getObjBox();
            collisionMatrix.mul(wBox);
            Box3F testBox = wBox;
            Point3F oldMin = testBox.min;
            Point3F oldMax = testBox.max;
            testBox.min.setMin(oldMin + (mVelocity * time));
            testBox.max.setMin(oldMax + (mVelocity * time));

            sEarlyOutPolyList.clear();
            sEarlyOutPolyList.mNormal.set(0, 0, 0);
            sEarlyOutPolyList.mPlaneList.setSize(6);
            sEarlyOutPolyList.mPlaneList[0].set(wBox.min, VectorF(-1, 0, 0));
            sEarlyOutPolyList.mPlaneList[1].set(wBox.max, VectorF(0, 1, 0));
            sEarlyOutPolyList.mPlaneList[2].set(wBox.max, VectorF(1, 0, 0));
            sEarlyOutPolyList.mPlaneList[3].set(wBox.min, VectorF(0, -1, 0));
            sEarlyOutPolyList.mPlaneList[4].set(wBox.min, VectorF(0, 0, -1));
            sEarlyOutPolyList.mPlaneList[5].set(wBox.max, VectorF(0, 0, 1));

            CollisionWorkingList& eorList = mConvex.getWorkingList();
            CollisionWorkingList* eopList = eorList.wLink.mNext;
            while (eopList != &eorList) {
                if ((eopList->mConvex->getObject()->getType() & mask) != 0)
                {
                    Box3F convexBox = eopList->mConvex->getBoundingBox();
                    if (testBox.isOverlapped(convexBox))
                    {
                        eopList->mConvex->getPolyList(&sEarlyOutPolyList);
                        if (sEarlyOutPolyList.isEmpty() == false)
                            break;
                    }
                }
                eopList = eopList->wLink.mNext;
            }
            if (sEarlyOutPolyList.isEmpty())
            {
                pos = end;
                break;
            }

            collisionMatrix.setColumn(3, pos);
            sBoxPolyhedron.buildBox(collisionMatrix, mObjBox);

            // Build extruded polyList...
            VectorF vector = end - pos;
            sExtrudedPolyList.extrude(sBoxPolyhedron, vector);
            sExtrudedPolyList.setVelocity(mVelocity);
            sExtrudedPolyList.setCollisionList(&collisionList);

            CollisionWorkingList& rList = mConvex.getWorkingList();
            CollisionWorkingList* pList = rList.wLink.mNext;
            while (pList != &rList) {
                if ((pList->mConvex->getObject()->getType() & mask) != 0)
                {
                    Box3F convexBox = pList->mConvex->getBoundingBox();
                    if (testBox.isOverlapped(convexBox))
                    {
                        pList->mConvex->getPolyList(&sExtrudedPolyList);
                    }
                }
                pList = pList->wLink.mNext;
            }

            if (collisionList.t < 1.0)
            {
                // Set to collision point
                F32 dt = time * collisionList.t;
                pos += mVelocity * dt;
                time -= dt;

                // Pick the most resistant surface
                F32 bd = 0;
                Collision* collision = 0;
                for (int c = 0; c < collisionList.count; c++) {
                    Collision& cp = collisionList.collision[c];
                    F32 dot = -mDot(mVelocity, cp.normal);
                    if (dot > bd) {
                        bd = dot;
                        collision = &cp;
                    }
                }

                if (collision && mDataBlock->sticky && collision->object->getType() & (STATIC_COLLISION_MASK)) {
                    mVelocity.set(0, 0, 0);
                    mAtRest = true;
                    mAtRestCounter = 0;
                    stickyNotify = true;
                    mStickyCollisionPos = collision->point;
                    mStickyCollisionNormal = collision->normal;
                    break;
                }
                else {
                    // Subtract out velocity into surface and friction
                    if (collision) {
                        VectorF fv = mVelocity + collision->normal * bd;
                        F32 fvl = fv.len();
                        if (fvl) {
                            F32 ff = bd * mDataBlock->friction;
                            if (ff < fvl) {
                                fv *= ff / fvl;
                                fvl = ff;
                            }
                        }
                        bd *= 1 + mDataBlock->elasticity;
                        VectorF dv = collision->normal * (bd + 0.002);
                        mVelocity += dv;
                        mVelocity -= fv;

                        // Keep track of what we hit
                        contact = true;
                        U32 typeMask = collision->object->getTypeMask();
                        if (!(typeMask & StaticObjectType))
                            nonStatic = true;
                        if (isServerObject() && (typeMask & ShapeBaseObjectType)) {
                            ShapeBase* col = static_cast<ShapeBase*>(collision->object);
                            queueCollision(col, mVelocity - col->getVelocity());
                        }
                    }
                }
            }
            else
            {
                pos = end;
                break;
            }
        }
        if (count == 3)
        {
            // Couldn't move...
            mVelocity.set(0, 0, 0);
        }
    }

    // If on the client, calculate delta for backstepping
    if (isGhost()) {
        delta.pos = pos;
        delta.posVec -= pos;
        delta.dt = 1;
    }

    // Update transform
    MatrixF mat = mObjToWorld;
    mat.setColumn(3, pos);
    Parent::setTransform(mat);
    enableCollision();
    if (mCollisionObject)
        mCollisionObject->enableCollision();
    updateContainer();

    //
    if (contact) {
        // Check for rest condition
        if (!nonStatic && mVelocity.len() < sAtRestVelocity) {
            mVelocity.x = mVelocity.y = mVelocity.z = 0;
            mAtRest = true;
            mAtRestCounter = 0;
        }

        // Only update the client if we hit a non-static shape or
        // if this is our final rest pos.
        if (nonStatic || mAtRest)
            setMaskBits(PositionMask);
    }

    // Collision callbacks. These need to be processed whether we hit
    // anything or not.
    if (!isGhost())
    {
        SimObjectPtr<Item> safePtr(this);
        if (stickyNotify)
        {
            notifyCollision();
            if (bool(safePtr))
                Con::executef(mDataBlock, 2, "onStickyCollision", scriptThis());
        }
        else
            notifyCollision();

        // water
        if (bool(safePtr))
        {
            if (!mInLiquid && mWaterCoverage != 0.0f)
            {
                Con::executef(mDataBlock, 4, "onEnterLiquid", scriptThis(), Con::getFloatArg(mWaterCoverage), Con::getIntArg(mLiquidType));
                mInLiquid = true;
            }
            else if (mInLiquid && mWaterCoverage == 0.0f)
            {
                Con::executef(mDataBlock, 3, "onLeaveLiquid", scriptThis(), Con::getIntArg(mLiquidType));
                mInLiquid = false;
            }
        }
    }
}


//----------------------------------------------------------------------------

static MatrixF IMat(1);

bool Item::buildPolyList(AbstractPolyList* polyList, const Box3F&, const SphereF&)
{
    // Collision with the item is always against the item's object
    // space bounding box axis aligned in world space.
    Point3F pos;
    mObjToWorld.getColumn(3, &pos);
    IMat.setColumn(3, pos);
    polyList->setTransform(&IMat, mObjScale);
    polyList->setObject(this);
    polyList->addBox(mObjBox);
    return true;
}


//----------------------------------------------------------------------------

U32 Item::packUpdate(NetConnection* connection, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(connection, mask, stream);

    if (stream->writeFlag(mask & InitialUpdateMask)) {
        stream->writeFlag(mRotate);
        stream->writeFlag(mRotate2);
        stream->writeFlag(mStatic);
        stream->writeFlag(mCollideable);
        if (stream->writeFlag(getScale() != Point3F(1, 1, 1)))
            mathWrite(*stream, getScale());

#ifdef MARBLE_BLAST
        stream->writeFlag(mPermanent);
#endif
#ifdef MB_ULTRA
        stream->writeFlag(mBuddyOn);
#endif
    }
    if (mask & ThrowSrcMask && mCollisionObject) {
        S32 gIndex = connection->getGhostIndex(mCollisionObject);
        if (stream->writeFlag(gIndex != -1))
            stream->writeInt(gIndex, 10);
    }
    else
        stream->writeFlag(false);

#ifndef MARBLE_BLAST
    if (stream->writeFlag(mask & RotationMask && !mRotate)) {
        // Assumes rotation is about the Z axis
        AngAxisF aa(mObjToWorld);
        stream->writeFlag(aa.axis.z < 0);
        stream->write(aa.angle);
    }
#endif
    if (stream->writeFlag(mask & PositionMask)) {

#ifdef MARBLE_BLAST
        stream->writeAffineTransform(mObjToWorld);
#endif
        Point3F pos;
        mObjToWorld.getColumn(3, &pos);
        mathWrite(*stream, pos);
        if (!stream->writeFlag(mAtRest)) {
            mathWrite(*stream, mVelocity);
        }
        stream->writeFlag(!(mask & NoWarpMask));
    }

#ifdef ITEM_FIX_SET_HIDDEN
    if (stream->writeFlag(mask & HiddenMask))
        stream->writeFlag(mHidden);
#endif
    return retMask;
}

void Item::unpackUpdate(NetConnection* connection, BitStream* stream)
{
    Parent::unpackUpdate(connection, stream);
    if (stream->readFlag()) {
        mRotate = stream->readFlag();
        mRotate2 = stream->readFlag();
        mStatic = stream->readFlag();
        mCollideable = stream->readFlag();
        if (stream->readFlag())
            mathRead(*stream, &mObjScale);
        else
            mObjScale.set(1, 1, 1);

#ifdef MARBLE_BLAST
        mPermanent = stream->readFlag();
#endif
#ifdef MB_ULTRA
        mBuddyOn = stream->readFlag();
#endif
    }
    if (stream->readFlag()) {
        S32 gIndex = stream->readInt(10);
        setCollisionTimeout(static_cast<ShapeBase*>(connection->resolveGhost(gIndex)));
    }
    MatrixF mat = mObjToWorld;
#ifndef MARBLE_BLAST
    if (stream->readFlag()) {
        // Assumes rotation is about the Z axis
        AngAxisF aa;
        aa.axis.set(0, 0, stream->readFlag() ? -1 : 1);
        stream->read(&aa.angle);
        aa.setMatrix(&mat);
        Point3F pos;
        mObjToWorld.getColumn(3, &pos);
        mat.setColumn(3, pos);
    }
#endif
    if (stream->readFlag()) {
#ifdef MARBLE_BLAST
        stream->readAffineTransform(&mat);
#endif
        Point3F pos;
        mathRead(*stream, &pos);
        F32 speed = mVelocity.len();
        if ((mAtRest = stream->readFlag()) == true)
            mVelocity.set(0, 0, 0);
        else
            mathRead(*stream, &mVelocity);

        if (stream->readFlag() && isProperlyAdded()) {
            // Determin number of ticks to warp based on the average
            // of the client and server velocities.
            delta.warpOffset = pos - delta.pos;
            F32 as = (speed + mVelocity.len()) * 0.5 * TickSec;
            F32 dt = (as > 0.00001f) ? delta.warpOffset.len() / as : sMaxWarpTicks;
            delta.warpTicks = (S32)((dt > sMinWarpTicks) ? getMax(mFloor(dt + 0.5), 1.0f) : 0.0f);

            if (delta.warpTicks) {
                // Setup the warp to start on the next tick, only the
                // object's position is warped.
                if (delta.warpTicks > sMaxWarpTicks)
                    delta.warpTicks = sMaxWarpTicks;
                delta.warpOffset /= delta.warpTicks;
            }
            else {
                // Going to skip the warp, server and client are real close.
                // Adjust the frame interpolation to move smoothly to the
                // new position within the current tick.
                Point3F cp = delta.pos + delta.posVec * delta.dt;
                VectorF vec = delta.pos - cp;
                F32 vl = vec.len();
                if (vl) {
                    F32 s = delta.posVec.len() / vl;
                    delta.posVec = (cp - pos) * s;
                }
                delta.pos = pos;
                mat.setColumn(3, pos);
            }
        }
        else {
            // Set the item to the server position
            delta.warpTicks = 0;
            delta.posVec.set(0, 0, 0);
            delta.pos = pos;
            delta.dt = 0;
            mat.setColumn(3, pos);
        }
    }

#ifdef ITEM_FIX_SET_HIDDEN
    if (stream->readFlag())
        mHidden = stream->readFlag();
#endif

    Parent::setTransform(mat);

#ifdef MB_ULTRA
    if (!mBuddy.isNull())
        mBuddy->setTransform(mat);
#endif
}


void Item::writePacketData(GameConnection* conn, BitStream* stream)
{
    Parent::writePacketData(conn, stream);
    stream->writeFlag(mHidden);
    stream->writeRangedU32(mHiddenTimer, 0, 62);
}

void Item::readPacketData(GameConnection* conn, BitStream* stream)
{
    Parent::readPacketData(conn, stream);
    setHidden(stream->readFlag());
    mHiddenTimer = stream->readRangedU32(0, 62);
}

#ifdef MARBLE_BLAST
void Item::setHidden(bool hidden)
{
    if (hidden && mHidden)
        mHiddenTimer = 0;
#ifdef MB_ULTRA
    if (!mBuddy.isNull())
        mBuddy->setHidden(hidden);
#endif

    Parent::setHidden(hidden);
}
#endif

#ifdef MB_ULTRA
void Item::setClientHidden(U32 timer)
{
    setHidden(true);
    if (timer >> 5 >= 2000)
        mHiddenTimer = 2000;
    else
        mHiddenTimer = timer >> 5;
}

void Item::createBuddy()
{
    destroyBuddy();
    if (!isGhost() || !mBuddyOn)
        return;

    if (!mDataBlock->buddyShapeName)
        return;

    TSStatic* buddy = new TSStatic;
    buddy->setTransform(getTransform());

    buddy->setShapeName(mDataBlock->buddyShapeName);

    buddy->mNetFlags.set(IsGhost);
    buddy->mNetFlags.clear(Ghostable);
    buddy->registerObject();

    if (mDataBlock->buddySequence)
        buddy->setSequence(mDataBlock->buddySequence);

    mBuddy = buddy;
}

void Item::destroyBuddy()
{
    if (!mBuddy.isNull())
    {
        SimObject* obj = mBuddy;
        mBuddy = NULL;
        obj->deleteObject();
    }
}

void Item::setBuddy(bool on)
{
    mBuddyOn = on;
}

ConsoleMethod(Item, setClientHidden, void, 3, 3, "(int timer)")
{
    object->setClientHidden(dAtoi(argv[2]));
}

ConsoleMethod(Item, setBuddy, void, 3, 3, "(bool buddyOn)")
{
    object->setBuddy(dAtob(argv[2]));
}

#endif

ConsoleMethod(Item, isStatic, bool, 2, 2, "()"
    "Is the object static (ie, non-movable)?")
{
    return object->isStatic();
}

ConsoleMethod(Item, isRotating, bool, 2, 2, "()"
    "Is the object still rotating?")
{
    return object->isRotating();
}

ConsoleMethod(Item, setCollisionTimeout, bool, 3, 3, "(ShapeBase obj)"
    "Temporarily disable collisions against obj.")
{
    ShapeBase* source;
    if (Sim::findObject(dAtoi(argv[2]), source)) {
        object->setCollisionTimeout(source);
        return true;
    }
    return false;
}

ConsoleMethod(Item, getLastStickyPos, const char*, 2, 2, "()"
    "Get the position on the surface on which the object is stuck.")
{
    char* ret = Con::getReturnBuffer(256);
    if (object->isServerObject())
        dSprintf(ret, 255, "%g %g %g",
            object->mStickyCollisionPos.x,
            object->mStickyCollisionPos.y,
            object->mStickyCollisionPos.z);
    else
        dStrcpy(ret, "0 0 0");

    return ret;
}

ConsoleMethod(Item, getLastStickyNormal, const char*, 2, 2, "()"
    "Get the normal of the surface on which the object is stuck.")
{
    char* ret = Con::getReturnBuffer(256);
    if (object->isServerObject())
        dSprintf(ret, 255, "%g %g %g",
            object->mStickyCollisionNormal.x,
            object->mStickyCollisionNormal.y,
            object->mStickyCollisionNormal.z);
    else
        dStrcpy(ret, "0 0 0");

    return ret;
}

//----------------------------------------------------------------------------

void Item::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Misc");
    addField("collideable", TypeBool, Offset(mCollideable, Item));
    addField("static", TypeBool, Offset(mStatic, Item));
    addField("rotate", TypeBool, Offset(mRotate, Item));
    addField("rotate2", TypeBool, Offset(mRotate2, Item));

#ifdef MARBLE_BLAST
    addField("permanent", TypeBool, Offset(mPermanent, Item));
#endif
    endGroup("Misc");
}

void Item::consoleInit()
{
    Con::addVariable("Item::minWarpTicks", TypeF32, &sMinWarpTicks);
    Con::addVariable("Item::maxWarpTicks", TypeS32, &sMaxWarpTicks);
}

//----------------------------------------------------------------------------

bool Item::prepRenderImage(SceneState* state,
    const U32   stateKey,
    const U32   startZone,
    const bool  modifyBaseState)
{
    // Items do NOT render if destroyed
    if (getDamageState() == Destroyed)
        return false;

    return Parent::prepRenderImage(state, stateKey, startZone, modifyBaseState);
}


void Item::renderImage(SceneState* state)
{
    Parent::renderImage(state);
}


void Item::buildConvex(const Box3F& box, Convex* convex)
{
    if (mShapeInstance == NULL)
        return;

    // These should really come out of a pool
    mConvexList->collectGarbage();

    if (box.isOverlapped(getWorldBox()) == false)
        return;

    // Just return a box convex for the entire shape...
    Convex* cc = 0;
    CollisionWorkingList& wl = convex->getWorkingList();
    for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
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

void Item::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);

    if (mRotate || mRotate2)
    {
        F32 r = (dt / sRotationSpeed) * M_2PI;
        Point3F pos = mRenderObjToWorld.getPosition();
        MatrixF rotMatrix;
        if (mRotate)
        {
            rotMatrix.set(EulerF(0.0, 0.0, r));
        }
        else
        {
            rotMatrix.set(EulerF(r * 0.5, 0.0, r));
        }
        MatrixF mat = mRenderObjToWorld;
        mat.setPosition(pos);
        mat.mul(rotMatrix);
        setRenderTransform(mat);
    }

}
