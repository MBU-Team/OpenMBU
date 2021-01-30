//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/gfxCardProfile.h"
#include "core/fileStream.h"
#include "console/console.h"
#include "platform/platform.h"
#include <ctype.h>

void GFXCardProfiler::loadProfileScript(const char* aScriptName)
{
    FileStream str;

    char scriptName[512];
    const char* profilePath = Con::getVariable("$Pref::Video::ProfilePath");
    dStrcpy(scriptName, (dStrlen(profilePath) > 0) ? profilePath : "profile");
    dStrcat(scriptName, "/");
    dStrcat(scriptName, aScriptName);

    if (!str.open(scriptName, FileStream::Read))
    {
        Con::warnf("      - No card profile %s exists", scriptName);
        return;
    }

    Con::printf("      - Loading card profile %s", scriptName);

    U32 size = str.getStreamSize();
    char* script = new char[size + 1];

    str.read(size, script);
    str.close();

    script[size] = 0;
    Con::executef(2, "eval", script);
    delete[] script;
}

void GFXCardProfiler::loadProfileScripts(const char* render, const char* vendor, const char* card, const char* version)
{
    char script[512] = "";

    dSprintf(script, 512, "%s.cs", render);
    loadProfileScript(script);

    dSprintf(script, 512, "%s.%s.cs", render, vendor);
    loadProfileScript(script);

    dSprintf(script, 512, "%s.%s.%s.cs", render, vendor, card);
    loadProfileScript(script);

    dSprintf(script, 512, "%s.%s.%s.%s.cs", render, vendor, card, version);
    loadProfileScript(script);
}

GFXCardProfiler::GFXCardProfiler()
{
}

GFXCardProfiler::~GFXCardProfiler()
{
    mCapDictionary.clear();
}

char* GFXCardProfiler::strippedString(const char* string)
{
    // Allocate the new buffer.
    char* res = new char[dStrlen(string) + 1];

    // And fill it with the stripped string...
    const char* a = string;
    char* b = res;
    while (*a)
    {
        if (isalnum(*a))
        {
            *b = *a;
            b++;
        }
        a++;
    }
    *b = '\0';

    return res;
}

void GFXCardProfiler::init()
{
    // Spew a bit...
    Con::printf("Initializing GFXCardProfiler (%s)", getRendererString());
    Con::printf("   o Vendor : '%s'", getVendorString());
    Con::printf("   o Card   : '%s'", getCardString());
    Con::printf("   o Version: '%s'", getVersionString());

    // Do card-specific setup...
    Con::printf("   - Scanning card capabilities...");

    setupCardCapabilities();

    // And finally, load stuff up...
    char* render, * vendor, * card, * version;
    render = strippedString(getRendererString());
    vendor = strippedString(getVendorString());
    card = strippedString(getCardString());
    version = strippedString(getVersionString());

    Con::printf("   - Loading card profiles...");
    loadProfileScripts(render, vendor, card, version);

    // Clean up.
    delete[]render;
    delete[]vendor;
    delete[]card;
    delete[]version;
}

U32 GFXCardProfiler::queryProfile(const char* cap)
{
    U32 res;
    if (_queryCardCap(cap, res))
        return res;

    if (mCapDictionary.contains(cap))
        return mCapDictionary[cap];
    else
    {
        Con::errorf("GFXCardProfiler (%s) - Unknown capability '%s'.", getRendererString(), cap);
        return 0;
    }
}

U32 GFXCardProfiler::queryProfile(const char* cap, U32 defaultValue)
{
    U32 res;
    if (_queryCardCap(cap, res))
        return res;

    if (mCapDictionary.contains(cap))
        return mCapDictionary[cap];
    else
        return defaultValue;
}

void GFXCardProfiler::setCapability(const char* cap, U32 value)
{
    // Check for dups.
    if (mCapDictionary.contains(cap))
    {
        Con::warnf("GFXCardProfiler (%s) - Setting capability '%s' multiple times.", getRendererString(), cap);
        mCapDictionary[cap] = value;
        return;
    }

    // Insert value as necessary.
    Con::warnf("GFXCardProfiler (%s) - Setting capability '%s' to %d.", getRendererString(), cap, value);
    mCapDictionary.insert(cap, value);
}

ConsoleMethodGroupBegin(GFXCardProfiler, Core, "Functions relating to the card profiler functionality.");

ConsoleStaticMethod(GFXCardProfiler, getVersion, const char*, 1, 1, "() - Returns the driver version (59.72).")
{
    return GFX->getCardProfiler()->getVersionString();
}

ConsoleStaticMethod(GFXCardProfiler, getCard, const char*, 1, 1, "() - Returns the card name (GeforceFX 5950 Ultra).")
{
    return GFX->getCardProfiler()->getCardString();
}

ConsoleStaticMethod(GFXCardProfiler, getVendor, const char*, 1, 1, "() - Returns the vendor name (nVidia, ATI).")
{
    return GFX->getCardProfiler()->getVendorString();
}

ConsoleStaticMethod(GFXCardProfiler, getRenderer, const char*, 1, 1, "() - Returns the renderer name (D3D9, for instance).")
{
    return GFX->getCardProfiler()->getRendererString();
}

ConsoleStaticMethod(GFXCardProfiler, setCapability, void, 3, 3, "setCapability(name, true/false) - Set a specific card capability.")
{
    argc;
    bool val = dAtob(argv[2]);
    GFX->getCardProfiler()->setCapability(argv[1], val);
    return;
}


ConsoleMethodGroupEnd(GFXCardProfiler, Core);
