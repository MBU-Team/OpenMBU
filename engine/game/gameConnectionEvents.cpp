//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "core/bitStream.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "sim/pathManager.h"
#include "sceneGraph/sceneGraph.h"
#include "sfx/sfxSystem.h"
#include "game/game.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"
#include "game/gameConnectionEvents.h"

#define DebugChecksum 0xF00DBAAD

//--------------------------------------------------------------------------
IMPLEMENT_CO_CLIENTEVENT_V1(SimDataBlockEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(Sim2DAudioEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(Sim3DAudioEvent);
IMPLEMENT_CO_CLIENTEVENT_V1(SetMissionCRCEvent);


//----------------------------------------------------------------------------

SimDataBlockEvent::~SimDataBlockEvent()
{
    delete mObj;
}
SimDataBlockEvent::SimDataBlockEvent(SimDataBlock* obj, U32 index, U32 total, U32 missionSequence)
{
    mObj = NULL;
    mIndex = index;
    mTotal = total;
    mMissionSequence = missionSequence;
    mProcess = false;

    if (obj)
    {
        id = obj->getId();
        AssertFatal(id >= DataBlockObjectIdFirst && id <= DataBlockObjectIdLast,
            "Out of range event data block id... check simBase.h");
        //      Con::printf("queuing data block: %d", mIndex);
    }
}

#ifdef TORQUE_DEBUG_NET
const char* SimDataBlockEvent::getDebugName()
{
    SimObject* obj = Sim::findObject(id);
    static char buffer[256];
    dSprintf(buffer, sizeof(buffer), "%s [%s - %s]",
        getClassName(),
        obj ? obj->getName() : "",
        obj ? obj->getClassName() : "NONE");
    return buffer;
}
#endif // TORQUE_DEBUG_NET

void SimDataBlockEvent::notifyDelivered(NetConnection* conn, bool)
{
    // if the modified key for this event is not the current one,
    // we've already resorted and resent some blocks, so fall out.
    if (conn->isRemoved())
        return;
    GameConnection* gc = (GameConnection*)conn;
    if (gc->getDataBlockSequence() != mMissionSequence)
        return;

    U32 nextIndex = mIndex + DataBlockQueueCount;
    SimDataBlockGroup* g = Sim::getDataBlockGroup();

    if (mIndex == g->size() - 1)
    {
        gc->setDataBlockModifiedKey(gc->getMaxDataBlockModifiedKey());
        gc->sendConnectionMessage(GameConnection::DataBlocksDone, mMissionSequence);
    }
    if (g->size() <= nextIndex)
    {
        return;
    }
    SimDataBlock* blk = (SimDataBlock*)(*g)[nextIndex];
    gc->postNetEvent(new SimDataBlockEvent(blk, nextIndex, g->size(), mMissionSequence));
}

void SimDataBlockEvent::pack(NetConnection* conn, BitStream* bstream)
{
    SimDataBlock* obj;
    Sim::findObject(id, obj);
    GameConnection* gc = (GameConnection*)conn;
    if (bstream->writeFlag(gc->getDataBlockModifiedKey() < obj->getModifiedKey()))
    {
        if (obj->getModifiedKey() > gc->getMaxDataBlockModifiedKey())
            gc->setMaxDataBlockModifiedKey(obj->getModifiedKey());

        AssertFatal(obj,
            "SimDataBlockEvent:: Data blocks cannot be deleted");
        bstream->writeInt(id - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);

        S32 classId = obj->getClassId(conn->getNetClassGroup());
        bstream->writeClassId(classId, NetClassTypeDataBlock, conn->getNetClassGroup());
        bstream->writeInt(mIndex, DataBlockObjectIdBitSize);
        bstream->writeInt(mTotal, DataBlockObjectIdBitSize + 1);
        obj->packData(bstream);
#ifdef TORQUE_DEBUG_NET
        bstream->writeInt(classId ^ DebugChecksum, 32);
#endif

    }
}

void SimDataBlockEvent::unpack(NetConnection* cptr, BitStream* bstream)
{
    if (bstream->readFlag())
    {
        mProcess = true;
        id = bstream->readInt(DataBlockObjectIdBitSize) + DataBlockObjectIdFirst;
        S32 classId = bstream->readClassId(NetClassTypeDataBlock, cptr->getNetClassGroup());
        mIndex = bstream->readInt(DataBlockObjectIdBitSize);
        mTotal = bstream->readInt(DataBlockObjectIdBitSize + 1);

        SimObject* ptr = (SimObject*)ConsoleObject::create(cptr->getNetClassGroup(), NetClassTypeDataBlock, classId);
        if ((mObj = dynamic_cast<SimDataBlock*>(ptr)) != 0) {
            //Con::printf(" - SimDataBlockEvent: unpacking event of type: %s", mObj->getClassName());
            mObj->unpackData(bstream);
        }
        else
        {
            //Con::printf(" - SimDataBlockEvent: INVALID PACKET!  Could not create class with classID: %d", classId);
            delete ptr;
            cptr->setLastError("Invalid packet in SimDataBlockEvent::unpack()");
        }

#ifdef TORQUE_DEBUG_NET
        U32 checksum = bstream->readInt(32);
        AssertISV((checksum ^ DebugChecksum) == (U32)classId,
            avar("unpack did not match pack for event of class %s.",
                mObj->getClassName()));
#endif

    }
}

void SimDataBlockEvent::write(NetConnection* cptr, BitStream* bstream)
{
    if (bstream->writeFlag(mProcess))
    {
        bstream->writeInt(id - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);
        S32 classId = mObj->getClassId(cptr->getNetClassGroup());
        bstream->writeClassId(classId, NetClassTypeDataBlock, cptr->getNetClassGroup());
        bstream->writeInt(mIndex, DataBlockObjectIdBitSize);
        bstream->writeInt(mTotal, DataBlockObjectIdBitSize + 1);
        mObj->packData(bstream);
    }
}

void SimDataBlockEvent::process(NetConnection* cptr)
{
    if (mProcess)
    {
        //call the console function to set the number of blocks to be sent
        Con::executef(3, "onDataBlockObjectReceived", Con::getIntArg(mIndex), Con::getIntArg(mTotal));

        SimDataBlock* obj = NULL;
        char* errorBuffer = NetConnection::getErrorBuffer();

        if (Sim::findObject(id, obj) && dStrcmp(obj->getClassName(), mObj->getClassName()) == 0)
        {
            U8 buf[1500];
            BitStream stream(buf, 1500);
            mObj->packData(&stream);
            stream.setPosition(0);
            obj->unpackData(&stream);
            obj->preload(false, errorBuffer);
        }
        else
        {
            if (obj != NULL)
            {
                Con::warnf("A '%s' datablock with id: %d already existed. Clobbering it with new '%s' datablock from server.", obj->getClassName(), id, mObj->getClassName());
                obj->deleteObject();
            }

            bool reg = mObj->registerObject(id);
            cptr->addObject(mObj);
            GameConnection* conn = dynamic_cast<GameConnection*>(cptr);
            if (conn)
            {
                conn->preloadDataBlock(mObj);
                mObj = NULL;
            }
        }
    }
}


//----------------------------------------------------------------------------


Sim2DAudioEvent::Sim2DAudioEvent(const SFXProfile* profile)
{
    mProfile = profile;
}

void Sim2DAudioEvent::pack(NetConnection*, BitStream* bstream)
{
    bstream->writeInt(mProfile->getId() - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);
}

void Sim2DAudioEvent::write(NetConnection*, BitStream* bstream)
{
    bstream->writeInt(mProfile->getId() - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);
}

void Sim2DAudioEvent::unpack(NetConnection*, BitStream* bstream)
{
    SimObjectId id = bstream->readInt(DataBlockObjectIdBitSize) + DataBlockObjectIdFirst;
    Sim::findObject(id, mProfile);
}

void Sim2DAudioEvent::process(NetConnection*)
{
    if (mProfile)
        SFX->playOnce(mProfile);
}

//----------------------------------------------------------------------------

static F32 SoundPosAccuracy = 0.5;
static S32 SoundRotBits = 8;


Sim3DAudioEvent::Sim3DAudioEvent(const SFXProfile* profile, const MatrixF* mat)
{
    mProfile = profile;
    if (mat)
        mTransform = *mat;
}

void Sim3DAudioEvent::pack(NetConnection* con, BitStream* bstream)
{
    bstream->writeInt(mProfile->getId() - DataBlockObjectIdFirst, DataBlockObjectIdBitSize);

    // If the sound has cone parameters, the orientation is
    // transmitted as well.
    SFXDescription* ad = mProfile->getDescription();
    if (bstream->writeFlag(ad->mConeInsideAngle || ad->mConeOutsideAngle))
    {
        QuatF q(mTransform);
        q.normalize();

        // LH - we can get a valid quat that's very slightly over 1 in and so
        // this fails (barely) check against zero.  So use some error-
        AssertFatal((1.0 - ((q.x * q.x) + (q.y * q.y) + (q.z * q.z))) >= (0.0 - 0.001),
            "QuatF::normalize() is broken in Sim3DAudioEvent");

        bstream->writeFloat(q.x, SoundRotBits);
        bstream->writeFloat(q.y, SoundRotBits);
        bstream->writeFloat(q.z, SoundRotBits);
        bstream->writeFlag(q.w < 0.0);
    }

    Point3F pos;
    mTransform.getColumn(3, &pos);
    bstream->writeCompressedPoint(pos, SoundPosAccuracy);
}

void Sim3DAudioEvent::write(NetConnection* con, BitStream* bstream)
{
    // Just do the normal pack...
    pack(con, bstream);
}

void Sim3DAudioEvent::unpack(NetConnection* con, BitStream* bstream)
{
    SimObjectId id = bstream->readInt(DataBlockObjectIdBitSize) + DataBlockObjectIdFirst;
    Sim::findObject(id, mProfile);

    if (bstream->readFlag()) {
        QuatF q;
        q.x = bstream->readFloat(SoundRotBits);
        q.y = bstream->readFloat(SoundRotBits);
        q.z = bstream->readFloat(SoundRotBits);
        F32 value = ((q.x * q.x) + (q.y * q.y) + (q.z * q.z));
        // #ifdef __linux
              // Hmm, this should never happen, but it does...
        if (value > 1.f)
            value = 1.f;
        // #endif
        q.w = mSqrt(1.f - value);
        if (bstream->readFlag())
            q.w = -q.w;
        q.setMatrix(&mTransform);
    }
    else
        mTransform.identity();

    Point3F pos;
    bstream->readCompressedPoint(&pos, SoundPosAccuracy);
    mTransform.setColumn(3, pos);
}

void Sim3DAudioEvent::process(NetConnection*)
{
    if (mProfile)
        SFX->playOnce(mProfile, &mTransform);
}

