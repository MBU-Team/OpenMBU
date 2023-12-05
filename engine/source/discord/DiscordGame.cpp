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

#include "discord.h"
#include "DiscordGame.h"
#include "platform/platform.h"

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

DiscordGame::DiscordGame()
{
    Con::printf("Initializing Discord Game SDK");
    mActive = false;

    mActivity = {};
    mStatus = nullptr;
    mDetails = nullptr;
    mGUID = nullptr;
    mLargeImageKey = nullptr;
    //mIcon = nullptr;
    //mIconText = nullptr;

    mCore = {};

    //auto result = discord::Core::Create(846933806711046144, DiscordCreateFlags_Default, &core);
    //auto result = discord::Core::Create(1181357649644769300, DiscordCreateFlags_NoRequireDiscord, &mCore);
    auto result = discord::Core::Create(846933806711046144, DiscordCreateFlags_NoRequireDiscord, &mCore);

    if (!mCore) {
        Con::printf("Failed to instantiate discord core! (err %i)", static_cast<int>(result));
        //std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result)
        //    << ")\n";
        //std::exit(-1);
        return;
    }

    mCore->SetLogHook(
        discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
            if (level == discord::LogLevel::Error)
                Con::printf("DiscordGameSDK::Error: %s", message);
            else if (level == discord::LogLevel::Warn)
                Con::printf("DiscordGameSDK::Warning: %s", message);
            else if (level == discord::LogLevel::Debug)
                Con::printf("DiscordGameSDK::Debug: %s", message);
            else
                Con::printf("DiscordGameSDK::Info: %s", message);

            //std::cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n";
        });

//    mCore->UserManager().OnCurrentUserUpdate.Connect([this]() {
//        mCore->UserManager().GetCurrentUser(&state.currentUser);
//
//        Con::printf("Current user updated: %s#%s", state.currentUser.GetUsername(), state.currentUser.GetDiscriminator());
//
//        //std::cout << "Current user updated: " << state.currentUser.GetUsername() << "#"
//        //    << state.currentUser.GetDiscriminator() << "\n";
//
//
//        });

    //mFilename = Platform::getPrefsPath(AUTOSPLITTER_FILE_NAME);
    //mFile.open(mFilename, std::ios_base::app);
    //if (!mFile.is_open())
    //{
    //    Con::errorf("Failed to open autosplitter file %s.", mFilename.c_str());
    //    Con::errorf("Autosplitter is disabled.");
    //    return;
    //}
    //Con::printf("Autosplitter Initialized to file %s", mFilename.c_str());
    mActive = true;
}

DiscordGame::~DiscordGame()
{
    //mFile.close();
    //std::remove(mFilename.c_str());
}

