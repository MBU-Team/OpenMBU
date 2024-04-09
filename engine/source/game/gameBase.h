//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMEBASE_H_
#define _GAMEBASE_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _PROCESSLIST_H_
#include "sim/processList.h"
#endif

struct TickCacheEntry;
struct TickCacheHead;
class NetConnection;
class ProcessList;
struct Move;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

/// Scriptable, demo-able datablock.
///
/// This variant of SimDataBlock performs these additional tasks:
///   - Linking datablock's namepsaces to the namespace of their C++ class, so
///     that datablocks can expose script functionality.
///   - Linking datablocks to a user defined scripting namespace, by setting the
///     className field at datablock definition time.
///   - Adds a category field; this is used by the world creator in the editor to
///     classify creatable shapes. Creatable shapes are placed under the Shapes
///     node in the treeview for this; additional levels are created, named after
///     the category fields.
///   - Adds support for demo stream recording. This support takes the form
///     of the member variable packed. When a demo is being recorded by a client,
///     data is unpacked, then packed again to the data stream, then, in the case
///     of datablocks, preload() is called to process the data. It is occasionally
///     the case that certain references in the datablock stream cannot be resolved
///     until preload is called, in which case a raw ID field is stored in the variable
///     which will eventually be used to store a pointer to the object. However, if
///     packData() is called before we resolve this ID, trying to call getID() on the
///     objecct ID would be a fatal error. Therefore, in these cases, we test packed;
///     if it is true, then we know we have to write the raw data, instead of trying
///     to resolve an ID.
///
/// @see SimDataBlock for further details about datablocks.
/// @see http://hosted.tribalwar.com/t2faq/datablocks.shtml for an excellent
///      explanation of the basics of datablocks from a scripting perspective.
/// @nosubgrouping
struct GameBaseData : public SimDataBlock {
private:
    typedef SimDataBlock Parent;

public:
    bool packed;
    StringTableEntry category;
    StringTableEntry className;

    bool onAdd();

    // The derived class should provide the following:
    DECLARE_CONOBJECT(GameBaseData);
    GameBaseData();
    static void initPersistFields();
    bool preload(bool server, char errorBuffer[256]);
    void unpackData(BitStream* stream);
};

DECLARE_CONSOLETYPE(GameBaseData)

//----------------------------------------------------------------------------
// A few utility methods for sending datablocks over the net
//----------------------------------------------------------------------------

bool UNPACK_DB_ID(BitStream*, U32& id);
bool PACK_DB_ID(BitStream*, U32 id);
bool PRELOAD_DB(U32& id, SimDataBlock**, bool server, const char* clientMissing = NULL, const char* serverMissing = NULL);

//----------------------------------------------------------------------------
class GameConnection;

// For truly it is written: "The wise man extends GameBase for his purposes,
// while the fool has the ability to eject shell casings from the belly of his
// dragon." -- KillerBunny

/// Base class for game objects which use datablocks, networking, are editable,
/// and need to process ticks.
///
/// @section GameBase_process GameBase and ProcessList
///
/// GameBase adds two kinds of time-based updates. Torque works off of a concept
/// of ticks. Ticks are slices of time 32 milliseconds in length. There are three
/// methods which are used to update GameBase objects that are registered with
/// the ProcessLists:
///      - processTick(Move*) is called on each object once for every tick, regardless
///        of the "real" framerate.
///      - interpolateTick(float) is called on client objects when they need to interpolate
///        to match the next tick.
///      - advanceTime(float) is called on client objects so they can do time-based behaviour,
///        like updating animations.
///
/// Torque maintains a server and a client processing list; in a local game, both
/// are populated, while in multiplayer situations, either one or the other is
/// populated.
///
/// You can control whether an object is considered for ticking by means of the
/// setProcessTick() method.
///
/// @section GameBase_datablock GameBase and Datablocks
///
/// GameBase adds support for datablocks. Datablocks are secondary classes which store
/// static data for types of game elements. For instance, this means that all "light human
/// male armor" type Players share the same datablock. Datablocks typically store not only
/// raw data, but perform precalculations, like finding nodes in the game model, or
/// validating movement parameters.
///
/// There are three parts to the datablock interface implemented in GameBase:
///      - <b>getDataBlock()</b>, which gets a pointer to the current datablock. This is
///        mostly for external use; for in-class use, it's better to directly access the
///        mDataBlock member.
///      - <b>setDataBlock()</b>, which sets mDataBlock to point to a new datablock; it
///        uses the next part of the interface to inform subclasses of this.
///      - <b>onNewDataBlock()</b> is called whenever a new datablock is assigned to a GameBase.
///
/// Datablocks are also usable through the scripting language.
///
/// @see SimDataBlock for more details.
///
/// @section GameBase_networking GameBase and Networking
///
/// writePacketData() and readPacketData() are called to transfer information needed for client
/// side prediction. They are usually used when updating a client of its control object state.
///
/// Subclasses of GameBase usually transmit positional and basic status data in the packUpdate()
/// functions, while giving velocity, momentum, and similar state information in the writePacketData().
///
/// writePacketData()/readPacketData() are called <i>in addition</i> to packUpdate/unpackUpdate().
///
/// @nosubgrouping
class GameBase : public SceneObject, public ProcessObject
{
private:
    typedef SceneObject Parent;
    friend class ProcessList;

