//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "console/consoleTypes.h"
#include "console/typeValidators.h"
#include "core/bitStream.h"
#include "game/fx/explosion.h"
#include "game/shapeBase.h"
#include "game/gameProcess.h"
#include "ts/tsShapeInstance.h"
#include "game/projectile.h"
#include "sfx/sfxSystem.h"
#include "math/mathUtils.h"
#include "math/mathIO.h"
#include "sim/netConnection.h"
#include "game/fx/particleEmitter.h"

IMPLEMENT_CO_DATABLOCK_V1(ProjectileData);
IMPLEMENT_CO_NETOBJECT_V1(Projectile);

const U32 Projectile::csmStaticCollisionMask = AtlasObjectType |
TerrainObjectType |
InteriorObjectType |
StaticObjectType;

const U32 Projectile::csmDynamicCollisionMask = PlayerObjectType |
VehicleObjectType |
DamagableItemObjectType;

const U32 Projectile::csmDamageableMask = Projectile::csmDynamicCollisionMask;

U32 Projectile::smProjectileWarpTicks = 5;


//--------------------------------------------------------------------------
//
ProjectileData::ProjectileData()
{
    projectileShapeName = NULL;

    sound = NULL;
    soundId = 0;

    explosion = NULL;
    explosionId = 0;

    hasLight = false;
    lightRadius = 1;
    lightColor.set(1, 1, 1);

    faceViewer = false;
    scale.set(1.0, 1.0, 1.0);

    isBallistic = false;

    velInheritFactor = 1.0;
    muzzleVelocity = 50;

    armingDelay = 0;
    fadeDelay = 20000 / 32;
    lifetime = 20000 / 32;

    activateSeq = -1;
    maintainSeq = -1;

    gravityMod = 1.0;
    bounceElasticity = 0.999;
    bounceFriction = 0.3;

    particleEmitter = NULL;
    particleEmitterId = 0;
}

//--------------------------------------------------------------------------
IMPLEMENT_CONSOLETYPE(ProjectileData)
IMPLEMENT_GETDATATYPE(ProjectileData)
IMPLEMENT_SETDATATYPE(ProjectileData)

void ProjectileData::initPersistFields()
{
    Parent::initPersistFields();

    addNamedField(particleEmitter, TypeParticleEmitterDataPtr, ProjectileData);
    addNamedField(projectileShapeName, TypeFilename, ProjectileData);
    addNamedField(scale, TypePoint3F, ProjectileData);

    addNamedField(sound, TypeSFXProfilePtr, ProjectileData);

    addNamedField(explosion, TypeExplosionDataPtr, ProjectileData);

    addNamedField(hasLight, TypeBool, ProjectileData);
    addNamedFieldV(lightRadius, TypeF32, ProjectileData, new FRangeValidator(1, 20));
    addNamedField(lightColor, TypeColorF, ProjectileData);

    addNamedField(isBallistic, TypeBool, ProjectileData);
    addNamedFieldV(velInheritFactor, TypeF32, ProjectileData, new FRangeValidator(0, 1));
    addNamedFieldV(muzzleVelocity, TypeF32, ProjectileData, new FRangeValidator(0, 10000));

    addNamedFieldV(lifetime, TypeS32, ProjectileData, new IRangeValidatorScaled(TickMs, 0, Projectile::MaxLivingTicks));
    addNamedFieldV(armingDelay, TypeS32, ProjectileData, new IRangeValidatorScaled(TickMs, 0, Projectile::MaxLivingTicks));
    addNamedFieldV(fadeDelay, TypeS32, ProjectileData, new IRangeValidatorScaled(TickMs, 0, Projectile::MaxLivingTicks));

    addNamedFieldV(bounceElasticity, TypeF32, ProjectileData, new FRangeValidator(0, 0.999));
    addNamedFieldV(bounceFriction, TypeF32, ProjectileData, new FRangeValidator(0, 1));
    addNamedFieldV(gravityMod, TypeF32, ProjectileData, new FRangeValidator(0, 1));
}


