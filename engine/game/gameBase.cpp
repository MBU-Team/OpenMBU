//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/gameBase.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "core/bitStream.h"
#include "sim/netConnection.h"
#include "game/gameConnection.h"
#include "math/mathIO.h"
#include "game/gameProcess.h"

//----------------------------------------------------------------------------
// Ghost update relative priority values

static F32 sUpFov = 1.0;
static F32 sUpDistance = 0.4;
static F32 sUpVelocity = 0.4;
static F32 sUpSkips = 0.2;
static F32 sUpOwnership = 0.2;
static F32 sUpInterest = 0.2;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(GameBaseData);

GameBaseData::GameBaseData()
{
    category = "";
    className = "";
    packed = false;
}

bool GameBaseData::onAdd()
{
    if (!Parent::onAdd())
        return false;

    // Link our object name to the datablock class name and
    // then onto our C++ class name.
    const char* name = getName();
    if (name && name[0] && getClassRep()) {
        bool linkSuccess = false;
        Namespace* parent = getClassRep()->getNameSpace();
        if (className && className[0] && dStricmp(className, parent->mName)) {
            linkSuccess = Con::linkNamespaces(parent->mName, className);
            if (linkSuccess)
                linkSuccess = Con::linkNamespaces(className, name);
        }
        else
            linkSuccess = Con::linkNamespaces(parent->mName, name);
        if (linkSuccess)
            mNameSpace = Con::lookupNamespace(name);
    }

    // If no className was specified, set it to our C++ class name
    if (!className || !className[0])
        className = getClassRep()->getClassName();

    return true;
}

void GameBaseData::initPersistFields()
{
    Parent::initPersistFields();
    addField("category", TypeCaseString, Offset(category, GameBaseData));
    addField("className", TypeString, Offset(className, GameBaseData));
}

bool GameBaseData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;
    packed = false;
    return true;
}

void GameBaseData::unpackData(BitStream* stream)
{
    Parent::unpackData(stream);
    packed = true;
}


//----------------------------------------------------------------------------
bool UNPACK_DB_ID(BitStream* stream, U32& id)
{
    if (stream->readFlag())
    {
        id = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
        return true;
    }
    return false;
}

bool PACK_DB_ID(BitStream* stream, U32 id)
{
    if (stream->writeFlag(id))
    {
        stream->writeRangedU32(id, DataBlockObjectIdFirst, DataBlockObjectIdLast);
        return true;
    }
    return false;
}

bool PRELOAD_DB(U32& id, SimDataBlock** data, bool server, const char* clientMissing, const char* serverMissing)
{
    if (server)
    {
        if (*data)
            id = (*data)->getId();
        else if (server && serverMissing)
        {
            Con::errorf(ConsoleLogEntry::General, serverMissing);
            return false;
        }
    }
    else
    {
        if (id && !Sim::findObject(id, *data) && clientMissing)
        {
            Con::errorf(ConsoleLogEntry::General, clientMissing);
            return false;
        }
    }
    return true;
}
//----------------------------------------------------------------------------

bool GameBase::gShowBoundingBox;

//----------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(GameBase);

GameBase::GameBase()
{
    mNetFlags.set(Ghostable);
    mTypeMask |= GameBaseObjectType;

    mProcessLink.next = mProcessLink.prev = this;
    mAfterObject = 0;
    mProcessTag = 0;
    mLastDelta = 0;
    mDataBlock = 0;
    mProcessTick = true;
    mNameTag = "";
    mControllingClient = 0;
}

GameBase::~GameBase()
{
    plUnlink();
}


//----------------------------------------------------------------------------

bool GameBase::onAdd()
{
    if (!Parent::onAdd() || !mDataBlock)
        return false;

    if (isClientObject() || gSPMode) {
        // Client datablock are initialized by the initial update
        getCurrentClientProcessList()->addObject(this);
    }
    else {
        getCurrentServerProcessList()->addObject(this);
    }

    // Datablock must be initialized on the server
    if (!isClientObject() && !onNewDataBlock(mDataBlock))
        return false;

    if (gSPMode)
        mTypeMask &= ~GameBaseObjectType;

    return true;
}

void GameBase::onRemove()
{
    plUnlink();
    Parent::onRemove();
}

bool GameBase::onNewDataBlock(GameBaseData* dptr)
{
    mDataBlock = dptr;

    if (!mDataBlock)
        return false;

    setMaskBits(DataBlockMask);
    return true;
}

