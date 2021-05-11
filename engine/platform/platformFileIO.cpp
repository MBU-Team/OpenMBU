//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "core/stringTable.h"
#include "platform/platform.h"
#include  "core/tVector.h"

typedef Vector<char*> CharVector;
static CharVector gPlatformDirectoryExcludeList;

void Platform::addExcludedDirectory(const char* pDir)
{
    gPlatformDirectoryExcludeList.push_back(dStrdup(pDir));
}

void Platform::clearExcludedDirectories()
{
    while (gPlatformDirectoryExcludeList.size())
    {
        dFree(gPlatformDirectoryExcludeList.last());
        gPlatformDirectoryExcludeList.pop_back();
    }
}

bool Platform::isExcludedDirectory(const char* pDir)
{
    for (CharVector::iterator i = gPlatformDirectoryExcludeList.begin(); i != gPlatformDirectoryExcludeList.end(); i++)
        if (!dStrcmp(pDir, *i))
            return true;

    return false;
}

//-----------------------------------------------------------------------------

StringTableEntry Platform::getPrefsPath(const char* file /* = NULL */)
{
#ifdef TORQUE_PLAYER
    return StringTable->insert(file ? file : "");
#else
    char buf[1024];
    const char* company = Con::getVariable("$Game::CompanyName");
    if (company == NULL || *company == 0)
        company = "GarageGames";

    const char* appName = Con::getVariable("$Game::GameName");
    if (appName == NULL || *appName == 0)
        appName = TORQUE_GAME_NAME;

    if (file)
    {
        if (dStrstr(file, ".."))
        {
            Con::errorf("getPrefsPath - filename cannot be relative");
            return NULL;
        }

        dSprintf(buf, sizeof(buf), "%s/%s/%s/%s", Platform::getUserDataDirectory(), company, appName, file);
    }
    else
        dSprintf(buf, sizeof(buf), "%s/%s/%s", Platform::getUserDataDirectory(), company, appName);

    return StringTable->insert(buf, true);
#endif
}

//-----------------------------------------------------------------------------

ConsoleFunction(getUserDataDirectory, const char*, 1, 1, "getUserDataDirectory()")
{
    return Platform::getUserDataDirectory();
}

ConsoleFunction(getUserHomeDirectory, const char*, 1, 1, "getUserHomeDirectory()")
{
    return Platform::getUserHomeDirectory();
}
