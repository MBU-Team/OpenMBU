#ifndef _AUTOSPLITTER_H_
#define _AUTOSPLITTER_H_

#include <iostream>
#include <fstream>

#include "console/console.h"

constexpr const char *AUTOSPLITTER_FILE_NAME = "autosplitter.txt";
constexpr U32 AUTOSPLITTER_BUF_SIZE = 512;

class Autosplitter
{
public:
    static void init();
    static void destroy();
    static Autosplitter *get();
    bool isActive() { return mActive; }
    void sendData(const char *data);
private:
    Autosplitter();
    ~Autosplitter();
    static Autosplitter *smInstance;
    bool mActive;
    std::string mFilename;
    std::fstream mFile;
};

#endif // _AUTOSPLITTER_H_