    /// @name Datablock
    /// @{
private:
    GameBaseData* mDataBlock;
    StringTableEntry  mNameTag;
    TickCacheHead* mTickCacheHead;

    /// @}

    // Control interface
    GameConnection* mControllingClient;
    //GameBase* mControllingObject;

public:
    static bool gShowBoundingBox;    ///< Should we render bounding boxes?
protected:
    bool mProcessTick;
    F32 mCameraFov;

#ifdef TORQUE_DEBUG_NET_MOVES
    U32 mLastMoveId;
    U32 mTicksSinceLastMove;
    bool mIsAiControlled;
#endif   

public:
    GameBase();
    virtual ~GameBase();

    enum GameBaseMasks {
        InitialUpdateMask = Parent::NextFreeMask,
        DataBlockMask = InitialUpdateMask << 1,
        ExtendedInfoMask = DataBlockMask << 1,
        ControlMask = ExtendedInfoMask << 1,
        NextFreeMask = ControlMask << 1
    };

    // net flags added by game base
    enum
    {
        NetOrdered = BIT(Parent::MaxNetFlagBit + 1), // if set, process in same order on client and server
        NetNearbyAdded = BIT(Parent::MaxNetFlagBit + 2), // work flag -- set during client catchup when neighbors have been checked
        GhostUpdated = BIT(Parent::MaxNetFlagBit + 3), // set whenever ghost updated (and reset) on client -- for hifi objects
        TickLast = BIT(Parent::MaxNetFlagBit + 4), // if set, tick this object after all others (except other tick last objects)
        NewGhost = BIT(Parent::MaxNetFlagBit + 5), // if set, this ghost was just added during the last update
        HiFiPassive = BIT(Parent::MaxNetFlagBit + 6), // hifi passive objects don't interact with other hifi passive objects
        MaxNetFlagBit = Parent::MaxNetFlagBit + 6
    };

    /// @name Inherited Functionality.
    /// @{

    bool onAdd();
    void onRemove();
    void inspectPostApply();
    static void initPersistFields();
    static void consoleInit();
    /// @}

    virtual void onUnhide() {}

    ///@name Datablock
    ///@{

    /// Assigns this object a datablock and loads attributes with onNewDataBlock.
    ///
    /// @see onNewDataBlock
    /// @param   dptr   Datablock
    bool          setDataBlock(GameBaseData* dptr);

    /// Returns the datablock for this object.
    GameBaseData* getDataBlock() { return mDataBlock; }

    /// Called when a new datablock is set. This allows subclasses to
    /// appropriately handle new datablocks.
    ///
    /// @see    setDataBlock()
    /// @param  dptr   New datablock
    virtual bool  onNewDataBlock(GameBaseData* dptr);
    ///@}

    /// @name Script
    /// The scriptOnXX methods are invoked by the leaf classes
    /// @{

    /// Executes the 'onAdd' script function for this object.
    /// @note This must be called after everything is ready
    void scriptOnAdd();

    /// Executes the 'onNewDataBlock' script function for this object.
    ///
    /// @note This must be called after everything is loaded.
    void scriptOnNewDataBlock();

    /// Executes the 'onRemove' script function for this object.
    /// @note This must be called while the object is still valid
    void scriptOnRemove();
    /// @}

    /// @name Tick Processing
    /// @{

    /// Set the status of tick processing.
    ///
    /// If this is set to true, processTick will be called; if false,
    /// then it will be skipped.
    ///
    /// @see processTick
    /// @param   t   If true, tick processing is enabled.
    void setProcessTick(bool t) { mProcessTick = t; }

