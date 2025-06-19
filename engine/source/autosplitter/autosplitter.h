#ifndef _AUTOSPLITTER_H_
#define _AUTOSPLITTER_H_

#include <fstream>

#include "platform/types.h"

constexpr const char *AUTOSPLITTER_FILE_NAME = "autosplitter.txt";
constexpr U32 AUTOSPLITTER_BUF_SIZE = 512;

// This data can be scanned for in memory and used to drive autosplitters more precisely than via a log file
struct AutosplitterData {
    char signature[16]; // Header set to "OMBU_ASR_abcdef", entry point for memory scan
    S32 currentLevel;   // The level index of the current loading/loaded level
    U8 isLoading;     // When a loading screen is active
    U8 levelStarted;  // When a level is started from the menu screen
    U8 levelFinished; // When the finish is entered
    U8 eggFound;      // When an easter egg is collected
    U8 quitToMenu;    // When the player quits to menu 
};

class Autosplitter
{
public:
    static void init();
    static void destroy();
    static Autosplitter *get();
    bool isActive() { return mActive; }
    void sendData(const char *data);
    AutosplitterData data;
private:
    Autosplitter();
    ~Autosplitter();
    static Autosplitter *smInstance;
    bool mActive;
    std::string mFilename;
    std::fstream mFile;
};

#endif // _AUTOSPLITTER_H_
