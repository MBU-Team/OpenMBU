//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/debris.h"
#include "core/bitStream.h"
#include "math/mathUtils.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "sim/netConnection.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/detailManager.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsPartInstance.h"
#include "game/fx/particleEmitter.h"
#include "game/fx/explosion.h"
#include "game/gameProcess.h"

const U32 csmStaticCollisionMask = AtlasObjectType |
TerrainObjectType |
InteriorObjectType;

const U32 csmDynamicCollisionMask = StaticShapeObjectType;



//**************************************************************************
// Debris Data
//**************************************************************************

IMPLEMENT_CO_DATABLOCK_V1(DebrisData);

//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
DebrisData::DebrisData()
{
    dMemset(emitterList, 0, sizeof(emitterList));
    dMemset(emitterIDList, 0, sizeof(emitterIDList));

    explosion = NULL;
    explosionId = 0;

    velocity = 0.0;
    velocityVariance = 0.0;
    elasticity = 0.3;
    friction = 0.2;
    numBounces = 0;
    bounceVariance = 0;
    minSpinSpeed = maxSpinSpeed = 0.0;
    render2D = false;
    staticOnMaxBounce = false;
    explodeOnMaxBounce = false;
    snapOnMaxBounce = false;
    lifetime = 3.0;
    lifetimeVariance = 0.0;
    minSpinSpeed = 0.0;
    maxSpinSpeed = 0.0;
    textureName = NULL;
    mTypeMask |= DebrisObjectType;
    shapeName = NULL;
    fade = true;
    useRadiusMass = false;
    baseRadius = 1.0;
    gravModifier = 1.0;
    terminalVelocity = 0.0;
    ignoreWater = true;
}


//--------------------------------------------------------------------------
// Initialize - Check data
//--------------------------------------------------------------------------
bool DebrisData::onAdd()
{
    if (!Parent::onAdd())
        return false;

    for (int i = 0; i < DDC_NUM_EMITTERS; i++)
    {
        if (!emitterList[i] && emitterIDList[i] != 0)
        {
            if (Sim::findObject(emitterIDList[i], emitterList[i]) == false)
            {
                Con::errorf(ConsoleLogEntry::General, "DebrisData::onAdd: Invalid packet, bad datablockId(emitter): 0x%x", emitterIDList[i]);
            }
        }
    }

    if (!explosion && explosionId != 0)
    {
        if (!Sim::findObject(SimObjectId(explosionId), explosion))
            Con::errorf(ConsoleLogEntry::General, "DebrisData::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", explosionId);
    }

    /*
       if( textureName )
       {
          texture = TextureHandle( textureName, MeshTexture );
       }
       else
       {
          texture = TextureHandle();  // set default NULL tex
       }
    */

    // validate data

    if (velocityVariance > velocity)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: velocityVariance invalid", getName());
        velocityVariance = velocity;
    }
    if (friction < -10.0 || friction > 10.0)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: friction invalid", getName());
        friction = 0.2;
    }
    if (elasticity < -10.0 || elasticity > 10.0)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: elasticity invalid", getName());
        elasticity = 0.2;
    }
    if (lifetime < 0.0 || lifetime > 1000.0)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: lifetime invalid", getName());
        lifetime = 3.0;
    }
    if (lifetimeVariance < 0.0 || lifetimeVariance > lifetime)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: lifetimeVariance invalid", getName());
        lifetimeVariance = 0.0;
    }
    if (numBounces < 0 || numBounces > 10000)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: numBounces invalid", getName());
        numBounces = 3;
    }
    if (bounceVariance < 0 || bounceVariance > numBounces)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: bounceVariance invalid", getName());
        bounceVariance = 0;
    }
    if (minSpinSpeed < -10000.0 || minSpinSpeed > 10000.0 || minSpinSpeed > maxSpinSpeed)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: minSpinSpeed invalid", getName());
        minSpinSpeed = maxSpinSpeed - 1.0;
    }
    if (maxSpinSpeed < -10000.0 || maxSpinSpeed > 10000.0)
    {
        Con::warnf(ConsoleLogEntry::General, "DebrisData(%s)::onAdd: maxSpinSpeed invalid", getName());
        maxSpinSpeed = 0.0;
    }




    return true;
}

