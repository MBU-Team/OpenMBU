//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/banList.h"
#include "console/consoleTypes.h"
#include "core/resManager.h"
#include "core/fileStream.h"

IMPLEMENT_CONOBJECT(BanList);
BanList gBanList;

//------------------------------------------------------------------------------

void BanList::addBan(S32 uniqueId, const char *TA, S32 banTime)
{
   S32 curTime = Platform::getTime();

   if(banTime != 0 && banTime < curTime)
      return;

   // make sure this bastard isn't already banned on this server
   Vector<BanInfo>::iterator i;
   for(i = list.begin();i != list.end();i++)
   {
      if(uniqueId == i->uniqueId)
      {
         i->bannedUntil = banTime;
         return;
      }
   }

   BanInfo b;
   dStrcpy(b.transportAddress, TA);
   b.uniqueId = uniqueId;
   b.bannedUntil = banTime;

   if(!dStrnicmp(b.transportAddress, "ip:", 3))
   {
      char *c = dStrchr(b.transportAddress+3, ':');
      if(c)
      {
         *(c+1) = '*';
         *(c+2) = 0;
      }
   }

   list.push_back(b);
}

//------------------------------------------------------------------------------

void BanList::addBanRelative(S32 uniqueId, const char *TA, S32 numSeconds)
{
   S32 curTime = Platform::getTime();
   S32 banTime = 0;
   if(numSeconds != -1)
      banTime = curTime + numSeconds;

   addBan(uniqueId, TA, banTime);
}

//------------------------------------------------------------------------------

void BanList::removeBan(S32 uniqueId, const char *)
{
   Vector<BanInfo>::iterator i;
   for(i = list.begin();i != list.end();i++)
   {
      if(uniqueId == i->uniqueId)
      {
         list.erase(i);
         return;
      }
   }
}

//------------------------------------------------------------------------------

bool BanList::isBanned(S32 uniqueId, const char *)
{
   S32 curTime = Platform::getTime();

   Vector<BanInfo>::iterator i;
   for(i = list.begin();i != list.end();)
   {
      if(i->bannedUntil != 0 && i->bannedUntil < curTime)
      {
         list.erase(i);
         continue;
      }
      else if(uniqueId == i->uniqueId)
         return true;
      i++;
   }
   return false;
}

//------------------------------------------------------------------------------

bool BanList::isTAEq(const char *bannedTA, const char *TA)
{
   char a, b;
   for(;;)
   {
      a = *bannedTA++;
      b = *TA++;
      if(a == '*' || (!a && b == ':')) // ignore port
         return true;
      if(dTolower(a) != dTolower(b))
         return false;
      if(!a)
         return true;
   }
}

//------------------------------------------------------------------------------

void BanList::exportToFile(const char *name)
{
   FileStream banlist;

   char filename[1024];
   Con::expandScriptFilename(filename, sizeof(filename), name);
   if(ResourceManager->openFileForWrite(banlist, filename, FileStream::Write))
   {
      char buf[1024];
      Vector<BanInfo>::iterator i;
      for(i = list.begin(); i != list.end(); i++)
      {
         dSprintf(buf, sizeof(buf), "BanList::addAbsolute(%d, \"%s\", %d);\r\n", i->uniqueId, i->transportAddress, i->bannedUntil);
         banlist.write(dStrlen(buf), buf);
      }
   }

   banlist.close();
}

// ---------------------------------------------------------
// console methods
// ---------------------------------------------------------

ConsoleStaticMethod( BanList, addAbsolute, void, 4, 4, "( int ID, TransportAddress TA, int banTime )"
              "Ban a user until a given time.\n\n"
              "@param   ID       Unique ID of the player.\n"
              "@param   TA       Address from which the player connected.\n"
              "@param   banTime  Time at which they will be allowed back in.")
{
   gBanList.addBan( dAtoi( argv[1] ), argv[2], dAtoi( argv[3] ) );
}

//------------------------------------------------------------------------------
ConsoleStaticMethod( BanList, add, void, 4, 4, "( int ID, TransportAddress TA, int banLength )"
              "Ban a user for banLength seconds.\n\n"
              "@param   ID       Unique ID of the player.\n"
              "@param   TA       Address from which the player connected.\n"
              "@param   banTime  Time at which they will be allowed back in.")
{
   gBanList.addBanRelative( dAtoi( argv[1] ), argv[2], dAtoi( argv[3] ) );
}

//------------------------------------------------------------------------------
ConsoleStaticMethod( BanList, removeBan, void, 3, 3, "( int ID, TransportAddress TA )"
              "Unban someone.\n\n"
              "@param   ID       Unique ID of the player.\n"
              "@param   TA       Address from which the player connected.\n")

{
   gBanList.removeBan( dAtoi( argv[1] ), argv[2] );
}

//------------------------------------------------------------------------------
ConsoleStaticMethod( BanList, isBanned, bool, 3, 3, "( int ID, TransportAddress TA )"
              "Is someone banned?\n\n"
              "@param   ID       Unique ID of the player.\n"
              "@param   TA       Address from which the player connected.\n")
{
   return (gBanList.isBanned( dAtoi( argv[1] ), argv[2] ));
}

//------------------------------------------------------------------------------
ConsoleStaticMethod( BanList, export, void, 2, 2, "(string filename)"
              "Dump the banlist to a file.")
{
   gBanList.exportToFile( argv[1] );
}
