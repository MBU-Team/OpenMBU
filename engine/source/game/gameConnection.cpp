//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "core/bitStream.h"
#include "sim/pathManager.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneLighting.h"
#include "sfx/sfxProfile.h"
#include "game/game.h"
#include "game/shapeBase.h"
#include "game/gameConnection.h"

#include "tickCache.h"
#include "game/gameConnectionEvents.h"
#include "game/gameProcess.h"
#include "game/auth.h"
#include "util/safeDelete.h"

//----------------------------------------------------------------------------
#define MAX_MOVE_PACKET_SENDS 4

#define ControlRequestTime 5000

const U32 GameConnection::CurrentProtocolVersion = TORQUE_PROTOCOL_VERSION;
const U32 GameConnection::MinRequiredProtocolVersion = TORQUE_PROTOCOL_VERSION;

//----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GameConnection);
S32 GameConnection::mLagThresholdMS = 0;

//----------------------------------------------------------------------------
GameConnection::GameConnection()
{
    mLagging = false;
    mControlObject = NULL;
    mCameraObject = NULL;

    mTotalServerTicks = ServerTicksUninitialized;
    mAvgMoveQueueSize = 3.0f;
    mSmoothMoveAvg = 0.15f;
    mMoveListSizeSlack = 1.0f;
    mLastMoveAck = 0;
    mLastClientMove = 0;
    mFirstMoveIndex = 0;
    mLastSentMove = 0;
#ifdef MB_DISABLE_INPUT_LAG
    mTargetMoveListSize = 0;
    mMaxMoveListSize = 1;
#else
    mTargetMoveListSize = 3;
    mMaxMoveListSize = 5;
#endif

    mDataBlockModifiedKey = 0;
    mMaxDataBlockModifiedKey = 0;
    mAuthInfo = NULL;
    mControlMismatch = false;
    mControlForceMismatch = false;
    mConnectArgc = 0;
    for (U32 i = 0; i < MaxConnectArgs; i++)
        mConnectArgv[i] = 0;

    mJoinPassword = NULL;

    mMissionCRC = 0xffffffff;

    mDamageFlash = mWhiteOut = 0;

    mCameraPos = 0;
    mCameraSpeed = 10;

    mCameraFov = 90.f;
    mUpdateCameraFov = false;

    mAIControlled = false;

    mDisconnectReason[0] = 0;

    //blackout vars
    mBlackOut = 0.0f;
    mBlackOutTimeMS = 0;
    mBlackOutStartTimeMS = 0;
    mFadeToBlack = false;

    // first person
    mFirstPerson = true;
    mUpdateFirstPerson = false;
}

GameConnection::~GameConnection()
{
    delete mAuthInfo;
    for (U32 i = 0; i < mConnectArgc; i++)
        dFree(mConnectArgv[i]);
    dFree(mJoinPassword);
}

//----------------------------------------------------------------------------

bool GameConnection::canRemoteCreate()
{
    return true;
}

void GameConnection::setConnectArgs(U32 argc, const char** argv)
{
    if (argc > MaxConnectArgs)
        argc = MaxConnectArgs;
    mConnectArgc = argc;
    for (U32 i = 0; i < mConnectArgc; i++)
        mConnectArgv[i] = dStrdup(argv[i]);
}

void GameConnection::setJoinPassword(const char* password)
{
    mJoinPassword = dStrdup(password);
}

ConsoleMethod(GameConnection, setJoinPassword, void, 3, 3, "")
{
    object->setJoinPassword(argv[2]);
}

ConsoleMethod(GameConnection, setConnectArgs, void, 3, 17, "")
{
    object->setConnectArgs(argc - 2, argv + 2);
}

void GameConnection::onTimedOut()
{
    if (isConnectionToServer())
    {
        Con::printf("Connection to server timed out");
        Con::executef(this, 1, "onConnectionTimedOut");
    }
    else
    {
        Con::printf("Client %d timed out.", getId());
        setDisconnectReason("TimedOut");
    }

}

void GameConnection::onConnectionEstablished(bool isInitiator)
{
    if (isInitiator)
    {
        setGhostFrom(false);
        setGhostTo(true);
        if (!isEstablished()) {
            setSendingEvents(true);
            setTranslatesStrings(true);
        }
        setIsConnectionToServer();
        mServerConnection = this;
        Con::printf("Connection established %d", getId());
        Con::executef(this, 1, "onConnectionAccepted");
    }
    else
    {
        setGhostFrom(true);
        setGhostTo(false);
        if (!isEstablished()) {
            setSendingEvents(true);
            setTranslatesStrings(true);
        }
        Sim::getClientGroup()->addObject(this);
        mTotalServerTicks = ServerTicksUninitialized;

        const char* argv[MaxConnectArgs + 2];
        argv[0] = "onConnect";
        for (U32 i = 0; i < mConnectArgc; i++)
            argv[i + 2] = mConnectArgv[i];
        Con::execute(this, mConnectArgc + 2, argv);
    }
}

void GameConnection::onConnectTimedOut()
{
    Con::executef(this, 1, "onConnectRequestTimedOut");
}

void GameConnection::onDisconnect(const char* reason)
{
    if (isConnectionToServer())
    {
        Con::printf("Connection with server lost.");
        Con::executef(this, 2, "onConnectionDropped", reason);
        mTotalServerTicks = ServerTicksUninitialized;
    }
    else
    {
        Con::printf("Client %d disconnected.", getId());
        setDisconnectReason(reason);
    }
}

void GameConnection::onConnectionRejected(const char* reason)
{
    Con::executef(this, 2, "onConnectRequestRejected", reason);
}

void GameConnection::handleStartupError(const char* errorString)
{
    Con::executef(this, 2, "onConnectRequestRejected", errorString);
}

void GameConnection::writeConnectAccept(BitStream* stream)
{
    Parent::writeConnectAccept(stream);
    stream->write(getProtocolVersion());
}

bool GameConnection::readConnectAccept(BitStream* stream, const char** errorString)
{
    if (!Parent::readConnectAccept(stream, errorString))
        return false;

    U32 protocolVersion;
    stream->read(&protocolVersion);
    if (protocolVersion < MinRequiredProtocolVersion || protocolVersion > CurrentProtocolVersion)
    {
        *errorString = "CHR_PROTOCOL"; // this should never happen unless someone is faking us out.
        return false;
    }
    return true;
}

void GameConnection::writeConnectRequest(BitStream* stream)
{
    Parent::writeConnectRequest(stream);
    stream->writeString(GameString);
    stream->write(CurrentProtocolVersion);
    stream->write(MinRequiredProtocolVersion);
    stream->writeString(mJoinPassword);

    stream->write(mConnectArgc);
    for (U32 i = 0; i < mConnectArgc; i++)
        stream->writeString(mConnectArgv[i]);
}