//--------------------------------------------------------------------------
// Preload
//--------------------------------------------------------------------------
bool DebrisData::preload(bool server, char errorBuffer[256])
{
    if (Parent::preload(server, errorBuffer) == false)
        return false;

    if (server) return true;

    if (shapeName && shapeName[0] != '\0' && !bool(shape))
    {
        shape = ResourceManager->load(shapeName);
        if (bool(shape) == false)
        {
            dSprintf(errorBuffer, sizeof(errorBuffer), "DebrisData::load: Couldn't load shape \"%s\"", shapeName);
            return false;
        }
        else
        {
            TSShapeInstance* pDummy = new TSShapeInstance(shape, !server);
            delete pDummy;
        }

    }

    return true;
}

//--------------------------------------------------------------------------
// Initialize console fields (static)
//--------------------------------------------------------------------------
IMPLEMENT_CONSOLETYPE(DebrisData)
IMPLEMENT_SETDATATYPE(DebrisData)
IMPLEMENT_GETDATATYPE(DebrisData)

void DebrisData::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Display");
    addField("texture", TypeString, Offset(textureName, DebrisData));
    addField("shapeFile", TypeFilename, Offset(shapeName, DebrisData));
    addField("render2D", TypeBool, Offset(render2D, DebrisData));
    endGroup("Display");

    addGroup("Datablocks");
    addField("emitters", TypeParticleEmitterDataPtr, Offset(emitterList, DebrisData), DDC_NUM_EMITTERS);
    addField("explosion", TypeExplosionDataPtr, Offset(explosion, DebrisData));
    endGroup("Datablocks");

    addGroup("Physical Properties");
    addField("elasticity", TypeF32, Offset(elasticity, DebrisData));
    addField("friction", TypeF32, Offset(friction, DebrisData));
    addField("numBounces", TypeS32, Offset(numBounces, DebrisData));
    addField("bounceVariance", TypeS32, Offset(bounceVariance, DebrisData));
    addField("minSpinSpeed", TypeF32, Offset(minSpinSpeed, DebrisData));
    addField("maxSpinSpeed", TypeF32, Offset(maxSpinSpeed, DebrisData));
    addField("gravModifier", TypeF32, Offset(gravModifier, DebrisData));
    addField("terminalVelocity", TypeF32, Offset(terminalVelocity, DebrisData));
    addField("velocity", TypeF32, Offset(velocity, DebrisData));
    addField("velocityVariance", TypeF32, Offset(velocityVariance, DebrisData));
    addField("lifetime", TypeF32, Offset(lifetime, DebrisData));
    addField("lifetimeVariance", TypeF32, Offset(lifetimeVariance, DebrisData));
    addField("useRadiusMass", TypeBool, Offset(useRadiusMass, DebrisData));
    addField("baseRadius", TypeF32, Offset(baseRadius, DebrisData));
    endGroup("Physical Properties");

    addGroup("Behavior");
    addField("explodeOnMaxBounce", TypeBool, Offset(explodeOnMaxBounce, DebrisData));
    addField("staticOnMaxBounce", TypeBool, Offset(staticOnMaxBounce, DebrisData));
    addField("snapOnMaxBounce", TypeBool, Offset(snapOnMaxBounce, DebrisData));
    addField("fade", TypeBool, Offset(fade, DebrisData));
    addField("ignoreWater", TypeBool, Offset(ignoreWater, DebrisData));
    endGroup("Behavior");
}

