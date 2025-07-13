#include <cstdio> 

#include "autosplitter.h"
#include "console/console.h"
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
    // Initialize AutosplitterData memory chunk
    dMemset(&data, 0, sizeof(data));
    sprintf(data.signature, "OMBU_ASR_");
    // We add the "abcdef" procedurally to guarantee that the sigscan only finds this buffer, rather than potentially
    // finding a full "OMBU_ASR_abcdef" string somewhere else in memory
    for (int i = 0; i < 6; i++)
    {
        data.signature[i + 9] = 'a' + i;
    }
    data.currentLevel = -1;
    // The rest is all zeros thanks to the memset, which is what we want.

    // Initialize autosplitter file
    mActive = false;
    mFilename = Platform::getPrefsPath(AUTOSPLITTER_FILE_NAME);
    mFile.open(mFilename, std::ios_base::app);
    if (!mFile.is_open())
    {
        Con::errorf("Failed to open autosplitter file %s.", mFilename.c_str());
        Con::errorf("Autosplitter file is disabled. Autosplitter memory will still work.");
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

ConsoleFunction(autosplitterSetLevel, void, 2, 2, "autosplitterSetLevel(level)")
{
    Autosplitter *autosplitter = Autosplitter::get();
    autosplitter->data.currentLevel = atoi(argv[1]);
}

ConsoleFunction(autosplitterSetIsLoading, void, 2, 2, "autosplitterSetIsLoading(isLoading)")
{
    Autosplitter *autosplitter = Autosplitter::get();
    if (dStrcmp(argv[1], "false") == 0 || dStrcmp(argv[1], "0") == 0)
        autosplitter->data.flags &= ~FLAG_IS_LOADING;
    else
        autosplitter->data.flags |= FLAG_IS_LOADING;
}

ConsoleFunction(autosplitterSetLevelStarted, void, 2, 2, "autosplitterSetLevelStarted(levelStarted)")
{
    Autosplitter *autosplitter = Autosplitter::get();
    if (dStrcmp(argv[1], "false") == 0 || dStrcmp(argv[1], "0") == 0)
        autosplitter->data.flags &= ~FLAG_LEVEL_STARTED;
    else
        autosplitter->data.flags |= FLAG_LEVEL_STARTED;
}

ConsoleFunction(autosplitterSetLevelFinished, void, 2, 2, "autosplitterSetLevelFinished(levelFinished)")
{
    Autosplitter *autosplitter = Autosplitter::get();
    if (dStrcmp(argv[1], "false") == 0 || dStrcmp(argv[1], "0") == 0)
        autosplitter->data.flags &= ~FLAG_LEVEL_FINISHED;
    else
        autosplitter->data.flags |= FLAG_LEVEL_FINISHED;
}

ConsoleFunction(autosplitterSetEggFound, void, 2, 2, "autosplitterSetEggFound(eggFound)")
{
    Autosplitter *autosplitter = Autosplitter::get();
    if (dStrcmp(argv[1], "false") == 0 || dStrcmp(argv[1], "0") == 0)
        autosplitter->data.flags &= ~FLAG_EGG_FOUND;
    else
        autosplitter->data.flags |= FLAG_EGG_FOUND;
}

ConsoleFunction(autosplitterSetQuitToMenu, void, 2, 2, "autosplitterSetQuitToMenu(quitToMenu)")
{
    Autosplitter *autosplitter = Autosplitter::get();
    if (dStrcmp(argv[1], "false") == 0 || dStrcmp(argv[1], "0") == 0)
        autosplitter->data.flags &= ~FLAG_QUIT_TO_MENU;
    else
        autosplitter->data.flags |= FLAG_QUIT_TO_MENU;
}