//--------------------------------------------------------------------------
bool ProjectileData::onAdd()
{
    if (!Parent::onAdd())
        return false;

    if (!particleEmitter && particleEmitterId != 0)
        if (Sim::findObject(particleEmitterId, particleEmitter) == false)
            Con::errorf(ConsoleLogEntry::General, "ProjectileData::onAdd: Invalid packet, bad datablockId(particleEmitter): %d", particleEmitterId);

    if (!explosion && explosionId != 0)
        if (Sim::findObject(explosionId, explosion) == false)
            Con::errorf(ConsoleLogEntry::General, "ProjectileData::onAdd: Invalid packet, bad datablockId(explosion): %d", explosionId);

    if (!sound && soundId != 0)
        if (Sim::findObject(soundId, sound) == false)
            Con::errorf(ConsoleLogEntry::General, "ProjectileData::onAdd: Invalid packet, bad datablockid(sound): %d", soundId);

    lightColor.clamp();

    return true;
}


bool ProjectileData::preload(bool server, char errorBuffer[256])
{
    if (Parent::preload(server, errorBuffer) == false)
        return false;

    if (projectileShapeName && projectileShapeName[0] != '\0')
    {
        projectileShape = ResourceManager->load(projectileShapeName);
        if (bool(projectileShape) == false)
        {
            dSprintf(errorBuffer, sizeof(errorBuffer), "ProjectileData::load: Couldn't load shape \"%s\"", projectileShapeName);
            return false;
        }
        activateSeq = projectileShape->findSequence("activate");
        maintainSeq = projectileShape->findSequence("maintain");
    }

    if (bool(projectileShape)) // create an instance to preload shape data
    {
        TSShapeInstance* pDummy = new TSShapeInstance(projectileShape, !server);
        delete pDummy;
    }

    return true;
}

//--------------------------------------------------------------------------
void ProjectileData::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->writeString(projectileShapeName);
    stream->writeFlag(faceViewer);
    if (stream->writeFlag(scale.x != 1 || scale.y != 1 || scale.z != 1))
    {
        stream->write(scale.x);
        stream->write(scale.y);
        stream->write(scale.z);
    }

    if (stream->writeFlag(particleEmitter != NULL))
        stream->writeRangedU32(particleEmitter->getId(), DataBlockObjectIdFirst,
            DataBlockObjectIdLast);
    if (stream->writeFlag(explosion != NULL))
        stream->writeRangedU32(explosion->getId(), DataBlockObjectIdFirst,
            DataBlockObjectIdLast);
    if (stream->writeFlag(sound != NULL))
        stream->writeRangedU32(sound->getId(), DataBlockObjectIdFirst,
            DataBlockObjectIdLast);


    if (stream->writeFlag(hasLight))
    {
        stream->writeFloat(lightRadius / 20.0, 8);
        stream->writeFloat(lightColor.red, 7);
        stream->writeFloat(lightColor.green, 7);
        stream->writeFloat(lightColor.blue, 7);
    }
    stream->writeRangedU32(lifetime, 0, Projectile::MaxLivingTicks);
    stream->writeRangedU32(armingDelay, 0, Projectile::MaxLivingTicks);
    stream->writeRangedU32(fadeDelay, 0, Projectile::MaxLivingTicks);

    if (stream->writeFlag(isBallistic))
    {
        stream->write(gravityMod);
        stream->write(bounceElasticity);
        stream->write(bounceFriction);
    }

}

void ProjectileData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    projectileShapeName = stream->readSTString();

    faceViewer = stream->readFlag();
    if (stream->readFlag())
    {
        stream->read(&scale.x);
        stream->read(&scale.y);
        stream->read(&scale.z);
    }
    else
        scale.set(1, 1, 1);

    if (stream->readFlag())
        particleEmitterId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    if (stream->readFlag())
        explosionId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    if (stream->readFlag())
        soundId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);

    hasLight = stream->readFlag();
    if (hasLight)
    {
        lightRadius = stream->readFloat(8) * 20;
        lightColor.red = stream->readFloat(7);
        lightColor.green = stream->readFloat(7);
        lightColor.blue = stream->readFloat(7);
    }
    lifetime = stream->readRangedU32(0, Projectile::MaxLivingTicks);
    armingDelay = stream->readRangedU32(0, Projectile::MaxLivingTicks);
    fadeDelay = stream->readRangedU32(0, Projectile::MaxLivingTicks);
    isBallistic = stream->readFlag();
    if (isBallistic)
    {
        stream->read(&gravityMod);
        stream->read(&bounceElasticity);
        stream->read(&bounceFriction);
    }
}


