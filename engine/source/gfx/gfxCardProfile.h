//-----------------------------------------------------------------------------
// Torque Shader Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFXCARDPROFILE_H_
#define _GFXCARDPROFILE_H_

#include "core/tDictionary.h"
#include "gfx/gfxDevice.h"

/// Graphics Card Profile
///
/// GFXCardProfiler provides a device independent wrapper around both the 
/// capabilities reported by the card/drivers and the exceptions recorded
/// in various scripts.
///
/// The materials system keeps track of most caps-related rendering 
/// optimizations and/or workarounds, but it is occasionally necessary to
/// expose capability information to higher level code (for instance, if some
/// feature depends on a specific subset of render functionality) or to keep
/// track of exceptions.
///
/// The proper way to fix this is to get the IHV to release fixed drivers 
/// and/or move to a single consistent rendering path that works. Of course, 
/// when you're releasing a game, especially on a timeline (or with a less than
/// infinite budget) this isn't always a valid solution.
///
/// It's also often convenient to be able to tweak performance/detail settings 
/// based on the identified card type.
///
/// GFXCardProfiler addresses both these needs by providing two data retrieval
/// methods and a generic interface for querying capability strings.
///
/// @note The GFXCardProfiler is at heart a system for implementing WORKAROUNDS.
///       It is not guaranteed to work in all cases. The capability strings it
///       responds to are specific to each implementation. You should be
///       EXTREMELY careful when working with this functionality. When used in
///       moderation it can be a project-saver, but if used to excess or without
///       forethought it can lead to complex, hard-to-maintain code.
/// 
/// The first data retrieval method that the GFXCardProfiler supports is a
/// card-specific capability query. This is implemented by each subclass. In the
/// case of DirectX, this means using the built-in capability query. For OpenGL
/// or other APIs, more exotic methods may be necessary. The goal of this method
/// is to retrieve some reasonable defaults that can be overridden later if
/// necessary.
///
/// The second data retrieval method is script based. In ./profile a collection of
/// script files are stored. They are named in one of the forms:
///
/// @code
/// Renderer.cs
/// Renderer.VendorString.CardString.cs
/// Renderer.VendorString.CardString.cs
/// Renderer.VendorString.CardString.VersionString.cs
/// @endcode
///
/// These files are found and executed from most general to most specific. For instance,
/// say we're working in the D3D renderer with an nVidia GeForce FX 5950, running driver
/// version 53.36. The following files would be found and executed:
///
/// @code
/// D3D.cs
/// D3D.nVidia.cs
/// D3D.nVidia.GeForceFX5950.cs
/// D3D.nVidia.GeForceFX5950.5336.cs
/// @endcode
///
/// The general rule for turning strings into filename parts is to strip all spaces and
/// punctuation. If a file is not found, no error is reported; it is assumed that the
/// absence of a file means all is well.
///
/// Several functions are made available to allow simple logic in the script functions (for
/// instance, to enable a workaround for a given range of driver versions). They are:
///      - GFXCardProfiler::getRenderer()
///      - GFXCardProfiler::getVendor()
///      - GFXCardProfiler::getCard()
///      - GFXCardProfiler::getVersion()
///
/// In addition, specific subclasses may expose other values (for instance, chipset IDs).
/// These are made available as static members of the specific subclass. For instance, 
/// a D3D-specific chipset query may be made available as GFXD3DCardProfiler::getChipset().
///
/// Finally, once a script file has reached a determination they may indicate their settings
/// to the GFXCardProfiler by calling GFXCardProfiler::setCapability(). For instance,
///
/// @code
/// // Indicate we can show the color red.
/// GFXCardProfiler::setCapability("supportsRed", true);
/// @endcode
///
/// GFXCardProfiler may be queried from script by calling
/// GFXCardProfiler::queryProfile() - for instance:
///
/// @code
/// GFXCardProfiler::queryProfile("supportsRed"); // Query without default.
/// GFXCardProfiler::queryProfile("supportsRed", false"); // Query with default.
/// @endcode
///
/// As in the C++ code, if a capability string isn't found and no default is found,
/// a console error will be reported.
///
class GFXCardProfiler
{
    /// @name icpi Internal Card Profile Interface
    ///
    /// This is the interface implemented by subclasses of this class in order
    /// to provide implementation-specific information about the current
    /// card/drivers.
    ///
    /// Basically, the implementation needs to provide some unique strings:
    ///      - getVersionString() indicating the current driver version of the
    ///        card in question. (For instance, "53.36")
    ///      - getVendorString() indicating the name of the vendor ("ATI")
    ///      - getCardString() indicating the name of the card ("Radeon 8500")
    ///      - getRendererString() indicating the name of the renderer ("DX9", "GL1.2").
    ///        Each card profiler subclass must return a unique constant so we can keep
    ///        data seperate. Bear in mind that punctuation is stripped from filenames.
    ///        
    /// The profiler also needs to implement setupCardCapabilities(), which is responsible
    /// for querying the active device and setting defaults based on the reported capabilities,
    /// and _queryCardCap, which is responsible for recognizing and responding to
    /// device-specific capability queries.
    ///
    /// @{

public:

    ///
    virtual const char* getVersionString() = 0;
    virtual const char* getCardString() = 0;
    virtual const char* getVendorString() = 0;
    virtual const char* getRendererString() = 0;

protected:

    virtual void setupCardCapabilities() = 0;

    /// Implementation specific query code. 
    ///
    /// This function is meant to be overridden by the specific implementation class.
    ///
    /// Some query strings are handled by the external implementation while others must
    /// be done by the specific implementation. This is given first chance to return
    /// a result, then the generic rules are applied.
    ///
    /// @param  query       Capability being queried.
    /// @param  foundResult Result to return to the caller. If the function returns true
    ///                     then this value is returned as the result of the query.
    virtual bool _queryCardCap(const char* query, U32& foundResult) = 0;

    /// @}

    /// @name helpergroup Helper Functions
    ///
    /// Various helper functions.

    /// Load a specified script file from the profiles directory, if it exists.
    void loadProfileScript(const char* scriptName);

    /// Load the script files in order for the specified card profile tuple.
    void loadProfileScripts(const char* render, const char* vendor, const char* card, const char* version);

    char* strippedString(const char* string);

    /// @}

    /// Capability dictionary.
    Map<const char*, U32> mCapDictionary;

public:

    /// @name ecpi External Card Profile Interface
    ///
    /// @{

    /// Called for a profile for a given device.
    GFXCardProfiler();
    virtual ~GFXCardProfiler();


    /// Set load script files and generally initialize things.
    virtual void init() = 0;

    /// Called to query a capability. Given a query string it returns a 
    /// bool indicating whether or not the capability holds. If you call
    /// this and cap isn't recognized then it returns false and prints
    /// a console error.
    U32 queryProfile(const char* cap);

    /// Same as queryProfile(), but a default can be specified to indicate
    /// what value should be returned if the profiler doesn't know anything
    /// about it. If cap is not recognized, defaultValue is returned and
    /// no error is reported.
    U32 queryProfile(const char* cap, U32 defaultValue);

    /// Set the specified capability to the specified value.
    void setCapability(const char* cap, U32 value);

    /// @}

};

#endif

