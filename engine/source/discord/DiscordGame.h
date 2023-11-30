#ifndef _DISCORDGAME_H_
#define _DISCORDGAME_H_

#include <iostream>
#include <fstream>

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
private:
    DiscordGame();
    ~DiscordGame();
    static DiscordGame* smInstance;
    bool mActive;
    std::string mFilename;
    std::fstream mFile;
};

#endif // _DISCORDGAME_H_