//--------------------------------------------------------------------------
//--------------------------------------
//
Projectile::Projectile()
{
    // Todo: ScopeAlways?
    mNetFlags.set(Ghostable);
    mTypeMask |= ProjectileObjectType;

    mCurrPosition.set(0, 0, 0);
    mCurrVelocity.set(0, 0, 1);

    mSourceObjectId = -1;
    mSourceObjectSlot = -1;

    mCurrTick = 0;

    mParticleEmitter = NULL;
    mSoundHandle = NULL;

    mProjectileShape = NULL;
    mActivateThread = NULL;
    mMaintainThread = NULL;

    mHidden = false;
    mFadeValue = 1.0;
}

Projectile::~Projectile()
{
    delete mProjectileShape;
    mProjectileShape = NULL;
}

//--------------------------------------------------------------------------
void Projectile::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Physics");
    addField("initialPosition", TypePoint3F, Offset(mCurrPosition, Projectile));
    addField("initialVelocity", TypePoint3F, Offset(mCurrVelocity, Projectile));
    endGroup("Physics");

    addGroup("Source");
    addField("sourceObject", TypeS32, Offset(mSourceObjectId, Projectile));
    addField("sourceSlot", TypeS32, Offset(mSourceObjectSlot, Projectile));
    endGroup("Source");
}

bool Projectile::calculateImpact(float,
    Point3F& pointOfImpact,
    float& impactTime)
{
    Con::warnf(ConsoleLogEntry::General, "Projectile::calculateImpact: Should never be called");

    impactTime = 0;
    pointOfImpact.set(0, 0, 0);
    return false;
}


//--------------------------------------------------------------------------
F32 Projectile::getUpdatePriority(CameraScopeQuery* camInfo, U32 updateMask, S32 updateSkips)
{
    F32 ret = Parent::getUpdatePriority(camInfo, updateMask, updateSkips);
    // if the camera "owns" this object, it should have a slightly higher priority
    if (mSourceObject == camInfo->camera)
        return ret + 0.2;
    return ret;
}

bool Projectile::onAdd()
{
    if (!Parent::onAdd())
        return false;

    if (isServerObject())
    {
        ShapeBase* ptr;
        if (Sim::findObject(mSourceObjectId, ptr))
            mSourceObject = ptr;
        else
        {
            if (mSourceObjectId != -1)
                Con::errorf(ConsoleLogEntry::General, "Projectile::onAdd: mSourceObjectId is invalid");
            mSourceObject = NULL;
        }

        // If we're on the server, we need to inherit some of our parent's velocity
        //
        mCurrTick = 0;
    }
    else
    {
        if (bool(mDataBlock->projectileShape))
        {
            mProjectileShape = new TSShapeInstance(mDataBlock->projectileShape, isClientObject());

            if (mDataBlock->activateSeq != -1)
            {
                mActivateThread = mProjectileShape->addThread();
                mProjectileShape->setTimeScale(mActivateThread, 1);
                mProjectileShape->setSequence(mActivateThread, mDataBlock->activateSeq, 0);
            }
        }
        if (mDataBlock->particleEmitter != NULL)
        {
            ParticleEmitter* pEmitter = new ParticleEmitter;
            pEmitter->onNewDataBlock(mDataBlock->particleEmitter);
            if (pEmitter->registerObject() == false)
            {
                Con::warnf(ConsoleLogEntry::General, "Could not register particle emitter for particle of class: %s", mDataBlock->getName());
                delete pEmitter;
                pEmitter = NULL;
            }
            mParticleEmitter = pEmitter;
        }
        if (mDataBlock->hasLight == true)
            Sim::getLightSet()->addObject(this);
    }
    if (bool(mSourceObject))
        processAfter(mSourceObject);

    // Setup our bounding box
    if (bool(mDataBlock->projectileShape) == true)
        mObjBox = mDataBlock->projectileShape->bounds;
    else
        mObjBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
    resetWorldBox();
    addToScene();

    return true;
}


