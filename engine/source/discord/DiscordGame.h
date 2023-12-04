#ifndef _DISCORDGAME_H_
#define _DISCORDGAME_H_

#include <iostream>
#include <fstream>
#include "discord.h"

#include "console/console.h"

//constexpr const char* AUTOSPLITTER_FILE_NAME = "autosplitter.txt";
constexpr U32 DISCORDSDK_BUF_SIZE = 512;

class DiscordGame
{
public:
    static void init();
    static void destroy();
    static DiscordGame* get();
    bool isActive() { return mActive; }
    //void sendData(const char* data);
    void update();

    void setStatus(const char* status) { mStatus = status; if (mStatus == nullptr) mStatus = ""; }
    void setDetails(const char* details) {mDetails = details; if (mDetails == nullptr) mDetails = ""; }
    //void setIcon(const char* icon, const char* iconText) { mIcon = icon; if (mIcon == nullptr) mIcon = ""; mIconText = iconText; if (mIconText == nullptr) mIconText = ""; }
private:
    DiscordGame();
    ~DiscordGame();
    static DiscordGame* smInstance;
    bool mActive;
    std::string mFilename;
    std::fstream mFile;
    discord::Core* mCore;
    discord::Activity mActivity;
    const char* mStatus;
    const char* mDetails;
    //const char* mIcon;
    //const char* mIconText;
};

#endif // _DISCORDGAME_H_