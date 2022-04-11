//-----------------------------------------------------------------------------
// Torque Shader Engine
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXInit_H_
#define _GFXInit_H_

#include "core/tVector.h"
#include "gfx/gfxDevice.h"

/// Interface for tracking GFX adapters and initializing them into devices.
/// @note Implement this class per platform.
/// @note This is just a class so it can be friends with GFXDevice)
class GFXInit
{
private:
    /// List of known adapters.
    static Vector<GFXAdapter*> smAdapters;

public:

    /// Prepares the adapter list.
    static void init();

    /// Creates a GFXDevice based on an adapter from the
    /// enumerateAdapters method.
    ///
    /// @param   adapter   Graphics adapter to create device
    static GFXDevice *createDevice( GFXAdapter *adapter );

    /// Enumerate all the graphics adapters on the system
    static void enumerateAdapters();

    /// Get the enumerated adapters.  Should only call this after
    /// a call to enumerateAdapters.
    static void getAdapters( Vector<GFXAdapter*> *adapters );

    /// Get the number of available adapters.
    static S32 getAdapterCount();

    /// Chooses a suitable GFXAdapter, based on type, preferences, and fallbacks.
    /// If the requested type is omitted, we use the prefs value.
    /// If the requested type isn't found, we use fallbacks: OpenGL, NullDevice
    /// This method never returns NULL.
    static GFXAdapter *chooseAdapter( GFXAdapterType type);

    /// Gets the first adapter of the requested type from the list of enumerated
    /// adapters. Should only call this after a call to enumerateAdapters.
    static GFXAdapter *getAdapterOfType( GFXAdapterType type );

    /// Converts a GFXAdapterType to a string name. Useful for writing out prefs
    static const char *getAdapterNameFromType( GFXAdapterType type );

    /// Converts a string to a GFXAdapterType. Useful for reading in prefs.
    static GFXAdapterType getAdapterTypeFromName( const char* name );

    /// Returns a GFXVideoMode that describes the current state of the main monitor.
    /// This should probably move to the abstract window manager
    static GFXVideoMode getDesktopResolution();

    /// Based on user preferences (or in the absence of a valid user selection,
    /// a heuristic), return a "best" adapter.
    static GFXAdapter *getBestAdapterChoice();

    /// Get the initial video mode based on user preferences (or a heuristic).
    static GFXVideoMode getInitialVideoMode();
};

#endif
