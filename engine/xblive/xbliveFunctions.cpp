#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"

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

    LangType lang = Platform::getSystemLanguage();

    const char* language;

    switch (lang)
    {
        case LANGTYPE_CHINESE:
            language = "chinese";
            break;
        case LANGTYPE_GERMAN:
            language = "german";
            break;
        case LANGTYPE_ENGLISH:
            language = "english";
            break;
        case LANGTYPE_SPANISH:
            language = "spanish";
            break;
        case LANGTYPE_FRENCH:
            language = "french";
            break;
        case LANGTYPE_ITALIAN:
            language = "italian";
            break;
        case LANGTYPE_JAPANESE:
            language = "japanese";
            break;
        case LANGTYPE_KOREAN:
            language = "korean";
            break;
        case LANGTYPE_PORTUGUESE:
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