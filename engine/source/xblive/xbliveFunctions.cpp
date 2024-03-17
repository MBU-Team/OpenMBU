#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"

#include "discord/DiscordGame.h"
#include "core/Guid.hpp"

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

    // TODO: Implement BZA loading

    const char* zipName = argv[2];
    Con::executef(3, "onZipLoaded", zipName, Con::getIntArg(0));
}

ConsoleFunction(isResourceBGLoaded, bool, 2, 2, "(file)")
{
    argc;

    // TODO: Implement BZA loading

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
        case LANGTYPE_POLISH:
            language = "polish";
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
    if (Con::getBoolVariable("pref::Player::UsingCustomName"))
    {
        return Con::getVariable("pref::Player::Name");
    }
    if (DiscordGame::get()->isActive())
    {
        const char* res = DiscordGame::get()->getUsername(15);
        Con::setVariable("Player::DiscordUsername", res);
        Con::executef(2, "getDiscordUsername", DiscordGame::get()->getUserId());
        return res;
    }

    // Use platform username until we set up a login system.
    return Platform::getUserName(15); // X360 only supported at max 15 characters.
}

ConsoleFunction(XBLiveGetUserId, const char*, 1, 1, "()")
{
    argc;

    // TODO: Implement

    // XBLiveGetUserID returns in the format "<xuid>\t0" according to the comments in LevelScoreGui.gui

    return "1\t0";
}

ConsoleFunction(XBLiveGetSignInPort, S32, 1, 1, "()")
{
    argc;

    // TODO: Implement

    return 0;
}

ConsoleFunction(XBLiveSetSignInPort, void, 2, 2, "(port)")
{
    argc;

    // TODO: Implement
}

ConsoleFunction(XBLiveUpdateSigninState, void, 1, 1, "()")
{
    argc;

    // TODO: Implement
}

ConsoleFunction(XBLiveShowSigninUI, void, 1, 1, "()")
{
    argc;

    // TODO: Implement
}

ConsoleFunction(XBShowMarketplaceUI, bool, 4, 4, "(port, category, itemId)")
{
    argc;

    return false;
}

ConsoleFunction(XBLiveShowGamercardUI, void, 3, 3, "(port, xuid)")
{
    argc;

    // TODO: Implement
}

ConsoleFunction(XBLiveShowPlayerReviewUI, void, 3, 3, "(port, xuid)")
{
    argc;

    // TODO: Implement
}

ConsoleFunction(XBLiveShowFriendRequestUI, void, 3, 3, "(port, xuid)")
{
    argc;

    // TODO: Implement
}

ConsoleFunction(XBLiveGetGamerZone, const char*, 1, 1, "(port)")
{
    argc;

    int port = dAtoi(argv[1]);

    // TODO: Implement

    return "";
}

ConsoleFunction(XBLiveSetRichPresence, void, 4, 5, "(port, presence, levelname, levelguid)")
{
#ifdef TORQUE_DISCORD_RPC
    argc;

    S32 port = dAtoi(argv[1]);
    S32 presence = dAtoi(argv[2]);
    const char* levelname;
    const char* levelguid;
    if (argc > 3)
        levelname = StringTable->insert(argv[3]);
    else
        levelname = StringTable->insert("");

    if (argc > 4)
        levelguid = StringTable->insert(argv[4]);
    else
        levelguid = StringTable->insert("");

    switch (presence)
    {
    case 0:
        Con::printf("Setting Rich Presence to Menus");
        DiscordGame::get()->setStatus("In Menus");
        DiscordGame::get()->setDetails("");
        DiscordGame::get()->setPartyDetails(0, 0, nullptr, nullptr);
        DiscordGame::get()->setTimer(0, 0);
        //DiscordGame::get()->setSmallImageKey("game_icon");
        DiscordGame::get()->setLevel("MainMenu");
        break;
    case 1:
        Con::printf("Setting Rich Presence to Singleplayer");
        DiscordGame::get()->setStatus(levelname);
        DiscordGame::get()->setDetails("Playing Singleplayer");
        DiscordGame::get()->setPartyDetails(0, 0, nullptr, nullptr);
        //DiscordGame::get()->setSmallImageKey("game_icon");
        DiscordGame::get()->setLevel(levelguid);
        break;
    case 2:
        Con::printf("Setting Rich Presence to Multiplayer");
        DiscordGame::get()->setStatus(levelname);
        DiscordGame::get()->setDetails("Playing Multiplayer");
        //DiscordGame::get()->setSmallImageKey("game_icon");
        DiscordGame::get()->setLevel(levelguid);
        break;
    }
#endif
}

