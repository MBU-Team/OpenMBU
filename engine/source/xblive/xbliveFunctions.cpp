#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"

#ifdef TORQUE_OS_WIN
#include <Windows.h>
#endif

ConsoleFunction(loadZip, void, 2, 2, "(zipName)")
{
    argc;

    const char* zipName = argv[2];
    Con::executef(3, "onZipLoaded", zipName, Con::getIntArg(0));
}

ConsoleFunction(isResourceBGLoaded, bool, 2, 2, "(file)")
{
    argc;
    return true;
}

ConsoleFunction(isDemoLaunch, bool, 1, 1, "()")
{
    argc;
    return false;
}

ConsoleFunction(stopDemoTimer, void, 1, 1, "()")
{
    argc;
}

const char* getSystemLanguage_forConsole()
{
    char* ret = Con::getReturnBuffer(1024);

    LangType lang = Platform::getSystemLanguage();

    const char* language;

    switch (lang)
    {
        case LANGTYPE_CHINESE:
            language = "chinese";
            break;
        case LANGTYPE_GERMAN:
            language = "german";
            break;
        case LANGTYPE_ENGLISH:
            language = "english";
            break;
        case LANGTYPE_SPANISH:
            language = "spanish";
            break;
        case LANGTYPE_FRENCH:
            language = "french";
            break;
        case LANGTYPE_ITALIAN:
            language = "italian";
            break;
        case LANGTYPE_JAPANESE:
            language = "japanese";
            break;
        case LANGTYPE_KOREAN:
            language = "korean";
            break;
        case LANGTYPE_PORTUGUESE:
            language = "portuguese";
            break;
        default:
            language = "english";
            break;
    }

    dSprintf(ret, 1024, "%s", language);
    return ret;
}

ConsoleFunction(getSystemLanguage, const char*, 1, 1, "()")
{
    argc;
    return getSystemLanguage_forConsole();
}

ConsoleFunction(getLanguage, const char*, 1, 1, "()")
{
    argc;
    const char* lang = Con::getVariable("pref::Language");

    if (*lang)
        return lang;

    Con::warnf("getLanguage: $pref::Language is not set, using system language");
    return getSystemLanguage_forConsole();
}

ConsoleFunction(getRegion, const char*, 1, 1, "()")
{
    // Possible values:
    // europe_aunz
    // europ_rest (yes this one is spelt like this in MBU360)
    // europe_all
    // restofworld_all
    // asia_all
    // asia_japan
    // asia_china
    // na_all
    // asia_rest

    return "na_all";
}

ConsoleFunction(XBLiveIsSignedIn, bool, 1, 2, "([port])")
{
    argc;
    return true;
}

ConsoleFunction(XBLiveIsSignedInSilver, bool, 1, 1, "()")
{
    argc;
    return true;
}

ConsoleFunction(XBLiveIsSignedInGold, bool, 1, 1, "()")
{
    argc;
    return true;
}

ConsoleFunction(XBLiveGetUserName, const char*, 1, 1, "()")
{
    argc;

    // Default to the name Alex, as that is the default username in MBO
    //char* ret = Con::getReturnBuffer(1024);
    //dSprintf(ret, 1024, "%s", "Alex");
    //return ret;

    // Use platform username until we set up a login system.
    return Platform::getUserName(15); // X360 only supported at max 15 characters.
}

ConsoleFunction(XBLiveGetUserId, S32, 1, 1, "()")
{
    argc;

    return 1;
}

ConsoleFunction(XBLiveGetSignInPort, S32, 1, 1, "()")
{
    argc;

    return 0;
}

ConsoleFunction(XBLiveSetSignInPort, void, 2, 2, "(port)")
{
    argc;
}

ConsoleFunction(XBLiveUpdateSigninState, void, 1, 1, "()")
{
    argc;
}

ConsoleFunction(XBLiveShowSigninUI, void, 1, 1, "()")
{
    argc;
}

ConsoleFunction(XBLiveSetRichPresence, void, 3, 3, "(port, presence)")
{
    argc;

    S32 port = atoi(argv[1]);
    S32 presence = atoi(argv[2]);

    switch (presence)
    {
        case 0:
            Con::printf("Setting Rich Presence to Menus");
            break;
        case 1:
            Con::printf("Setting Rich Presence to Singleplayer");
            break;
        case 2:
            Con::printf("Setting Rich Presence to Multiplayer");
            break;
    }
}

bool xbliveSessionActive = false;

ConsoleFunction(XBLiveIsStatsSessionActive, bool, 1, 1, "()")
{
    argc;

    return xbliveSessionActive;
}