void Projectile::onRemove()
{
    if (bool(mParticleEmitter)) {
        mParticleEmitter->deleteWhenEmpty();
        mParticleEmitter = NULL;
    }

    SFX_DELETE(mSoundHandle);

    removeFromScene();
    Parent::onRemove();
}


bool Projectile::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<ProjectileData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    return true;
}


//--------------------------------------------------------------------------


void Projectile::registerLights(LightManager* lightManager, bool lightingScene)
{
    if (lightingScene)
        return;

    if (mDataBlock->hasLight && mHidden == false)
    {
        mLight.mType = LightInfo::Point;
        getRenderTransform().getColumn(3, &mLight.mPos);
        mLight.mRadius = mDataBlock->lightRadius;
        mLight.mColor = mDataBlock->lightColor;
        lightManager->sgRegisterGlobalLight(&mLight, this, false);
    }
}


//----------------------------------------------------------------------------

void Projectile::emitParticles(const Point3F& from, const Point3F& to, const Point3F& vel, const U32 ms)
{
    if (mHidden)
        return;

    Point3F axis = -vel;

    if (axis.isZero())
        axis.set(0.0, 0.0, 1.0);
    else
        axis.normalize();

    if (bool(mParticleEmitter))
    {
        mParticleEmitter->emitParticles(from, to,
            axis, vel,
            ms);
    }
}


//----------------------------------------------------------------------------

class ObjectDeleteEvent : public SimEvent
{
public:
    void process(SimObject* object)
    {
        object->deleteObject();
    }
};

void Projectile::explode(const Point3F& p, const Point3F& n, const U32 collideType)
{
    // Make sure we don't explode twice...
    if (mHidden == true)
        return;

    mHidden = true;
    if (isServerObject()) {
        // Do what the server needs to do, damage the surrounding objects, etc.
        mExplosionPosition = p + (n * 0.01);
        mExplosionNormal = n;
        mCollideHitType = collideType;

        char buffer[128];
        dSprintf(buffer, sizeof(buffer), "%g %g %g", mExplosionPosition.x,
            mExplosionPosition.y,
            mExplosionPosition.z);
        Con::executef(mDataBlock, 4, "onExplode", scriptThis(), buffer, Con::getFloatArg(mFadeValue));

        setMaskBits(ExplosionMask);
        Sim::postEvent(this, new ObjectDeleteEvent, Sim::getCurrentTime() + DeleteWaitTime);
    }
    else {
        // Client just plays the explosion at the right place...
        //
        Explosion* pExplosion = NULL;

        F32 waterHeight;
        if (mDataBlock->explosion)
        {
            pExplosion = new Explosion;
            pExplosion->onNewDataBlock(mDataBlock->explosion);
        }

        if (pExplosion)
        {
            MatrixF xform(true);
            xform.setPosition(p);
            pExplosion->setTransform(xform);
            pExplosion->setInitialState(p, n);
            pExplosion->setCollideType(collideType);
            if (pExplosion->registerObject() == false)
            {
                Con::errorf(ConsoleLogEntry::General, "Projectile(%s)::explode: couldn't register explosion",
                    mDataBlock->getName());
                delete pExplosion;
                pExplosion = NULL;
            }
        }

        // Client object
        updateSound();
    }
}

void Projectile::updateSound()
{
    if (!mDataBlock->sound)
        return;

    if (mHidden && mSoundHandle)
        mSoundHandle->stop();

    else if (!mHidden && mSoundHandle)
    {
        if (mSoundHandle->isPlaying())
            mSoundHandle->play();

        mSoundHandle->setVelocity(getVelocity());
        mSoundHandle->setTransform(getRenderTransform());
    }
}

Point3F Projectile::getVelocity() const
{
    return mCurrVelocity;
}