void DiscordGame::update()
{
    if (!mActive)
        return;

    const char* oldStatus = mActivity.GetState();
    const char* oldDetails = mActivity.GetDetails();

    bool statusChanged = oldStatus != nullptr && mStatus != nullptr && dStrcmp(oldStatus, mStatus) != 0;
    statusChanged |= (oldStatus == nullptr && mStatus != nullptr);
    statusChanged |= (oldStatus != nullptr && mStatus == nullptr);

    bool detailsChanged = oldDetails != nullptr && mDetails != nullptr && dStrcmp(oldDetails, mDetails) != 0;
    detailsChanged |= (oldDetails == nullptr && mDetails != nullptr);
    detailsChanged |= (oldDetails != nullptr && mDetails == nullptr);

    if (statusChanged || detailsChanged)
    {
        if (mStatus == nullptr)
            mStatus = "";
        mActivity.SetState(mStatus);

        if (mDetails == nullptr)
            mDetails = "";
        mActivity.SetDetails(mDetails);

        //mActivity.GetAssets().SetLargeImage("game_icon");
        mActivity.GetAssets().SetLargeText("OpenMBU");

        if (mGUID != nullptr) {
            //Con::printf("DiscordGameSDK::Info: %s", "Setting Large Image Key", mGUID);
            mLargeImageKey = DiscordGame::ProcessLevel(mGUID);
            Con::printf("DiscordGameSDK::Info: %s %s", "Setting Large Image Key GUID:", mGUID);
            mActivity.GetAssets().SetLargeImage(mLargeImageKey);
            
        }
        else {
            mActivity.GetAssets().SetLargeImage("game_icon");
        }
            //mImgSm = "loading_icon";

        mActivity.GetAssets().SetSmallImage("game_icon");
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

        mActivity.SetType(discord::ActivityType::Playing);
        //mActivity.SetInstance(true);

        mCore->ActivityManager().UpdateActivity(mActivity, [](discord::Result result)
        {
            if (result == discord::Result::Ok)
                Con::printf("DiscordGameSDK::Activity updated!");
            else
                Con::errorf("DiscordGameSDK::Activity update failed! (err %i)", static_cast<int>(result));
        });
    }

    // -----------------------

    auto result = mCore->RunCallbacks();
    if (result != discord::Result::Ok)
        Con::errorf("DiscordGameSDK::RunCallbacks failed! (err %i)", static_cast<int>(result));
}

