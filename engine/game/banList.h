//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BANLIST_H_
#define _BANLIST_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

/// Helper class to keep track of bans.
class BanList : public SimObject
{
   typedef SimObject Parent;
public:

   struct BanInfo
   {
      S32      uniqueId;
      char     transportAddress[128];
      S32      bannedUntil;
   };

   Vector<BanInfo> list;

   BanList(){}
   ~BanList(){}

   void addBan(S32 uniqueId, const char *TA, S32 banTime);
   void addBanRelative(S32 uniqueId, const char *TA, S32 numSeconds);
   void removeBan(S32 uniqueId, const char *TA);
   bool isBanned(S32 uniqueId, const char *TA);
   bool isTAEq(const char *bannedTA, const char *TA);
   void exportToFile(const char *fileName);

   DECLARE_CONOBJECT(BanList);
};

extern BanList gBanList;

#endif
