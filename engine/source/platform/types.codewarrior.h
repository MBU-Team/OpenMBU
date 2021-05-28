
#ifndef INCLUDED_TYPES_CODEWARRIOR_H
#define INCLUDED_TYPES_CODEWARRIOR_H

#pragma once

// If using the IDE detect if DEBUG build was requested
#if __ide_target("Torque-W32-Debug")
   #define TORQUE_DEBUG
#elif __ide_target("Torque-MacCarb-Debug")
   #define TORQUE_DEBUG
#elif __ide_target("Torque-MacX-Debug")
   #define TORQUE_DEBUG
#endif


//--------------------------------------
// Types
typedef signed long long   S64;     ///< Compiler independent Signed 64-bit integer
typedef unsigned long long U64;     ///< Compiler independent Unsigned 64-bit integer



//--------------------------------------
// Compiler Version
#define TORQUE_COMPILER_CODEWARRIOR __MWERKS__

#define TORQUE_COMPILER_STRING "CODEWARRIOR"


//--------------------------------------
// Identify the Operating System
#if defined(_WIN32)
#  define TORQUE_OS_STRING "Win32"
#  define TORQUE_OS_WIN32
#  include "platform/types.win32.h"

#elif defined(macintosh) || defined(__APPLE__)
#  define TORQUE_OS_STRING "Mac"
#  define TORQUE_OS_MAC
#  if defined(__MACH__)
#     define TORQUE_OS_MAC_OSX
#  endif
#  include "platform/types.ppc.h"
// for the moment:
#  include "platformMacCarb/macCarb_common_prefix.h"
#else 
#  error "CW: Unsupported Operating System"
#endif


//--------------------------------------
// Identify the CPU
#if defined(_M_IX86)
#  define TORQUE_CPU_STRING "x86"
#  define TORQUE_CPU_X86
#  define TORQUE_LITTLE_ENDIAN
#  define TORQUE_SUPPORTS_NASM
#  define TORQUE_SUPPORTS_VC_INLINE_X86_ASM

   // Compiling with the CW IDE we cannot use NASM :(
#  if __ide_target("Torque-W32-Debug")
#     undef TORQUE_SUPPORTS_NASM
#  elif __ide_target("Torque-W32-Release")
#     undef TORQUE_SUPPORTS_NASM
#  endif

#elif defined(__POWERPC__)
#  define TORQUE_CPU_STRING "PowerPC"
#  define TORQUE_CPU_PPC
#  define TORQUE_BIG_ENDIAN

#else
#  error "CW: Unsupported Target CPU"
#endif


#endif // INCLUDED_TYPES_CODEWARRIOR_H