void Projectile::processTick(const Move* move)
{
    Parent::processTick(move);

    mCurrTick++;
    if (mSourceObject && mCurrTick > SourceIdTimeoutTicks)
    {
        mSourceObject = 0;
        mSourceObjectId = 0;
    }

    // See if we can get out of here the easy way ...
    if (isServerObject() && mCurrTick >= mDataBlock->lifetime)
    {
        deleteObject();
        return;
    }
    else if (mHidden == true)
        return;

    // ... otherwise, we have to do some simulation work.
    F32 timeLeft;
    RayInfo rInfo;
    Point3F oldPosition;
    Point3F newPosition;

    oldPosition = mCurrPosition;
    if (mDataBlock->isBallistic)
        mCurrVelocity.z -= 9.81 * mDataBlock->gravityMod * (F32(TickMs) / 1000.0f);

    newPosition = oldPosition + mCurrVelocity * (F32(TickMs) / 1000.0f);

    // disable the source objects collision reponse while we determine
    // if the projectile is capable of moving from the old position
    // to the new position, otherwise we'll hit ourself
    if (bool(mSourceObject))
        mSourceObject->disableCollision();

    // Determine if the projectile is going to hit any object between the previous
    // position and the new position. This code is executed both on the server
    // and on the client (for prediction purposes). It is possible that the server
    // will have registered a collision while the client prediction has not. If this
    // happens the client will be corrected in the next packet update.
    if (getContainer()->castRay(oldPosition, newPosition, csmDynamicCollisionMask | csmStaticCollisionMask, &rInfo) == true)
    {
        // make sure the client knows to bounce
        if (isServerObject() && (rInfo.object->getType() & csmStaticCollisionMask) == 0)
            setMaskBits(BounceMask);

        // Next order of business: do we explode on this hit?
        if (mCurrTick > mDataBlock->armingDelay)
        {
            MatrixF xform(true);
            xform.setColumn(3, rInfo.point);
            setTransform(xform);
            mCurrPosition = rInfo.point;
            mCurrVelocity = Point3F(0, 0, 0);

            // Get the object type before the onCollision call, in case
            // the object is destroyed.
            U32 objectType = rInfo.object->getType();

            // re-enable the collision response on the source object since
            // we need to process the onCollision and explode calls
            if (mSourceObject)
                mSourceObject->enableCollision();

            // Ok, here is how this works:
            // onCollision is called to notify the server scripts that a collision has occured, then
            // a call to explode is made to start the explosion process. The call to explode is made
            // twice, once on the server and once on the client.
            // The server process is responsible for two things:
            //    1) setting the ExplosionMask network bit to guarantee that the client calls explode
            //    2) initiate the explosion process on the server scripts
            // The client process is responsible for only one thing:
            //    1) drawing the appropriate explosion

            // It is possible that during the processTick the server may have decided that a hit
            // has occured while the client prediction has decided that a hit has not occured.
            // In this particular scenario the client will have failed to call onCollision and
            // explode during the processTick. However, the explode function will be called
            // during the next packet update, due to the ExplosionMask network bit being set.
            // onCollision will remain uncalled on the client however, therefore no client
            // specific code should be placed inside the function!
            onCollision(rInfo.point, rInfo.normal, rInfo.object);
            explode(rInfo.point, rInfo.normal, objectType);

            // break out of the collision check, since we've exploded
            // we dont want to mess with the position and velocity
        }
        else
        {
            if (mDataBlock->isBallistic)
            {
                // Otherwise, this represents a bounce.  First, reflect our velocity
                //  around the normal...
                Point3F bounceVel = mCurrVelocity - rInfo.normal * (mDot(mCurrVelocity, rInfo.normal) * 2.0);;
                mCurrVelocity = bounceVel;

                // Add in surface friction...
                Point3F tangent = bounceVel - rInfo.normal * mDot(bounceVel, rInfo.normal);
                mCurrVelocity -= tangent * mDataBlock->bounceFriction;

                // Now, take elasticity into account for modulating the speed of the grenade
                mCurrVelocity *= mDataBlock->bounceElasticity;

                U32 timeLeft = 1.0 * (1.0 - rInfo.t);
                oldPosition = rInfo.point + rInfo.normal * 0.05;
                newPosition = oldPosition + (mCurrVelocity * ((timeLeft / 1000.0) * TickMs));
            }
        }
    }

    // re-enable the collision response on the source object now
    // that we are done processing the ballistic movment
    if (bool(mSourceObject))
        mSourceObject->enableCollision();

    if (isClientObject())
    {
        emitParticles(mCurrPosition, newPosition, mCurrVelocity, TickMs);
        updateSound();
    }

    mCurrDeltaBase = newPosition;
    mCurrBackDelta = mCurrPosition - newPosition;
    mCurrPosition = newPosition;

    MatrixF xform(true);
    xform.setColumn(3, mCurrPosition);
    setTransform(xform);
}


