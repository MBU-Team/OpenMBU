//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/gameBase.h"

#include "tickCache.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "core/bitStream.h"
#include "sim/netConnection.h"
#include "game/gameConnection.h"
#include "math/mathIO.h"
#include "game/gameProcess.h"
#include "gui/core/guiCanvas.h"

#ifdef TORQUE_DEBUG_NET_MOVES
#include "game/aiConnection.h"
#endif

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
    
    Canvas->RefreshAndRepaint();

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

    mProcessTag = 0;
    mDataBlock = NULL;
    mProcessTick = true;
    mNameTag = "";
    mControllingClient = 0;
    mTickCacheHead = NULL;

#ifdef TORQUE_DEBUG_NET_MOVES
    mLastMoveId = 0;
    mTicksSinceLastMove = 0;
    mIsAiControlled = false;
#endif
}

GameBase::~GameBase()
{
    plUnlink();
    if (mTickCacheHead)
    {
        setTickCacheSize(0);
        TickCacheHead::free(mTickCacheHead);
        mTickCacheHead = NULL;
    }
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
        mTypeMask &= ~GameBaseHiFiObjectType;

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
void GameBase::processTick(const Move* move)
{
#ifdef TORQUE_DEBUG_NET_MOVES
    if (!move)
        mTicksSinceLastMove++;

    const char* srv = isClientObject() ? "client" : "server";
    const char* who = "";
    if (isClientObject())
    {
        if (this == (GameBase*)GameConnection::getConnectionToServer()->getControlObject())
            who = " player";
        else
            who = " ghost";
        if (mIsAiControlled)
            who = " ai";
    }
    if (isServerObject())
    {
        if (dynamic_cast<AIConnection*>(getControllingClient()))
        {
            who = " ai";
            mIsAiControlled = true;
        }
        else if (getControllingClient())
        {
            who = " player";
            mIsAiControlled = false;
        }
        else
        {
            who = "";
            mIsAiControlled = false;
        }
    }
    U32 moveid = mLastMoveId + mTicksSinceLastMove;
    if (move)
        moveid = move->id;

    if (getType() & GameBaseHiFiObjectType)
    {
        if (move)
            Con::printf("Processing (%s%s id %i) move %i", srv, who, getId(), move->id);
        else
            Con::printf("Processing (%s%s id %i) move %i (%i)", srv, who, getId(), mLastMoveId + mTicksSinceLastMove, mTicksSinceLastMove);
    }

    if (move)
    {
        mLastMoveId = move->id;
        mTicksSinceLastMove = 0;
    }
#endif
}

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
void GameBase::processPhysicsTick(const Move *move, F32 dt)
{

}
#endif

void GameBase::interpolateTick(F32 delta)
{
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


//----------------------------------------------------------------------------
void GameBase::processAfter(GameBase* obj)
{
    mAfterObject = obj;
    if ((const GameBase*)obj->mAfterObject == this)
        obj->mAfterObject = 0;
    if (isGhost())
        getCurrentClientProcessList()->markDirty();
    else
        getCurrentServerProcessList()->markDirty();
}

void GameBase::clearProcessAfter()
{
    mAfterObject = 0;
}
//----------------------------------------------------------------------------

void GameBase::beginTickCacheList()
{
    // get ready iterate from oldest to newest entry
    if (mTickCacheHead)
        mTickCacheHead->next = mTickCacheHead->oldest;
    // if no head, that's ok, we'll just add entries as we go
}

TickCacheEntry* GameBase::incTickCacheList(bool addIfNeeded)
{
    // continue iterating through cache, returning current entry
    // we'll add new entries if need be
    TickCacheEntry * ret = NULL;
    if (mTickCacheHead && mTickCacheHead->next)
    {
        ret = mTickCacheHead->next;
        mTickCacheHead->next = mTickCacheHead->next->next;
    }
    else if (addIfNeeded)
    {
        addTickCacheEntry();
        ret = mTickCacheHead->newest;
    }
    return ret;
}

TickCacheEntry* GameBase::addTickCacheEntry()
{
    // Add a new entry, creating head if needed
    if (!mTickCacheHead)
    {
        mTickCacheHead = TickCacheHead::alloc();
        mTickCacheHead->newest = mTickCacheHead->oldest = mTickCacheHead->next = NULL;
        mTickCacheHead->numEntry = 0;
    }
    if (!mTickCacheHead->newest)
    {
        mTickCacheHead->newest = mTickCacheHead->oldest = TickCacheEntry::alloc();
    }
    else
    {
        mTickCacheHead->newest->next = TickCacheEntry::alloc();
        mTickCacheHead->newest = mTickCacheHead->newest->next;
    }
    mTickCacheHead->newest->next = NULL;
    mTickCacheHead->newest->move = NULL;
    mTickCacheHead->numEntry++;
    return mTickCacheHead->newest;
}

void GameBase::ageTickCache(S32 numToAge, S32 len)
{
    AssertFatal(mTickCacheHead,"No tick cache head");
    AssertFatal(mTickCacheHead->numEntry>=numToAge,"Too few entries!");
    AssertFatal(mTickCacheHead->numEntry>numToAge,"Too few entries!");

    while (numToAge--)
        dropOldest();
    while (mTickCacheHead->numEntry>len)
        dropNextOldest();
    while (mTickCacheHead->numEntry<len)
        addTickCacheEntry();
}

void GameBase::setTickCacheSize(int len)
{
    // grow cache to len size, adding to newest side of the list
    while (!mTickCacheHead || mTickCacheHead->numEntry < len)
        addTickCacheEntry();
    // shrink tick cache down to given size, popping off oldest entries first
    while (mTickCacheHead && mTickCacheHead->numEntry > len)
        dropOldest();
}

void GameBase::dropOldest()
{
    AssertFatal(mTickCacheHead->oldest,"Popping off too many tick cache entries");
    TickCacheEntry * oldest = mTickCacheHead->oldest;
    mTickCacheHead->oldest = oldest->next;
    if (oldest->move)
        TickCacheEntry::freeMove(oldest->move);
    TickCacheEntry::free(oldest);
    mTickCacheHead->numEntry--;
    if (mTickCacheHead->numEntry < 2)
        mTickCacheHead->newest = mTickCacheHead->oldest;
}

void GameBase::dropNextOldest()
{
    AssertFatal(mTickCacheHead->oldest && mTickCacheHead->numEntry>1,"Popping off too many tick cache entries");
    TickCacheEntry * oldest = mTickCacheHead->oldest;
    TickCacheEntry * nextoldest = mTickCacheHead->oldest->next;
    oldest->next = nextoldest->next;
    if (nextoldest->move)
        TickCacheEntry::freeMove(nextoldest->move);
    TickCacheEntry::free(nextoldest);
    mTickCacheHead->numEntry--;
    if (mTickCacheHead->numEntry==1)
        mTickCacheHead->newest = mTickCacheHead->oldest;
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

U32 GameBase::getPacketDataChecksum(GameConnection* connection)
{
   // just write the packet data into a buffer
   // then we can CRC the buffer.  This should always let us
   // know when there is a checksum problem.

    static U8 buffer[1500] = { 0, };
    BitStream stream(buffer, sizeof(buffer));

    writePacketData(connection, &stream);
    U32 byteCount = stream.getPosition();
    U32 ret = calculateCRC(buffer, byteCount, 0xFFFFFFFF);
    dMemset(buffer, 0, byteCount);
    return ret;
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
        if (stream->writeFlag(mNetFlags.test(NetOrdered)))
            stream->writeInt(mNetOrderId, 16);
    }
    stream->writeFlag(mHidden);

#ifdef TORQUE_DEBUG_NET_MOVES
    stream->write(mLastMoveId);
    stream->writeFlag(mIsAiControlled);
#endif

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


        if (stream->readFlag())
            mNetOrderId = stream->readInt(16);

        if (!Sim::findObject(id, dptr) || !setDataBlock(dptr))
            con->setLastError("Invalid packet GameBase::unpackUpdate()");
    }

    bool wasHidden = isHidden();
    setHidden(stream->readFlag());
    if (wasHidden && !isHidden())
        onUnhide();

#ifdef TORQUE_DEBUG_NET_MOVES
    stream->read(&mLastMoveId);
    mTicksSinceLastMove = 0;
    mIsAiControlled = stream->readFlag();
#endif
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
