//------------------------------
//win_common_prefix.h
//------------------------------

//------------------------------
// basic build defines
                                 
// define our platform
//#define TARG_WIN32              1
//#define WIN32                   1

// normally, this should be on for a PC build
//#define USEASSEMBLYTERRBLEND    1


//------------------------------
// setup compiler-specific flags

// METROWERKS CODEWARRIOR
//#if __MWERKS__
//#pragma once
// we turn this on to use inlined CW6-safe ASM in CWProject builds.
// ... and thus not require NASM for CWProject building ...
//#define TARG_INLINED_ASM        1
//#endif


// these are general build flags Torque uses.
//#define PNG_NO_READ_tIME        1
//#define PNG_NO_WRITE_TIME       1

//#define NO_MILES_OPENAL         1
