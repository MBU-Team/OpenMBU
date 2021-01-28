//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/bitTables.h"

bool     BitTables::mTablesBuilt = false;
S8       BitTables::mHighBit[256];
S8       BitTables::mWhichOn[256][8];
S8       BitTables::mNumOn[256];
static   BitTables  sBuildTheTables;     // invoke ctor first-time work

BitTables::BitTables()
{
   if(! mTablesBuilt){
      // This code only happens once - it relies on the tables being clear.
      for( U32 byte = 0; byte < 256; byte++ )
         for( U32 bit = 0; bit < 8; bit++ )
            if( byte & (1 << bit) )
               mHighBit[byte] = (mWhichOn[byte][mNumOn[byte]++] = bit) + 1;

      mTablesBuilt = true;
   }
}
