//maccarb_release_prefix.h

#include "maccarb_common_prefix.h"

// our defines
#define TGE_RELEASE               1
#define TGE_NO_ASSERTS            1
#define BUILD_SUFFIX              ""

// these should be off in a release build
//#define DEBUG                    1
//#define ENABLE_ASSERTS           1

// Mac uses C version of ITFDump code.
#define ITFDUMP_NOASM             1

//#define USEASSEMBLYTERRBLEND    1

#define PNG_NO_READ_tIME          1
#define PNG_NO_WRITE_TIME         1

#define NO_MILES_OPENAL           1

