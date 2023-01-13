//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#include "gfx/gfxInit.h"
#include "gfx/Null/gfxNullDevice.h"

inline static void _GFXInitReportAdapters(Vector<GFXAdapter*>&);
inline static void _GFXInitGetInitialRes(GFXVideoMode&, const Point2I&);

//-----------------------------------------------------------------------------

inline static void _GFXInitReportAdapters(Vector<GFXAdapter*> &adapters)
{
    for (U32 i = 0; i < adapters.size(); i++)
    {
        switch (adapters[i]->mType)
        {
            case Direct3D9:
                Con::printf("Direct 3D (version 9.x) device found");
                break;
            case OpenGL:
                Con::printf("OpenGL device found");
                break;
            case NullDevice:
                Con::printf("Null device found");
                break;
            case Direct3D8:
                Con::printf("Direct 3D (version 8.1) device found");
                break;
            default :
                Con::printf("Unknown device found");
                break;
        }
    }
}

inline static void _GFXInitGetInitialRes(GFXVideoMode &vm, const Point2I &initialSize)
{
    const U32 kDefaultWindowSizeX = 800;
    const U32 kDefaultWindowSizeY = 600;
    const bool kDefaultFullscreen = false;
    const bool kDefaultBorderless = false;
    const U32 kDefaultBitDepth = 32;
    const U32 kDefaultRefreshRate = 60;

    // cache the desktop size of the main screen
    GFXVideoMode desktopVm = GFXInit::getDesktopResolution();

    // load pref variables, properly choose windowed / fullscreen
    const char * resString = Con::getVariable("$pref::Video::mode");

    // Set defaults into the video mode, then have it parse the user string.
    vm.resolution.x = kDefaultWindowSizeX;
    vm.resolution.y = kDefaultWindowSizeY;
    vm.fullScreen   = kDefaultFullscreen;
    vm.bitDepth     = kDefaultBitDepth;
    vm.refreshRate  = kDefaultRefreshRate;
    vm.borderless   = kDefaultBorderless;
    //vm.wideScreen = false;

    vm.parseFromString(resString);
}

void GFXInit::init()
{
    // init only once.
    static bool doneOnce = false;
    if(doneOnce)
        return;
    doneOnce = true;

    Con::printf( "GFX Init:" );

    //find our adapters
    Vector<GFXAdapter*> adapters;
    GFXInit::enumerateAdapters();
    GFXInit::getAdapters(&adapters);

    if(!adapters.size())
        Con::errorf("Could not find a display adapter");

    //loop through and tell the user what kind of adapters we found
    _GFXInitReportAdapters(adapters);
    Con::printf( "" );
}

GFXAdapter* GFXInit::getAdapterOfType( GFXAdapterType type )
{
    GFXAdapter* adapter = NULL;
    for( U32 i = 0; i < smAdapters.size(); i++ )
    {
        if( smAdapters[i]->mType == type )
        {
            adapter = smAdapters[i];
            break;
        }
    }
    return adapter;
}

GFXAdapter* GFXInit::chooseAdapter( GFXAdapterType type)
{
    GFXAdapter* adapter = GFXInit::getAdapterOfType(type);

    if(!adapter && type != OpenGL)
        adapter = GFXInit::getAdapterOfType(OpenGL);

    //if(!adapter && type != Direct3D9_360)
    //    adapter = GFXInit::getAdapterOfType(Direct3D9_360);

    if(!adapter && type != Direct3D8)
        adapter = GFXInit::getAdapterOfType(Direct3D8);

    if(!adapter)
    {
        Con::errorf("Neither the Direct3D or OpenGL renderers seem to be available. Trying NullDevice.");
        adapter = GFXInit::getAdapterOfType(NullDevice);
    }

    AssertISV( adapter, "There is no rendering device available whatsoever.");
    return adapter;
}

const char* GFXInit::getAdapterNameFromType(GFXAdapterType type)
{
    static const char* _names[] = { "OpenGL", "D3D9", "D3D8", "NullDevice", "Xenon" };

    if( type < 0 || type >= GFXAdapterType_Count )
    {
#ifdef TORQUE_OS_MAC
        Con::errorf( "GFXInit::getAdapterNameFromType - Invalid renderer type, defaulting to OpenGL" );
      return _names[OpenGL];
#else
#ifdef TORQUE_OS_XENON
        Con::errorf( "GFXInit::getAdapterNameFromType - Invalid renderer type, defaulting to Direct3D9_360" );
         return _names[Direct3D9_360];
#else
        Con::errorf( "GFXInit::getAdapterNameFromType - Invalid renderer type, defaulting to Direct3D9" );
        return _names[Direct3D9];
#endif
#endif
    }

    return _names[type];
}

