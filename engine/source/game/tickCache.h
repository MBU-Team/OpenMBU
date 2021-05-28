//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TICKCACHE_H_
#define _TICKCACHE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#include "core/dataChunker.h"

struct Move;

struct TickCacheEntry
{
    enum { MaxPacketSize = 140 };

    U8 packetData[MaxPacketSize];
    TickCacheEntry* next;
    Move* move;

    static TickCacheEntry* alloc() { return smTickCacheEntryStore.alloc(); }
    static void free(TickCacheEntry* entry) { smTickCacheEntryStore.free(entry); }

    // If you want to assign moves to tick cache for later playback, allocate them here
    static Move* allocMove() { return smMoveStore.alloc(); }
    static void freeMove(Move* move) { smMoveStore.free(move); }

    static FreeListChunker<TickCacheEntry> smTickCacheEntryStore;
    static FreeListChunker<Move> smMoveStore;

};

struct TickCacheHead
{
    TickCacheEntry* oldest;
    TickCacheEntry* newest;
    TickCacheEntry* next;
    U32 numEntry;

    static TickCacheHead* alloc() { return smTickCacheHeadStore.alloc(); }
    static void free(TickCacheHead* head) { smTickCacheHeadStore.free(head); }

    static FreeListChunker<TickCacheHead> smTickCacheHeadStore;
};

#endif // _TICKCACHE_H_