//--------------------------------------------------------------------------
// Pack data
//--------------------------------------------------------------------------
void DebrisData::packData(BitStream* stream)
{
    Parent::packData(stream);

    stream->write(elasticity);
    stream->write(friction);
    stream->write(numBounces);
    stream->write(bounceVariance);
    stream->write(minSpinSpeed);
    stream->write(maxSpinSpeed);
    stream->write(render2D);
    stream->write(explodeOnMaxBounce);
    stream->write(staticOnMaxBounce);
    stream->write(snapOnMaxBounce);
    stream->write(lifetime);
    stream->write(lifetimeVariance);
    stream->write(minSpinSpeed);
    stream->write(maxSpinSpeed);
    stream->write(velocity);
    stream->write(velocityVariance);
    stream->write(fade);
    stream->write(useRadiusMass);
    stream->write(baseRadius);
    stream->write(gravModifier);
    stream->write(terminalVelocity);
    stream->write(ignoreWater);

    stream->writeString(textureName);
    stream->writeString(shapeName);

    for (int i = 0; i < DDC_NUM_EMITTERS; i++)
    {
        if (stream->writeFlag(emitterList[i] != NULL))
        {
            stream->writeRangedU32(emitterList[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    if (stream->writeFlag(explosion))
    {
        stream->writeRangedU32(packed ? static_cast<SimObjectId>(reinterpret_cast<size_t>(explosion)) :
            explosion->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }

}


//--------------------------------------------------------------------------
// Unpack data
//--------------------------------------------------------------------------
void DebrisData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);

    stream->read(&elasticity);
    stream->read(&friction);
    stream->read(&numBounces);
    stream->read(&bounceVariance);
    stream->read(&minSpinSpeed);
    stream->read(&maxSpinSpeed);
    stream->read(&render2D);
    stream->read(&explodeOnMaxBounce);
    stream->read(&staticOnMaxBounce);
    stream->read(&snapOnMaxBounce);
    stream->read(&lifetime);
    stream->read(&lifetimeVariance);
    stream->read(&minSpinSpeed);
    stream->read(&maxSpinSpeed);
    stream->read(&velocity);
    stream->read(&velocityVariance);
    stream->read(&fade);
    stream->read(&useRadiusMass);
    stream->read(&baseRadius);
    stream->read(&gravModifier);
    stream->read(&terminalVelocity);
    stream->read(&ignoreWater);

    textureName = stream->readSTString();
    shapeName = stream->readSTString();

    for (int i = 0; i < DDC_NUM_EMITTERS; i++)
    {
        if (stream->readFlag())
        {
            emitterIDList[i] = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
        }
    }

    if (stream->readFlag())
    {
        explosionId = (S32)stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
    }
    else
    {
        explosionId = 0;
    }

}


//**************************************************************************
// Debris
//**************************************************************************

IMPLEMENT_CO_NETOBJECT_V1(Debris);

//----------------------------------------------------------------------------
// Initialize debris piece
//----------------------------------------------------------------------------
ConsoleMethod(Debris, init, bool, 4, 4, "(Point3F position, Point3F velocity)"
    "Set this piece of debris at the given position with the given velocity.")
{
    Point3F pos;
    dSscanf(argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z);

    Point3F vel;
    dSscanf(argv[3], "%f %f %f", &vel.x, &vel.y, &vel.z);

    object->init(pos, vel);

    return true;
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Debris::Debris()
{
    mVelocity = Point3F(0.0, 0.0, 4.0);
    mLifetime = gRandGen.randF(1.0, 10.0);
    mLastPos = getPosition();
    mNumBounces = gRandGen.randI(0, 1);
    mSize = 2.0;
    mElapsedTime = 0.0;
    mShape = NULL;
    mPart = NULL;
    mXRotSpeed = 0.0;
    mZRotSpeed = 0.0;
    mInitialTrans.identity();
    mRadius = 0.2;
    mStatic = false;

    dMemset(mEmitterList, 0, sizeof(mEmitterList));
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
Debris::~Debris()
{
    if (mShape)
    {
        delete mShape;
        mShape = NULL;
    }

    if (mPart)
    {
        delete mPart;
        mPart = NULL;
    }
}

//----------------------------------------------------------------------------
// Init class (static)
//----------------------------------------------------------------------------
void Debris::initPersistFields()
{
    addGroup("Misc");
    addField("lifetime", TypeF32, Offset(mLifetime, Debris));
    endGroup("Misc");
}

//----------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------
void Debris::init(const Point3F& position, const Point3F& velocity)
{
    setPosition(position);
    setVelocity(velocity);
}

//----------------------------------------------------------------------------
// On new data block
//----------------------------------------------------------------------------
bool Debris::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dynamic_cast<DebrisData*>(dptr);
    if (!mDataBlock || !Parent::onNewDataBlock(dptr))
        return false;

    scriptOnNewDataBlock();
    return true;

}

//----------------------------------------------------------------------------
// On Add
//----------------------------------------------------------------------------
bool Debris::onAdd()
{
    if (!Parent::onAdd())
    {
        return false;
    }

    // create emitters
    for (int i = 0; i < DebrisData::DDC_NUM_EMITTERS; i++)
    {
        if (mDataBlock->emitterList[i] != NULL)
        {
            ParticleEmitter* pEmitter = new ParticleEmitter;
            pEmitter->onNewDataBlock(mDataBlock->emitterList[i]);
            if (!pEmitter->registerObject())
            {
                Con::warnf(ConsoleLogEntry::General, "Could not register emitter for particle of class: %s", mDataBlock->getName());
                delete pEmitter;
                pEmitter = NULL;
            }
            mEmitterList[i] = pEmitter;
        }
    }

    // set particle sizes based on debris size
    F32 sizeList[ParticleData::PDC_NUM_KEYS];

    if (mEmitterList[0])
    {
        sizeList[0] = mSize * 0.5;
        sizeList[1] = mSize;
        sizeList[2] = mSize * 1.5;

        mEmitterList[0]->setSizes(sizeList);
    }

    if (mEmitterList[1])
    {
        sizeList[0] = 0.0;
        sizeList[1] = mSize * 0.5;
        sizeList[2] = mSize;

        mEmitterList[1]->setSizes(sizeList);
    }

    S32 bounceVar = gRandGen.randI(-mDataBlock->bounceVariance, mDataBlock->bounceVariance);
    mNumBounces = mDataBlock->numBounces + bounceVar;

    F32 lifeVar = (mDataBlock->lifetimeVariance * 2.0f * gRandGen.randF(-1.0, 1.0)) - mDataBlock->lifetimeVariance;
    mLifetime = mDataBlock->lifetime + lifeVar;

    F32 xRotSpeed = gRandGen.randF(mDataBlock->minSpinSpeed, mDataBlock->maxSpinSpeed);
    F32 zRotSpeed = gRandGen.randF(mDataBlock->minSpinSpeed, mDataBlock->maxSpinSpeed);
    zRotSpeed *= gRandGen.randF(0.1, 0.5);

    mRotAngles.set(xRotSpeed, 0.0, zRotSpeed);

    mElasticity = mDataBlock->elasticity;
    mFriction = mDataBlock->friction;

    // Setup our bounding box
    if (mDataBlock->shape)
    {
        mObjBox = mDataBlock->shape->bounds;
    }
    else
    {
        mObjBox = Box3F(Point3F(-1, -1, -1), Point3F(1, 1, 1));
    }

    if (mDataBlock->shape)
    {
        mShape = new TSShapeInstance(mDataBlock->shape, true);
    }

    if (mPart)
    {
        // use half radius becuase we want debris to stick in ground
        mRadius = mPart->getRadius() * 0.5;
        mObjBox = mPart->getBounds();
    }

    resetWorldBox();

    mInitialTrans = getTransform();

    if (mDataBlock->velocity != 0.0)
    {
        F32 velocity = mDataBlock->velocity + gRandGen.randF(-mDataBlock->velocityVariance, mDataBlock->velocityVariance);

        mVelocity.normalizeSafe();
        mVelocity *= velocity;
    }

    // mass calculations
    if (mDataBlock->useRadiusMass)
    {
        if (mRadius < mDataBlock->baseRadius)
        {
            mRadius = mDataBlock->baseRadius;
        }

        // linear falloff
        F32 multFactor = mDataBlock->baseRadius / mRadius;

        mElasticity *= multFactor;
        mFriction *= multFactor;
        mRotAngles *= multFactor;
    }


    // tell engine the debris exists
    getCurrentClientContainer()->addObject(this);
    getCurrentClientSceneGraph()->addObjectToScene(this);

    removeFromProcessList();
    gClientProcessList.addObject(this);

    NetConnection* pNC = NetConnection::getConnectionToServer();
    AssertFatal(pNC != NULL, "Error, must have a connection to the server!");
    pNC->addObject(this);

    return true;
}

//----------------------------------------------------------------------------
// On Remove
//----------------------------------------------------------------------------
void Debris::onRemove()
{
    for (int i = 0; i < DebrisData::DDC_NUM_EMITTERS; i++)
    {
        if (mEmitterList[i])
        {
            mEmitterList[i]->deleteWhenEmpty();
            mEmitterList[i] = NULL;
        }
    }

    if (mPart)
    {
        TSShapeInstance* ss = mPart->getSourceShapeInstance();
        if (ss)
        {
            ss->decDebrisRefCount();
            if (ss->getDebrisRefCount() == 0)
            {
                delete ss;
            }
        }
    }

    mSceneManager->removeObjectFromScene(this);
    getContainer()->removeObject(this);

    Parent::onRemove();
}

//----------------------------------------------------------------------------
// Process tick
//----------------------------------------------------------------------------
void Debris::processTick(const Move*)
{
    if (mLifetime <= 0.0)
        deleteObject();
}

//----------------------------------------------------------------------------
// Advance Time
//----------------------------------------------------------------------------
void Debris::advanceTime(F32 dt)
{
    mElapsedTime += dt;

    mLifetime -= dt;
    if (mLifetime <= 0.0)
    {
        mLifetime = 0.0;
        return;
    }

    mLastPos = getPosition();

    if (!mStatic)
    {
        rotate(dt);

        Point3F nextPos = getPosition();
        computeNewState(nextPos, mVelocity, dt);

        if (bounce(nextPos, dt))
        {
            --mNumBounces;
            if (mNumBounces <= 0)
            {
                if (mDataBlock->explodeOnMaxBounce)
                {
                    explode();
                    mLifetime = 0.0;
                }
                if (mDataBlock->snapOnMaxBounce)
                {
                    // orient debris so it's flat
                    MatrixF stat = getTransform();

                    Point3F dir;
                    stat.getColumn(1, &dir);
                    dir.z = 0.0;

                    MatrixF newTrans = MathUtils::createOrientFromDir(dir);

                    // hack for shell casings to get them above ground.  Need something better - bramage
                    newTrans.setPosition(getPosition() + Point3F(0.0, 0.0, 0.10));

                    setTransform(newTrans);
                }
                if (mDataBlock->staticOnMaxBounce)
                {
                    mStatic = true;
                }
            }
        }
        else
        {
            setPosition(nextPos);
        }
    }

    Point3F pos(getPosition());
    updateEmitters(pos, mVelocity, (U32)(dt * 1000.0));

}

//----------------------------------------------------------------------------
// Rotate debris
//----------------------------------------------------------------------------
void Debris::rotate(F32 dt)
{

    MatrixF curTrans = getTransform();
    curTrans.setPosition(Point3F(0.0, 0.0, 0.0));

    Point3F curAngles = mRotAngles * dt * M_PI / 180.0;
    MatrixF rotMatrix(EulerF(curAngles.x, curAngles.y, curAngles.z));

    curTrans.mul(rotMatrix);
    curTrans.setPosition(getPosition());
    setTransform(curTrans);

}

//----------------------------------------------------------------------------
// Bounce the debris - returns true if debris bounces
//----------------------------------------------------------------------------
bool Debris::bounce(const Point3F& nextPos, F32 dt)
{
    Point3F curPos = getPosition();

    Point3F dir = nextPos - curPos;
    if (dir.magnitudeSafe() == 0.0) return false;
    dir.normalizeSafe();
    Point3F extent = nextPos + dir * mRadius;
    F32 totalDist = Point3F(extent - curPos).magnitudeSafe();
    F32 moveDist = Point3F(nextPos - curPos).magnitudeSafe();
    F32 movePercent = (moveDist / totalDist);

    RayInfo rayInfo;
    U32 collisionMask = csmStaticCollisionMask;
    if (!mDataBlock->ignoreWater)
    {
        collisionMask |= WaterObjectType;
    }

    if (getContainer()->castRay(curPos, extent, collisionMask, &rayInfo))
    {

        Point3F reflection = mVelocity - rayInfo.normal * (mDot(mVelocity, rayInfo.normal) * 2.0);
        mVelocity = reflection;

        Point3F tangent = reflection - rayInfo.normal * mDot(reflection, rayInfo.normal);
        mVelocity -= tangent * mFriction;

        Point3F velDir = mVelocity;
        velDir.normalizeSafe();

        mVelocity *= mElasticity;

        Point3F bouncePos = curPos + dir * rayInfo.t * movePercent;
        bouncePos += mVelocity * dt;

        setPosition(bouncePos);

        mRotAngles *= mElasticity;

        return true;

    }

    return false;

}

//----------------------------------------------------------------------------
// Explode
//----------------------------------------------------------------------------
void Debris::explode()
{

    if (!mDataBlock->explosion) return;

    Point3F explosionPos = getPosition();

    Explosion* pExplosion = new Explosion;
    pExplosion->onNewDataBlock(mDataBlock->explosion);

    MatrixF trans(true);
    trans.setPosition(getPosition());

    pExplosion->setTransform(trans);
    pExplosion->setInitialState(explosionPos, VectorF(0, 0, 1), 1);
    if (!pExplosion->registerObject())
        delete pExplosion;
}

//----------------------------------------------------------------------------
// Compute state of debris as if it hasn't collided with anything
//----------------------------------------------------------------------------
void Debris::computeNewState(Point3F& newPos, Point3F& newVel, F32 dt)
{

    // apply gravity
    Point3F force = Point3F(0, 0, -9.81 * mDataBlock->gravModifier);

    if (mDataBlock->terminalVelocity > 0.0001)
    {
        if (newVel.magnitudeSafe() > mDataBlock->terminalVelocity)
        {
            newVel.normalizeSafe();
            newVel *= mDataBlock->terminalVelocity;
        }
        else
        {
            newVel += force * dt;
        }
    }
    else
    {
        newVel += force * dt;
    }

    newPos += newVel * dt;

}

//----------------------------------------------------------------------------
// Update emitters
//----------------------------------------------------------------------------
void Debris::updateEmitters(Point3F& pos, Point3F& vel, U32 ms)
{

    Point3F axis = -vel;

    if (axis.magnitudeSafe() == 0.0)
    {
        axis = Point3F(0.0, 0.0, 1.0);
    }
    axis.normalizeSafe();


    Point3F lastPos = mLastPos;

    for (int i = 0; i < DebrisData::DDC_NUM_EMITTERS; i++)
    {
        if (mEmitterList[i])
        {
            mEmitterList[i]->emitParticles(lastPos, pos, axis, vel, ms);
        }
    }

}

//----------------------------------------------------------------------------
// Render debris
//----------------------------------------------------------------------------
bool Debris::prepRenderImage(SceneState* state, const U32 stateKey,
    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
    if (isLastState(state, stateKey))
        return false;
    setLastState(state, stateKey);

    // This should be sufficient for most objects that don't manage zones, and
    //  don't need to return a specialized RenderImage...
    if (state->isObjectRendered(this) && (mPart || mShape))
    {
        Point3F cameraOffset;
        mObjToWorld.getColumn(3, &cameraOffset);
        cameraOffset -= state->getCameraPosition();
        F32 dist = cameraOffset.len();
        F32 invScale = (1.0f / getMax(getMax(mObjScale.x, mObjScale.y), mObjScale.z));

        if (mShape)
        {
            DetailManager::selectPotentialDetails(mShape, dist, invScale);
            if (mShape->getCurrentDetail() < 0)
            {
                return false;
            }
        }

        if (mPart)
        {
            DetailManager::selectPotentialDetails(mPart, dist, invScale);
        }

        prepBatchRender(state);
    }

    return false;
}

//----------------------------------------------------------------------------
// Render Object
//----------------------------------------------------------------------------
void Debris::prepBatchRender(SceneState* state)
{
    RectI viewport = GFX->getViewport();
    MatrixF proj = GFX->getProjectionMatrix();
    GFX->pushWorldMatrix();

    state->setupObjectProjection(this);

    F32 alpha = 1.0;
    if (mDataBlock->fade)
    {
        if (mLifetime < 1.0) alpha = mLifetime;
    }

    if ((mShape && DetailManager::selectCurrentDetail(mShape)) ||
        (mPart && DetailManager::selectCurrentDetail(mPart)))
    {
        MatrixF world = GFX->getWorldMatrix();
        GFX->pushWorldMatrix();
        world *= mObjToWorld;

        Point3F cameraOffset;
        mObjToWorld.getColumn(3, &cameraOffset);
        cameraOffset -= state->getCameraPosition();
        F32 fogAmount = state->getHazeAndFog(cameraOffset.len(), cameraOffset.z);

        world = GFX->getWorldMatrix();
        TSMesh::setCamTrans(world);
        TSMesh::setSceneState(state);
        TSMesh::setObject(this);


        if (mShape)
        {
            MatrixF mat = getRenderTransform();
            GFX->setWorldMatrix(mat);

            TSMesh::setOverrideFade(alpha);
            mShape->setupFog(fogAmount, state->getFogColor());
            mShape->render();
            TSMesh::setOverrideFade(1.0);
        }
        else
        {
            if (mPart->getCurrentObjectDetail() != -1)
            {
                TSShapeInstance* parent = mPart->getSourceShapeInstance();

                parent->setupFog(fogAmount, state->getFogColor());
                TSMesh::setOverrideFade(alpha);
                mPart->render();
                TSMesh::setOverrideFade(1.0);
            }
        }

        GFX->popWorldMatrix();
    }

    render2D();

    GFX->popWorldMatrix();
    GFX->setProjectionMatrix(proj);
    GFX->setViewport(viewport);

}

//----------------------------------------------------------------------------
// Render 2D debris
//----------------------------------------------------------------------------
void Debris::render2D()
{
    /*
       if( !mDataBlock->render2D ) return;

       glBindTexture( GL_TEXTURE_2D, mDataBlock->texture.getGLName() );
       glColor4f( 1.0, 1.0, 1.0, 1.0 );

       glDisable(GL_CULL_FACE);
       glEnable(GL_BLEND);
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
       glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
       glEnable(GL_TEXTURE_2D);

       dglDrawBillboard( getPosition(), 0.1, 0.0 );

       glDisable(GL_TEXTURE_2D);
       glDisable(GL_BLEND);
       glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    */
}

//----------------------------------------------------------------------------
// Set size
//----------------------------------------------------------------------------
void Debris::setSize(F32 size)
{
    mSize = size;

}
