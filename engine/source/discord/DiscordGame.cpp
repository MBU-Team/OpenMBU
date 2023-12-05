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
    //mIcon = nullptr;
    //mIconText = nullptr;

    mCore = {};

    //auto result = discord::Core::Create(846933806711046144, DiscordCreateFlags_Default, &core);
    auto result = discord::Core::Create(1181357649644769300, DiscordCreateFlags_NoRequireDiscord, &mCore);

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

        mActivity.GetAssets().SetLargeImage("game_icon");
        mActivity.GetAssets().SetLargeText("OpenMBU");
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