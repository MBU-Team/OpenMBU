//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMECONNECTION_H_
#define _GAMECONNECTION_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif
#ifndef _NETCONNECTION_H_
#include "sim/netConnection.h"
#endif
#ifndef _MOVEMANAGER_H_
#include "game/moveManager.h"
#endif
#ifndef _BITVECTOR_H_
#include "core/bitVector.h"
#endif

enum GameConnectionConstants
{
    MaxClients = 126,
    DataBlockQueueCount = 16
};

class SFXProfile;
class MatrixF;
class MatrixF;
class Point3F;
class MoveManager;
struct Move;
struct AuthInfo;

#define GameString TORQUE_GAME_NAME

class GameConnection : public NetConnection
{
private:
    typedef NetConnection Parent;
    friend class ProcessList;

    enum PrivateConstants {
        MoveCountBits = 5,
        /// MaxMoveCount should not exceed the MoveManager's
        /// own maximum (MaxMoveQueueSize)
        MaxMoveCount = 30,
    };
    typedef Vector<Move> MoveList;

    SimObjectPtr<ShapeBase> mControlObject;
    SimObjectPtr<ShapeBase> mCameraObject;
    U32 mDataBlockSequence;
    char mDisconnectReason[256];

    U32  mMissionCRC;             // crc of the current mission file from the server

private:
    U32 mLastControlRequestTime;
    S32 mDataBlockModifiedKey;
    S32 mMaxDataBlockModifiedKey;

    /// @name Client side first/third person
    /// @{

    ///
    bool  mFirstPerson;     ///< Are we currently first person or not.
    bool  mUpdateFirstPerson; ///< Set to notify client or server of first person change.
    bool  mUpdateCameraFov; ///< Set to notify server of camera FOV change.
    F32   mCameraFov;       ///< Current camera fov (in degrees).
    F32   mCameraPos;       ///< Current camera pos (0-1).
    F32   mCameraSpeed;     ///< Camera in/out speed.
    /// @}

    /// @name Move Packets
    /// Write/read move data to the packet.
    /// @{

    ///
    void moveWritePacket(BitStream* bstream);
    void moveReadPacket(BitStream* bstream);
    /// @}

public:

    /// @name Protocol Versions
    ///
    /// Protocol versions are used to indicated changes in network traffic.
    /// These could be changes in how any object transmits or processes
    /// network information. You can specify backwards compatibility by
    /// specifying a MinRequireProtocolVersion.  If the client
    /// protocol is >= this min value, the connection is accepted.
    ///
    /// Torque (V12) SDK 1.0 uses protocol  =  1
    ///
    /// Torque SDK 1.1 uses protocol = 2
    /// Torque SDK 1.4 uses protocol = 12
    /// @{
    static const U32 CurrentProtocolVersion;
    static const U32 MinRequiredProtocolVersion;
    /// @}

    /// Configuration
    enum Constants {
        BlockTypeMove = NetConnectionBlockTypeCount,
        GameConnectionBlockTypeCount,
        MaxConnectArgs = 16,
        DataBlocksDone = NumConnectionMessages,
        DataBlocksDownloadDone,
    };

    /// Set connection arguments; these are passed to the server when we connect.
    void setConnectArgs(U32 argc, const char** argv);

    /// Set the server password to use when we join.
    void setJoinPassword(const char* password);

    /// @name Event Handling
    /// @{

    virtual void onTimedOut();
    virtual void onConnectTimedOut();
    virtual void onDisconnect(const char* reason);
    virtual void onConnectionRejected(const char* reason);
    virtual void onConnectionEstablished(bool isInitiator);
    virtual void handleStartupError(const char* errorString);
    /// @}

    /// @name Packet I/O
    /// @{

    virtual void writeConnectRequest(BitStream* stream);
    virtual bool readConnectRequest(BitStream* stream, const char** errorString);
    virtual void writeConnectAccept(BitStream* stream);
    virtual bool readConnectAccept(BitStream* stream, const char** errorString);
    /// @}

    bool canRemoteCreate();

private:
    /// @name Connection State
    /// This data is set with setConnectArgs() and setJoinPassword(), and
    /// sent across the wire when we connect.
    /// @{

    U32      mConnectArgc;
    char* mConnectArgv[MaxConnectArgs];
    char* mJoinPassword;
    /// @}

protected:
    struct GamePacketNotify : public NetConnection::PacketNotify
    {
        S32 cameraFov;
        GamePacketNotify();
    };
    PacketNotify* allocNotify();

    U32 mLastMoveAck;
    U32 mLastClientMove;
    U32 mFirstMoveIndex;
    U32 mLastSentMove;
    bool mControlMismatch;
    bool mControlForceMismatch;
    F32 mAvgMoveQueueSize;
    U32 mTargetMoveListSize;
    U32 mMaxMoveListSize;
    F32 mSmoothMoveAvg;
    F32 mMoveListSizeSlack;
    U32 mTotalServerTicks;

    Vector<SimDataBlock*> mDataBlockLoadList;

    MoveList    mMoveList;
    void resetMoveList();
    
