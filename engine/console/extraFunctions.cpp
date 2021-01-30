#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"

ConsoleFunction(loadZip, void, 2, 2, "(zipName)")
{
    argc;

    const char* zipName = argv[2];
    Con::executef(3, "onZipLoaded", zipName, Con::getIntArg(0));
}

ConsoleFunction(isResourceBGLoaded, bool, 2, 2, "()")
{
    argc;
    return true;
}

ConsoleFunction(isDemoLaunch, bool, 1, 1, "()")
{
    argc;
    return false;
}

ConsoleFunction(getLanguage, const char*, 1, 1, "()")
{
    argc;
    char* ret = Con::getReturnBuffer(1024);
    dSprintf(ret, 1024, "%s", "english");
    return ret;
}

ConsoleFunction(XBLiveIsSignedIn, bool, 2, 2, "(port)")
{
    argc;
    return true;
}