//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TICKCACHE_H_
#define _TICKCACHE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

struct Move;

struct TickCacheEntry
{
    enum { MaxPacketSize = 140 };

    U8 packetData[MaxPacketSize];
    TickCacheEntry* next;
    Move* move;

    // If you want to assign moves to tick cache for later playback, allocate them here
    Move* allocateMove();
};

struct TickCacheHead;

class TickCache
{
public:
    TickCache();
    ~TickCache();

    TickCacheEntry* addCacheEntry();
    void dropOldest();
    void dropNextOldest();
    void ageCache(S32 numToAge, S32 len);
    void setCacheSize(S32 len);
    void beginCacheList();
    TickCacheEntry* incCacheList(bool addIfNeeded = true);

private:
    TickCacheHead* mTickCacheHead;
};

inline TickCache::TickCache()
{
    mTickCacheHead = NULL;
}

#endif // _TICKCACHE_H_