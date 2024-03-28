#define _CRT_SECURE_NO_WARNINGS

#include <cstdio> 
#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
//#include<iostream>
#include <string>

#include <discord_rpc.h>
#include "DiscordGame.h"
#include "platform/platform.h"
#include <core/stringTable.h>

#if defined(_WIN32)
#pragma pack(push, 1)
struct BitmapImageHeader {
    uint32_t const structSize{ sizeof(BitmapImageHeader) };
    int32_t width{ 0 };
    int32_t height{ 0 };
    uint16_t const planes{ 1 };
    uint16_t const bpp{ 32 };
    uint32_t const pad0{ 0 };
    uint32_t const pad1{ 0 };
    uint32_t const hres{ 2835 };
    uint32_t const vres{ 2835 };
    uint32_t const pad4{ 0 };
    uint32_t const pad5{ 0 };

    BitmapImageHeader& operator=(BitmapImageHeader const&) = delete;
};

struct BitmapFileHeader {
    uint8_t const magic0{ 'B' };
    uint8_t const magic1{ 'M' };
    uint32_t size{ 0 };
    uint32_t const pad{ 0 };
    uint32_t const offset{ sizeof(BitmapFileHeader) + sizeof(BitmapImageHeader) };

    BitmapFileHeader& operator=(BitmapFileHeader const&) = delete;
};
#pragma pack(pop)
#endif

namespace {
    volatile bool interrupted{ false };
}

DiscordGame* DiscordGame::smInstance = nullptr;

void DiscordGame::init()
{
    if (smInstance)
        return;

    smInstance = new DiscordGame();
}

void DiscordGame::destroy()
{
    if (!smInstance)
        return;

    delete smInstance;
    smInstance = nullptr;
}

DiscordGame* DiscordGame::get()
{
    if (!smInstance)
        init();

    return smInstance;
}

void onReady(const DiscordUser* request)
{
    DiscordGame::get()->setActive(true);
    Con::printf("Starting RPC for user %s", request->username);
    DiscordGame::get()->setUsername(request->username);
    DiscordGame::get()->setUserId(request->userId);
}

void onError(int errorCode, const char* message)
{
    DiscordGame::get()->setActive(false);
    Con::errorf("DISCORD RPC ERROR %d %s", errorCode, message);
}

void onDisconnected(int errorCode, const char* message)
{
    DiscordGame::get()->setActive(false);
    Con::errorf("DISCORD RPC DISCONNECTED %d %s", errorCode, message);
}

void onJoinGame(const char* joinSecret)
{
    Con::executef(2, "onDiscordJoinGame", joinSecret);
}

void onJoinRequest(const DiscordUser* request)
{
    Con::executef(4, "onDiscordJoinRequest", request->userId, request->username, request->avatar);
}


DiscordGame::DiscordGame()
{
    Con::printf("Initializing Discord Game SDK");
    mActive = false;

    mStatus = nullptr;
    mDetails = nullptr;
    mGUID = nullptr;
    mLargeImageKey = nullptr;
	mJoinSecret = nullptr;
	mPartyId = nullptr;
    mStartTime = 0;
    mStopTime = 0;
    mMaxPlayers = 0;
    mPlayerCount = 0;

    DiscordEventHandlers handlers;
    dMemset(&handlers, 0, sizeof(handlers));
    handlers.ready = onReady;
    handlers.errored = onError;
    handlers.disconnected = onDisconnected;
    handlers.joinGame = onJoinGame;
    handlers.spectateGame = NULL;
    handlers.joinRequest = onJoinRequest;

    // Discord_Initialize(const char* applicationId, DiscordEventHandlers* handlers, int autoRegister, const char* optionalSteamId)
    Discord_Initialize("846933806711046144", &handlers, 0, nullptr);
}

DiscordGame::~DiscordGame()
{
    if (mActive)
        Discord_Shutdown();
}