void GameBase::inspectPostApply()
{
    Parent::inspectPostApply();
    setMaskBits(ExtendedInfoMask);
}

//----------------------------------------------------------------------------
void GameBase::processTick(const Move*)
{
    mLastDelta = 0;
}

void GameBase::interpolateTick(F32 delta)
{
    mLastDelta = delta;
}

void GameBase::advanceTime(F32)
{
}


//----------------------------------------------------------------------------

F32 GameBase::getUpdatePriority(CameraScopeQuery* camInfo, U32 updateMask, S32 updateSkips)
{
    updateMask;

    // Calculate a priority used to decide if this object
    // will be updated on the client.  All the weights
    // are calculated 0 -> 1  Then weighted together at the
    // end to produce a priority.
    Point3F pos;
    getWorldBox().getCenter(&pos);
    pos -= camInfo->pos;
    F32 dist = pos.len();
    if (dist == 0.0f) dist = 0.001f;
    pos *= 1.0f / dist;

    // Weight based on linear distance, the basic stuff.
    F32 wDistance = (dist < camInfo->visibleDistance) ?
        1.0f - (dist / camInfo->visibleDistance) : 0.0f;

    // Weight by field of view, objects directly in front
    // will be weighted 1, objects behind will be 0
    F32 dot = mDot(pos, camInfo->orientation);
    bool inFov = dot > camInfo->cosFov;
    F32 wFov = inFov ? 1.0f : 0;

    // Weight by linear velocity parallel to the viewing plane
    // (if it's the field of view, 0 if it's not).
    F32 wVelocity = 0.0f;
    if (inFov)
    {
        Point3F vec;
        mCross(camInfo->orientation, getVelocity(), &vec);
        wVelocity = (vec.len() * camInfo->fov) /
            (camInfo->fov * camInfo->visibleDistance);
        if (wVelocity > 1.0f)
            wVelocity = 1.0f;
    }

    // Weight by interest.
    F32 wInterest;
    if (getType() & PlayerObjectType)
        wInterest = 0.75f;
    else if (getType() & ProjectileObjectType)
    {
        // Projectiles are more interesting if they
        // are heading for us.
        wInterest = 0.30f;
        F32 dot = -mDot(pos, getVelocity());
        if (dot > 0.0f)
            wInterest += 0.20 * dot;
    }
    else
    {
        if (getType() & ItemObjectType)
            wInterest = 0.25f;
        else
            // Everything else is less interesting.
            wInterest = 0.0f;
    }

    // Weight by updateSkips
    F32 wSkips = updateSkips * 0.5;

    // Calculate final priority, should total to about 1.0f
    //
    return
        wFov * sUpFov +
        wDistance * sUpDistance +
        wVelocity * sUpVelocity +
        wSkips * sUpSkips +
        wInterest * sUpInterest;
}


Point3F GameBase::getVelocity() const
{
    return Point3F(0, 0, 0);
}


//----------------------------------------------------------------------------
bool GameBase::setDataBlock(GameBaseData* dptr)
{
    if (gSPMode || isGhost() || isProperlyAdded()) {
        if (mDataBlock != dptr)
            return onNewDataBlock(dptr);
    }
    else
        mDataBlock = dptr;
    return true;
}


//--------------------------------------------------------------------------
void GameBase::scriptOnAdd()
{
    // Script onAdd() must be called by the leaf class after
    // everything is ready.
    if (!isGhost())
        Con::executef(mDataBlock, 2, "onAdd", scriptThis());
}

void GameBase::scriptOnNewDataBlock()
{
    // Script onNewDataBlock() must be called by the leaf class
    // after everything is loaded.
    if (!isGhost())
        Con::executef(mDataBlock, 2, "onNewDataBlock", scriptThis());
}

void GameBase::scriptOnRemove()
{
    // Script onRemove() must be called by leaf class while
    // the object state is still valid.
    if (!isGhost() && mDataBlock)
        Con::executef(mDataBlock, 2, "onRemove", scriptThis());
}


//--------------------------------------------------------------------------
void GameBase::plUnlink()
{
    mProcessLink.next->mProcessLink.prev = mProcessLink.prev;
    mProcessLink.prev->mProcessLink.next = mProcessLink.next;
    mProcessLink.next = mProcessLink.prev = this;
}