    // client side tracking of server ticks
    enum { TotalTicksBits = 10, TotalTicksMask = (1 << TotalTicksBits) - 1, ServerTicksUninitialized = 0xFFFFFFFF };

    S32 getServerTicks(U32 serverTickNum);
    bool serverTicksInitialized();
    void updateClientServerTickDiff(S32& tickDiff);

    bool        mAIControlled;
    AuthInfo* mAuthInfo;

    static S32  mLagThresholdMS;
    S32         mLastPacketTime;
    bool        mLagging;

    /// @name Flashing
    ////
    /// Note, these variables are not networked, they are for the local connection only.
    /// @{
    F32 mDamageFlash;
    F32 mWhiteOut;

    F32   mBlackOut;
    S32   mBlackOutTimeMS;
    S32   mBlackOutStartTimeMS;
    bool  mFadeToBlack;

    /// @}

    /// @name Packet I/O
    /// @{

    void readPacket(BitStream* bstream);
    void writePacket(BitStream* bstream, PacketNotify* note);
    void packetReceived(PacketNotify* note);
    void packetDropped(PacketNotify* note);
    void connectionError(const char* errorString);

    void writeDemoStartBlock(ResizeBitStream* stream);
    bool readDemoStartBlock(BitStream* stream);
    void handleRecordedBlock(U32 type, U32 size, void* data);
    /// @}
    ///
    void ghostReadExtra(NetObject*, BitStream*, bool newGhost);
    void ghostPreRead(NetObject*, bool newGhost);
    
public:

    DECLARE_CONOBJECT(GameConnection);
    void handleConnectionMessage(U32 message, U32 sequence, U32 ghostCount);
    void preloadDataBlock(SimDataBlock* block);
    void fileDownloadSegmentComplete();
    void preloadNextDataBlock(bool hadNew);
    static void consoleInit();

    void setDisconnectReason(const char* reason);
    GameConnection();
    ~GameConnection();

    U32 getDataBlockSequence() { return mDataBlockSequence; }
    void setDataBlockSequence(U32 seq) { mDataBlockSequence = seq; }

    bool onAdd();
    void onRemove();

    static GameConnection* getConnectionToServer() { return dynamic_cast<GameConnection*>((NetConnection*)mServerConnection); }
    static GameConnection* getLocalClientConnection() { return dynamic_cast<GameConnection*>((NetConnection*)mLocalClientConnection); }

    /// @name Control object
    /// @{

    ///
    void setControlObject(ShapeBase* co);
    ShapeBase* getControlObject() { return  mControlObject; }
    void setCameraObject(ShapeBase* co);
    ShapeBase* getCameraObject();
    bool getControlCameraTransform(F32 dt, MatrixF* mat);
    bool getControlCameraVelocity(Point3F* vel);

    bool getControlCameraFov(F32* fov);
    bool setControlCameraFov(F32 fov);
    bool isValidControlCameraFov(F32 fov);

    void setFirstPerson(bool firstPerson);

    /// @}

    void detectLag();

    /// @name Datablock management
    /// @{

    S32  getDataBlockModifiedKey() { return mDataBlockModifiedKey; }
    void setDataBlockModifiedKey(S32 key) { mDataBlockModifiedKey = key; }
    S32  getMaxDataBlockModifiedKey() { return mMaxDataBlockModifiedKey; }
    void setMaxDataBlockModifiedKey(S32 key) { mMaxDataBlockModifiedKey = key; }
    /// @}

    /// @name Fade control
    /// @{

    F32 getDamageFlash() { return mDamageFlash; }
    F32 getWhiteOut() { return mWhiteOut; }

    void setBlackOut(bool fadeToBlack, S32 timeMS);
    F32  getBlackOut();
    /// @}

    /// @name Move Management
    /// @{

    void           pushMove(const Move& mv);
    bool           getNextMove(Move& curMove);

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
    bool           getNextMove2(Move& curMove);
#endif
    bool           isBacklogged();
    virtual U32   getMoveList(Move**, U32* numMoves);
    MoveList&      getMoves() { return mMoveList; }
    void           resetClientMoves();
    virtual void   clearMoves(U32 count);
    void           collectMove(U32 simTime);
    virtual bool   areMovesPending();
    void           incLastSentMove();
    /// @}

    /// @name Authentication
    ///
    /// This is remnant code from Tribes 2.
    /// @{

    void            setAuthInfo(const AuthInfo* info);
    const AuthInfo* getAuthInfo();
    /// @}

    /// @name Sound
    /// @{

    void play2D(const SFXProfile* profile);
    void play3D(const SFXProfile* profile, const MatrixF* transform);
    /// @}

    /// @name Misc.
    /// @{

    bool isFirstPerson() { return mCameraPos == 0; }
    bool isAIControlled() { return mAIControlled; }

    void doneScopingScene();
    void demoPlaybackComplete();

    void setMissionCRC(U32 crc) { mMissionCRC = crc; }
    U32  getMissionCRC() { return(mMissionCRC); }
    /// @}
};

#endif