void Projectile::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);

    if (mHidden == true || dt == 0.0)
        return;

    if (mActivateThread &&
        mProjectileShape->getDuration(mActivateThread) > mProjectileShape->getTime(mActivateThread) + dt)
    {
        mProjectileShape->advanceTime(dt, mActivateThread);
    }
    else
    {

        if (mMaintainThread)
        {
            mProjectileShape->advanceTime(dt, mMaintainThread);
        }
        else if (mActivateThread && mDataBlock->maintainSeq != -1)
        {
            mMaintainThread = mProjectileShape->addThread();
            mProjectileShape->setTimeScale(mMaintainThread, 1);
            mProjectileShape->setSequence(mMaintainThread, mDataBlock->maintainSeq, 0);
            mProjectileShape->advanceTime(dt, mMaintainThread);
        }
    }
}

void Projectile::interpolateTick(F32 delta)
{
    Parent::interpolateTick(delta);

    if (mHidden == true)
        return;

    Point3F interpPos = mCurrDeltaBase + mCurrBackDelta * delta;
    Point3F dir = mCurrVelocity;
    if (dir.isZero())
        dir.set(0, 0, 1);
    else
        dir.normalize();

    MatrixF xform(true);
    xform = MathUtils::createOrientFromDir(dir);
    xform.setPosition(interpPos);
    setRenderTransform(xform);

    // fade out the projectile image
    S32 time = (S32)(mCurrTick - delta);
    if (time > mDataBlock->fadeDelay)
    {
        F32 fade = F32(time - mDataBlock->fadeDelay);
        mFadeValue = 1.0 - (fade / F32(mDataBlock->lifetime));
    }
    else
        mFadeValue = 1.0;

    updateSound();
}



//--------------------------------------------------------------------------
void Projectile::onCollision(const Point3F& hitPosition, const Point3F& hitNormal, SceneObject* hitObject)
{
    // No client specific code should be placed or branched from this function
    if (isClientObject())
        return;

    if (hitObject != NULL && isServerObject())
    {
        char* posArg = Con::getArgBuffer(64);
        char* normalArg = Con::getArgBuffer(64);

        dSprintf(posArg, 64, "%g %g %g", hitPosition.x, hitPosition.y, hitPosition.z);
        dSprintf(normalArg, 64, "%g %g %g", hitNormal.x, hitNormal.y, hitNormal.z);

        Con::executef(mDataBlock, 6, "onCollision",
            scriptThis(),
            Con::getIntArg(hitObject->getId()),
            Con::getFloatArg(mFadeValue),
            posArg,
            normalArg);
    }
}

//--------------------------------------------------------------------------
U32 Projectile::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
    U32 retMask = Parent::packUpdate(con, mask, stream);

    // Initial update
    if (stream->writeFlag(mask & GameBase::InitialUpdateMask))
    {
        Point3F pos;
        getTransform().getColumn(3, &pos);
        stream->writeCompressedPoint(pos);
        F32 len = mCurrVelocity.len();
        if (stream->writeFlag(len > 0.02))
        {
            Point3F outVel = mCurrVelocity;
            outVel *= 1 / len;
            stream->writeNormalVector(outVel, 10);
            len *= 32.0; // 5 bits for fraction
            if (len > 8191)
                len = 8191;
            stream->writeInt((S32)len, 13);
        }

        stream->writeRangedU32(mCurrTick, 0, MaxLivingTicks);
        if (bool(mSourceObject))
        {
            // Potentially have to write this to the client, let's make sure it has a
            //  ghost on the other side...
            S32 ghostIndex = con->getGhostIndex(mSourceObject);
            if (stream->writeFlag(ghostIndex != -1))
            {
                stream->writeRangedU32(U32(ghostIndex), 0, NetConnection::MaxGhostCount);
                stream->writeRangedU32(U32(mSourceObjectSlot),
                    0, ShapeBase::MaxMountedImages - 1);
            }
            else // havn't recieved the ghost for the source object yet, try again later
                retMask |= GameBase::InitialUpdateMask;
        }
        else
            stream->writeFlag(false);
    }

    // explosion update
    if (stream->writeFlag((mask & ExplosionMask) && mHidden))
    {
        mathWrite(*stream, mExplosionPosition);
        mathWrite(*stream, mExplosionNormal);
        stream->write(mCollideHitType);
    }

    // bounce update
    if (stream->writeFlag(mask & BounceMask))
    {
        // Bounce against dynamic object
        mathWrite(*stream, mCurrPosition);
        mathWrite(*stream, mCurrVelocity);
    }

    return retMask;
}

