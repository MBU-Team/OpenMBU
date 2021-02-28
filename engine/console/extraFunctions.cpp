#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"

#include <Windows.h> // TODO: Move to platform specific code

ConsoleFunction(loadZip, void, 2, 2, "(zipName)")
{
    argc;

    const char* zipName = argv[2];
    Con::executef(3, "onZipLoaded", zipName, Con::getIntArg(0));
}

ConsoleFunction(isResourceBGLoaded, bool, 2, 2, "(file)")
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

    // TODO: Move to platform specific code
    //LCID id = GetUserDefaultLCID();
    LANGID id = GetUserDefaultUILanguage();

    U8 lang = (U8)id;

    const char* language;

    // Information obtained from:
    // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/70feba9f-294e-491e-b6eb-56532684c37f
    //
    // PDF Used (14.0)
    // https://winprotocoldoc.blob.core.windows.net/productionwindowsarchives/MS-LCID/%5bMS-LCID%5d.pdf

    switch (lang)
    {
        case 0x04: // zh
            language = "chinese";
            break;
        case 0x07: // de
            language = "german";
            break;
        case 0x09: // en
            language = "english";
            break;
        case 0x0A: // es
            language = "spanish";
            break;
        case 0x0C: // fr
            language = "french";
            break;
        case 0x10: // it
            language = "italian";
            break;
        case 0x11: // ja
            language = "japanese";
            break;
        case 0x12: // ko
            language = "korean";
            break;
        case 0x16: // pt
            language = "portuguese";
            break;
        default:
            language = "english";
            break;
    }

    dSprintf(ret, 1024, "%s", language);
    return ret;
}

ConsoleFunction(XBLiveIsSignedIn, bool, 1, 2, "([port])")
{
    argc;
    return true;
}

ConsoleFunction(XBLiveGetUserName, const char*, 1, 1, "()")
{
    argc;
    char* ret = Con::getReturnBuffer(1024);
    dSprintf(ret, 1024, "%s", "Alex");
    return ret;
}

ConsoleFunction(XBLiveGetUserId, S32, 1, 1, "()")
{
    argc;

    return 1;
}