ConsoleFunction(XBLiveStartStatsSession, void, 1, 1, "()")
{
    argc;

    xbliveSessionActive = true;
}

ConsoleFunction(XBLiveEndStatsSession, void, 1, 1, "()")
{
    argc;

    xbliveSessionActive = false;
}

ConsoleFunction(XBLiveAreStatsLoaded, bool, 2, 2, "(level)")
{
    argc;

    return false;
}

ConsoleFunction(XBLiveGetStatValue, S32, 3, 3, "(level, lb)")
{
    argc;

    return 0;
}

ConsoleFunction(XBLiveReadStats, void, 6, 6, "()")
{
    argc;
}

ConsoleFunction(XBLiveIsRanked, bool, 1, 1, "()")
{
    argc;

    return false;
}

ConsoleFunction(XBLiveLoadAchievements, void, 3, 3, "(port, callback)")
{
    argc;
}

// Were not using this, and this function is only present for completion.
ConsoleFunction(XBLiveGetYAxisInversion, S32, 2, 2, "(port)")
{
    argc;

    return -1; // Not Implemented
}

ConsoleFunction(XBLiveCheckForInvite, void, 2, 2, "(port)")
{
    argc;
}

ConsoleFunction(checkForPDLC, void, 1, 1, "()")
{
    argc;
}

ConsoleFunction(PDLCAllowMission, bool, 2, 2, "(levelId)")
{
    argc;

    S32 levelId = dAtoi(argv[1]);
    if (levelId < 80)
        return true;
    
    // original x360 code
    // return ((1 << ((levelId - 80) & 7)) & *((U8*)dlcmissionoffset + ((levelId - 80) >> 3))) != 0;

    // Just allow all maps in our version
    return true;
}

ConsoleFunction(ContentQuery, const char*, 1, 2, "([contentCategory])")
{
    S32 category = -1;
    if (argc > 1)
        category = dAtoi(argv[1]);

    Con::printf(" >> Checking for content from categories: %d", category);

    S32 newContentCount = 0;
    S32 totalContentCount = 3;

    char* result = Con::getReturnBuffer(64);

    dSprintf(result, 64, "%d %d", newContentCount, totalContentCount);

    return result;
}

// TODO: This should probably be moved to a better place
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

ConsoleFunction(outputdebugline, void, 2, 2, "()")
{
#if TORQUE_DEBUG
#ifdef TORQUE_OS_WIN
    OutputDebugStringA(argv[1]);
#endif
#endif
}

// ---------------------------------------------------------------------------------
// Leaderboards
// ---------------------------------------------------------------------------------

ConsoleFunction(XBLiveLoadLeaderboard, void, 6, 6, "()")
{
    S32 lbId = dAtoi(argv[1]);
    S32 leaderboard = dAtoi(argv[2]);
    S32 entryStart = dAtoi(argv[3]);
    S32 maxEntries = dAtoi(argv[4]);
    const char* callback = argv[5];

    Con::evaluatef("%s", callback);
}

ConsoleFunction(XBLiveGetLeaderboardRowCount, S32, 3, 3, "()")
{
    S32 lbId = dAtoi(argv[1]);
    S32 leaderboard = dAtoi(argv[2]);

    return 8;
}

ConsoleFunction(XBLiveLeaderboardHasColumn, bool, 3, 3, "()")
{
    S32 lbId = dAtoi(argv[1]);
    const char* name = argv[2];

    if (dStrcmp(name, "time") == 0)
        return true;
    if (dStrcmp(name, "gems") == 0)
        return false;
    if (dStrcmp(name, "rankedskill") == 0)
        return false;

    return false;
}

ConsoleFunction(XBLiveGetLeaderboardRow, const char*, 4, 4, "()")
{
    S32 lbId = dAtoi(argv[1]);
    S32 leaderboard = dAtoi(argv[2]);
    S32 rowIndex = dAtoi(argv[3]);

    char* gamerTag = "Test";

    // For Testing
    char gamerTagBuf[128];
    dSprintf(gamerTagBuf, 128, "%s%d", gamerTag, rowIndex);

    S32 xuid = 0;
    S32 rank = 1;
    S32 score = 100;
    S32 data = 60000;

    char* buf = Con::getReturnBuffer(1024);
    dSprintf(buf, 128, "%s\t%d\t%d\t%d\t%d", gamerTagBuf, xuid, rank, score, data);
    return buf;
}

ConsoleFunction(XBLiveClearLoadedLeaderboards, void, 1, 1, "()")
{

}

ConsoleFunction(XBLiveCancelLoading, void, 1, 1, "()")
{

}
