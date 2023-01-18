
#ifndef INCLUDED_TYPES_VISUALC_H
#define INCLUDED_TYPES_VISUALC_H


// For more information on VisualC++ predefined macros
// http://support.microsoft.com/default.aspx?scid=kb;EN-US;q65472

//--------------------------------------
// Types
typedef signed _int64   S64;
typedef unsigned _int64 U64;


//--------------------------------------
// Compiler Version
#define TORQUE_COMPILER_VISUALC _MSC_VER

//--------------------------------------
// Identify the compiler string
#if _MSC_VER < 1200
   // No support for old compilers
#  error "VC: Minimum VisualC++ 6.0 or newer required"
#else _MSC_VER >= 1200
#  define TORQUE_COMPILER_STRING "VisualC++"
#endif


//--------------------------------------
// Identify the Operating System
#if _XBOX_VER >= 200 
#  define TORQUE_OS_STRING "Xenon"
#  define TORQUE_OS_XENON
#  include "platform/types.xenon.h"
#elif defined( _XBOX_VER )
#  define TORQUE_OS_STRING "Xbox"
#  define TORQUE_OS_XBOX
#  include "platform/types.win32.h"
#elif defined(_WIN32) && !defined ( _WIN64 )
#  define TORQUE_OS_STRING "Win32"
#  define TORQUE_OS_WIN
#  define TORQUE_OS_WIN32
#  include "platform/types.win32.h"
#elif defined( _WIN64 )
#  define TORQUE_OS_STRING "Win64"
#  define TORQUE_OS_WIN
#  define TORQUE_OS_WIN64
#  include "platform/types.win32.h"
#else
#  error "VC: Unsupported Operating System"
#endif


//--------------------------------------
// Identify the CPU
#if defined( _M_X64 )
#  define TORQUE_CPU_STRING "x64"
#  define TORQUE_CPU_X64
#  define TORQUE_LITTLE_ENDIAN
#elif defined(_M_IX86)
#  define TORQUE_CPU_STRING "x86"
#  define TORQUE_CPU_X86
#  define TORQUE_LITTLE_ENDIAN
#ifndef __clang__ // asm not yet supported with clang
#  define TORQUE_SUPPORTS_NASM
#  define TORQUE_SUPPORTS_VC_INLINE_X86_ASM
#endif
#elif defined(TORQUE_OS_XENON)
#  define TORQUE_CPU_STRING "ppc"
#  define TORQUE_CPU_PPC
#  define TORQUE_BIG_ENDIAN
#else
#  error "VC: Unsupported Target CPU"
#endif

#ifndef FN_CDECL
#  define FN_CDECL __cdecl            ///< Calling convention
#endif

#if _MSC_VER < 1700
#define for if(false) {} else for   ///< Hack to work around Microsoft VC's non-C++ compliance on variable scoping
#endif

// disable warning caused by memory layer
// see msdn.microsoft.com "Compiler Warning (level 1) C4291" for more details
#pragma warning(disable: 4291) 

// [tom, 7/14/2005] Needed for wchar_t
#include <string.h>

#endif // INCLUDED_TYPES_VISUALC_H

