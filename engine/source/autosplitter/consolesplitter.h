#ifndef _CONSOLESPLITTER_H_
#define _CONSOLESPLITTER_H_

#include <Windows.h>

#include "console/console.h"

constexpr const char *AUTOSPLITTER_PIPE_NAME = "\\\\.\\pipe\\mbuautosplitter";
constexpr U32 AUTOSPLITTER_BUF_SIZE = 512;

class Autosplitter
{
public:
    static void init();
    static void destroy();
    static Autosplitter *get();
    bool isActive() { return mActive; }
    bool sendData(const char *data);
private:
    Autosplitter();
    ~Autosplitter();
    static Autosplitter *smInstance;
    bool mActive;
    HANDLE mPipe;
};

#endif // _CONSOLESPLITTER_H_