void DiscordGame::update()
{
    Discord_UpdateConnection();
    Discord_RunCallbacks();

    if (!mActive)
        return;

    if (mChanged)
    {
        mChanged = false;
        if (mStatus == nullptr)
            mStatus = "";

        DiscordRichPresence discordPresence;
        dMemset(&discordPresence, 0, sizeof(discordPresence));
        discordPresence.state = mStatus;
        

        if (mDetails == nullptr)
            mDetails = "";
        discordPresence.details = mDetails;
        discordPresence.largeImageText = "OpenMBU";

		discordPresence.partySize = mPlayerCount;
		discordPresence.partyMax = mMaxPlayers;
		discordPresence.partyId = mPartyId;
		discordPresence.joinSecret = mJoinSecret;
        discordPresence.startTimestamp = mStartTime;
        discordPresence.endTimestamp = mStopTime;

        //mActivity.GetAssets().SetLargeImage("game_icon");

        if (mGUID != nullptr) {
            //Con::printf("DiscordGameSDK::Info: %s", "Setting Large Image Key", mGUID);
            if (dStrcmp(mGUID, "MainMenu") == 0) {
                mLargeImageKey = "mbu_logo";
            }
            else {
                mLargeImageKey = DiscordGame::ProcessLevel(mGUID);
            }
            discordPresence.largeImageKey = mLargeImageKey;
            //Con::printf("DiscordGameSDK::Info: %s %s", "Setting Large Image Key GUID:", mGUID);
            
        }
        else {
            discordPresence.largeImageKey = "mbu_logo";
        }
            //mImgSm = "loading_icon";
        discordPresence.smallImageKey = "game_icon";
//        if (mIcon != nullptr)
//            mActivity.GetAssets().SetSmallImage(mIcon);
//        else
//            mActivity.GetAssets().SetSmallImage("");
//
//        if (mIconText != nullptr)
//            mActivity.GetAssets().SetSmallText(mIconText);
//        else
//            mActivity.GetAssets().SetSmallText("");

        //mActivity.GetAssets().SetSmallImage("game_icon");
        //mActivity.GetAssets().SetSmallText("OpenMBU");

        //discordPresence.
        //mActivity.SetType(discord::ActivityType::Playing);
        //mActivity.SetInstance(true);

        Discord_UpdatePresence(&discordPresence);
    }

    // -----------------------
}

