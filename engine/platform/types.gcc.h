
#ifndef INCLUDED_TYPES_GCC_H
#define INCLUDED_TYPES_GCC_H


// For additional information on GCC predefined macros
// http://gcc.gnu.org/onlinedocs/gcc-3.0.2/cpp.html


//--------------------------------------
// Types
typedef signed long long    S64;
typedef unsigned long long  U64;


//--------------------------------------
// Compiler Version
#define TORQUE_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)


//--------------------------------------
// Identify the compiler string

#if defined(__MINGW32__)
#  define TORQUE_COMPILER_STRING "GCC (MinGW)"
#  define TORQUE_COMPILER_MINGW
#elif defined(__CYGWIN__)
#  define TORQUE_COMPILER_STRING "GCC (Cygwin)"
#  define TORQUE_COMPILER_MINGW
#else
#  define TORQUE_COMPILER_STRING "GCC "
#endif


//--------------------------------------
// Identify the Operating System
#if defined(__WIN32__) || defined(_WIN32)
#  define TORQUE_OS_STRING "Win32"
#  define TORQUE_OS_WIN32
#  define TORQUE_SUPPORTS_NASM
#  define TORQUE_SUPPORTS_GCC_INLINE_X86_ASM
#  include "platform/types.win32.h"

#elif defined(linux)
#  define TORQUE_OS_STRING "Linux"
#  define TORQUE_OS_LINUX
#  define TORQUE_SUPPORTS_NASM
#  define TORQUE_SUPPORTS_GCC_INLINE_X86_ASM
#  include "platform/types.posix.h"

#elif defined(__OpenBSD__)
#  define TORQUE_OS_STRING "OpenBSD"
#  define TORQUE_OS_OPENBSD
#  define TORQUE_SUPPORTS_NASM
#  define TORQUE_SUPPORTS_GCC_INLINE_X86_ASM
#  include "platform/types.posix.h"

#elif defined(__FreeBSD__)
#  define TORQUE_OS_STRING "FreeBSD"
#  define TORQUE_OS_FREEBSD
#  define TORQUE_SUPPORTS_NASM
#  define TORQUE_SUPPORTS_GCC_INLINE_X86_ASM
#  include "platform/types.posix.h"

#elif defined(__APPLE__)
#  define TORQUE_OS_MAC
#  define TORQUE_OS_MAC_OSX
#  include "platform/types.ppc.h"
// for the moment:
#  include "platformMacCarb/macCarb_common_prefix.h"
#else 
#  error "GCC: Unsupported Operating System"
#endif


//--------------------------------------
// Identify the CPU
#if defined(i386)
#  define TORQUE_CPU_STRING "Intel x86"
#  define TORQUE_CPU_X86
#  define TORQUE_LITTLE_ENDIAN

#elif defined(__ppc__)
#  define TORQUE_CPU_STRING "PowerPC"
#  define TORQUE_CPU_PPC
#  define TORQUE_BIG_ENDIAN

#else
#  error "GCC: Unsupported Target CPU"
#endif


#endif // INCLUDED_TYPES_GCC_H

