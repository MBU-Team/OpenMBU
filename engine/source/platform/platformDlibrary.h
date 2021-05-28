//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef OS_DLIBRARY_H
#define OS_DLIBRARY_H

#include "core/refBase.h"

// DLLs use the standard calling convention
#define DLL_CALL __stdcall
#define DLL_EXPORT_CALL __declspec(dllexport)
#define DLL_IMPORT_CALL __declspec(dllimport)

// Export functions from the DLL
#if defined(DLL_CODE)
   #define DLL_DECL DLL_EXPORT_CALL
#else
   #define DLL_DECL DLL_IMPORT_CALL
#endif


//-----------------------------------------------------------------------------

///@defgroup KernelDLL Loadable Libraries
/// Basic DLL handling and symbol resolving. When a library is first loaded
/// it's "open" function will be called, and it's "close" function is called
/// right before the library is unloaded.
///@ingroup OsModule
///@{

/// Dynamic Library
/// Interface for library objects loaded using the loadLibrary() function.
class DLibrary: public RefBase
{
public:
   virtual ~DLibrary() {}
   virtual void *bind( const char *name ) = 0;
};
typedef RefPtr<DLibrary> DLibraryRef;

/// Load a library
/// Returns 0 if the library fails to load. Symbols are
/// resolved through the DLibrary interface.
DLibraryRef OsLoadLibrary( const char *file );

///@}

//-----------------------------------------------------------------------------



#endif