const char* DiscordGame::ProcessLevel(StringTableEntry guid) {
    if (mGuidLookup.size() == 0)
    {
        mGuidLookup = {
            //Beginner Level GUIDS
            {"{D7DD1833-2929-4D0F-B05F-E2BB4C410408}"_ts, "level_01"_ts},
            {"{4340C070-D0C3-464D-BC4A-E5AB6F63BEE9}"_ts, "level_02"_ts},
            {"{39F047AB-B2C6-46D1-8243-6C2A5E8EBE86}"_ts, "level_03"_ts},
            {"{0AB4B5D7-7B42-46D1-9257-33E68942F0EC}"_ts, "level_04"_ts},
            {"{28CA2E65-E718-4ED0-A0A6-D3C289DBB326}"_ts, "level_05"_ts},
            {"{E4538A09-5ACA-4012-B32C-5631C83A6A40}"_ts, "level_06"_ts},
            {"{68E77476-1DEF-4A6A-B80B-12D7D5E3AA83}"_ts, "level_07"_ts},
            {"{4FAE738C-7565-41D9-9EAD-93F9EA107720}"_ts, "level_08"_ts},
            {"{DCFACD70-A1F1-48D8-8F79-6F7AB83D8717}"_ts, "level_09"_ts},
            {"{1DF4EDE4-39DB-4604-B656-A5B990CE70B6}"_ts, "level_10"_ts},
            {"{42A49D1B-D64A-419A-8D18-A947DF0996A4}"_ts, "level_11"_ts},
            {"{10C05077-25FA-42CA-8D85-9D1A7A55CD8E}"_ts, "level_12"_ts},
            {"{706F5ABB-04FD-4C95-A71E-802D59277329}"_ts, "level_13"_ts},
            {"{84402A6C-C9DE-44F4-A025-A229A736FA27}"_ts, "level_14"_ts},
            {"{69B70D35-6D43-45B2-98C6-FEC7A6D71CC2}"_ts, "level_15"_ts},
            {"{1682115C-88F2-44B1-8DC1-0E28A000DEB2}"_ts, "level_16"_ts},
            {"{D339DF52-9388-4615-933F-E6BB9E1FCD9F}"_ts, "level_17"_ts},
            {"{FE60AB24-C91E-4BB0-A3E2-21256BE1627E}"_ts, "level_18"_ts},
            {"{05500723-A9D7-4D74-9BD6-8199A864422A}"_ts, "level_19"_ts},
            {"{2BFE62B4-D91A-4C9B-B81B-55EABB01C311}"_ts, "level_20"_ts},
            //Intermediate Level GUIDS
            {"{AF781BFC-B941-4335-86C6-B7CE542AA4CE}"_ts, "level_21"_ts},
            {"{8AC4C432-8A5C-4ADE-9B38-08B462EB1354}"_ts, "level_22"_ts},
            {"{8F284E76-0C97-4D44-93A6-287DC35F4DD2}"_ts, "level_23"_ts},
            {"{E0E23377-0DAD-4B2A-9F9F-7DAF1BD18CBF}"_ts, "level_24"_ts},
            {"{BED8DF2C-4F41-4FE5-B253-27FBDEF3F68A}"_ts, "level_25"_ts},
            {"{9648B38D-880B-4EAD-8443-E9DFBE17B273}"_ts, "level_26"_ts},
            {"{129746F6-F695-4DCE-89A6-817AF582EA3A}"_ts, "level_27"_ts},
            {"{8BAAA76E-ADF2-4337-BB89-8885D8E4188F}"_ts, "level_28"_ts},
            {"{C375FCB5-1203-4BDC-AFDA-F8C22B463DA9}"_ts, "level_29"_ts},
            {"{A356C0DA-9F6E-4A7E-9D69-E8B91EC013BB}"_ts, "level_30"_ts},
            {"{05EFFBDF-75CA-4107-AB93-7865F4383107}"_ts, "level_31"_ts},
            {"{5FDB0AE3-A9E5-45F2-B43B-0969E7809FA7}"_ts, "level_32"_ts},
            {"{802D17DF-0168-4A17-B8A6-EF62D0ED5D7B}"_ts, "level_33"_ts},
            {"{AE6F5499-652A-43A7-BADE-4561EFD2859A}"_ts, "level_34"_ts},
            {"{62329829-CB12-403D-A0DD-CC6B5D2B70D0}"_ts, "level_35"_ts},
            {"{8E02E49C-1465-473D-BE60-990AC25753CE}"_ts, "level_36"_ts},
            {"{958557D1-641F-474C-BB27-AAA1E90925FD}"_ts, "level_37"_ts},
            {"{40D18ABA-D319-4D37-ACDD-239DC8B9D0B8}"_ts, "level_38"_ts},
            {"{435771D3-BC67-4B48-B353-1D8C280DD7CF}"_ts, "level_39"_ts},
            {"{7A7BF863-D4E5-471D-91C3-B4DD7041AA4B}"_ts, "level_40"_ts},
            //Advanced Level GUIDS
            {"{BA97253E-1B1D-40BF-9F4E-938FCC1DCDAC}"_ts, "level_41"_ts},
            {"{427A70F3-DFB3-4A85-BF21-8E6373B58AAE}"_ts, "level_42"_ts},
            {"{25193471-D1F7-4A31-B30F-792891C35558}"_ts, "level_43"_ts},
            {"{0CD63D0D-D29B-4F2A-A8D8-2B8B5AB85635}"_ts, "level_44"_ts},
            {"{AD4C4328-BAE3-427F-9DFA-2B0294855D48}"_ts, "level_45"_ts},
            {"{834F4547-B534-4B2C-A9C0-11AFA87DC633}"_ts, "level_46"_ts},
            {"{777E5EA8-8A92-4BA9-B0F1-2E4CE2CE9F1A}"_ts, "level_47"_ts},
            {"{FC86DD0D-3646-4565-BCCC-ED24BDAFFCA2}"_ts, "level_48"_ts},
            {"{BB8FA1E2-C6E4-4981-821F-55AD5BBB587A}"_ts, "level_49"_ts},
            {"{268C3D5D-37B4-4386-8FCD-C91EA984AC8A}"_ts, "level_50"_ts},
            {"{60552BB6-BDCC-4E77-971F-C55065A69887}"_ts, "level_51"_ts},
            {"{694E35EF-D063-4852-8453-39DB8EFDE292}"_ts, "level_52"_ts},
            {"{2E115F91-3CD4-49F9-B36E-5901B95D4107}"_ts, "level_53"_ts},
            {"{A57A2C49-DE68-41B8-B01E-4A6776990F32}"_ts, "level_54"_ts},
            {"{76487562-ACEC-46BE-9C22-5D2F7AD572C1}"_ts, "level_55"_ts},
            {"{38FD3570-6217-4FE1-8C79-C0A8D4B45068}"_ts, "level_56"_ts},
            {"{35D90D68-7FFA-4EB8-967D-7373B5EFC718}"_ts, "level_57"_ts},
            {"{6AD733E7-A256-4ECF-A9ED-ED98AB9B832A}"_ts, "level_58"_ts},
            {"{989F14E6-8F25-4AE1-B216-3A1FA3073370}"_ts, "level_59"_ts},
            {"{7E67B719-0465-4CB4-BC05-0E7766AAB1AE}"_ts, "level_60"_ts},
            //Multiplayer Level GUIDS
            {"{CF18B4E3-5D01-4556-BD5B-38732CDFC297}"_ts, "level_61"_ts},
            {"{70E49DF8-CE3F-4136-A081-AC38E865B809}"_ts, "level_62"_ts},
            {"{CD6C0624-475C-46CC-AE15-655B9FFF4E93}"_ts, "level_63"_ts},
            {"{2D001C28-8D65-46D4-98F3-93C16CFD9477}"_ts, "level_64"_ts},
            {"{5D7CFB46-1657-4121-BE0A-1BB9D551A17E}"_ts, "level_65"_ts},
            {"{7DEF697A-3F76-43A9-BF7D-A16B2D913250}"_ts, "level_66"_ts},
            {"{49A510A5-E540-40C2-B555-37A37CA5B01A}"_ts, "level_67"_ts},
            //marble city 2
            {"{241F26A1-D3FA-4583-87DB-611C21278F86}"_ts, "level_68"_ts},
            {"{872E0141-D1EA-4B68-AFA0-C6B9CEF4669B}"_ts, "level_69"_ts},
            {"{917B670B-2DB1-4746-A345-A8EF799DB682}"_ts, "level_70"_ts},
            {"{87206CA6-73AC-483B-B220-954FE3BE2E8A}"_ts, "level_71"_ts},
            {"{41268C4B-7DAD-4972-B4CC-0A39F008FD4E}"_ts, "level_72"_ts},
            {"{C015BB57-FC15-4A76-A31C-544402339670}"_ts, "level_73"_ts},
            {"{63096AD7-D99D-4E96-8E58-2789AB73ADF7}"_ts, "level_74"_ts},
            {"{92A2B64A-6419-4779-AD02-4AE4686036DB}"_ts, "level_75"_ts},
            {"{B65ACF73-A6C5-48A0-BEEC-D4C6D1E9D33E}"_ts, "level_76"_ts},
            {"{D29EB181-677D-471D-B72A-D0C8092B34F9}"_ts, "level_77"_ts},
            {"{E7FED2CC-6FD2-4D4A-A62F-D6D7EFEBC582}"_ts, "level_78"_ts},
            {"{96E7EB66-B551-4E8F-809C-A83780065C05}"_ts, "level_79"_ts},
            {"{F2F59069-4D07-4665-B649-95E018FAAB28}"_ts, "level_80"_ts},
            {"{3DCFC6EE-A2DE-465F-B040-6FC31D5C0B6E}"_ts, "level_81"_ts},
            //marble city 1
            {"{D18409B9-AAA3-4260-8129-C477062BF6CA}"_ts, "level_82"_ts},
            {"{758E4F71-3802-4937-A0CE-4B6791C1EA6D}"_ts, "level_83"_ts},
            {"{D4A795F8-88B8-4B82-B097-AC4A0FB4A492}"_ts, "level_84"_ts},
            {"{7F6D6908-8357-4FFB-BC4F-A66245310CD3}"_ts, "level_85"_ts},
            {"{663167C1-C00D-461D-9A30-01235D0A555A}"_ts, "level_86"_ts},
        };
    }

    if (mGuidLookup.find(mGUID) == mGuidLookup.end()) {
        return "custom_level";
    }
    else
        return mGuidLookup[mGUID];
    
}

void DiscordGame::respondJoinRequest(const char* userId, int value)
{
    Discord_Respond(userId, value);
}
