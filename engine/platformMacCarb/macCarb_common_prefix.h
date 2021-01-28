//------------------------------
//maccarb_common_prefix.h
//------------------------------

#define TORQUE_OS_MAC_CARB       1     // always defined right now...

#if defined(TORQUE_OS_MAC_OSX)
//#define Z_PREFIX  // OSX comes with zlib, so generate unique symbols.
#endif

// defines for the mac headers to activate proper Carbon codepaths.
#define TARGET_API_MAC_CARBON    1   // apple carbon header flag to take right defpaths.
//#define OTCARBONAPPLICATION      1   // means we can use the old-style funcnames

// determine the OS version we're building on...
//  MAC_OS_X_VERSION_MAX_ALLOWED will have the local OSX version,
//  or it will have the version of OSX for the sdk we're cross compiling with.
#include <AvailabilityMacros.h>

// Pasteboards were introduced in 10.3, and are not available before 10.3
//  MAC_OS_X_VERSION_10_3 == 1030 , and may not exist if we're pre 10.3, so use the raw value here
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1030
#define TORQUE_MAC_HAS_PASTEBOARD
#endif