char* DiscordGame::ProcessLevel(const char* guid) {
    //Beginner Level GUIDS
    if (strcmp(mGUID, "{D7DD1833-2929-4D0F-B05F-E2BB4C410408}") == 0) {
        return "level_01";
    }
    else if (strcmp(mGUID, "{4340C070-D0C3-464D-BC4A-E5AB6F63BEE9}") == 0) {
        return "level_02";
    }
    else if (strcmp(mGUID, "{39F047AB-B2C6-46D1-8243-6C2A5E8EBE86}") == 0) {
        return "level_03";
    }
    else if (strcmp(mGUID, "{0AB4B5D7-7B42-46D1-9257-33E68942F0EC}") == 0) {
        return "level_04";
    }
    else if (strcmp(mGUID, "{28CA2E65-E718-4ED0-A0A6-D3C289DBB326}") == 0) {
        return "level_05";
    }
    else if (strcmp(mGUID, "{E4538A09-5ACA-4012-B32C-5631C83A6A40}") == 0) {
        return "level_06";
    }
    else if (strcmp(mGUID, "{68E77476-1DEF-4A6A-B80B-12D7D5E3AA83}") == 0) {
        return "level_07";
    }
    else if (strcmp(mGUID, "{4FAE738C-7565-41D9-9EAD-93F9EA107720}") == 0) {
        return "level_08";
    }
    else if (strcmp(mGUID, "{DCFACD70-A1F1-48D8-8F79-6F7AB83D8717}") == 0) {
        return "level_09";
    }
    else if (strcmp(mGUID, "{1DF4EDE4-39DB-4604-B656-A5B990CE70B6}") == 0) {
        return "level_10";
    }
    else if (strcmp(mGUID, "{42A49D1B-D64A-419A-8D18-A947DF0996A4}") == 0) {
        return "level_11";
    }
    else if (strcmp(mGUID, "{10C05077-25FA-42CA-8D85-9D1A7A55CD8E}") == 0) {
        return "level_12";
    }
    else if (strcmp(mGUID, "{706F5ABB-04FD-4C95-A71E-802D59277329}") == 0) {
        return "level_13";
    }
    else if (strcmp(mGUID, "{84402A6C-C9DE-44F4-A025-A229A736FA27}") == 0) {
        return "level_14";
    }
    else if (strcmp(mGUID, "{69B70D35-6D43-45B2-98C6-FEC7A6D71CC2}") == 0) {
        return "level_15";
    }
    else if (strcmp(mGUID, "{1682115C-88F2-44B1-8DC1-0E28A000DEB2}") == 0) {
        return "level_16";
    }
    else if (strcmp(mGUID, "{D339DF52-9388-4615-933F-E6BB9E1FCD9F}") == 0) {
        return "level_17";
    }
    else if (strcmp(mGUID, "{FE60AB24-C91E-4BB0-A3E2-21256BE1627E}") == 0) {
        return "level_18";
    }
    else if (strcmp(mGUID, "{05500723-A9D7-4D74-9BD6-8199A864422A}") == 0) {
        return "level_19";
    }
    else if (strcmp(mGUID, "{2BFE62B4-D91A-4C9B-B81B-55EABB01C311}") == 0) {
        return "level_20";
    }
    //Intermediate Level GUIDS
    else if (strcmp(mGUID, "{AF781BFC-B941-4335-86C6-B7CE542AA4CE}") == 0) {
        return "level_21";
    }
    else if (strcmp(mGUID, "{8AC4C432-8A5C-4ADE-9B38-08B462EB1354}") == 0) {
        return "level_22";
    }
    else if (strcmp(mGUID, "{8F284E76-0C97-4D44-93A6-287DC35F4DD2}") == 0) {
        return "level_23";
    }
    else if (strcmp(mGUID, "{E0E23377-0DAD-4B2A-9F9F-7DAF1BD18CBF}") == 0) {
        return "level_24";
    }
    else if (strcmp(mGUID, "{BED8DF2C-4F41-4FE5-B253-27FBDEF3F68A}") == 0) {
        return "level_25";
    }
    else if (strcmp(mGUID, "{9648B38D-880B-4EAD-8443-E9DFBE17B273}") == 0) {
        return "level_26";
    }
    else if (strcmp(mGUID, "{129746F6-F695-4DCE-89A6-817AF582EA3A}") == 0) {
        return "level_27";
    }
    else if (strcmp(mGUID, "{8BAAA76E-ADF2-4337-BB89-8885D8E4188F}") == 0) {
        return "level_28";
    }
    else if (strcmp(mGUID, "{C375FCB5-1203-4BDC-AFDA-F8C22B463DA9}") == 0) {
        return "level_29";
    }
    else if (strcmp(mGUID, "{A356C0DA-9F6E-4A7E-9D69-E8B91EC013BB}") == 0) {
        return "level_30";
    }
    else if (strcmp(mGUID, "{05EFFBDF-75CA-4107-AB93-7865F4383107}") == 0) {
        return "level_31";
    }
    else if (strcmp(mGUID, "{5FDB0AE3-A9E5-45F2-B43B-0969E7809FA7}") == 0) {
        return "level_32";
    }
    else if (strcmp(mGUID, "{802D17DF-0168-4A17-B8A6-EF62D0ED5D7B}") == 0) {
        return "level_33";
    }
    else if (strcmp(mGUID, "{AE6F5499-652A-43A7-BADE-4561EFD2859A}") == 0) {
        return "level_34";
    }
    else if (strcmp(mGUID, "{62329829-CB12-403D-A0DD-CC6B5D2B70D0}") == 0) {
        return "level_35";
    }
    else if (strcmp(mGUID, "{8E02E49C-1465-473D-BE60-990AC25753CE}") == 0) {
        return "level_36";
    }
    else if (strcmp(mGUID, "{958557D1-641F-474C-BB27-AAA1E90925FD}") == 0) {
        return "level_37";
    }
    else if (strcmp(mGUID, "{40D18ABA-D319-4D37-ACDD-239DC8B9D0B8}") == 0) {
        return "level_38";
    }
    else if (strcmp(mGUID, "{435771D3-BC67-4B48-B353-1D8C280DD7CF}") == 0) {
        return "level_39";
    }
    else if (strcmp(mGUID, "{7A7BF863-D4E5-471D-91C3-B4DD7041AA4B}") == 0) {
        return "level_40";
    }
    //Advanced Level GUIDS
    //Multiplayer Level GUIDS
    else {
        return "game_icon";
    }
    
}