GFXAdapterType GFXInit::getAdapterTypeFromName(const char* name)
{
    S32 ret = -1;
    for(S32 i = 0; i < GFXAdapterType_Count; i++)
    {
        if( !dStricmp( getAdapterNameFromType((GFXAdapterType)i), name ) )
            ret = i;
    }

    if( ret == -1 )
#ifdef TORQUE_OS_MAC
        ret = OpenGL;
#else
#ifdef TORQUE_OS_XENON
        ret = Direct3D9_360;
#else
        ret = Direct3D9;
#endif
#endif

    return (GFXAdapterType)ret;
}

GFXAdapter *GFXInit::getBestAdapterChoice()
{
    // Get the user's preference for device...
    char* renderer = NULL;
    if ( Con::getBoolVariable("Server::Dedicated") )
        renderer = "NullDevice";
    else
        renderer   = (char*)Con::getVariable("$pref::Video::displayDevice");
    GFXAdapterType adapterType = getAdapterTypeFromName(renderer);
    GFXAdapter     *adapter    = chooseAdapter(adapterType);

    // Did they have one? Return it.
    if(adapter)
        return adapter;

    // Didn't have one. So make it up. Find the highest SM available. Prefer
    // D3D to GL because if we have a D3D device at all we're on windows,
    // and in an unknown situation on Windows D3D is probably the safest bet.
    //
    // If D3D is unavailable, we're not on windows, so GL is de facto the
    // best choice!
    F32 highestSM9 = 0.f, highestSMGL = 0.f;
    GFXAdapter  *foundAdapter8 = NULL, *foundAdapter9 = NULL,
        *foundAdapterGL = NULL;

    for(S32 i=0; i<smAdapters.size(); i++)
    {
        GFXAdapter *a = smAdapters[i];
        switch(a->mType)
        {
            case Direct3D9:
                if(a->mShaderModel > highestSM9)
                {
                    highestSM9 = a->mShaderModel;
                    foundAdapter9 = a;
                }
                break;

            case OpenGL:
                if(a->mShaderModel > highestSMGL)
                {
                    highestSMGL = a->mShaderModel;
                    foundAdapterGL = a;
                }
                break;

            case Direct3D8:
                if(!foundAdapter8)
                    foundAdapter8 = a;
                break;
        }
    }

    // Return best found in order DX9, GL, DX8.
    if(foundAdapter9)
        return foundAdapter9;

    if(foundAdapterGL)
        return foundAdapterGL;

    if(foundAdapter8)
        return foundAdapter8;

    // Uh oh - we didn't find anything. Grab whatever we can that's not Null...
    for(S32 i=0; i<smAdapters.size(); i++)
        if(smAdapters[i]->mType != NullDevice)
            return smAdapters[i];

    // Dare we return a null device? No. Just return NULL.
    return NULL;
}

GFXVideoMode GFXInit::getInitialVideoMode()
{
    GFXVideoMode vm;
    _GFXInitGetInitialRes(vm, Point2I(800,600));
    return vm;
}

S32 GFXInit::getAdapterCount()
{
    return smAdapters.size();
}

//-----------------------------------------------------------------------------
ConsoleFunction( getDesktopResolution, const char*, 1, 1, "Get the width, height, and bitdepth of the screen.")
{
    static char resBuf[16];
    GFXVideoMode res = GFXInit::getDesktopResolution();
    dSprintf( resBuf, sizeof(resBuf), "%d %d %d",
              res.resolution.x, res.resolution.y, res.bitDepth );
    return( resBuf );
}

ConsoleStaticMethod( GFXInit, getAdapterCount, S32, 1, 1, "() Return the number of adapters available.")
{
    return GFXInit::getAdapterCount();
}

