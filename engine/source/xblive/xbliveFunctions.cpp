#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"

#ifdef TORQUE_OS_WIN
#include <Windows.h>
#endif

static S32 sgMPScrumSkill = 99996;

void InitXBLive()
{
    Con::addVariable("$Leaderboard::MPScrumSkill", TypeS32, &sgMPScrumSkill);
}

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

ConsoleFunction(XBLiveGetUserId, const char*, 1, 1, "()")
{
    argc;

    // XBLiveGetUserID returns in the format "<xuid>\t0" according to the comments in LevelScoreGui.gui

    return "1\t0";
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

ConsoleFunction(XBShowMarketplaceUI, bool, 4, 4, "(port, category, itemId)")
{
    argc;

    return false;
}

ConsoleFunction(XBLiveShowGamercardUI, void, 3, 3, "(port, xuid)")
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

ConsoleFunction(XBLiveRegisterPlayer, void, 4, 4, "(name, xbLiveId, invited)")
{
    argc;

    const char* name = argv[1];
    const char* xbLiveId = argv[2];
    bool invited = dAtob(argv[3]);

    Con::printf(" >> Registering player: %s, %s, %d", name, xbLiveId, invited);
}

ConsoleFunction(XBLiveUnregisterPlayer, void, 3, 3, "(name, xbLiveId)")
{
    argc;

    const char* name = argv[1];
    const char* xbLiveId = argv[2];

    Con::printf(" >> Unregistering player: %s, %s", name, xbLiveId);
}

ConsoleFunction(XBLiveRegisterRemoteTalker, void, 3, 3, "(xbLiveId, address)")
{
    argc;

    const char* xbLiveId = argv[1];
    const char* address = argv[2];

    Con::printf(" >> Registering remote talker: %s, %s", xbLiveId, address);
}

ConsoleFunction(XBLiveUnregisterRemoteTalker, void, 2, 2, "(xbLiveId)")
{
    argc;

    const char* xbLiveId = argv[1];

    Con::printf(" >> Unregistering remote talker: %s", xbLiveId);
}

ConsoleFunction(XBLiveRegisterLocalTalker, void, 2, 2, "(xbLiveId)")
{
    argc;

    const char* xbLiveId = argv[1];

    Con::printf(" >> Registering local talker: %s", xbLiveId);
}

ConsoleFunction(XBLiveUnregisterLocalTalker, void, 1, 1, "()")
{
    argc;

    Con::printf(" >> Unregistering local talker");
}

ConsoleFunction(XBLiveUpdateRemoteVoiceStatus, void, 3, 3, "(xbLiveId, xbLiveVoice)")
{
    argc;

    const char* xbLiveId = argv[1];
    const char* xbLiveVoice = argv[2];

    Con::printf(" >> Updating remote voice status: %s, %s", xbLiveId, xbLiveVoice);
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
    const char* lbId = argv[1];
    S32 leaderboard = dAtoi(argv[2]);
    S32 entryStart = dAtoi(argv[3]);
    S32 maxEntries = dAtoi(argv[4]);
    const char* callback = argv[5];

    if (entryStart == -1)
        entryStart = 0;

    Con::setIntVariable("$XBLive::LeaderboardStartingIndex", entryStart);

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
    const char* lbId = argv[1];
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
    const char* lbId = argv[1];
    S32 leaderboard = dAtoi(argv[2]); // 0 = global, 1 = mine, 2 = friends
    S32 rowIndex = dAtoi(argv[3]);

    char* gamerTag = "Test";

    // For Testing
    char gamerTagBuf[128];
    dSprintf(gamerTagBuf, 128, "%s%d", gamerTag, rowIndex + 1);

    S32 xuid = 0;
    S32 rank = rowIndex + 1;
    S32 score = 100;
    S32 data = 60000;

    char* buf = Con::getReturnBuffer(1024);
    dSprintf(buf, 128, "%s\t%d\t%d\t%d\t%d", gamerTagBuf, xuid, rank, score, data);
    return buf;
}

ConsoleFunction(XBLiveClearLoadedLeaderboards, void, 1, 1, "()")
{
    argc;
}

ConsoleFunction(XBLiveCancelLoading, void, 1, 1, "()")
{
    argc;
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

    const char* lbId = argv[1];

    return false;
}

ConsoleFunction(XBLiveSetStatsDirty, void, 2, 2, "(level)")
{
    argc;

    const char* lbId = argv[1];
}

ConsoleFunction(XBLiveGetStatValue, S32, 3, 3, "(level, lb)")
{
    argc;

    const char* lbId = argv[1];
    const char* leaderboard = argv[2];

    return 0;
}

ConsoleFunction(XBLiveWriteStats, void, 5, 5, "(lbid, lb, score, callback)")
{
    argc;

    const char* lbId = argv[1];
    const char* leaderboard = argv[2];
    S32 score = dAtoi(argv[3]);
    const char* callback = argv[4];

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

ConsoleFunction(XBLiveWriteStatsXuid, void, 6, 6, "(xbLiveId, leaderboardId, type, score, callback)")
{
    argc;

    const char* xbLiveId = argv[1];
    const char* leaderboardId = argv[2];
    const char* type = argv[3];
    S32 score = dAtoi(argv[4]);
    const char* callback = argv[5];


}

ConsoleFunction(unscientific, const char*, 2, 2, "(value)")
{
    Con::setVariable("$unscientific_hack_variable_hope_nobody_uses_this_name", "");

    char buf[4096];
    dSprintf(buf, 4096, "%s = %s;", "$unscientific_hack_variable_hope_nobody_uses_this_name", argv[1]);
    Con::evaluate(buf);

    const char* var = Con::getVariable("$unscientific_hack_variable_hope_nobody_uses_this_name");

    F32 f;
    if (var && *var && dSscanf(var, "%g", &f))
    {
        char* ret = Con::getReturnBuffer(256);

        dSprintf(ret, 256, "%g", (F64)f);
        char* num;
        for (dsize_t i = dStrlen(ret) - 1; i >= 0; *num = 0)
        {
            num = &ret[i];
            int c = ret[i];
            if (c == '.')
                break;
            if (c != '0')
                break;
            i--;
        }

        S32 len = dStrlen(ret);
        if (ret[len - 1] == '.')
            ret[len - 1] = 0;

        return ret;
    }

    return "";
}
