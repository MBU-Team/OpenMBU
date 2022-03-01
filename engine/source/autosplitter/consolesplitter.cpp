#include <windows.h> 
#include <stdio.h> 

#include "consolesplitter.h"


Autosplitter *Autosplitter::smInstance = nullptr;

void Autosplitter::init()
{
    if (smInstance)
        return;

    smInstance = new Autosplitter();
}

void Autosplitter::destroy()
{
    if (!smInstance)
        return;

    delete smInstance;
}

Autosplitter *Autosplitter::get()
{
    if (!smInstance)
        init();

    return smInstance;
}

Autosplitter::Autosplitter()
{
    mActive = false;
    mPipe = CreateNamedPipeA(
        AUTOSPLITTER_PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        AUTOSPLITTER_BUF_SIZE,
        AUTOSPLITTER_BUF_SIZE,
        0,
        nullptr);
    if (mPipe == INVALID_HANDLE_VALUE)
    {
        Con::errorf("CreateNamePipe failed, GLE=%d.", GetLastError());
        Con::errorf("Autosplitter is disabled.");
        return;
    }
    bool connected = ConnectNamedPipe(mPipe, nullptr);
    if (!connected || GetLastError() != ERROR_PIPE_CONNECTED)
    {
        Con::errorf("Client not connected to autosplitter");
        Con::errorf("Autosplitter is disabled.");
        return;
    }
    Con::printf("Autosplitter Initialized");
    mActive = true;
}

Autosplitter::~Autosplitter()
{
    CloseHandle(mPipe);
}

bool Autosplitter::sendData(const char *data)
{
    DWORD bytesWritten;
    char buf[AUTOSPLITTER_BUF_SIZE];
    sprintf(buf, "%s\n", data);
    U32 length = strlen(buf) + 1;
    bool fSuccess = WriteFile(mPipe, buf, length, &bytesWritten, nullptr);

    return (fSuccess && bytesWritten == length);
}

ConsoleFunction(sendAutosplitterData, void, 2, 2, "")
{
    Autosplitter *autosplitter = Autosplitter::get();
    if (!autosplitter->isActive())
        return;

    Con::printf("Sending autosplitter message '%s'", argv[1]);

    bool success = autosplitter->sendData(argv[1]);

    if (!success)
    {
        Con::warnf("Did not successfully send message to autosplitter");
    }
}
