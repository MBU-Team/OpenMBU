#ifndef _DISCORDGAME_H_
#define _DISCORDGAME_H_

#include <iostream>
#include <fstream>
#include <unordered_map>

#include "core/stringTable.h"
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
    const char* ProcessLevel(StringTableEntry guid);
    void respondJoinRequest(const char* userId, int value);

    void setStatus(const char* status) 
    { 
        mStatus = status; 
        if (mStatus == nullptr) 
            mStatus = ""; 
        notifyParamsChange();

    }
    void setDetails(const char* details) 
    { 
        mDetails = details; 
        if (mDetails == nullptr) 
            mDetails = ""; 
        notifyParamsChange();
    }
    void setLevel(const char* guid) 
    {
        mGUID = guid; 
        if (mGUID == nullptr) 
            mGUID = ""; 
        notifyParamsChange();
    }
    void setSmallImageKey(const char* sImgKey) 
    {
        mImgSm = sImgKey; 
        if (mImgSm == nullptr) 
            mImgSm = "loading_icon"; 
        notifyParamsChange();
    }
    void setPartyDetails(int count, int maxp, StringTableEntry joinSecret, StringTableEntry partyId)
    {
        mPlayerCount = count;
        mMaxPlayers = maxp;
        mJoinSecret = joinSecret;
        mPartyId = partyId;
        notifyParamsChange();
    }
    void updatePartyDetails(int count, int maxp)
    {
        mPlayerCount = count;
        mMaxPlayers = maxp;
        notifyParamsChange();
    }
    void setTimer(uint64_t start, uint64_t stop = 0)
    {
        mStartTime = start;
        mStopTime = stop;
        notifyParamsChange();
    }
    void setUsername(const char* uname)
    {
        mUsername = uname;
    }
    StringTableEntry getUsername(int charLimit)
    {
        char* buf = new char[charLimit + 1];
        int minLength = fmin(charLimit, mUsername.length());
        for (int i = 0; i < minLength; i++)
            buf[i] = mUsername[i];
        buf[minLength] = '\0';
        StringTableEntry ste = StringTable->insert(buf, true);
        delete[] buf;
        return ste;
    }
    void setActive(bool a)
    {
        mActive = a;
    }
    void notifyParamsChange() { mChanged = true; }
    //void setIcon(const char* icon, const char* iconText) { mIcon = icon; if (mIcon == nullptr) mIcon = ""; mIconText = iconText; if (mIconText == nullptr) mIconText = ""; }
private:
    DiscordGame();
    ~DiscordGame();
    static DiscordGame* smInstance;
    bool mActive;
    bool mChanged;
    std::unordered_map<const char*, const char*> mGuidLookup;
    const char* mStatus;
    const char* mDetails;
    const char* mGUID;
    const char* mImgSm;
    const char* mLargeImageKey;
    int mMaxPlayers;
    int mPlayerCount;
    uint64_t mStartTime;
    uint64_t mStopTime;
    StringTableEntry mJoinSecret;
    StringTableEntry mPartyId;
    std::string mUsername;
    //const char* mIcon;
    //const char* mIconText;
};

#endif // _DISCORDGAME_H_