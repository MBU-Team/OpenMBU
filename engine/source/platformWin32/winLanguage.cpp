//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"

#ifdef XB_LIVE
LangType Platform::getSystemLanguage()
{
    //LCID id = GetUserDefaultLCID();
    LANGID id = GetUserDefaultUILanguage();

    U8 lang = (U8)id;

    // Information obtained from:
    // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/70feba9f-294e-491e-b6eb-56532684c37f
    //
    // PDF Used (14.0)
    // https://winprotocoldoc.blob.core.windows.net/productionwindowsarchives/MS-LCID/%5bMS-LCID%5d.pdf

    switch (lang)
    {
        case 0x04: // zh
            return LANGTYPE_CHINESE;
        case 0x07: // de
            return LANGTYPE_GERMAN;
        case 0x09: // en
            return LANGTYPE_ENGLISH;
        case 0x0A: // es
            return LANGTYPE_SPANISH;
        case 0x0C: // fr
            return LANGTYPE_FRENCH;
        case 0x10: // it
            return LANGTYPE_ITALIAN;
        case 0x11: // ja
            return LANGTYPE_JAPANESE;
        case 0x12: // ko
            return LANGTYPE_KOREAN;
        case 0x15: // pl
            return LANGTYPE_POLISH;
        case 0x16: // pt
            return LANGTYPE_PORTUGUESE;
    }

    return LANGTYPE_UNSUPPORTED;
}
#endif
