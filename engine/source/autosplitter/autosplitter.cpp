#include <cstdio> 

#include "autosplitter.h"
#include "platform/platform.h"


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
    smInstance = nullptr;
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
    mFilename = Platform::getPrefsPath(AUTOSPLITTER_FILE_NAME);
    mFile.open(mFilename, std::ios_base::app);
    if (!mFile.is_open())
    {
        Con::errorf("Failed to open autosplitter file %s.", mFilename.c_str());
        Con::errorf("Autosplitter is disabled.");
        return;
    }
    Con::printf("Autosplitter Initialized to file %s", mFilename.c_str());
    mActive = true;
}

Autosplitter::~Autosplitter()
{
    mFile.close();
    std::remove(mFilename.c_str());
}

void Autosplitter::sendData(const char *data)
{
    if (!mActive)
        return;

    mFile << data << std::endl;
}

ConsoleFunction(sendAutosplitterData, void, 2, 2, "")
{
    Autosplitter *autosplitter = Autosplitter::get();
    if (!autosplitter->isActive())
        return;

    Con::printf("Sending autosplitter message '%s'", argv[1]);

    autosplitter->sendData(argv[1]);
}