void Projectile::unpackUpdate(NetConnection* con, BitStream* stream)
{
    Parent::unpackUpdate(con, stream);

    // initial update
    if (stream->readFlag())
    {
        Point3F pos;
        stream->readCompressedPoint(&pos);
        if (stream->readFlag())
        {
            stream->readNormalVector(&mCurrVelocity, 10);
            mCurrVelocity *= stream->readInt(13) / 32.0f;
        }
        else
            mCurrVelocity.set(0, 0, 0);

        mCurrDeltaBase = pos;
        mCurrBackDelta = mCurrPosition - pos;
        mCurrPosition = pos;
        setPosition(mCurrPosition);

        mCurrTick = stream->readRangedU32(0, MaxLivingTicks);
        if (stream->readFlag())
        {
            mSourceObjectId = stream->readRangedU32(0, NetConnection::MaxGhostCount);
            mSourceObjectSlot = stream->readRangedU32(0, ShapeBase::MaxMountedImages - 1);

            NetObject* pObject = con->resolveGhost(mSourceObjectId);
            if (pObject != NULL)
                mSourceObject = dynamic_cast<ShapeBase*>(pObject);
        }
        else
        {
            mSourceObjectId = -1;
            mSourceObjectSlot = -1;
            mSourceObject = NULL;
        }
    }

    // explosion update
    if (stream->readFlag())
    {
        Point3F explodePoint;
        Point3F explodeNormal;
        mathRead(*stream, &explodePoint);
        mathRead(*stream, &explodeNormal);
        stream->read(&mCollideHitType);

        // start the explosion visuals
        explode(explodePoint, explodeNormal, mCollideHitType);
    }

    // bounce update
    if (stream->readFlag())
    {
        mathRead(*stream, &mCurrPosition);
        mathRead(*stream, &mCurrVelocity);
    }
}

//--------------------------------------------------------------------------
void Projectile::prepModelView(SceneState* state)
{
    /*
       Point3F targetVector;
       if( mDataBlock->faceViewer )
       {
          targetVector = state->getCameraPosition() - getRenderPosition();
          targetVector.normalize();

          MatrixF explOrient = MathUtils::createOrientFromDir( targetVector );
          explOrient.setPosition( getRenderPosition() );
          dglMultMatrix( &explOrient );
       }
       else
       {
          dglMultMatrix( &getRenderTransform() );
       }
    */
}

//--------------------------------------------------------------------------
bool Projectile::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    if (mHidden == true || mFadeValue <= (1.0 / 255.0))
        return false;

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    if (state->isObjectRendered(this))
    {
        prepBatchRender(state);
    }

    return false;
}

void Projectile::prepBatchRender(SceneState* state)
{
    // DAW: New code for TSE
    MatrixF proj = GFX->getProjectionMatrix();

    RectI viewport = GFX->getViewport();
    state->setupObjectProjection(this);

    // hack until new scenegraph in place
    MatrixF world = GFX->getWorldMatrix();
    TSMesh::setCamTrans(world);
    TSMesh::setSceneState(state);

    // DAW: Uncomment below if projectiles support refraction.  Don't forget
    // to uncomment the code in ::prepRenderImage()
    // TSMesh::setRefract( image->sortType == SceneRenderImage::Refraction );


    GFX->pushWorldMatrix();

    MatrixF mat = getRenderTransform();
    mat.scale(mObjScale);
    GFX->setWorldMatrix(mat);

    if (mProjectileShape)
    {
        AssertFatal(mProjectileShape != NULL,
            "Projectile::renderObject: Error, projectile shape should always be present in renderObject");
        mProjectileShape->selectCurrentDetail();
        mProjectileShape->animate();

        mProjectileShape->render();
    }


    GFX->popWorldMatrix();

    GFX->setProjectionMatrix(proj);
    GFX->setViewport(viewport);
}