ConsoleFunction(XBLivePresenceStartTimer, void, 1, 2, "([stopTime])")
{
    U64 startTime = static_cast<U64>(time(nullptr));
    if (argc > 1)
    {
        int secsRemaining = atoi(argv[1]);
        DiscordGame::get()->setTimer(startTime,  startTime + secsRemaining);
    }
    else
    {
        DiscordGame::get()->setTimer(startTime);
    }

}

ConsoleFunction(XBLivePresenceStopTimer, void, 1, 1, "()")
{
    DiscordGame::get()->setTimer(0, 0);
}

ConsoleFunction(XBLiveRespondJoinRequest, void, 3, 3, "(userId, reply)")
{
    DiscordGame::get()->respondJoinRequest(argv[1], atoi(argv[2]));
}

ConsoleFunction(XBLiveGetPartyId, const char*, 1, 1, "()")
{
    StringTableEntry id = DiscordGame::get()->getPartyId();
    return id != nullptr ? id : "";
}

ConsoleFunction(XBLiveSetPartyId, void, 2, 2, "(partyId)")
{
    const char* partyId = argv[1];
    if (dStrcmp(partyId, "") == 0) 
    {
        DiscordGame::get()->setPartyId(nullptr);
    }
    else 
    {
        DiscordGame::get()->setPartyId(StringTable->insert(partyId, true));
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

    // TODO: Implement
}

ConsoleFunction(XBLiveUnregisterPlayer, void, 3, 3, "(name, xbLiveId)")
{
    argc;

    const char* name = argv[1];
    const char* xbLiveId = argv[2];

    Con::printf(" >> Unregistering player: %s, %s", name, xbLiveId);

    // TODO: Implement
}

ConsoleFunction(XBLiveRegisterRemoteTalker, void, 3, 3, "(xbLiveId, address)")
{
    argc;

    const char* xbLiveId = argv[1];
    const char* address = argv[2];

    Con::printf(" >> Registering remote talker: %s, %s", xbLiveId, address);

    // TODO: Implement
}

ConsoleFunction(XBLiveUnregisterRemoteTalker, void, 2, 2, "(xbLiveId)")
{
    argc;

    const char* xbLiveId = argv[1];

    Con::printf(" >> Unregistering remote talker: %s", xbLiveId);

    // TODO: Implement
}

ConsoleFunction(XBLiveRegisterLocalTalker, void, 2, 2, "(xbLiveId)")
{
    argc;

    const char* xbLiveId = argv[1];

    Con::printf(" >> Registering local talker: %s", xbLiveId);

    // TODO: Implement
}

ConsoleFunction(XBLiveUnregisterLocalTalker, void, 1, 1, "()")
{
    argc;

    Con::printf(" >> Unregistering local talker");

    // TODO: Implement
}

ConsoleFunction(XBLiveUpdateRemoteVoiceStatus, void, 3, 3, "(xbLiveId, xbLiveVoice)")
{
    argc;

    const char* xbLiveId = argv[1];
    const char* xbLiveVoice = argv[2];

    Con::printf(" >> Updating remote voice status: %s, %s", xbLiveId, xbLiveVoice);

    // TODO: Implement
}

ConsoleFunction(XBLiveIsPlayerMuted, bool, 2, 2, "(xbLiveId)")
{
    argc;

    const char *xbLiveId = argv[1];

    Con::printf(" >> Checking if player is muted: %s", xbLiveId);

    // TODO: Implement

    return false;
}

ConsoleFunction(XBLiveIsFriend, bool, 2, 2, "(port, xbLiveId)")
{
    argc;

    int port = dAtoi(argv[1]);
    const char *xbLiveId = argv[2];

    Con::printf(" >> Checking if player is a friend: %s", xbLiveId);

    // TODO: Implement

    return false;
}

ConsoleFunction(XBLiveXnAddrToString, const char*, 2, 2, "(address)")
{
    argc;

    const char *address = argv[1];

    Con::printf(" >> Converting address to string: %s", address);

    // TODO: Implement

    return address;
}

ConsoleFunction(XBLiveAddPolledHost, void, 2, 2, "(index)")
{
    argc;

    int index = dAtoi(argv[1]);

    Con::printf(" >> Adding polled host: %d", index);

    // TODO: Implement
}

ConsoleFunction(XBLiveIsArbRegistered, bool, 1, 1, "()")
{
    argc;

    Con::printf(" >> Checking if ARB is registered");

    // TODO: Implement

    return true;
}

ConsoleFunction(XBLiveCreateHostedMatch, void, 8, 8, "(name, gamemode, mission, maxplayers, privateslots, joinSecret, callback)")
{
    argc;

    const char* name = argv[1];
    const char* gamemode = argv[2];
    const char* mission = argv[3];
    const char* maxplayers = argv[4];
    const char* privateslots = argv[5];
    StringTableEntry joinSecret = StringTable->insert(argv[6]);
    const char* callback = argv[7];


    auto g = xg::newGuid();
    std::string guid = g.str();
    const char* ret = guid.c_str();
    char* retBuffer = Con::getReturnBuffer(256);
    dSprintf(retBuffer, 256, "{%s}", ret);

    Con::printf(" >> Creating hosted match: %s, %s, %s, %s, %s", name, gamemode, mission, maxplayers, privateslots);

#ifdef TORQUE_DISCORD_RPC
    DiscordGame::get()->setPartyDetails(1, atoi(maxplayers), joinSecret, StringTable->insert(retBuffer, true));
#endif

    // TODO: Implement
}

ConsoleFunction(XBLiveUpdateHostedMatch, void, 9, 9, "(publicused, publicfree, privateused, privatefree, status, gamemode, mission, disablejip)")
{
    argc;
    
    int publicused = atoi(argv[1]);
    int publicfree = atoi(argv[2]);
    int privateused = atoi(argv[3]);
    int privatefree = atoi(argv[4]);
    bool disableJip = dAtob(argv[8]);

    if (disableJip)
    {
        publicfree = 0;
        publicused = 0;
        privatefree = 0;
        privateused = 0;
    }

#ifdef TORQUE_DISCORD_RPC
    DiscordGame::get()->updatePartyDetails(publicused + privateused, publicfree + privatefree + publicused + privateused);
#endif

    // TODO: Implement
}

ConsoleFunction(XBLiveSearchForMatches, void, 6, 6, "(gamemode, mission, maxplayers, unknown, callback)")
{
    argc;

    // TODO: most likely calls onXBLiveQoSPollComplete when done

    const char* gamemode = argv[1];
    const char* mission = argv[2];
    const char* maxplayers = argv[3];
    const char* unknown = argv[4]; // TODO: figure out unknown argument
    const char* callback = argv[5];

    Con::printf(" >> Searching for matches");
    // TODO: Implement

    Con::evaluatef("%s", callback);
}

ConsoleFunction(XBLiveProbeLocalQoS, void, 2, 2, "(callback)")
{
    argc;

    const char *callback = argv[1];

    Con::printf(" >> Probing local QoS");

    // TODO: Implement
}

ConsoleFunction(XBLiveGetLocalQoSInfo, const char*, 1, 1, "()")
{
    argc;

    Con::printf(" >> Getting local QoS info");

    // TODO: Implement

    return "";
}

ConsoleFunction(XBLiveStartQoSPoll, void, 1, 1, "()")
{
    argc;

    Con::printf(" >> Starting QoS poll");

    // TODO: Implement
}

ConsoleFunction(XBLiveCancelQoSPoll, void, 1, 1, "()")
{
    argc;

    Con::printf(" >> Cancelling QoS poll");

    // TODO: Implement
}

ConsoleFunction(XBLiveGetMatchResultsEntry, const char*, 3, 3, "(index)")
{
    argc;

    S32 index = dAtoi(argv[1]);

    Con::printf(" >> Getting match results entry: %d", index);

    // TODO: Implement

    return "";
}

ConsoleFunction(XBLiveSetEnabled, void, 2, 2, "(enabled)")
{
    argc;

    bool enabled = dAtob(argv[1]);

    Con::printf(" >> Setting enabled: %d", enabled);

    // TODO: Implement
}

ConsoleFunction(XBLiveConnect, void, 2, 2, "(index, invited, callback)")
{
    argc;

    S32 index = dAtoi(argv[1]);
    bool invited = dAtob(argv[2]);
    const char* callback = argv[3];

    Con::printf(" >> Connecting to match: %d, %d", index, invited);

    // TODO: Implement
    // TODO: XBLiveConnect sets $XBLive::secureHostAddress

    Con::evaluatef("%s", callback);
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

ConsoleFunction(XBLiveReadStats, void, 4, 6, "()")
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