ConsoleStaticMethod( GFXInit, getAdapterName, const char *, 2, 2, "(int id) Returns the name of a given adapter.")
{
    Vector<GFXAdapter*> adapters;
    GFXInit::getAdapters(&adapters);

    S32 idx = dAtoi(argv[1]);

    if(idx >= adapters.size())
    {
        Con::errorf("GFXInit::getAdapterName - out of range adapter index.");
        return "";
    }

    return StringTable->insert(adapters[idx]->mName);
}

ConsoleStaticMethod( GFXInit, getAdapterType, const char *, 2, 2, "(int id) Returns the type (D3D9, D3D8, GL, Null) of a given adapter.")
{
    Vector<GFXAdapter*> adapters;
    GFXInit::getAdapters(&adapters);

    S32 idx = dAtoi(argv[1]);

    if(idx >= adapters.size())
    {
        Con::errorf("GFXInit::getAdapterType - out of range adapter index.");
        return "";
    }

    return StringTable->insert(GFXInit::getAdapterNameFromType(adapters[idx]->mType));
}

ConsoleStaticMethod( GFXInit, getAdapterShaderModel, F32, 2, 2, "(int id) Returns the SM supported by a given adapter.")
{
    Vector<GFXAdapter*> adapters;
    GFXInit::getAdapters(&adapters);

    S32 idx = dAtoi(argv[1]);

    if(idx >= adapters.size())
    {
        Con::errorf("GFXInit::getAdapterShaderModel - out of range adapter index.");
        return -1.f;
    }

    return adapters[idx]->mShaderModel;
}

ConsoleStaticMethod( GFXInit, getDefaultAdapterIndex, S32, 1, 1, "() Returns the index of the adapter we'll be starting up with.")
{
    // Get the chosen adapter, and locate it in the list. (Kind of silly.)
    GFXAdapter *a = GFXInit::getBestAdapterChoice();

    Vector<GFXAdapter*> adapters;
    for(S32 i=0; i<adapters.size(); i++)
        if(adapters[i]->mIndex == a->mIndex && adapters[i]->mType == a->mType)
            return i;

    Con::warnf("GFXInit::getDefaultAdapterIndex - didn't find the chosen adapter in the adapter list!");
    return -1;
}

ConsoleStaticMethod(GFXInit, getAdapterModeCount, S32, 2, 2,
                    "(int id)\n"
                    "Gets the number of modes available on the specified adapter.\n\n"
                    "\\param id Index of the adapter to get data from.\n"
                    "\\return (int) The number of video modes supported by the adapter, or -1 if the given adapter was not found.")
{
    Vector<GFXAdapter*> adapters;
    GFXInit::getAdapters(&adapters);

    S32 idx = dAtoi(argv[1]);

    if(idx >= adapters.size())
    {
        Con::errorf("GFXInit::getAdapterModeCount - You specified an out of range adapter index of %d. Please specify an index in the range [0, %d).", idx, idx >= adapters.size());
        return -1;
    }

    return adapters[idx]->mAvailableModes.size();
}

ConsoleStaticMethod(GFXInit, getAdapterMode, const char *, 3, 3,
                    "(int id, int modeId)\n"
                    "Gets information on the specified adapter and mode.\n\n"
                    "\\param id Index of the adapter to get data from.\n"
                    "\\param modeId Index of the mode to get data from.\n"
                    "\\return (string) A video mode string given an adapter and mode index. See GuiCanvas.getVideoMode()")
{
    Vector<GFXAdapter*> adapters;
    GFXInit::getAdapters(&adapters);

    S32 adapIdx = dAtoi(argv[1]);
    if((adapIdx < 0) || (adapIdx >= adapters.size()))
    {
        Con::errorf("GFXInit::getAdapterMode - You specified an out of range adapter index of %d. Please specify an index in the range [0, %d).", adapIdx, adapIdx >= adapters.size());
        return "";
    }

    S32 modeIdx = dAtoi(argv[2]);
    if((modeIdx < 0) || (modeIdx >= adapters[adapIdx]->mAvailableModes.size()))
    {
        Con::errorf("GFXInit::getAdapterMode - You requested an out of range mode index of %d. Please specify an index in the range [0, %d).", modeIdx, adapters[adapIdx]->mAvailableModes.size());
        return "";
    }

    GFXVideoMode vm = adapters[adapIdx]->mAvailableModes[modeIdx];

    // Format and return to console.
    char* buf = Con::getReturnBuffer(vm.toString());
    return buf;
}