    /// Force this object to process after some other object.
    ///
    /// For example, a player mounted to a vehicle would want to process after the vehicle,
    /// to prevent a visible "lagging" from occuring when the vehicle motions, so the player
    /// would be set to processAfter(theVehicle);
    ///
    /// @param   obj   Object to process after
    void processAfter(GameBase* obj);

    /// Clears the effects of a call to processAfter()
    void clearProcessAfter();

    /// Returns the object that this processes after.
    ///
    /// @see processAfter
    GameBase* getProcessAfter() { return mAfterObject; }

    /// Removes this object from the tick-processing list
    void removeFromProcessList() { plUnlink(); }

    void beginTickCacheList();
    TickCacheEntry* incTickCacheList(bool addIfNeeded);
    TickCacheEntry* addTickCacheEntry();
    void ageTickCache(S32 numToAge, S32 len);
    void setTickCacheSize(int len);
    void dropOldest();
    void dropNextOldest();

    /// Processes a move event and updates object state once every 32 milliseconds.
    ///
    /// This takes place both on the client and server, every 32 milliseconds (1 tick).
    ///
    /// @see    ProcessList
    /// @param  move   Move event corresponding to this tick, or NULL.
    virtual void processTick(const Move* move);

#ifdef MB_CLIENT_PHYSICS_EVERY_FRAME
    virtual void processPhysicsTick(const Move* move, F32 dt);
#endif

    /// Interpolates between tick events.  This takes place on the CLIENT ONLY.
    ///
    /// @param   delta   Time since last call to interpolate
    virtual void interpolateTick(F32 delta);

    /// Advances simulation time for animations. This is called every frame.
    ///
    /// @param   dt   Time since last advance call
    virtual void advanceTime(F32 dt);

    /// Allow object a chance to tweak move before it is sent to client and server.
    virtual void preprocessMove(Move* move) {}
    /// @}

#ifdef TORQUE_HIFI_NET
    // tick cache methods for hifi networking...
    void setGhostUpdated(bool b) { if (b) mNetFlags.set(GhostUpdated); else mNetFlags.clear(GhostUpdated); }
    bool isGhostUpdated() const { return mNetFlags.test(GhostUpdated); }
    void setNewGhost(bool n) { if (n) mNetFlags.set(NewGhost); else mNetFlags.clear(NewGhost); }
    bool isNewGhost() { return mNetFlags.test(NewGhost); }
    virtual void computeNetSmooth(F32 backDelta) {}
#endif

    /// Returns the velocity of this object.
    virtual Point3F getVelocity() const;

#ifdef MARBLE_BLAST
    /// Gets the force of this object.
    /// Returns whether or not a force was applied
    ///
    ///  @param   pos   Position of the object to check against
    ///  @param   force The resulting force
    virtual bool getForce(Point3F& pos, Point3F* force) { return false; }
#endif

    /// @name Network
    /// @see NetObject, NetConnection
    /// @{

    F32  getUpdatePriority(CameraScopeQuery* focusObject, U32 updateMask, S32 updateSkips);
    U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream);

    /// Write state information necessary to perform client side prediction of an object.
    ///
    /// This information is sent only to the controling object. For example, if you are a client
    /// controlling a Player, the server uses writePacketData() instead of packUpdate() to
    /// generate the data you receive.
    ///
    /// @param   conn     Connection for which we're generating this data.
    /// @param   stream   Bitstream for output.
    virtual void writePacketData(GameConnection* conn, BitStream* stream);

    /// Read data written with writePacketData() and update the object state.
    ///
    /// @param   conn    Connection for which we're generating this data.
    /// @param   stream  Bitstream to read.
    virtual void readPacketData(GameConnection* conn, BitStream* stream);

    /// Gets the checksum for packet data.
    ///
    /// Basically writes a packet, does a CRC check on it, and returns
    /// that CRC.
    ///
    /// @see writePacketData
    /// @param   conn   Game connection
    virtual U32 getPacketDataChecksum(GameConnection* conn);
    ///@}

    /// @name User control
    /// @{

    /// Returns the client controling this object
    GameConnection* getControllingClient() { return mControllingClient; }

    /// Sets the client controling this object
    /// @param  client   Client that is now controling this object
    virtual void setControllingClient(GameConnection* client);
    /// @}
    DECLARE_CONOBJECT(GameBase);
};

#endif
