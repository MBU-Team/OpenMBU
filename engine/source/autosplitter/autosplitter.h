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
    U32 flags;          // Boolean flags. 32 is probably overkill but is good for future proofing
};

enum {
    FLAG_IS_LOADING     = (1 << 0),
    FLAG_LEVEL_STARTED  = (1 << 1),
    FLAG_LEVEL_FINISHED = (1 << 2),
    FLAG_EGG_FOUND      = (1 << 3),
    FLAG_QUIT_TO_MENU   = (1 << 4),
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
