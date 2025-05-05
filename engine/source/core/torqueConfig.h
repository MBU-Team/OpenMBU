//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TORQUECONFIG_H_
#define _TORQUECONFIG_H_

//-----------------------------------------------------------------------------
//Hi, and welcome to the Torque Config file.
//
//This file is a central reference for the various configuration flags that
//you'll be using when controlling what sort of a Torque build you have. In
//general, the information here is global for your entire codebase, applying
//not only to your game proper, but also to all of your tools.
//
//This file also contains information which is used for various other needs,
//for instance, defines indicating what engine we're building, or what version
//we're at.

/// Version number is major * 1000 + minor * 100 + revision * 10.
/// Different engines (TGE, T2D, etc.) will have different version numbers.
#define TORQUE_VERSION              907 // version 0.9
#define TORQUE_PROTOCOL_VERSION     14  // increment this when we change the protocol

/// What engine are we running? The presence and value of this define are
/// used to determine what engine (TGE, T2D, etc.) and version thereof we're
/// running - useful if you're a content pack or other 3rd party code
/// snippet!
///
//#define TORQUE_GAME_ENGINE          TORQUE_VERSION   // we are not TGE, so this isn't defined
#define TORQUE_SHADER_ENGINE        TORQUE_VERSION

/// What's the name of your game? Used in a variety of places.
#define TORQUE_GAME_NAME            "Marble Blast Ultra" //"Torque Shader Engine Demo"

/// Human readable version string.
#define TORQUE_GAME_VERSION_STRING  "1.0.0" //"Torque Demo 1.4 (TGE 1.4)"

/// Define me if you want to enable multithreading support.
#define TORQUE_MULTITHREAD

/// Define me to enable unicode support.
//#define TORQUE_UNICODE

/// Define me to enable Torque HiFi support
#define TORQUE_HIFI_NET

/// Define me to enable Torque Hole Punching support
#define TORQUE_NET_HOLEPUNCHING

/// Define me to enable shader caching support (Unfinished and really only needed for some versions of Wine or Proton)
//#define TORQUE_SHADER_CACHING

/// Define me to enable torque terrain
//#define TORQUE_TERRAIN

/// Define me to enable discord rich presence
#define TORQUE_DISCORD_RPC

/// Define to enable new faster file transfer protocol
#define TORQUE_FAST_FILE_TRANSFER

/// Define me to force process all local packets at once
#define TORQUE_FORCE_PROCESS_ALL_LOCAL_PACKETS

//-----------------------------------------------------------------------------
// Marble Blast related configuration defines

// Define me to enable MarbleBlast features
#define MARBLE_BLAST

// Define me to enable MarbleBlastUltra specific features
//#define MB_ULTRA

// Define me to enable MarbleBlastGold specific features
//#define MB_GOLD

// Define me to enable MarbleBlastXP dynamic camera
//#define MBXP_DYNAMIC_CAMERA

// Define me to enable MarbleBlastXP camera shake
//#define MBXP_CAMERA_SHAKE

// Define me to enable MarbleBlastXP emotives
//#define MBXP_EMOTIVES

// If Ultra is not defined, define Gold
#if defined(MARBLE_BLAST) && !defined(MB_ULTRA) && !defined(MB_GOLD)
#define MB_ULTRA
#endif

// Define me to allow switching physics systems in script
//#define MB_PHYSICS_SWITCHABLE

/// Define me to disable input lag
//#define MB_DISABLE_INPUT_LAG

// Define me to make client physics run every frame
//#define MB_CLIENT_PHYSICS_EVERY_FRAME

// Define me to use MBO physics (does nothing if MB_PHYSICS_SWITCHABLE is defined)
//#define MBO_PHYSICS

// Define me to use MBG physics
//#define MBG_PHYSICS

// Define me to force marble to correct size
#define MB_FORCE_MARBLE_SIZE

// Define me to use a framerate-independent finish pad animation
#define MBU_FINISH_PAD_FIX

// TEMP: Define me for a temporary fix for moving platform jitter
#define MBU_TEMP_MP_DESYNC_FIX

