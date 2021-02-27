//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _APP_GAMEPROCESS_H_
#define _APP_GAMEPROCESS_H_

#include "platform/platform.h"
#include "sim/processList.h"

#include "game/game.h"

class GameBase;
class GameConnection;
struct Move;

//----------------------------------------------------------------------------

/// List to keep track of GameBases to process.
class ClientProcessList : public ProcessList
{
    typedef ProcessList Parent;

#ifdef TORQUE_HIFI
protected:
    U32 mSkipAdvanceObjectsMs;
private:
    bool mForceHifiReset;
    U32 mCatchup;
#endif

protected:

    void onTickObject(ProcessObject*);
    void advanceObjects();
    void onAdvanceObjects();
    bool doBacklogged(SimTime timeDelta);
    GameBase* getGameBase(ProcessObject* obj);


public:
    ClientProcessList();

    void addObject(ProcessObject* obj);

    bool advanceTime(SimTime timeDelta);

    /// @}
    // after update from server, catch back up to where we were
    void clientCatchup(GameConnection*);

#ifdef TORQUE_HIFI
    void setCatchup(U32 catchup) { mCatchup = catchup; }

    // tick cache functions -- client only
    void ageTickCache(S32 numToAge, S32 len);
    void forceHifiReset(bool reset) { mForceHifiReset = reset; }
    U32 getTotalTicks() { return mTotalTicks; }
    void updateMoveSync(S32 moveDiff);
    void skipAdvanceObjects(U32 ms) { mSkipAdvanceObjectsMs += ms; }
#endif
};

class ServerProcessList : public ProcessList
{
    typedef ProcessList Parent;

protected:

    void onTickObject(ProcessObject*);
    void advanceObjects();
    GameBase* getGameBase(ProcessObject* obj);

public:
    ServerProcessList();

    void addObject(ProcessObject* obj);
};

class SPModeProcessList : public ClientProcessList
{
    typedef ClientProcessList Parent;

protected:

    void onTickObject(ProcessObject*);
    GameBase* getGameBase(ProcessObject* obj);

public:
    SPModeProcessList();

    bool advanceTime(SimTime timeDelta);
};

extern ClientProcessList gClientProcessList;
extern ServerProcessList gServerProcessList;
extern SPModeProcessList gSPModeProcessList;

inline ProcessList* getCurrentServerProcessList()
{
    if (gSPMode)
        return &gSPModeProcessList;

    return &gServerProcessList;
}

inline ClientProcessList* getCurrentClientProcessList()
{
    if (gSPMode)
        return &gSPModeProcessList;

    return &gClientProcessList;
}

#endif