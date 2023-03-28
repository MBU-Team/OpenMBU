//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/version.h"
#include "console/console.h"

static const U32 csgVersionNumber = TORQUE_VERSION;

U32 getVersionNumber()
{
    return csgVersionNumber;
}

const char* getVersionString()
{
    return TORQUE_GAME_VERSION_STRING;
}

const char* getCompileTimeString()
{
    return __DATE__ " at " __TIME__;
}
//----------------------------------------------------------------

ConsoleFunctionGroupBegin(CompileInformation, "Functions to get version information about the current executable.")

ConsoleFunction(isDebugBuild, bool, 1, 1, "isDebugBuild() - Returns true if the script is running in a debug Torque executable")
{
#ifdef TORQUE_DEBUG
    return true;
#else
    return false;
#endif
}

ConsoleFunction(isRelease, bool, 1, 1, "isRelease() - Returns true if the script is running in a release build of Torque")
{
#ifdef TORQUE_DEBUG
    return false;
#else
    return true;
#endif
}

ConsoleFunction(getVersionNumber, S32, 1, 1, "Get the version of the build, as a string.")
{
    return getVersionNumber();
}

ConsoleFunction(getVersionString, const char*, 1, 1, "Get the version of the build, as a string.")
{
    return getVersionString();
}

ConsoleFunction(getCompileTimeString, const char*, 1, 1, "Get the time of compilation.")
{
    return getCompileTimeString();
}

ConsoleFunction(getBuildString, const char*, 1, 1, "Get the type of build, \"Debug\" or \"Release\".")
{
#ifdef TORQUE_DEBUG
    return "Debug";
#else
    return "Release";
#endif
}

ConsoleFunction(getCPPVersion, const char*, 1, 1, "()")
{
    argc;

#ifdef _MSVC_LANG
    U32 version = _MSVC_LANG;
#else
    U32 version = __cplusplus;
#endif

    const char* versionString;
    switch (version)
    {
        case 1L:
            versionString = "pre-C++98";
            break;
        case 199711L:
            versionString = "C++98";
            break;
        case 201103L:
            versionString = "C++11";
            break;
        case 201402L:
            versionString = "C++14";
            break;
        case 201703L:
            versionString = "C++17";
            break;
        case 202002L:
            versionString = "C++20";
            break;
        default:
            versionString = "Unknown";
            break;
    }

    char *ret = Con::getReturnBuffer(1024);
    dSprintf(ret, 1024, "%s (%dL)", versionString, version);

    return ret;
}

ConsoleFunctionGroupEnd(CompileInformation);