//#define EXPERIMENTAL_MP_LAG_FIX

// Re-order the sky rendering like MBO does (WIP)
//#define MB_FLIP_SKY

// Define me to enable MarbleBlastUltra Preview System
#define MB_ULTRA_PREVIEWS

// Define me to enable xblive functions
#define XB_LIVE

// Define me to use MBG Moving Platform Timing
//#define MBG_MOVING_PLATFORM_TIMING

// Define me to allow phasing into platforms (MBG does this, and MBU did before the title update)
//#define MB_PHYSICS_PHASE_INTO_PLATFORMS

// Define me to fix the shape base images for MBG
//#define MBG_SHAPEBASEFIX

// Define me to not render all six faces of marble cubemap in a single frame
#define MB_CUBEMAP_FAST

//-----------------------------------------------------------------------------
// Here we specify the build configuration defines.  These are usually 
// defined by the build system (makefiles, visual studio, etc) and not 
// in this file.  

/// Define me to enable debug mode; enables a great number of additional
/// sanity checks, as well as making AssertFatal and AssertWarn do something.
/// This is usually defined by the build system.
//#define TORQUE_DEBUG

/// Define me if this is a shipping build; if defined I will instruct Torque
/// to batten down some hatches and generally be more "final game" oriented.
/// Notably this disables a liberal resource manager file searching, and
/// console help strings.
/// This is usually defined by the build system.
//#define TORQUE_SHIPPING

//-----------------------------------------------------------------------------
// Here we specify various optional configuration defines.  If you define
// them in this section, they will be enabled for all build configurations.
// Thus it may be preferable to enable them only in the configuration specific
// sections below (TORQUE_DEBUG, TORQUE_SHIPPING, etc)

// Define me if you want to warn during void assignment in script
//#define CONSOLE_WARN_VOID_ASSIGNMENT

/// Define me if you want asserts.  This is automatically enabled if you 
/// define TORQUE_DEBUG.  However it may be useful to enable it for 
/// your optimized non-ship configuration (where TORQUE_DEBUG is 
/// typically not defined).
//#define TORQUE_ENABLE_ASSERTS

/// Define me if you want to enable the profiler. 
///    See also the TORQUE_SHIPPING block below
#define TORQUE_ENABLE_PROFILER

/// Define me if you want the memory manager to hook into the profiler stack
/// so that you can see where particular allocations come from when 
/// debugging memory leaks.  This information is provided when you use the 
/// dumpUnflaggedAllocs() function of the memory manager.
#define TORQUE_ENABLE_PROFILE_PATH

/// Define me to enable a variety of network debugging aids.
//#define TORQUE_DEBUG_NET

/// Define me to enable a variety of network move packet debugging aids.
//#define TORQUE_DEBUG_NET_MOVES

/// Define me to enable network bandwidth tracking at the ConsoleObject level.
//#define TORQUE_NET_STATS

/// Modify me to enable metric gathering code in the renderers.
///
/// 0 does nothing; higher numbers enable higher levels of metric gathering.
//#define TORQUE_GATHER_METRICS 0

/// Define me to disable the Torque Memory Manager.  Memory allocations will
/// be handled directly by the system.  This is useful if you want to 
/// minimize your memory footprint, since the TMM never frees pages that 
/// it allocates.  It is also useful for debugging, since if you write 
/// into memory that is owned by the system, you generally crash immediately
/// (whereas with the TMM enabled, you are writing into Torque's process
/// memory so the system might not crash at all, unless you happen to write
/// over a memory header and TORQUE_DEBUG_GUARD is enabled, see below).
///
/// However, there can be a performance hit when not using the memory manager,
/// because Torque objects often continuously allocate and free bits of memory
/// every frame.  Some system allocators (i.e. Xbox1) do not deal with this 
/// usage very well, leading to a cumulative framerate decrease.
///
/// When the TMM is off, its tools for detecting memory leaks are not 
/// available.
#define TORQUE_DISABLE_MEMORY_MANAGER

