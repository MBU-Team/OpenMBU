/*********************************************************************NVMH4****
Path:  SDK\LIBS\inc\shared
File:  NV_Error.h

Copyright NVIDIA Corporation 2002
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.



Comments:

No longer requires separate .cpp file.  Functions defined in the .h with the
magic of the static keyword.



******************************************************************************/


#pragma warning( disable : 4505 )		// unreferenced local function has been removed

#ifndef	H_NV_ERROR_H
#define	H_NV_ERROR_H

#pragma warning( disable : 4995 )		// old string functions marked as #pragma deprecated

#include    <stdlib.h>          // for exit()
#include    <stdio.h>
#include    <windows.h>

#if _MSC_VER >=1300
#include    <strsafe.h>			// for StringCbVPrintfW
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//---------------------------------------------------------------

#ifndef OUTPUT_POINTER
#define OUTPUT_POINTER(p) { FMsg("%35s = %8.8x\n", #p, p ); }
#endif

#ifndef NULLCHECK
#define NULLCHECK(q, msg,quit) {if(q==NULL) { DoError(msg, quit); }}
#endif

#ifndef IFNULLRET
#define IFNULLRET(q, msg)	   {if(q==NULL) { FDebug(msg); return;}}
#endif

#ifndef FAILRET
#define FAILRET(hres, msg) {if(FAILED(hres)){FDebug("*** %s   HRESULT: %d\n",msg, hres);return hres;}}
#endif

#ifndef HRESCHECK
#define HRESCHECK(q, msg)	 {if(FAILED(q)) { FDebug(msg); return;}}
#endif

#ifndef NULLASSERT
#define NULLASSERT( q, msg,quit )   {if(q==NULL) { FDebug(msg); assert(false); if(quit) exit(0); }}
#endif

/////////////////////////////////////////////////////////////////


static void OkMsgBox( char * szCaption, char * szFormat, ... )
{
	char buffer[256];

    va_list args;
    va_start( args, szFormat );
#if _MSC_VER >=1300
	StringCbVPrintfA( buffer, 256, szFormat, args );
#else
    _vsnprintf( buffer, 256, szFormat, args );
#endif
    va_end( args );

	//	buffer[256-1] = '\0';			// terminate in case of overflow
	MessageBoxA( NULL, buffer, szCaption, MB_OK );
}


#ifdef _DEBUG
	static void FDebug ( char * szFormat, ... )
	{	
		// It does not work to call FMsg( szFormat ).  The variable arg list will be incorrect
		static char buffer[2048];

		va_list args;
		va_start( args, szFormat );
		_vsnprintf( buffer, 2048, szFormat, args );
		va_end( args );

		buffer[2048-1] = '\0';			// terminate in case of overflow
		OutputDebugStringA( buffer );

		Sleep( 2 );		// OutputDebugString sometimes misses lines if
						//  called too rapidly in succession.
	}

	#pragma warning( disable : 4100 )	// unreferenced formal parameter
	inline static void NullFunc( char * szFormat, ... ) {}
	#pragma warning( default : 4100 )

	#if 0
		#define WMDIAG(str) { OutputDebugString(str); }
	#else
		#define WMDIAG(str) {}
	#endif
#else
	inline static void FDebug( char * szFormat, ... )		{ szFormat; }
	inline static void NullFunc( char * szFormat, ... )		{ szFormat; }

	#define WMDIAG(str) {}
#endif


static void FMsg( char * szFormat, ... )
{	
	static char buffer[2048];

	va_list args;
	va_start( args, szFormat );

#if _MSC_VER >=1300
	StringCbVPrintfA( buffer, 2048, szFormat, args );
#else
    _vsnprintf( buffer, 2048, szFormat, args );
#endif
	va_end( args );

	//	buffer[2048-1] = '\0';			// terminate in case of overflow
	OutputDebugStringA( buffer );

	Sleep( 2 );		// OutputDebugString sometimes misses lines if
					//  called too rapidly in succession.
}

static void FMsgW( WCHAR * wszFormat, ... )
{
	WCHAR wbuff[2048];
	va_list args;
	va_start( args, wszFormat );

#if _MSC_VER >=1300
    StringCbVPrintfW( wbuff, 2048, wszFormat, args );
#else
    _vsnwprintf( wbuff, 2048, wszFormat, args );
#endif

    va_end( args );


	// convert wide char string to multibyte string
	char mbBuf[2048];
	wcstombs( mbBuf, wbuff, 2048 );

	OutputDebugStringA(mbBuf);
}



#endif