bool GameConnection::readConnectRequest(BitStream* stream, const char** errorString)
{
    if (!Parent::readConnectRequest(stream, errorString))
        return false;
    U32 currentProtocol, minProtocol;
    char gameString[256];
    stream->readString(gameString);
    if (dStrcmp(gameString, GameString))
    {
        *errorString = "CHR_GAME";
        return false;
    }

    stream->read(&currentProtocol);
    stream->read(&minProtocol);

    char joinPassword[256];
    stream->readString(joinPassword);

    if (currentProtocol < MinRequiredProtocolVersion)
    {
        *errorString = "CHR_PROTOCOL_LESS";
        return false;
    }
    if (minProtocol > CurrentProtocolVersion)
    {
        *errorString = "CHR_PROTOCOL_GREATER";
        return false;
    }
    setProtocolVersion(currentProtocol < CurrentProtocolVersion ? currentProtocol : CurrentProtocolVersion);

    const char* serverPassword = Con::getVariable("Pref::Server::Password");
    if (serverPassword[0])
    {
        if (dStrcmp(joinPassword, serverPassword))
        {
            *errorString = "CHR_PASSWORD";
            return false;
        }
    }

    stream->read(&mConnectArgc);
    if (mConnectArgc > MaxConnectArgs)
    {
        *errorString = "CR_INVALID_ARGS";
        return false;
    }
    const char* connectArgv[MaxConnectArgs + 3];
    for (U32 i = 0; i < mConnectArgc; i++)
    {
        char argString[256];
        stream->readString(argString);
        mConnectArgv[i] = dStrdup(argString);
        connectArgv[i + 3] = mConnectArgv[i];
    }
    connectArgv[0] = "onConnectRequest";
    char buffer[256];
    Net::addressToString(getNetAddress(), buffer);
    connectArgv[2] = buffer;

    const char* ret = Con::execute(this, mConnectArgc + 3, connectArgv);
    if (ret[0])
    {
        *errorString = ret;
        return false;
    }
    return true;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameConnection::connectionError(const char* errorString)
{
    if (isConnectionToServer())
    {
        Con::printf("Connection error: %s.", errorString);
        Con::executef(this, 2, "onConnectionError", errorString);
    }
    else
    {
        Con::printf("Client %d packet error: %s.", getId(), errorString);
        setDisconnectReason("Packet Error.");
    }
    deleteObject();
}

//----------------------------------------------------------------------------

void GameConnection::resetMoveList()
{
    mMoveList.clear();
    mLastMoveAck = 0;
    mLastClientMove = 0;
    mFirstMoveIndex = 0;
    mLastSentMove = 0;
}

S32 GameConnection::getServerTicks(U32 serverTickNum)
{
    S32 serverTicks = 0;
    if (serverTicksInitialized())
    {
        // handle tick wrapping...
        const S32 MaxTickCount = (1 << TotalTicksBits);
        const S32 HalfMaxTickCount = MaxTickCount >> 1;
        U32 prevTickNum = mTotalServerTicks & TotalTicksMask;
        serverTicks = serverTickNum - prevTickNum;
        if (serverTicks > HalfMaxTickCount)
            serverTicks -= MaxTickCount;
        else if (-serverTicks > HalfMaxTickCount)
            serverTicks += MaxTickCount;
        // TEMP: This assert keeps getting hit when using preview mode, disabling until we can find out why.
        //AssertFatal(serverTicks >= 0, "Server can't tick backwards!!!");
        if (serverTicks < 0)
            serverTicks = 0;
    }
    mTotalServerTicks = serverTickNum;
    return serverTicks;
}

bool GameConnection::serverTicksInitialized()
{
    return mTotalServerTicks != ServerTicksUninitialized;
}

void GameConnection::updateClientServerTickDiff(S32& tickDiff)
{
    if (mLastMoveAck == 0)
        tickDiff = 0;

    // Make adjustments to move list to account for tick mis-matches between client and server.
    if (tickDiff > 0)
    {
        // Server ticked more than client.  Adjust for this by reseting all hifi objects
        // to a later position in the tick cache (see ageTickCache below) and at the same
        // time pulling back some moves we thought we had made (so that time on client
        // doesn't change).
        S32 dropTicks = tickDiff;
        while (dropTicks)
        {
#ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("dropping move%s", mLastClientMove > mFirstMoveIndex ? "" : " but none there");
#endif
            if (mLastClientMove > mFirstMoveIndex)
                mLastClientMove--;
            else
                tickDiff--;
            dropTicks--;
        }
        AssertFatal(mLastClientMove >= mFirstMoveIndex, "Bad move request");
        AssertFatal(mLastClientMove - mFirstMoveIndex <= mMoveList.size(), "Desynched first and last move.");
    }
    else
    {
        // Client ticked more than server.  Adjust for this by taking extra moves
        // (either adding back moves that were dropped above, or taking new ones).
        for (S32 i = 0; i < -tickDiff; i++)
        {
            if (mMoveList.size() > mLastClientMove - mFirstMoveIndex)
            {
#ifdef TORQUE_DEBUG_NET_MOVES
                Con::printf("add back move");
#endif
                mLastClientMove++;
            }
            else
            {
#ifdef TORQUE_DEBUG_NET_MOVES
                Con::printf("add back move -- create one");
#endif
                collectMove(0);
                mLastClientMove++;
            }
        }
    }

    // drop moves that are not made yet (because we rolled them back) and not yet sent   
    U32 len = getMax(mLastClientMove - mFirstMoveIndex, mLastSentMove - mFirstMoveIndex);
    mMoveList.setSize(len);

#ifdef TORQUE_DEBUG_NET_MOVES
    Con::printf("move list size: %i, last move: %i, last sent: %i", mMoveList.size(), mLastClientMove - mFirstMoveIndex, mLastSentMove - mFirstMoveIndex);
#endif       
}

//----------------------------------------------------------------------------


void GameConnection::setAuthInfo(const AuthInfo* info)
{
    mAuthInfo = new AuthInfo;
    *mAuthInfo = *info;
}

const AuthInfo* GameConnection::getAuthInfo()
{
    return mAuthInfo;
}


//----------------------------------------------------------------------------

void GameConnection::setControlObject(ShapeBase* obj)
{
    if (mControlObject == obj)
        return;

    if (mControlObject && mControlObject != mCameraObject)
        mControlObject->setControllingClient(0);

    if (obj)
    {
        // Nothing else is permitted to control this object.
        if (ShapeBase* coo = obj->getControllingObject())
            coo->setControlObject(0);
        if (GameConnection* con = obj->getControllingClient())
        {
            if (this != con)
            {
                // was it controlled via camera or control
                if (con->getControlObject() == obj)
                    con->setControlObject(0);
                else
                    con->setCameraObject(0);
            }
        }

        // We are now the controlling client of this object.
        obj->setControllingClient(this);
    }

    // Okay, set our control object.
    mControlObject = obj;
    mControlForceMismatch = true;
    if (mCameraObject.isNull())
        setScopeObject(mControlObject);

    if (gSPMode)
        this->getConnectionToServer()->setControlObject(obj);
}

void GameConnection::setCameraObject(ShapeBase* obj)
{
    if (mCameraObject == obj)
        return;

    if (mCameraObject && mCameraObject != mControlObject)
        mCameraObject->setControllingClient(0);

    if (obj)
    {
        // nothing else is permitted to control this object
        if (ShapeBase* coo = obj->getControllingObject())
            coo->setControlObject(0);

        if (GameConnection* con = obj->getControllingClient())
        {
            if (this != con)
            {
                // was it controlled via camera or control
                if (con->getControlObject() == obj)
                    con->setControlObject(0);
                else
                    con->setCameraObject(0);
            }
        }

        // we are now the controlling client of this object
        obj->setControllingClient(this);
    }

    // Okay, set our camera object.
    mCameraObject = obj;

    if (mCameraObject.isNull())
        setScopeObject(mControlObject);
    else
    {
        setScopeObject(mCameraObject);

        // if this is a client then set the fov and active image
        if (isConnectionToServer())
        {
            F32 fov = mCameraObject->getDefaultCameraFov();
            GameSetCameraFov(fov);
        }
    }
}

ShapeBase* GameConnection::getCameraObject()
{
    // If there is no camera object, or if we're first person, return
    // the control object.
    if (!mControlObject.isNull() && (mCameraObject.isNull() || mFirstPerson))
        return mControlObject;
    return  mCameraObject;
}

static S32 sChaseQueueSize = 0;
static MatrixF* sChaseQueue = 0;
static S32 sChaseQueueHead = 0;
static S32 sChaseQueueTail = 0;

bool GameConnection::getControlCameraTransform(F32 dt, MatrixF* mat)
{
    ShapeBase* obj = getCameraObject();
    if (!obj)
        return false;

    ShapeBase* cObj = obj;
    while ((cObj = cObj->getControlObject()) != 0)
    {
        if (cObj->useObjsEyePoint())
            obj = cObj;
    }

    if (dt)
    {
        if (mFirstPerson || obj->onlyFirstPerson())
        {
            if (mCameraPos > 0)
                if ((mCameraPos -= mCameraSpeed * dt) <= 0)
                    mCameraPos = 0;
        }
        else
        {
            if (mCameraPos < 1)
                if ((mCameraPos += mCameraSpeed * dt) > 1)
                    mCameraPos = 1;
        }
    }

    if (!sChaseQueueSize || mFirstPerson || obj->onlyFirstPerson())
        obj->getCameraTransform(&mCameraPos, mat);
    else
    {
        MatrixF& hm = sChaseQueue[sChaseQueueHead];
        MatrixF& tm = sChaseQueue[sChaseQueueTail];
        obj->getCameraTransform(&mCameraPos, &hm);
        *mat = tm;
        if (dt)
        {
            if ((sChaseQueueHead += 1) >= sChaseQueueSize)
                sChaseQueueHead = 0;
            if (sChaseQueueHead == sChaseQueueTail)
                if ((sChaseQueueTail += 1) >= sChaseQueueSize)
                    sChaseQueueTail = 0;
        }
    }
    return true;
}

bool GameConnection::getControlCameraFov(F32* fov)
{
    //find the last control object in the chain (client->player->turret->whatever...)
    ShapeBase* obj = getCameraObject();
    ShapeBase* cObj = NULL;
    while (obj)
    {
        cObj = obj;
        obj = obj->getControlObject();
    }
    if (cObj)
    {
        *fov = cObj->getCameraFov();
        return(true);
    }

    return(false);
}

bool GameConnection::isValidControlCameraFov(F32 fov)
{
    //find the last control object in the chain (client->player->turret->whatever...)
    ShapeBase* obj = getCameraObject();
    ShapeBase* cObj = NULL;
    while (obj)
    {
        cObj = obj;
        obj = obj->getControlObject();
    }

    return cObj ? cObj->isValidCameraFov(fov) : NULL;
}

bool GameConnection::setControlCameraFov(F32 fov)
{
    //find the last control object in the chain (client->player->turret->whatever...)
    ShapeBase* obj = getCameraObject();
    ShapeBase* cObj = NULL;
    while (obj)
    {
        cObj = obj;
        obj = obj->getControlObject();
    }
    if (cObj)
    {
        // allow shapebase to clamp fov to its datablock values
        cObj->setCameraFov(mClampF(fov, MinCameraFov, MaxCameraFov));
        fov = cObj->getCameraFov();

        // server fov of client has 1degree resolution
        if (S32(fov) != S32(mCameraFov))
            mUpdateCameraFov = true;

        mCameraFov = fov;
        return(true);
    }
    return(false);
}

bool GameConnection::getControlCameraVelocity(Point3F* vel)
{
    if (ShapeBase* obj = getCameraObject()) {
        *vel = obj->getVelocity();
        return true;
    }
    return false;
}

void GameConnection::setFirstPerson(bool firstPerson)
{
    mFirstPerson = firstPerson;
    mUpdateFirstPerson = true;
}



//----------------------------------------------------------------------------

bool GameConnection::onAdd()
{
    if (!Parent::onAdd())
        return false;

    return true;
}

void GameConnection::onRemove()
{
    if (isNetworkConnection())
    {
        sendDisconnectPacket(mDisconnectReason);
    }
    else if (isLocalConnection() && isConnectionToServer())
    {
        // we're a client-side but local connection
        // delete the server side of the connection on our local server so that it updates 
        // clientgroup and what not (this is so that we can disconnect from a local server
        // without needing to destroy and recreate the server before we can connect to it 
        // again.  this capability was used in MBU)
        getRemoteConnection()->deleteObject();
        setRemoteConnectionObject(NULL);
    }
    if (!isConnectionToServer())
        Con::executef(this, 2, "onDrop", mDisconnectReason);

    if (mControlObject)
        mControlObject->setControllingClient(0);
    Parent::onRemove();
}

void GameConnection::setDisconnectReason(const char* str)
{
    dStrncpy(mDisconnectReason, str, sizeof(mDisconnectReason) - 1);
    mDisconnectReason[sizeof(mDisconnectReason) - 1] = 0;
}

//----------------------------------------------------------------------------

void GameConnection::handleRecordedBlock(U32 type, U32 size, void* data)
{
    switch (type)
    {
    case BlockTypeMove:
        pushMove(*((Move*)data));
        if (isRecording()) // put it back into the stream
            recordBlock(type, size, data);
        break;
    default:
        Parent::handleRecordedBlock(type, size, data);
        break;
    }
}

void GameConnection::writeDemoStartBlock(ResizeBitStream* stream)
{
    // write all the data blocks to the stream:

    for (SimObjectId i = DataBlockObjectIdFirst; i <= DataBlockObjectIdLast; i++)
    {
        SimDataBlock* data;
        if (Sim::findObject(i, data))
        {
            stream->writeFlag(true);
            SimDataBlockEvent evt(data);
            evt.pack(this, stream);
            stream->validate();
        }
    }
    stream->writeFlag(false);
    stream->write(mFirstPerson);
    stream->write(mCameraPos);
    stream->write(mCameraSpeed);
    stream->write(mLastMoveAck);
    stream->write(mLastClientMove);
    stream->write(mFirstMoveIndex);

    stream->writeString(Con::getVariable("$Client::MissionFile"));

    stream->write(U32(mMoveList.size()));
    for (U32 j = 0; j < mMoveList.size(); j++)
        mMoveList[j].pack(stream);

    // dump all the "demo" vars associated with this connection:
    SimFieldDictionaryIterator itr(getFieldDictionary());

    SimFieldDictionary::Entry* entry;
    while ((entry = *itr) != NULL)
    {
        if (!dStrnicmp(entry->slotName, "demo", 4))
        {
            stream->writeFlag(true);
            stream->writeString(entry->slotName + 4);
            stream->writeString(entry->value);
            stream->validate();
        }
        ++itr;
    }
    stream->writeFlag(false);
    Parent::writeDemoStartBlock(stream);

    stream->validate();

    // dump out the control object ghost id
    S32 idx = mControlObject ? getGhostIndex(mControlObject) : -1;
    stream->write(idx);
    if (mControlObject)
    {
#ifdef TORQUE_NET_STATS
        U32 beginPos = stream->getCurPos();
#endif
        mControlObject->writePacketData(this, stream);
#ifdef TORQUE_NET_STATS
        mControlObject->getClassRep()->updateNetStatWriteData(stream->getCurPos() - beginPos);
#endif
    }
    idx = mCameraObject ? getGhostIndex(mCameraObject) : -1;
    stream->write(idx);
    if (mCameraObject && mCameraObject != mControlObject)
    {
#ifdef TORQUE_NET_STATS
        U32 beginPos = stream->getCurPos();
#endif
        mCameraObject->writePacketData(this, stream);
#ifdef TORQUE_NET_STATS
        mCameraObject->getClassRep()->updateNetStatWriteData(stream->getCurPos() - beginPos);
#endif
    }
    mLastControlRequestTime = Platform::getVirtualMilliseconds();
}

bool GameConnection::readDemoStartBlock(BitStream* stream)
{
    while (stream->readFlag())
    {
        SimDataBlockEvent evt;
        evt.unpack(this, stream);
        evt.process(this);
    }

    while (mDataBlockLoadList.size())
    {
        preloadNextDataBlock(false);
        if (mErrorBuffer[0])
            return false;
    }

    stream->read(&mFirstPerson);
    stream->read(&mCameraPos);
    stream->read(&mCameraSpeed);
    stream->read(&mLastMoveAck);
    stream->read(&mLastClientMove);
    stream->read(&mFirstMoveIndex);

    char buf[256];
    stream->readString(buf);
    Con::setVariable("$Client::MissionFile", buf);

    U32 size;
    Move mv;
    stream->read(&size);
    mMoveList.clear();
    while (size--)
    {
        mv.unpack(stream);
        pushMove(mv);
    }

    // read in all the demo vars associated with this recording
    // they are all tagged on to the object and start with the
    // string "demo"

    while (stream->readFlag())
    {
        StringTableEntry slotName = StringTable->insert("demo");
        char array[256];
        char value[256];
        stream->readString(array);
        stream->readString(value);
        setDataField(slotName, array, value);
    }
    bool ret = Parent::readDemoStartBlock(stream);
    // grab the control object
    S32 idx;
    stream->read(&idx);

    ShapeBase* obj = 0;
    if (idx != -1)
    {
        obj = dynamic_cast<ShapeBase*>(resolveGhost(idx));
        setControlObject(obj);
        obj->readPacketData(this, stream);
    }

    // Get the camera object, and read it in if it's different
    S32 idx2;
    stream->read(&idx2);
    obj = 0;
    if (idx2 != -1 && idx2 != idx)
    {
        obj = dynamic_cast<ShapeBase*>(resolveGhost(idx2));
        setCameraObject(obj);
        obj->readPacketData(this, stream);
    }
    return ret;
}

void GameConnection::demoPlaybackComplete()
{
    static const char* demoPlaybackArgv[1] = { "demoPlaybackComplete" };
    Sim::postCurrentEvent(Sim::getRootGroup(), new SimConsoleEvent(1, demoPlaybackArgv, false));
    Parent::demoPlaybackComplete();
}

void GameConnection::ghostPreRead(NetObject* nobj, bool newGhost)
{
    if ((nobj->getType() & GameBaseHiFiObjectType) != 0 && !newGhost)
    {
        AssertFatal(dynamic_cast<GameBase*>(nobj),"Should be a gamebase");
        GameBase* obj = static_cast<GameBase*>(nobj);

        // set next cache entry to start
        obj->beginTickCacheList();

        // reset to old state because we are about to unpack (and then tick forward)
        TickCacheEntry* tce = obj->incTickCacheList(false);
        if (tce)
        {
            BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
            obj->readPacketData(this, &bs);
        }
    }
}

void GameConnection::ghostReadExtra(NetObject* nobj, BitStream* bstream, bool newGhost)
{
    if ((nobj->getType() & GameBaseHiFiObjectType) != 0)
    {
        AssertFatal(dynamic_cast<GameBase*>(nobj),"Should be a gamebase");
        GameBase* obj = static_cast<GameBase*>(nobj);

        // mark ghost so that it updates correctly
        obj->setGhostUpdated(true);
        obj->setNewGhost(newGhost);

        // set next cache entry to start
        obj->beginTickCacheList();

        // save state for future update
        TickCacheEntry* tce = obj->incTickCacheList(true);
        BitStream bs(tce->packetData, TickCacheEntry::MaxPacketSize);
        obj->writePacketData(this, &bs);
    }
}


//----------------------------------------------------------------------------

void GameConnection::readPacket(BitStream* bstream)
{
    char stringBuf[256];
    stringBuf[0] = 0;
    bstream->setStringBuffer(stringBuf);

    U32 totalCatchup = 0;

    bstream->clearCompressionPoint();
    if (isConnectionToServer())
    {
        if (!serverTicksInitialized())
            resetMoveList();

        bool spMode = bstream->readFlag();
        if (spMode != gSPMode)
        {
            gSPMode = spMode;
            Con::executef(this, 2, "switchedSinglePlayerMode", Con::getIntArg((S32)gSPMode));
        }

#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("pre move ack: %i", mLastMoveAck);
#endif
        
        mLastMoveAck = bstream->readInt(32);

#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("post move ack %i, first move %i, last move %i", mLastMoveAck, mFirstMoveIndex, mLastClientMove);
#endif

        // This is how many times we've ticked since last ack -- before adjustments below
        U32 ourTicks = mLastMoveAck - mFirstMoveIndex;

        if (mLastMoveAck < mFirstMoveIndex)
            mLastMoveAck = mFirstMoveIndex;

        if (mLastMoveAck > mLastClientMove)
        {
            ourTicks -= mLastMoveAck - mLastClientMove;
            mLastClientMove = mLastMoveAck;
        }
        while (mFirstMoveIndex < mLastMoveAck)
        {
            if (!mMoveList.empty())
            {
                mMoveList.pop_front();
                mFirstMoveIndex++;
            } else
            {
                AssertWarn(1, "Popping off too many moves!");
                mFirstMoveIndex = mLastMoveAck;
            }
        }

        U32 serverTickNum = bstream->readInt(TotalTicksBits);
        S32 serverTicks = getServerTicks(serverTickNum);
        S32 tickDiff = serverTicks - ourTicks;

#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("server ticks: %i, client ticks: %i, diff: %i%s", serverTicks, ourTicks, tickDiff, !tickDiff ? "" : " (ticks mis-match)");
#endif

        // Apply the first (of two) client-side synchronization mechanisms.  Key is that
        // we need to both synchronize client/server move streams (so first move in list is made
        // at same "time" on both client and server) and maintain the "time" at which the most
        // recent move was made on the server.  In both cases, "time" is the number of ticks
        // it took to get to that move.
        updateClientServerTickDiff(tickDiff);

        // Apply the second (and final) client-side synchronization mechanism.  The tickDiff adjustments above 
        // make sure time is preserved on client.  But that assumes that a future (or previous) update will adjust
        // time in the other direction, so that we don't get too far behind or ahead of the server.  The updateMoveSync
        // mechanism tracks us over time to make sure we eventually return to be in sync, and makes adjustments
        // if we don't after a certain time period (number of updates).  Unlike the tickDiff mechanism, when
        // the updateMoveSync acts time is not preserved on the client.
        getCurrentClientProcessList()->updateMoveSync(mLastSentMove - mLastClientMove);

        // set catchup parameters...
        totalCatchup = mLastClientMove - mFirstMoveIndex;

        getCurrentClientProcessList()->ageTickCache(ourTicks + (tickDiff > 0 ? tickDiff : 0), totalCatchup + 1);
        getCurrentClientProcessList()->forceHifiReset(tickDiff != 0);

        mDamageFlash = 0;
        mWhiteOut = 0;
        if (bstream->readFlag())
        {
            if (bstream->readFlag())
                mDamageFlash = bstream->readFloat(7);
            if (bstream->readFlag())
                mWhiteOut = bstream->readFloat(7) * 1.5;
        }

        if (bstream->readFlag())
        {
            if (bstream->readFlag())
            {
                // the control object is dirty...
                // so we get an update:
                mLastClientMove = mLastMoveAck;
                bool callScript = false;
                bool callScript2 = false;
                if (mControlObject.isNull())
                    callScript = true;

                S32 gIndex = bstream->readInt(NetConnection::GhostIdBitSize);
                ShapeBase* obj = static_cast<ShapeBase*>(resolveGhost(gIndex));
                if (mControlObject != obj)
                {
                    callScript2 = true;
                    setControlObject(obj);
                }
#ifdef TORQUE_NET_STATS
                U32 beginSize = bstream->getCurPos();
#endif
                obj->readPacketData(this, bstream);
                obj->setGhostUpdated(true);
                obj->beginTickCacheList();
                TickCacheEntry* tickCache = obj->incTickCacheList(true);
                BitStream bs(tickCache->packetData, TickCacheEntry::MaxPacketSize);
                obj->writePacketData(this, &bs);
#ifdef TORQUE_NET_STATS
                obj->getClassRep()->updateNetStatReadData(bstream->getCurPos() - beginSize);
#endif

                if (callScript)
                    Con::executef(this, 2, "initialControlSet");
                else if (callScript2)
                    Con::executef(this, 2, "controlObjectUpdated");
            }
            else
            {
                // read out the compression point
                Point3F pos;
                bstream->read(&pos.x);
                bstream->read(&pos.y);
                bstream->read(&pos.z);
                bstream->setCompressionPoint(pos);
            }
        }

        if (bstream->readFlag())
        {
            S32 gIndex = bstream->readInt(NetConnection::GhostIdBitSize);
            ShapeBase* obj = static_cast<ShapeBase*>(resolveGhost(gIndex));
            setCameraObject(obj);
            obj->readPacketData(this, bstream);
        }
        else
            setCameraObject(0);

        // server changed first person
        if (bstream->readFlag()) {
            setFirstPerson(bstream->readFlag());
            mUpdateFirstPerson = false;
        }

        // server forcing a fov change?
        if (bstream->readFlag())
        {
            S32 fov = bstream->readInt(8);
            setControlCameraFov(fov);

            // don't bother telling the server if we were able to set the fov
            F32 setFov;
            if (getControlCameraFov(&setFov) && (S32(setFov) == fov))
                mUpdateCameraFov = false;

            // update the games fov info
            GameSetCameraFov(fov);
        }
    }
    else
    {
        bool fp = bstream->readFlag();
        if (fp)
            mCameraPos = 0;
        else
            mCameraPos = 1;

        if (bstream->readFlag())
            mControlForceMismatch = true;
        
        moveReadPacket(bstream);

        if (mMoveList.size() > mMaxMoveListSize)
        {
            U32 drop = mMoveList.size() - mTargetMoveListSize;
            clearMoves(drop);
            mAvgMoveQueueSize = (F32)mTargetMoveListSize;

#ifdef TORQUE_DEBUG_NET_MOVES
            Con::printf("too many moves on server, dropping moves (%i)", drop);
#endif
        }

        // client changed first person
        if (bstream->readFlag()) {
            setFirstPerson(bstream->readFlag());
            mUpdateFirstPerson = false;
        }

        // check fov change.. 1degree granularity on server
        if (bstream->readFlag())
        {
            S32 fov = mClamp(bstream->readInt(8), S32(MinCameraFov), S32(MaxCameraFov));
            setControlCameraFov(fov);

            // may need to force client back to a valid fov
            F32 setFov;
            if (getControlCameraFov(&setFov) && (S32(setFov) == fov))
                mUpdateCameraFov = false;
        }
    }

    Parent::readPacket(bstream);
    bstream->clearCompressionPoint();
    bstream->setStringBuffer(NULL);

    if (isConnectionToServer())
    {
        if (mControlObject && mControlObject->isGhostUpdated())
            mLastClientMove = mLastMoveAck;

        PROFILE_START(ClientCatchup);
        getCurrentClientProcessList()->clientCatchup(this, totalCatchup);
        PROFILE_END();
    }
}

void GameConnection::writePacket(BitStream* bstream, PacketNotify* note)
{
    char stringBuf[256];
    bstream->clearCompressionPoint();
    stringBuf[0] = 0;
    bstream->setStringBuffer(stringBuf);

    GamePacketNotify* gnote = (GamePacketNotify*)note;

    U32 startPos = bstream->getCurPos();
    if (isConnectionToServer())
    {
        if (!serverTicksInitialized())
            resetMoveList();

        bstream->writeFlag(mCameraPos == 0);

        // if we're recording, we want to make sure that we get periodic updates of the
        // control object "just in case" - ie if the math copro is different between the
        // recording machine (SIMD vs FPU), we get periodic corrections

        bool forceUpdate = false;
        if (isRecording())
        {
            U32 currentTime = Platform::getVirtualMilliseconds();
            if (currentTime - mLastControlRequestTime > ControlRequestTime)
            {
                mLastControlRequestTime = currentTime;
                forceUpdate = true;
            }
        }
        bstream->writeFlag(forceUpdate);

        moveWritePacket(bstream);

        // first person changed?
        if (bstream->writeFlag(mUpdateFirstPerson))
        {
            bstream->writeFlag(mFirstPerson);
            mUpdateFirstPerson = false;
        }

        // camera fov changed? (server fov resolution is 1 degree)
        if (bstream->writeFlag(mUpdateCameraFov))
        {
            bstream->writeInt(mClamp(S32(mCameraFov), S32(MinCameraFov), S32(MaxCameraFov)), 8);
            mUpdateCameraFov = false;
        }
        DEBUG_LOG(("PKLOG %d CLIENTMOVES: %d", getId(), bstream->getCurPos() - startPos));
    }
    else
    {
        bstream->writeFlag(gSPMode);

        // The only time mMoveList will not be empty at this
        // point is during a change in control object.

#ifdef TORQUE_DEBUG_NET_MOVES
        Con::printf("ack %i minus %i", mLastMoveAck, mMoveList.size());
#endif

        bstream->writeInt(mLastMoveAck - mMoveList.size(), 32);

        bstream->writeInt(getCurrentServerProcessList()->getTotalTicks() & TotalTicksMask, TotalTicksBits);

        // get the ghost index of the control object, and write out
        // all the damage flash & white out

        S32 gIndex = -1;
        if (!mControlObject.isNull())
        {
            gIndex = getGhostIndex(mControlObject);

            F32 flash = mControlObject->getDamageFlash();
            F32 whiteOut = mControlObject->getWhiteOut();
            if (bstream->writeFlag(flash != 0 || whiteOut != 0))
            {
                if (bstream->writeFlag(flash != 0))
                    bstream->writeFloat(flash, 7);
                if (bstream->writeFlag(whiteOut != 0))
                    bstream->writeFloat(whiteOut / 1.5, 7);
            }
        }
        else
            bstream->writeFlag(false);

        if (bstream->writeFlag(gIndex != -1))
        {
            // assume that the control object will write in a compression point
            if (bstream->writeFlag(mControlMismatch || mControlForceMismatch))
            {
#ifdef TORQUE_DEBUG_NET
                if (mControlMismatch)
                    Con::printf("packetDataChecksum disagree!");
                else
                    Con::printf("packetDataChecksum disagree! (force)");
#endif
                bstream->writeInt(gIndex, NetConnection::GhostIdBitSize);
#ifdef TORQUE_NET_STATS
                U32 beginSize = bstream->getCurPos();
#endif
                mControlObject->writePacketData(this, bstream);
#ifdef TORQUE_NET_STATS
                mControlObject->getClassRep()->updateNetStatWriteData(bstream->getCurPos() - beginSize);
#endif
                mControlForceMismatch = false;
            }
            else
            {
                // we'll have to use the control object's position as the compression point
                // should make this lower res for better space usage:
                Point3F coPos = mControlObject->getPosition();
                bstream->write(coPos.x);
                bstream->write(coPos.y);
                bstream->write(coPos.z);
                bstream->setCompressionPoint(coPos);
            }
        }
        DEBUG_LOG(("PKLOG %d CONTROLOBJECTSTATE: %d", getId(), bstream->getCurPos() - startPos));
        startPos = bstream->getCurPos();

        if (!mCameraObject.isNull() && mCameraObject != mControlObject)
        {
            gIndex = getGhostIndex(mCameraObject);
            if (bstream->writeFlag(gIndex != -1))
            {
                bstream->writeInt(gIndex, NetConnection::GhostIdBitSize);
                mCameraObject->writePacketData(this, bstream);
            }
        }
        else
            bstream->writeFlag(false);

        // first person changed?
        if (bstream->writeFlag(mUpdateFirstPerson)) {
            bstream->writeFlag(mFirstPerson);
            mUpdateFirstPerson = false;
        }

        // server forcing client fov?
        gnote->cameraFov = -1;
        if (bstream->writeFlag(mUpdateCameraFov))
        {
            gnote->cameraFov = mClamp(S32(mCameraFov), S32(MinCameraFov), S32(MaxCameraFov));
            bstream->writeInt(gnote->cameraFov, 8);
            mUpdateCameraFov = false;
        }
        DEBUG_LOG(("PKLOG %d PINGCAMSTATE: %d", getId(), bstream->getCurPos() - startPos));
    }

    Parent::writePacket(bstream, note);
    bstream->clearCompressionPoint();
    bstream->setStringBuffer(NULL);
}


void GameConnection::detectLag()
{
    //see if we're lagging...
    S32 curTime = Sim::getCurrentTime();
    if (curTime - mLastPacketTime > mLagThresholdMS)
    {
        if (!mLagging)
        {
            mLagging = true;
            Con::executef(this, 2, "setLagIcon", "true");
        }
    }
    else if (mLagging)
    {
        mLagging = false;
        Con::executef(this, 2, "setLagIcon", "false");
    }
}

GameConnection::GamePacketNotify::GamePacketNotify()
{
    // need to fill in empty notifes for demo start block
    cameraFov = 0;
}

NetConnection::PacketNotify* GameConnection::allocNotify()
{
    return new GamePacketNotify;
}

void GameConnection::packetReceived(PacketNotify* note)
{
    //record the time so we can tell if we're lagging...
    mLastPacketTime = Sim::getCurrentTime();
    GamePacketNotify* gnote = (GamePacketNotify*)note;

    Parent::packetReceived(note);
}

void GameConnection::packetDropped(PacketNotify* note)
{
    Parent::packetDropped(note);
    GamePacketNotify* gnote = (GamePacketNotify*)note;
    if (gnote->cameraFov != -1)
        mUpdateCameraFov = true;
}

//----------------------------------------------------------------------------

void GameConnection::play2D(const SFXProfile* profile)
{
    postNetEvent(new Sim2DAudioEvent(profile));
}

void GameConnection::play3D(const SFXProfile* profile, const MatrixF* transform)
{
    if (!transform)
        play2D(profile);

    else if (!mControlObject)
        postNetEvent(new Sim3DAudioEvent(profile, transform));

    else
    {
        // TODO: Maybe improve this to account for the duration
        // of the sound effect and if the control object can get
        // into hearing range within time?

        // Only post the event if it's within audible range
        // of the control object.
        Point3F ear, pos;
        transform->getColumn(3, &pos);
        mControlObject->getTransform().getColumn(3, &ear);
        if ((ear - pos).len() < profile->getDescription()->mMaxDistance)
            postNetEvent(new Sim3DAudioEvent(profile, transform));
    }
}

void GameConnection::doneScopingScene()
{
    // Could add special post-scene scoping here, such as scoping
    // objects not visible to the camera, but visible to sensors.
}

void GameConnection::preloadDataBlock(SimDataBlock* db)
{
    mDataBlockLoadList.push_back(db);
    if (mDataBlockLoadList.size() == 1)
        preloadNextDataBlock(true);
}

void GameConnection::fileDownloadSegmentComplete()
{
    // this is called when a the file list has finished processing...
    // at this point we can try again to add the object
    // subclasses can override this to do, for example, datablock redos.
    if (mDataBlockLoadList.size())
        preloadNextDataBlock(mNumDownloadedFiles != 0);
    Parent::fileDownloadSegmentComplete();
}

void GameConnection::preloadNextDataBlock(bool hadNewFiles)
{
    if (!mDataBlockLoadList.size())
        return;
    while (mDataBlockLoadList.size())
    {
        // only check for new files if this is the first load, or if new
        // files were downloaded from the server.
        if (hadNewFiles)
            ResourceManager->setMissingFileLogging(true);
        ResourceManager->clearMissingFileList();
        SimDataBlock* object = mDataBlockLoadList[0];
        if (!object)
        {
            // a null object is used to signify that the last ghost in the list is down
            mDataBlockLoadList.pop_front();
            AssertFatal(mDataBlockLoadList.size() == 0, "Error! Datablock save list should be empty!");
            sendConnectionMessage(DataBlocksDownloadDone, mDataBlockSequence);

            ResourceManager->setMissingFileLogging(false);
            return;
        }
        mFilesWereDownloaded = hadNewFiles;
        if (!object->preload(false, mErrorBuffer))
        {
            mFilesWereDownloaded = false;
            // make sure there's an error message if necessary
            if (!mErrorBuffer[0])
                setLastError("Invalid packet.");

            // if there were no new files, make sure the error message
            // is the one from the last time we tried to add this object
            if (!hadNewFiles)
            {
                //dStrcpy(mErrorBuffer, mLastFileErrorBuffer);
                ResourceManager->setMissingFileLogging(false);
                return;
            }
            // object failed to load, let's see if it had any missing files

            if (!ResourceManager->getMissingFileList(mMissingFileList))
            {
                // no missing files, must be an error
                // connection will automagically delete the ghost always list
                // when this error is reported.
                ResourceManager->setMissingFileLogging(false);
                return;
            }

            // ok, copy the error buffer out to a scratch pad for now
            //dStrcpy(mLastFileErrorBuffer, mErrorBuffer);
            mErrorBuffer[0] = 0;

            // request the missing files...
            mNumDownloadedFiles = 0;
            sendNextFileDownloadRequest();
            break;
        }
        mFilesWereDownloaded = false;
        ResourceManager->setMissingFileLogging(false);
        mDataBlockLoadList.pop_front();
        hadNewFiles = true;
    }
}


//----------------------------------------------------------------------------
//localconnection only blackout functions
void GameConnection::setBlackOut(bool fadeToBlack, S32 timeMS)
{
    mFadeToBlack = fadeToBlack;
    mBlackOutStartTimeMS = Sim::getCurrentTime();
    mBlackOutTimeMS = timeMS;

    //if timeMS <= 0 set the value instantly
    if (mBlackOutTimeMS <= 0)
        mBlackOut = (mFadeToBlack ? 1.0f : 0.0f);
}

F32 GameConnection::getBlackOut()
{
    S32 curTime = Sim::getCurrentTime();

    //see if we're in the middle of a black out
    if (curTime < mBlackOutStartTimeMS + mBlackOutTimeMS)
    {
        S32 elapsedTime = curTime - mBlackOutStartTimeMS;
        F32 timePercent = F32(elapsedTime) / F32(mBlackOutTimeMS);
        mBlackOut = (mFadeToBlack ? timePercent : 1.0f - timePercent);
    }
    else
        mBlackOut = (mFadeToBlack ? 1.0f : 0.0f);

    //return the blackout time
    return mBlackOut;
}

void GameConnection::handleConnectionMessage(U32 message, U32 sequence, U32 ghostCount)
{
    if (isConnectionToServer())
    {
        if (message == DataBlocksDone)
        {
            mDataBlockLoadList.push_back(NULL);
            mDataBlockSequence = sequence;
            if (mDataBlockLoadList.size() == 1)
                preloadNextDataBlock(true);
        }
    }
    else
    {
        if (message == DataBlocksDownloadDone)
        {
            if (getDataBlockSequence() == sequence)
                Con::executef(this, 2, "onDataBlocksDone", Con::getIntArg(getDataBlockSequence()));
        }
    }
    Parent::handleConnectionMessage(message, sequence, ghostCount);
}

//----------------------------------------------------------------------------

ConsoleMethod(GameConnection, transmitDataBlocks, void, 3, 3, "(int sequence)")
{
    object->setDataBlockSequence(dAtoi(argv[2]));
    SimDataBlockGroup* g = Sim::getDataBlockGroup();

    // find the first one we haven't sent:
    U32 i, groupCount = g->size();
    S32 key = object->getDataBlockModifiedKey();
    for (i = 0; i < groupCount; i++)
        if (((SimDataBlock*)(*g)[i])->getModifiedKey() > key)
            break;
    if (i == groupCount) {
        object->sendConnectionMessage(GameConnection::DataBlocksDone, object->getDataBlockSequence());
        return;
    }
    object->setMaxDataBlockModifiedKey(key);

    // Ship the rest off...
    U32 max = getMin(i + DataBlockQueueCount, groupCount);
    for (; i < max; i++) {
        SimDataBlock* data = (SimDataBlock*)(*g)[i];
        object->postNetEvent(new SimDataBlockEvent(data, i, groupCount, object->getDataBlockSequence()));
    }
}

ConsoleMethod(GameConnection, activateGhosting, void, 2, 2, "")
{
    object->activateGhosting();
}

ConsoleMethod(GameConnection, resetGhosting, void, 2, 2, "")
{
    object->resetGhosting();
}

ConsoleMethod(GameConnection, setControlObject, bool, 3, 3, "(ShapeBase object)")
{
    ShapeBase* gb;
    if (!Sim::findObject(argv[2], gb))
        return false;

    object->setControlObject(gb);
    return true;
}

ConsoleMethod(GameConnection, getControlObject, S32, 2, 2, "")
{
    argv;
    SimObject* cp = object->getControlObject();
    return cp ? cp->getId() : 0;
}

ConsoleMethod(GameConnection, isAIControlled, bool, 2, 2, "")
{
    return object->isAIControlled();
}

ConsoleMethod(GameConnection, play2D, bool, 3, 3, "(SFXProfile ap)")
{
    SFXProfile* profile;
    if (!Sim::findObject(argv[2], profile))
        return false;
    object->play2D(profile);
    return true;
}

ConsoleMethod(GameConnection, play3D, bool, 4, 4, "(SFXProfile ap, Transform pos)")
{
    SFXProfile* profile;
    if (!Sim::findObject(argv[2], profile))
        return false;

    Point3F pos(0, 0, 0);
    AngAxisF aa;
    aa.axis.set(0, 0, 1);
    aa.angle = 0;
    dSscanf(argv[3], "%g %g %g %g %g %g %g",
        &pos.x, &pos.y, &pos.z, &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);
    MatrixF mat;
    aa.setMatrix(&mat);
    mat.setColumn(3, pos);

    object->play3D(profile, &mat);
    return true;
}

ConsoleMethod(GameConnection, chaseCam, bool, 3, 3, "(int size)")
{
    S32 size = dAtoi(argv[2]);
    if (size != sChaseQueueSize)
    {
        SAFE_DELETE_ARRAY(sChaseQueue);

        sChaseQueueSize = size;
        sChaseQueueHead = sChaseQueueTail = 0;

        if (size)
        {
            sChaseQueue = new MatrixF[size];
            return true;
        }
    }
    return false;
}

ConsoleMethod(GameConnection, setControlCameraFov, void, 3, 3, "(int newFOV)"
    "Set new FOV in degrees.")
{
    object->setControlCameraFov(dAtoi(argv[2]));
}

ConsoleMethod(GameConnection, getControlCameraFov, F32, 2, 2, "")
{
    F32 fov = 0.f;
    if (!object->getControlCameraFov(&fov))
        return(0.f);
    return(fov);
}

ConsoleMethod(GameConnection, setBlackOut, void, 4, 4, "(bool doFade, int timeMS)")
{
    object->setBlackOut(dAtob(argv[2]), dAtoi(argv[3]));
}

ConsoleMethod(GameConnection, setMissionCRC, void, 3, 3, "(int CRC)")
{
    if (object->isConnectionToServer())
        return;

    object->postNetEvent(new SetMissionCRCEvent(dAtoi(argv[2])));
}

ConsoleMethod(GameConnection, delete, void, 2, 3, "(string reason=NULL) Disconnect a client; reason is sent as part of the disconnect packet.")
{
    if (argc == 3)
        object->setDisconnectReason(argv[2]);
    object->deleteObject();
}


//--------------------------------------------------------------------------
void GameConnection::consoleInit()
{
    Con::addVariable("Pref::Net::LagThreshold", TypeS32, &mLagThresholdMS);
    Con::addVariable("specialFog", TypeBool, &SceneGraph::useSpecial);
}

ConsoleMethod(GameConnection, startRecording, void, 3, 3, "(string fileName)records the network connection to a demo file.")
{
    char fileName[1024];
    Con::expandScriptFilename(fileName, sizeof(fileName), argv[2]);
    object->startDemoRecord(fileName);
}

ConsoleMethod(GameConnection, stopRecording, void, 2, 2, "()stops the demo recording.")
{
    object->stopRecording();
}

ConsoleMethod(GameConnection, playDemo, bool, 3, 3, "(string demoFileName)plays a previously recorded demo.")
{
    char filename[1024];
    Con::expandScriptFilename(filename, sizeof(filename), argv[2]);

    // Note that calling onConnectionEstablished will change the values in argv!
    object->onConnectionEstablished(true);
    object->setEstablished();

    if (!object->replayDemoRecord(filename))
    {
        Con::printf("Unable to open demo file %s.", filename);
        object->deleteObject();
        return false;
    }

    // After demo has loaded, execute the scene re-light the scene
    SceneLighting::lightScene(0, 0);

    return true;
}

ConsoleMethod(GameConnection, isDemoPlaying, bool, 2, 2, "isDemoPlaying();")
{
    argc;
    argv;
    return object->isPlayingBack();
}

ConsoleMethod(GameConnection, isDemoRecording, bool, 2, 2, "()")
{
    argc;
    argv;
    return object->isRecording();
}

ConsoleMethod(GameConnection, listClassIDs, void, 2, 2, "() List all of the "
    "classes that this connection knows about, and what their IDs "
    "are. Useful for debugging network problems.")
{
    Con::printf("--------------- Class ID Listing ----------------");
    Con::printf(" id    |   name");

    for (AbstractClassRep* rep = AbstractClassRep::getClassList();
        rep;
        rep = rep->getNextClass())
    {
        ConsoleObject* obj = rep->create();
        if (obj && rep->getClassId(object->getNetClassGroup()) >= 0)
            Con::printf("%7.7d| %s", rep->getClassId(object->getNetClassGroup()), rep->getClassName());
        delete obj;
    }
}

ConsoleStaticMethod(GameConnection, getServerConnection, S32, 2, 2, "() Get the server connection if any.")
{
    if (GameConnection::getConnectionToServer())
        return GameConnection::getConnectionToServer()->getId();
    else
    {
        Con::errorf("GameConnection::getServerConnection - no connection available.");
        return -1;
    }
}

ConsoleMethod(GameConnection, setCameraObject, S32, 3, 3, "")
{
    ShapeBase* obj;
    if (!Sim::findObject(argv[2], obj))
        return false;

    object->setCameraObject(obj);
    return true;
}

ConsoleMethod(GameConnection, getCameraObject, S32, 2, 2, "")
{
    SimObject* obj = object->getCameraObject();
    return obj ? obj->getId() : 0;
}

ConsoleMethod(GameConnection, clearCameraObject, void, 2, 2, "")
{
    object->setCameraObject(NULL);
}

ConsoleMethod(GameConnection, isFirstPerson, bool, 2, 2, "() True if this connection is in first person mode.")
{
    // Note: Transition to first person occurs over time via mCameraPos, so this
    // won't immediately return true after a set.
    return object->isFirstPerson();
}

ConsoleMethod(GameConnection, setFirstPerson, void, 3, 3, "(bool firstPerson) Sets this connection into or out of first person mode.")
{
    object->setFirstPerson(dAtob(argv[2]));
}
