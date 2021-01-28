//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include  "core/tVector.h"

typedef Vector<char*> CharVector;
static CharVector gPlatformDirectoryExcludeList;

void Platform::addExcludedDirectory(const char *pDir)
{
   gPlatformDirectoryExcludeList.push_back(dStrdup(pDir));
}

void Platform::clearExcludedDirectories()
{
   while(gPlatformDirectoryExcludeList.size())
   {
      dFree(gPlatformDirectoryExcludeList.last());
      gPlatformDirectoryExcludeList.pop_back();
   }
}

bool Platform::isExcludedDirectory(const char *pDir)
{
   for(CharVector::iterator i=gPlatformDirectoryExcludeList.begin(); i!=gPlatformDirectoryExcludeList.end(); i++)
      if(!dStrcmp(pDir, *i))
         return true;

   return false;
}
