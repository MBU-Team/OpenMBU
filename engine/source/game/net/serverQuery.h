//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SERVERQUERY_H_
#define _SERVERQUERY_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _EVENT_H_
#include "platform/event.h"
#endif
#ifndef _BITSET_H_
#include "core/bitSet.h"
#endif

#include "sim/netConnection.h"
#include "sim/netInterface.h"
//-----------------------------------------------------------------------------
// Master server information

class DemoNetInterface : public NetInterface
{
public:
    void handleInfoPacket(const NetAddress* address, U8 packetType, BitStream* stream);
};

struct MasterInfo
{
    NetAddress address;
    U32 region;
};


//-----------------------------------------------------------------------------
// Game Server Information

struct ServerInfo
{
    enum StatusFlags
    {
        // Info flags (0-7):
        Status_Dedicated = BIT(0),
        Status_Passworded = BIT(1),
        Status_Linux = BIT(2),
        Status_Private = BIT(3),

        // Status flags:
        Status_New = 0,
        Status_Querying = BIT(28),
        Status_Updating = BIT(29),
        Status_Responded = BIT(30),
        Status_TimedOut = BIT(31),
    };

    U8          numPlayers;
    U8          maxPlayers;
    U8          numBots;
    char* name;
    char* gameType;
    char* missionName;
    char* missionType;
    char* statusString;
    char* infoString;
    NetAddress  address;
    U32         version;
    U32         ping;
    U32         cpuSpeed;
    bool        isFavorite;
    BitSet32    status;
    bool        isLocal;

    ServerInfo()
    {
        numPlayers = 0;
        maxPlayers = 0;
        numBots = 0;
        name = NULL;
        gameType = NULL;
        missionType = NULL;
        missionName = NULL;
        statusString = NULL;
        infoString = NULL;
        version = 0;
        ping = 0;
        cpuSpeed = 0;
        isFavorite = false;
        status = Status_New;
        isLocal = false;
    }
    ~ServerInfo();

    bool isNew() { return(status == Status_New); }
    bool isQuerying() { return(status.test(Status_Querying)); }
    bool isUpdating() { return(status.test(Status_Updating)); }
    bool hasResponded() { return(status.test(Status_Responded)); }
    bool isTimedOut() { return(status.test(Status_TimedOut)); }

    bool isDedicated() { return(status.test(Status_Dedicated)); }
    bool isPassworded() { return(status.test(Status_Passworded)); }
    bool isLinux() { return(status.test(Status_Linux)); }
};


//-----------------------------------------------------------------------------

extern Vector<ServerInfo> gServerList;
extern bool gServerBrowserDirty;
extern NetConnection* relayNetConnection;
extern void clearServerList(bool clearServerInfo = true);
extern void queryLanServers(U32 port, U8 flags, const char* gameType, const char* missionType,
    U8 minPlayers, U8 maxPlayers, U8 maxBots, U32 regionMask, U32 maxPing, U16 minCPU,
    U8 filterFlags, bool clearServerInfo, bool useFilters);
extern void queryMasterGameTypes();
extern void queryMasterServer(U16 port, U8 flags, const char* gameType, const char* missionType,
    U8 minPlayers, U8 maxPlayers, U8 maxBots, U32 regionMask, U32 maxPing, U16 minCPU,
    U8 filterFlags, U8 buddyCount, U32* buddyList);
extern void queryFavoriteServers(U8 flags);
extern void querySingleServer(const NetAddress* addr, U8 flags);
extern void startHeartbeat();
extern void sendHeartbeat(U8 flags);
extern Vector<MasterInfo>* getMasterServerList();

#ifdef TORQUE_DEBUG
extern void addFakeServers(S32 howMany);
#endif // DEBUG

#endif