void GameBase::plLinkAfter(GameBase* obj)
{
    // Link this after obj
    mProcessLink.next = obj->mProcessLink.next;
    mProcessLink.prev = obj;
    obj->mProcessLink.next = this;
    mProcessLink.next->mProcessLink.prev = this;
}

void GameBase::plLinkBefore(GameBase* obj)
{
    // Link this before obj
    mProcessLink.next = obj;
    mProcessLink.prev = obj->mProcessLink.prev;
    obj->mProcessLink.prev = this;
    mProcessLink.prev->mProcessLink.next = this;
}

void GameBase::plJoin(GameBase* head)
{
    GameBase* tail1 = head->mProcessLink.prev;
    GameBase* tail2 = mProcessLink.prev;
    tail1->mProcessLink.next = this;
    mProcessLink.prev = tail1;
    tail2->mProcessLink.next = head;
    head->mProcessLink.prev = tail2;
}


//----------------------------------------------------------------------------
void GameBase::processAfter(GameBase* obj)
{
    mAfterObject = obj;
    if ((const GameBase*)obj->mAfterObject == this)
        obj->mAfterObject = 0;
    if (isGhost())
        gClientProcessList.markDirty();
    else
        gServerProcessList.markDirty();
}

void GameBase::clearProcessAfter()
{
    mAfterObject = 0;
}


//----------------------------------------------------------------------------

void GameBase::setControllingClient(GameConnection* client)
{
    if (isClientObject())
    {
        if (mControllingClient)
            Con::executef(this, 3, "setControl", "0");
        if (client)
            Con::executef(this, 3, "setControl", "1");
    }

    mControllingClient = client;
    setMaskBits(ControlMask);
}

U32 GameBase::getPacketDataChecksum(GameConnection*)
{
    return 0;
}

void GameBase::writePacketData(GameConnection*, BitStream*)
{
}

void GameBase::readPacketData(GameConnection*, BitStream*)
{
}

U32 GameBase::packUpdate(NetConnection*, U32 mask, BitStream* stream)
{
    // Check the mask for the ScaleMask; if it's true, pass that in.
    if (stream->writeFlag(mask & ScaleMask))
    {
        mathWrite(*stream, Parent::getScale());
    }

    if (stream->writeFlag((mask & DataBlockMask) && mDataBlock != NULL))
    {
        stream->writeRangedU32(mDataBlock->getId(),
            DataBlockObjectIdFirst,
            DataBlockObjectIdLast);
    }

    return 0;
}

void GameBase::unpackUpdate(NetConnection* con, BitStream* stream)
{
    if (stream->readFlag()) {
        VectorF scale;
        mathRead(*stream, &scale);
        setScale(scale);
    }
    if (stream->readFlag()) {
        GameBaseData* dptr = 0;
        SimObjectId id = stream->readRangedU32(DataBlockObjectIdFirst,
            DataBlockObjectIdLast);

        if (!Sim::findObject(id, dptr) || !setDataBlock(dptr))
            con->setLastError("Invalid packet GameBase::unpackUpdate()");
    }
}

//----------------------------------------------------------------------------
ConsoleMethod(GameBase, getDataBlock, S32, 2, 2, "()"
    "Return the datablock this GameBase is using.")
{
    return object->getDataBlock() ? object->getDataBlock()->getId() : 0;
}

//----------------------------------------------------------------------------
ConsoleMethod(GameBase, setDataBlock, bool, 3, 3, "(DataBlock db)"
    "Assign this GameBase to use the specified datablock.")
{
    GameBaseData* data;
    if (Sim::findObject(argv[2], data)) {
        return object->setDataBlock(data);
    }
    Con::errorf("Could not find data block \"%s\"", argv[2]);
    return false;
}

//----------------------------------------------------------------------------
IMPLEMENT_CONSOLETYPE(GameBaseData)
IMPLEMENT_GETDATATYPE(GameBaseData)
IMPLEMENT_SETDATATYPE(GameBaseData)

void GameBase::initPersistFields()
{
    Parent::initPersistFields();

    addGroup("Misc");
    addField("nameTag", TypeCaseString, Offset(mNameTag, GameBase));
    addField("dataBlock", TypeGameBaseDataPtr, Offset(mDataBlock, GameBase));
    endGroup("Misc");
}

void GameBase::consoleInit()
{
#ifdef TORQUE_DEBUG
    Con::addVariable("GameBase::boundingBox", TypeBool, &gShowBoundingBox);
#endif
}
