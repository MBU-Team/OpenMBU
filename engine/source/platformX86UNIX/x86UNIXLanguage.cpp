#include "platformX86UNIX/platformX86UNIX.h"

#ifdef XB_LIVE
LangType Platform::getSystemLanguage()
{
    // TODO: Support other languages
    return LANGTYPE_ENGLISH;
}
#endif