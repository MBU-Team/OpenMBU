// WIN64 Cleanup: Shuler
// These defines will be in a future basetsd.h, probably
// in VC 7.0. Until then, they must be defined here.

#ifndef UDM_HFILE
#define UDM_HFILE


#if defined(_WIN32) && !defined(_WIN64)

#include <wtypes.h>

// We can assume that if this is already defined, we're building with a
// version of the platform SDK that already supports UDM
#ifndef SetWindowLongPtr

typedef DWORD DWORD_PTR, *PDWORD_PTR;
typedef long LONG_PTR;
typedef unsigned long ULONG_PTR;
typedef int intptr_t;
typedef unsigned int uintptr_t;

#define SetWindowLongPtr SetWindowLong
#define GetWindowLongPtr GetWindowLong
#define SetClassLongPtr SetClassLong
#define GetClassLongPtr GetClassLong

#define GWLP_WNDPROC GWL_WNDPROC
#define GWLP_HINSTANCE GWL_HINSTANCE
#define GWLP_HWNDPARENT GWL_HWNDPARENT
#define GWLP_USERDATA GWL_USERDATA
#define GWLP_ID GWL_ID

#define GCLP_MENUNAME GCL_MENUNAME
#define GCLP_HBRBACKGROUND GCL_HBRBACKGROUND
#define GCLP_HCURSOR GCL_HCURSOR
#define GCLP_HICON GCL_HICON
#define GCLP_HMODULE GCL_HMODULE
#define GCLP_WNDPROC GCL_WNDPROC
#define GCLP_HICONSM GCL_HICONSM

#define DWLP_MSGRESULT DWL_MSGRESULT
#define DWLP_DLGPROC DWL_DLGPROC
#define DWLP_USER DWL_USER

#ifndef PtrToInt
#define PtrToInt( i )	((int)i)
#endif

// These types are used to fix the wrong
// UDM typedefs in MSVC++ 6.0. Use global replace
// to get rid of them for MSVC++ 7.0.
typedef int INT_PTR_MSVC70;		// becomes INT_PTR
typedef unsigned int UINT_PTR_MSVC70;	// becomes UINT_PTR
#else
// Even if we're using the platform SDK - still need these
typedef int INT_PTR_MSVC70;		// becomes INT_PTR
typedef unsigned int UINT_PTR_MSVC70;	// becomes UINT_PTR
typedef unsigned int uintptr_t;
#endif

#else 

#include <basetsd.h>

// These types allow progress on Win64 port
// without breaking WIN32.
// Get rid of them for MSVC++ 7.0.
typedef INT_PTR INT_PTR_MSVC70;
typedef UINT_PTR UINT_PTR_MSVC70;

#endif WIN64

#endif UDM_HFILE
