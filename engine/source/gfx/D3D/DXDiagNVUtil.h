// This file horribly hacked to run inside of Torque -- BJG

/*********************************************************************NVMH4****
Path:  SDK\LIBS\src\NV_D3DCommon
File:  DXDiagNVUtil.h

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

This class allows you to work with DXDiag info in a few ways:
1)  Use D3D9's IDxDiagContainer interface to querry for specific fields by name.
	You must supply the names of fields to search for, and must sometimes supply
	the result of one querry to a later querry (ie. get num display devices in order
	to be able to get device video memory).
	This is fast.
	It requires you to know the COM names of the various IDxDiagContainer nodes.
	You can generate a complete list of these names using #2) below.

2)  Use D3D9's IDxDiagContainer interface to enumerate all field names for the
	COM object.  The field names returned are different than what you see in a
	dxdiag.exe text dump.
	The field names are written using OutputDebugString(..)
	** This is slow

The old functions for running DXDIAG.exe, saving the result to a text file, opening,
	and parsing that file have been removed.

See Microsoft's:
DXSDK\Samples\C++\Misc\DXDiagOutput demo for more code related to reading DXDiag info
DXSDK\Samples\C++\Misc\DxDiagReport


******************************************************************************/

#ifndef H_DXDIAGNVUTIL_GJ_H
#define H_DXDIAGNVUTIL_GJ_H

//#undef _DEBUG
#ifdef DUMMYDEF

namespace NVDXDiagWrapper
{

class wstring;
class string;
typedef unsigned long DWORD;

#else

#include <vector>
#include <string>

namespace NVDXDiagWrapper
{

   using namespace std;

#endif

#include <dxdiag.h>		// in DXSDK\include

#include "NV_Error.h"
#include "NV_Common.h"


// forward decls for things defined externally 
struct IDxDiagProvider;
struct IDxDiagContainer;

//------------------------------------------------------------

class DXDiagNVUtil
{
public:
	//-------- Main interface functions -----------------------------------
	// Interface #1  (prefered)
	// Call these to allocate & free the IDxDiagContainer interface
	// Init will initialize the m_pDxDiagRoot variable
	HRESULT InitIDxDiagContainer();
	HRESULT FreeIDxDiagContainer();

	// ---- The functions below must be called after InitIDxDiagContainer() and before FreeIDxDiagContainer()

	// Recursive traverse all children of the pDxDiagContainer node and list their COM names
	// This is provided so you can easily create and browse a full list of names 
	// Names are output to the debug console via OutputDebugString(..)
	HRESULT ListAllDXDiagPropertyNames();
	HRESULT	ListAllDXDiagPropertyNames( IDxDiagContainer * pDxDiagContainer,
										WCHAR* wszParentName = NULL );
	HRESULT ListAllDXDiagPropertyNamesToTxtFile(	FILE * pOpenTextFile,
													bool bGetPropertyValues = false,
													IDxDiagContainer * pDxDiagContainer = NULL,
													WCHAR * wszParentName = NULL );

	HRESULT	GetNumDisplayDevices(	DWORD * out_dwNumAdapters );
	HRESULT GetDisplayDeviceNode(	DWORD dwDeviceIndex, IDxDiagContainer ** ppNode );
	HRESULT GetDisplayDeviceDescription( 		DWORD dwDevice, wstring *	out_pwstrName );
   HRESULT GetDisplayDeviceDescription( 		DWORD dwDevice, char *	out_pwstrName );
	HRESULT GetDisplayDeviceManufacturer( 		DWORD dwDevice, char *	out_pwstrName );
   HRESULT GetDisplayDeviceChipSet( 		DWORD dwDevice, char *	out_pwstrName );
	HRESULT GetDisplayDeviceNVDriverVersion(    DWORD dwDevice, float *	out_fVersion );
	HRESULT GetDisplayDeviceDriverVersionStr(   DWORD dwDevice, wstring *	out_pwstrVers );
   HRESULT GetDisplayDeviceDriverVersionStr(   DWORD dwDevice, char    *	out_pwstrVers );
	HRESULT GetDisplayDeviceMemoryInMB(			DWORD dwDevice, int *		out_nDisplayMemory );
	HRESULT GetDisplayDeviceAGPMemoryStatus(	DWORD dwDevice, wstring * pwstrAGPEnabled, wstring * pwstrAGPExists, wstring * pwstrAGPStatus );
	HRESULT GetDisplayDeviceProp(				DWORD dwDevice, LPCWSTR in_prop_name, wstring * out_pwstrProp );

	HRESULT GetPhysicalMemoryInMB( float * out_pfMemory );
	HRESULT GetDebugLevels( wstring * pwstrLevels );			// all debug levels returned in 1 string
	HRESULT GetDirectXVersion( DWORD * pdwDirectXVersionMajor, 
								 DWORD * pdwDirectXVersionMinor,
								 TCHAR * pcDirectXVersionLetter );

	// ---- Seconardy interface functions for the #1 interface
	// Use these to directly querry for a field based on the COM property name.
	// You can generate a full list of these names by using ListAllDXDiagPropertyNames()
	// Example:
	//   GetProperty( L"DxDiag_SystemInfo", L"ullPhysicalMemory", & strPropVal );

	HRESULT GetProperty( IDxDiagContainer * pContainer, LPCWSTR property_name, wstring * out_value );

	// Get properties of containers off of the m_pDxDiagRoot node
	HRESULT GetProperty( LPCWSTR container_name0, LPCWSTR property_name, wstring * out_value );
	HRESULT GetProperty( LPCWSTR container_name0, LPCWSTR container_name1, LPCWSTR property_name, wstring * out_value );
	HRESULT GetProperty( LPCWSTR container_name0, LPCWSTR container_name1, LPCWSTR container_name2, LPCWSTR property_name, wstring * out_value );

	HRESULT GetChildContainer( LPCWSTR container_name0, IDxDiagContainer ** out_ppChild );
	HRESULT GetChildContainer( LPCWSTR container_name0, LPCWSTR container_name1, IDxDiagContainer ** out_ppChild );
	HRESULT GetChildContainer( LPCWSTR container_name0, LPCWSTR container_name1, LPCWSTR container_name2, IDxDiagContainer ** out_ppChild );

	HRESULT GetChildByIndex( IDxDiagContainer * in_pParent, DWORD dwIndex, IDxDiagContainer ** out_ppChild );

	// ---- End of functions that should be called between InitIDxDiagContainer() and FreeIDxDiagContainer()

	// Utility functions for converting strings, etc.
	static string WStringToString( const wstring * in_pwstring );
	static string WStringToString( wstring & in_wstring );
	static string lpcwstrToString( const LPCWSTR in_lpcwstr );

protected:
	// data for interface #1  (preferred)
	bool				m_bCleanupCOM;
	IDxDiagProvider	 *	m_pDxDiagProvider;
	IDxDiagContainer *	m_pDxDiagRoot;			// root node of the IDxDiagContainer data


public:
	DXDiagNVUtil()
	{
		SetAllNull();	
	};
	~DXDiagNVUtil()
	{
		FreeIDxDiagContainer();
		SetAllNull();
	};
	void	SetAllNull()
	{
		// data for interface #1 (preferred)
		m_bCleanupCOM				= false;
		m_pDxDiagProvider			= NULL;
		m_pDxDiagRoot				= NULL;
	}
};

};

#endif
