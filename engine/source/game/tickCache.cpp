//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "tickCache.h"

#include "game/moveManager.h"

FreeListChunker<TickCacheEntry> TickCacheEntry::smTickCacheEntryStore;
FreeListChunker<Move> TickCacheEntry::smMoveStore;
FreeListChunker<TickCacheHead> TickCacheHead::smTickCacheHeadStore;