/// Define me if you want to enable debug guards in the memory manager.
///
/// Debug guards are known values placed before and after every block of
/// allocated memory. They are checked periodically by Memory::validate(),
/// and if they are modified (indicating an access to memory the app doesn't
/// "own"), an error is flagged (ie, you'll see a crash in the memory 
/// manager's validate code). Using this and a debugger, you can track down
/// memory corruption issues quickly.
///
/// Debug guard requires the Torque Memory manager.
//#define TORQUE_DEBUG_GUARD

/// Define to enable multiple binds per console function in an actionmap.  
/// With this, you can bind multiple keys to the same "jump()" function 
/// for instance.  Without it, you need to declare multiple jump functions
/// that do the same thing and bind them separately (this is the Torque 
/// default).
#define TORQUE_ALLOW_MULTIPLE_ACTIONMAP_BINDS

/// Define me to disable the password on the telnet console.  Only applies in 
/// non-ship builds. Use for situations where you are using the telnet console 
/// a lot and don't like typing the password over and over and OVER (i.e. xbox)
//#define TORQUE_DISABLE_TELNET_CONSOLE_PASSWORD

/// Define to disable Ogg Vorbis audio support. Libs are compiled without this by
/// default.
//#define TORQUE_NO_OGGVORBIS

/// This #define is used by the FrameAllocator to align starting addresses to
/// be byte aligned to this value. This is important on the 360 and possibly
/// on other platforms as well. Use this #define anywhere alignment is needed.
///
/// NOTE: Do not change this value per-platform unless you have a very good
/// reason for doing so. It has the potential to cause inconsistencies in
/// memory which is allocated and expected to be contiguous.
///
///@ TODO: Make sure that everywhere this should be used, it is being used.
#define TORQUE_BYTE_ALIGNMENT 4

/// This #define is used by the FrameAllocator to set the size of the frame.
///
/// It was previously set to 3MB but I've increased it to 16MB due to the
/// FrameAllocator being used as temporary storage for bitmaps in the D3D9
/// texture manager.
#define TORQUE_FRAME_SIZE     16 << 20

/// Define if you want nVIDIA's NVPerfHUD to work with TSE
#define TORQUE_NVPERFHUD

/// Define to disable DSO file generation.  Note that existing DSOs will still be 
// used by the engine.  Script files will load a bit more slowly because they need
// to be compiled each time.
#define TORQUE_NO_DSO_GENERATION

// Used to check internal GFX state, D3D/OGL states, etc.  
//#define TORQUE_DEBUG_RENDER

// If this is defined, and a material is not found, it will be created
//#define CREATE_MISSING_MATERIALS

// Enable ShaderGen
#define TORQUE_SHADERGEN

// Use legacy font shadow rendering
//#define MBG_FONT_SHADOW_RENDERING

//-----------------------------------------------------------------------------
// Finally, we define some dependent #defines for the various build 
// configurations. This enables some subsidiary functionality to get 
// automatically turned on in certain configurations.

#ifdef TORQUE_DEBUG
#  define TORQUE_GATHER_METRICS 0
#  define TORQUE_ENABLE_PROFILER
#  define TORQUE_ENABLE_PROFILE_PATH
#  define TORQUE_DEBUG_GUARD
#endif

#ifdef TORQUE_RELEASE
  // If it's not DEBUG, it's a RELEASE build, put appropriate things here.
#endif

#ifdef TORQUE_SHIPPING
 // TORQUE_SHIPPING flags here.
#else
   // enable the profiler by default, if we're not doing a shipping build
#  define TORQUE_ENABLE_PROFILER
#endif

#ifdef TORQUE_LIB
#ifndef TORQUE_NO_OGGVORBIS
#define TORQUE_NO_OGGVORBIS
#endif
#endif

// This define is for the shader constant include string
#define SHADER_CONSTANT_INCLUDE_FILE "../../game/shaders/shdrConsts.h"

// Someday, it might make sense to do some pragma magic here so we error
// on inconsistent flags.

#endif
