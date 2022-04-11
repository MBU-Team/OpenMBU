/*********************************************************************NVMH4****
Path:  SDK\LIBS\src\NV_D3DCommon
File:  DXDiagNVUtil.cpp

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
See the DXDiagNVUtil.h file for additional comments

******************************************************************************/

#ifndef INITGUID
#  define INITGUID		// to define _CLSID_DxDiagProvider, _IID_IDxDiagProvider
#endif

#if defined(MSVC6)

// Make sure we don't have any exceptions getting compiled in (GROSS HACK)
#include <xstddef>

 #undef _TRY_BEGIN
 #define _TRY_BEGIN
 #undef _CATCH
 #define _CATCH(x)	
 #undef _CATCH_ALL
 #define _CATCH_ALL
 #undef _CATCH_END
 #define _CATCH_END
 #undef _RAISE
 #define _RAISE(x)	
 #undef _RERAISE	
 #undef _THROW0
 #define _THROW0()
 #undef _THROW1
 #define _THROW1(x)
 #undef _THROW
 #define _THROW(x, y)
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4996) // turn off "deprecation" warnings
#endif

#include "DxDiagNVUtil.h"

namespace NVDXDiagWrapper 
{

#include <io.h>			// for _filelength()
#include <direct.h>		// for getcwd()
#include <stdio.h>		// for L text macro
#include <dxdiag.h>
#include <math.h>

#pragma comment( lib, "dxguid.lib" )

// use #if 1 to turn trace messages on
#if 0
	#define TRACE0(v)	v
#else
	#define TRACE0(v)	{}
#endif

using namespace std;

// use as an alternative to wcstombs()
string DXDiagNVUtil::WStringToString( const wstring * in_pwstring )
{
	if( in_pwstring == NULL )
		return( "" );

	return( lpcwstrToString( in_pwstring->c_str() ));
}

string DXDiagNVUtil::WStringToString( wstring & in_wstring )
{
	return( lpcwstrToString( in_wstring.c_str() ));
}

float hackWToF( wstring & in_wstring)
{
   string t = DXDiagNVUtil::WStringToString(in_wstring);
   return atof(t.c_str());
}

int hackWToI( wstring & in_wstring)
{
   string t = DXDiagNVUtil::WStringToString(in_wstring);
   return atoi(t.c_str());
}

string DXDiagNVUtil::lpcwstrToString( const LPCWSTR in_lpcwstr )
{
   //@ consider using windows.h  WideCharToMultiByte(..)
   char * mbBuf;
   size_t sz;

   if( !in_lpcwstr || !in_lpcwstr[0] )
   {
      mbBuf = new char[2];
      mbBuf[0] = mbBuf[1] = 0;
   }
   else
   {
      sz = 2 * wcslen( in_lpcwstr );
      mbBuf = new char[sz];
      wcstombs( mbBuf, in_lpcwstr, sz );		// convert the string 
   }
   string outstr;
   outstr = mbBuf;
   SAFE_ARRAY_DELETE( mbBuf );
   return( outstr );
}


HRESULT DXDiagNVUtil::GetChildByIndex(  IDxDiagContainer * pParent,
										DWORD dwIndex,
										IDxDiagContainer ** ppChild )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( pParent );
	FAIL_IF_NULL( ppChild );

	// We need to look up the child by name, not by index, so first
	//  get the name of the child at the appropriate index.

	// Get a single child container name, writing the name to wstr
	WCHAR wstr[256];
	hr = pParent->EnumChildContainerNames( dwIndex, wstr, 256 );
	RET_VAL_IF_FAILED( hr );

	// Now get the child container by name
	hr = pParent->GetChildContainer( wstr, ppChild );
	return( hr );
}


HRESULT	DXDiagNVUtil::GetNumDisplayDevices( DWORD * out_pdwNumAdapters )
{
	HRESULT hr = S_OK;
	MSGVARNAME_AND_FAIL_IF_NULL( m_pDxDiagRoot );
	MSGVARNAME_AND_FAIL_IF_NULL( out_pdwNumAdapters );

	IDxDiagContainer * pChild;
	hr = GetChildContainer( L"DxDiag_DisplayDevices", & pChild );
	MSG_AND_RET_VAL_IF( FAILED(hr), "GetNumDisplayDevices() Couldn't get display devices container!\n", hr );

	hr = pChild->GetNumberOfChildContainers( out_pdwNumAdapters );
	MSG_AND_BREAK_IF( FAILED(hr), "GetNumDisplayDevices() Couldn't get number of child containers!\n" );

	SAFE_RELEASE( pChild );
	return( hr );
}


HRESULT DXDiagNVUtil::GetDisplayDeviceNode( DWORD dwDeviceIndex, IDxDiagContainer ** ppNode )
{
	// fill *ppNode with pointer to the display device at index nDevice of the DXDiag_DisplayDevices container
	// you must SAFE_RELEASE( *ppNode ) outside of this function.
	HRESULT hr = S_OK;
	FAIL_IF_NULL( ppNode );

	IDxDiagContainer * pDevicesNode;
	hr = GetChildContainer( L"DxDiag_DisplayDevices", & pDevicesNode );
	MSG_AND_RET_VAL_IF( FAILED(hr), "GetDisplayDeviceNode() couldn't get devices node!\n", hr );

	// Get the device at the appropriate index
	hr = GetChildByIndex( pDevicesNode, dwDeviceIndex, ppNode );

	SAFE_RELEASE( pDevicesNode );
	return( hr );
}



HRESULT DXDiagNVUtil::GetDisplayDeviceDescription( DWORD dwDevice, wstring * pwstrName )
{
	HRESULT hr = S_OK;
	MSGVARNAME_AND_FAIL_IF_NULL( m_pDxDiagRoot );
	MSGVARNAME_AND_FAIL_IF_NULL( pwstrName );

	IDxDiagContainer * pDisplayDevice;
	hr = GetDisplayDeviceNode( dwDevice, &pDisplayDevice );
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( pDisplayDevice );

	// Get the device's name:
	hr = GetProperty( pDisplayDevice, L"szDescription", pwstrName );
	MSG_IF( FAILED(hr), "GetDisplayDeviceName() Couldn't get device's name property!\n");

	SAFE_RELEASE( pDisplayDevice );
	return( hr );
}

HRESULT DXDiagNVUtil::GetDisplayDeviceDescription( DWORD dwDevice, char * out_pwstrName )
{
   wstring out;
   HRESULT res = GetDisplayDeviceDescription(dwDevice, &out);

   // Stuff it into the char*
   string pony2 = WStringToString(out);
   strcpy(out_pwstrName, pony2.c_str());

   return res;
}

HRESULT DXDiagNVUtil::GetDisplayDeviceManufacturer( DWORD dwDevice, char * pwstrName )
{
	HRESULT hr = S_OK;
	MSGVARNAME_AND_FAIL_IF_NULL( m_pDxDiagRoot );
	MSGVARNAME_AND_FAIL_IF_NULL( pwstrName );

	IDxDiagContainer * pDisplayDevice;
	hr = GetDisplayDeviceNode( dwDevice, &pDisplayDevice );
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( pDisplayDevice );

	// Get the device's name:
   wstring pony;
	hr = GetProperty( pDisplayDevice, L"szManufacturer", &pony );
	MSG_IF( FAILED(hr), "GetDisplayDeviceName() Couldn't get device's name property!\n");

   // Stuff it into the char*
   string pony2 = WStringToString(pony);
   strcpy(pwstrName, pony2.c_str());

	SAFE_RELEASE( pDisplayDevice );
	return( hr );
}

HRESULT DXDiagNVUtil::GetDisplayDeviceChipSet( DWORD dwDevice, char * pwstrName )
{
	HRESULT hr = S_OK;
	MSGVARNAME_AND_FAIL_IF_NULL( m_pDxDiagRoot );
	MSGVARNAME_AND_FAIL_IF_NULL( pwstrName );

	IDxDiagContainer * pDisplayDevice;
	hr = GetDisplayDeviceNode( dwDevice, &pDisplayDevice );
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( pDisplayDevice );

	// Get the device's name:
   wstring pony;
	hr = GetProperty( pDisplayDevice, L"szChipType", &pony );
	MSG_IF( FAILED(hr), "GetDisplayDeviceName() Couldn't get device's name property!\n");

   // Stuff it into the char*
   string pony2 = WStringToString(pony);
   strcpy(pwstrName, pony2.c_str());

	SAFE_RELEASE( pDisplayDevice );
	return( hr );
}

HRESULT DXDiagNVUtil::GetDisplayDeviceNVDriverVersion( DWORD dwDevice, float * fVersion )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( fVersion );
	*fVersion = 0.0f;
	wstring wstr;

	hr = GetDisplayDeviceDriverVersionStr( dwDevice, & wstr );
	MSG_IF( FAILED(hr), "GetDisplayDeviceNVDriverVersion() couldn't get driver version string\n");
	RET_VAL_IF_FAILED(hr);

	// nv driver string will be something like:  "6.14.0010.5216"
	// where the #s we care about are the last digits
	size_t pos;
	pos = wstr.find_last_of( '.' );
	wstr.erase( 0, pos );
	//FMsgW(L"wstr: %s\n", wstr.c_str() );

	// decode the string
	// multiply by 100 to convert .5216 into 52.16
	*fVersion = (float) hackWToF( wstr ) * 100.0f;
	return( hr );
}

HRESULT DXDiagNVUtil::GetDisplayDeviceDriverVersionStr( DWORD dwDevice, wstring * pwstrVers )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( pwstrVers );

	hr = GetDisplayDeviceProp( dwDevice, L"szDriverVersion", pwstrVers );
	MSG_IF( FAILED(hr), "GetDisplayDeviceName() Couldn't get device's driver version string!\n");

	return( hr );
}

HRESULT DXDiagNVUtil::GetDisplayDeviceDriverVersionStr( DWORD dwDevice, char * pwstrVers )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( pwstrVers );

   wstring pony;
	hr = GetDisplayDeviceProp( dwDevice, L"szDriverVersion", &pony );
	MSG_IF( FAILED(hr), "GetDisplayDeviceName() Couldn't get device's driver version string!\n");

   // Stuff it into the char*
   string pony2 = WStringToString(pony);
   strcpy(pwstrVers, pony2.c_str());

	return( hr );
}


HRESULT DXDiagNVUtil::GetDisplayDeviceMemoryInMB( DWORD dwDevice, int * pDisplayMemory )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( pDisplayMemory );
	*pDisplayMemory = 0;
	wstring wstr;
	hr = GetDisplayDeviceProp( dwDevice, L"szDisplayMemoryEnglish", &wstr );
	MSG_IF( FAILED(hr), "GetDisplayDeviceMemoryInMB() Couldn't get the property string!\n");

	// decode the value to an int
	// assume 1st string is the integer number
	int nMem;
	int num_fields;
	num_fields = swscanf( wstr.c_str(), L"%d", &nMem );
	if( num_fields != 1 )
	{
		FMsg("GetDisplayDeviceMemoryInMB error scanning memory string description!\n");
		return( E_FAIL );
	}

	*pDisplayMemory = nMem;
	return( hr );
}

HRESULT DXDiagNVUtil::GetDisplayDeviceProp( DWORD dwDevice, LPCWSTR prop_name, wstring * pwstrProp )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( prop_name );
	FAIL_IF_NULL( pwstrProp );
	*pwstrProp = L"";

	IDxDiagContainer * pDisplayDevice;
	hr = GetDisplayDeviceNode( dwDevice, &pDisplayDevice );
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( pDisplayDevice );

	hr = GetProperty( pDisplayDevice, prop_name, pwstrProp );
	MSG_IF( FAILED(hr), "GetDisplayDeviceProp() Couldn't get the property string!\n");

	return( hr );
}

/*HRESULT DXDiagNVUtil::GetDisplayDeviceProp2( DWORD dwDevice, char* prop_name, char * pwstrProp )
{
   wstring realProp = string(prop_name).;
   wstring pony;
   GetDisplayDeviceProp(dwDevice, wstring(prop_name), &pony);


   string pony2 = WStringToString(pony);

   strcpy(pwstrProp, pony2.c_str());

   return S_OK;
}*/

HRESULT DXDiagNVUtil::GetPhysicalMemoryInMB( float * out_pfMemory )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( out_pfMemory );
	wstring property;
	hr = GetProperty( L"DxDiag_SystemInfo", L"ullPhysicalMemory", &property );

	float mem = (float) hackWToF( property );
	// convert bytes to MB
	mem = mem / (1024 * 1024);
	*out_pfMemory = mem;
	return( hr );
}

HRESULT DXDiagNVUtil::GetDisplayDeviceAGPMemoryStatus( DWORD dwDeviceIndex, wstring * pwstrAGPEnabled,
													   wstring * pwstrAGPExists, wstring * pwstrAGPStatus )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( m_pDxDiagRoot );
	FAIL_IF_NULL( pwstrAGPEnabled );
	FAIL_IF_NULL( pwstrAGPExists );
	FAIL_IF_NULL( pwstrAGPStatus );
	*pwstrAGPEnabled = L"";
	*pwstrAGPExists = L"";
	*pwstrAGPStatus = L"";
	hr = GetDisplayDeviceProp( dwDeviceIndex, L"bAGPEnabled", pwstrAGPEnabled );
	hr = GetDisplayDeviceProp( dwDeviceIndex, L"szAGPStatusEnglish", pwstrAGPStatus );

	wstring property;
	hr = GetDisplayDeviceProp( dwDeviceIndex, L"bAGPExistenceValid", &property );
	if( SUCCEEDED(hr) && property == L"true" )
	{
		hr = GetDisplayDeviceProp( dwDeviceIndex, L"bAGPExists", pwstrAGPExists );
	}
	return( hr );
}


HRESULT DXDiagNVUtil::GetDebugLevels( wstring * pwstrLevels )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( pwstrLevels );
	*pwstrLevels = L"";
	wstring propval;

	hr = GetProperty( L"DxDiag_SystemInfo", L"nD3DDebugLevel", &propval );
	pwstrLevels->append( L"D3DDebugLevel =    "); pwstrLevels->append( propval ); pwstrLevels->append(L"\n");

	hr = GetProperty( L"DxDiag_SystemInfo", L"nDDrawDebugLevel", &propval );
	pwstrLevels->append( L"DDrawDebugLevel =  "); pwstrLevels->append( propval ); pwstrLevels->append(L"\n");

	hr = GetProperty( L"DxDiag_SystemInfo", L"nDIDebugLevel", &propval );
	pwstrLevels->append( L"DIDebugLevel =     "); pwstrLevels->append( propval ); pwstrLevels->append(L"\n");

	hr = GetProperty( L"DxDiag_SystemInfo", L"nDMusicDebugLevel", &propval );
	pwstrLevels->append( L"DMusicDebugLevel = "); pwstrLevels->append( propval ); pwstrLevels->append(L"\n");

	hr = GetProperty( L"DxDiag_SystemInfo", L"nDPlayDebugLevel", &propval );
	pwstrLevels->append( L"DPlayDebugLevel =  "); pwstrLevels->append( propval ); pwstrLevels->append(L"\n");

	hr = GetProperty( L"DxDiag_SystemInfo", L"nDSoundDebugLevel", &propval );
	pwstrLevels->append( L"DSoundDebugLevel = "); pwstrLevels->append( propval ); pwstrLevels->append(L"\n");

	hr = GetProperty( L"DxDiag_SystemInfo", L"nDShowDebugLevel", &propval );
	pwstrLevels->append( L"DShowDebugLevel =  "); pwstrLevels->append( propval ); pwstrLevels->append(L"\n");

	return( hr );
}

HRESULT DXDiagNVUtil::GetDirectXVersion( DWORD * pdwDirectXVersionMajor, 
										 DWORD * pdwDirectXVersionMinor,
										 TCHAR * pcDirectXVersionLetter )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( m_pDxDiagRoot );
	FAIL_IF_NULL( pdwDirectXVersionMajor );
	FAIL_IF_NULL( pdwDirectXVersionMinor );
	FAIL_IF_NULL( pcDirectXVersionLetter );

	wstring propval;
	GetProperty( L"DxDiag_SystemInfo", L"dwDirectXVersionMajor", &propval );
	*pdwDirectXVersionMajor = hackWToI( propval );

	GetProperty( L"DxDiag_SystemInfo", L"dwDirectXVersionMinor", &propval );
	*pdwDirectXVersionMinor = hackWToI( propval );

	GetProperty( L"DxDiag_SystemInfo", L"szDirectXVersionLetter", &propval );
	string str;

	str = WStringToString( &propval );
	if( str.length() > 0 )
		*pcDirectXVersionLetter = str.at(0);
	else
		*pcDirectXVersionLetter = ' ';

	return( hr );
}



//------------------------------------------------------------------------
// Get the property at pContainer by name
// Decode the VARIANT property to a string and return the string
HRESULT DXDiagNVUtil::GetProperty( IDxDiagContainer * pContainer, LPCWSTR property_name, wstring * out_value )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( pContainer );
	FAIL_IF_NULL( property_name );
	FAIL_IF_NULL( out_value );

    WCHAR wszPropValue[256];
	VARIANT var;
	VariantInit( &var );

	hr = pContainer->GetProp( property_name, &var );
	if( SUCCEEDED(hr) )
	{
		// Switch according to the type.  There's 4 different types:
		switch( var.vt )
		{
			case VT_UI4:
				swprintf( wszPropValue, L"%d", var.ulVal );
				break;
			case VT_I4:
				swprintf( wszPropValue, L"%d", var.lVal );
				break;
			case VT_BOOL:
				swprintf( wszPropValue, L"%s", (var.boolVal) ? L"true" : L"false" );
				break;
			case VT_BSTR:
				wcsncpy( wszPropValue, var.bstrVal, 255 );
				wszPropValue[255] = 0;
				break;
		}
		// copy string to the output string
		(*out_value) = wszPropValue;
	}
	else
	{
		FMsgW(L"GetProperty( cont, prop, val ) Couldn't get prop [%s]\n", property_name );
		//char  mbBuf[512];
		//wcstombs( mbBuf, property_name, 512 );
		//FMsg("GetProperty( cont, prop, val ) Couldn't get prop [%s]\n", mbBuf );
	}

	VariantClear( &var );
	return( hr );
}


HRESULT DXDiagNVUtil::GetProperty( LPCWSTR container_name0, 
									LPCWSTR property_name, 
									wstring * out_value )
{
	HRESULT hr = S_OK;

	IDxDiagContainer * pContainer;
	hr = GetChildContainer( container_name0, &pContainer );
	TRACE0( MSG_IF( FAILED(hr), "GetProperty(cn,pn,&v) GetChildContainer(<1>) failed!\n" ));
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( pContainer );

	hr = GetProperty( pContainer, property_name, out_value );
	TRACE0( MSG_IF( FAILED(hr), "GetProperty(<1>) pContainer->GetProperty(c,n,v) failed!\n"));

	SAFE_RELEASE( pContainer );
	return( hr );
}

HRESULT DXDiagNVUtil::GetProperty( LPCWSTR container_name0, 
									LPCWSTR container_name1, 
									LPCWSTR property_name, 
									wstring * out_value )
{
	HRESULT hr = S_OK;

	IDxDiagContainer * pContainer;
	hr = GetChildContainer( container_name0, container_name1, &pContainer );
	MSG_AND_RET_VAL_IF( FAILED(hr), "GetProperty(cn,cn,pn,&v) couldn't get child container!\n", hr );

	hr = GetProperty( pContainer, property_name, out_value );
	MSG_AND_RET_VAL_IF( FAILED(hr), "GetProperty(cn,cn,pn,&v) couldn't get container property!\n", hr );

	SAFE_RELEASE( pContainer );
	return( hr );
}

HRESULT DXDiagNVUtil::GetProperty( LPCWSTR container_name0, 
									LPCWSTR container_name1, 
									LPCWSTR container_name2,
									LPCWSTR property_name,
									wstring * out_value )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( out_value );
	IDxDiagContainer * pContainer;
	hr = GetChildContainer( container_name0, container_name1, container_name2, &pContainer );
	MSG_AND_RET_VAL_IF( FAILED(hr), "GetProperty(cn,cn,cn,pn,&v) couldn't get child container!\n", hr );

	hr = GetProperty( pContainer, property_name, out_value );
	MSG_AND_RET_VAL_IF( FAILED(hr), "GetProperty(cn,cn,cn,pn,&v) couldn't get container property!\n", hr );

	SAFE_RELEASE( pContainer );
	return( hr );
}



//------------------------------

HRESULT DXDiagNVUtil::GetChildContainer( LPCWSTR name0, IDxDiagContainer ** ppChild )
{
	// you must release *ppChild on your own when you're done with it
	HRESULT hr = S_OK;
	MSGVARNAME_AND_FAIL_IF_NULL( m_pDxDiagRoot );
	MSGVARNAME_AND_FAIL_IF_NULL( ppChild );

	hr = m_pDxDiagRoot->GetChildContainer( name0, ppChild );
	TRACE0(	MSG_IF( FAILED(hr), "GetChildContainer(<1>) m_pDxDiagRoot->GetChildContainer() failed\n" ));
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( *ppChild );

	return( hr );
}

HRESULT DXDiagNVUtil::GetChildContainer( LPCWSTR name0, 
											LPCWSTR name1, 
											IDxDiagContainer ** ppChild )
{
	// intermediate children are released before returning
	// you only have to release() *ppChild
	HRESULT hr = S_OK;

	IDxDiagContainer * pChild;
	hr = GetChildContainer( name0, &pChild );
	TRACE0(	MSG_IF( FAILED(hr), "GetChildContainer(<2>) GetChildContainer(<1>) failed\n" ) );
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( pChild );
	
	hr = pChild->GetChildContainer( name1, ppChild );
	TRACE0(	MSG_IF( FAILED(hr), "GetChildContainer(<2>) IDxDiagContainer->GetChildContainer() failed\n" ) );

	SAFE_RELEASE( pChild );		// release the intermediate
	return( hr );
}

HRESULT DXDiagNVUtil::GetChildContainer( LPCWSTR name0, 
											LPCWSTR name1, 
											LPCWSTR name2, 
											IDxDiagContainer ** ppChild )
{
	// intermediate children are released before returning
	// you only have to release() *ppChild
	HRESULT hr = S_OK;

	IDxDiagContainer * pChild;
	hr = GetChildContainer( name0, name1, &pChild );
	TRACE0(	MSG_IF( FAILED(hr), "GetChildContainer(<3>) GetChildContainer(<2>) failed\n" ) );
	RET_VAL_IF_FAILED( hr );
	FAIL_IF_NULL( pChild );
	
	hr = pChild->GetChildContainer( name2, ppChild );
	TRACE0(	MSG_IF( FAILED(hr), "GetChildContainer(<3>) IDxDiagContainer->GetChildContainer() failed\n" ) );

	SAFE_RELEASE( pChild );		// release the intermediate
	return( hr );
}


// public interface to display all node and subnode names
HRESULT DXDiagNVUtil::ListAllDXDiagPropertyNames()
{
	HRESULT hr = S_OK;
	hr = ListAllDXDiagPropertyNames( m_pDxDiagRoot );
	return( hr );
}


HRESULT DXDiagNVUtil::ListAllDXDiagPropertyNames( IDxDiagContainer * pDxDiagContainer,
													WCHAR* wszParentName )
{
	// Recurse through all properties and child properties, 
	// outputting their names via OutputDebugString(..)
	// Start this by calling with the root IDxDiagContainer and a NULL string ptr.
	//
	// This function can take a while to complete.
	//
	// Adapted from DXSDK example file DxDiagOutput.cpp

	HRESULT hr = S_OK;	
	FAIL_IF_NULL( pDxDiagContainer );

    DWORD dwPropCount;
    DWORD dwPropIndex;
    WCHAR wszPropName[256];
    DWORD dwChildCount;
    DWORD dwChildIndex;
    WCHAR wszChildName[256];
    IDxDiagContainer* pChildContainer = NULL;

	const bool bGetPropertyValue = false;

    hr = pDxDiagContainer->GetNumberOfProps( &dwPropCount );
    if( SUCCEEDED(hr) ) 
    {
        // Print each property in this container
        for( dwPropIndex = 0; dwPropIndex < dwPropCount; dwPropIndex++ )
        {
            hr = pDxDiagContainer->EnumPropNames( dwPropIndex, wszPropName, 256 );

			if( SUCCEEDED( hr ) )
			{
				if( bGetPropertyValue )
				{
					wstring propval;
					hr = GetProperty( pDxDiagContainer, wszPropName, & propval );
					FMsgW(L"%35s : prop \"%s\" = %s\n", wszParentName, wszPropName, propval.c_str() );
				}
				else
				{
					// otherwise, just list the property name and parent container info
					FMsgW(L"%35s : prop \"%s\"\n", wszParentName, wszPropName );					
				}
			}
        }
    }

    // Recursivly call this function for each of its child containers
    hr = pDxDiagContainer->GetNumberOfChildContainers( &dwChildCount );
    if( SUCCEEDED(hr) )
    {
        for( dwChildIndex = 0; dwChildIndex < dwChildCount; dwChildIndex++ )
        {
            hr = pDxDiagContainer->EnumChildContainerNames( dwChildIndex, wszChildName, 256 );
            if( SUCCEEDED(hr) )
            {
                hr = pDxDiagContainer->GetChildContainer( wszChildName, &pChildContainer );
                if( SUCCEEDED(hr) )
                {
                    // wszFullChildName isn't needed but is used for text output 
                    WCHAR wszFullChildName[256];

					// Append child name to parent and pass down in the recursion
					//  as full parent name.
                    if( wszParentName )
                        swprintf( wszFullChildName, L"%s.%s", wszParentName, wszChildName );
                    else
                        swprintf( wszFullChildName, L"%s", wszChildName );

					// recurse
                    ListAllDXDiagPropertyNames( pChildContainer, wszFullChildName );
                    SAFE_RELEASE( pChildContainer );
                }
            }
        }
    }
	return( hr );
}

// Recurse through all properties and child properties, outputting their names
//  to the file.
// This function is slow and takes a while to output all the nodes, so it can be
//  called with a sub-node initial argument if you only care about a small part of the
//  COM data structure tree.
// Adapted from DXSDK example file DxDiagOutput.cpp
// pOpenTextFile   Must be a file opened for writing text
// Call this with NULL 2nd and 3rd args to start from the top of the node structure.
HRESULT DXDiagNVUtil::ListAllDXDiagPropertyNamesToTxtFile(	FILE * pOpenTextFile,
															bool bGetPropertyValues,
															IDxDiagContainer * pDxDiagContainer,
															WCHAR * wszParentName )
{
	HRESULT hr = S_OK;
	FAIL_IF_NULL( pOpenTextFile );
	if( pDxDiagContainer == NULL )
		pDxDiagContainer = m_pDxDiagRoot;
	FAIL_IF_NULL( pDxDiagContainer );

    DWORD dwPropCount;
    DWORD dwPropIndex;
    WCHAR wszPropName[256];
    DWORD dwChildCount;
    DWORD dwChildIndex;
    WCHAR wszChildName[256];
    IDxDiagContainer* pChildContainer = NULL;
	string strParentName, strPropName, strPropVal;
	wstring wstrParentName, wstrPropName;

    hr = pDxDiagContainer->GetNumberOfProps( &dwPropCount );
    if( SUCCEEDED(hr) ) 
    {
		if( wszParentName != NULL )
			wstrParentName = wszParentName;
		else
			wstrParentName = L"";
		strParentName = WStringToString( wstrParentName );

        // Print each property in this container
        for( dwPropIndex = 0; dwPropIndex < dwPropCount; dwPropIndex++ )
        {
            hr = pDxDiagContainer->EnumPropNames( dwPropIndex, wszPropName, 256 );
			if( wszPropName != NULL )
				wstrPropName = wszPropName;
			else
				wstrPropName = L"";
			strPropName = WStringToString( wstrPropName );

			if( SUCCEEDED( hr ) )
			{
				if( bGetPropertyValues )
				{
					wstring wstrPropVal;
					hr = GetProperty( pDxDiagContainer, wszPropName, & wstrPropVal );
					strPropVal = WStringToString( wstrPropVal );

					fprintf( pOpenTextFile, "%35s : prop \"%s\" = %s\n", strParentName.c_str(), strPropName.c_str(), strPropVal.c_str() );
				}
				else
				{
					// otherwise, just list the property name and parent container info
					fprintf( pOpenTextFile, "%35s : prop \"%s\"\n", strParentName.c_str(), strPropName.c_str() );
				}
			}
        }
    }

    // Recursivly call this function for each of its child containers
    hr = pDxDiagContainer->GetNumberOfChildContainers( &dwChildCount );
    if( SUCCEEDED(hr) )
    {
        for( dwChildIndex = 0; dwChildIndex < dwChildCount; dwChildIndex++ )
        {
            hr = pDxDiagContainer->EnumChildContainerNames( dwChildIndex, wszChildName, 256 );
            if( SUCCEEDED(hr) )
            {
                hr = pDxDiagContainer->GetChildContainer( wszChildName, &pChildContainer );
                if( SUCCEEDED(hr) )
                {
                    // wszFullChildName isn't needed but is used for text output 
                    WCHAR wszFullChildName[256];

					// Append child name to parent and pass down in the recursion
					//  as full parent name.
                    if( wszParentName )
                        swprintf( wszFullChildName, L"%s.%s", wszParentName, wszChildName );
                    else
                        swprintf( wszFullChildName, L"%s", wszChildName );

					// recurse
                    ListAllDXDiagPropertyNamesToTxtFile( pOpenTextFile, bGetPropertyValues,
														 pChildContainer, wszFullChildName );
                    SAFE_RELEASE( pChildContainer );
                }
            }
        }
    }
	return( hr );
}


HRESULT DXDiagNVUtil::InitIDxDiagContainer()
{
	// Call FreeIDxDiagContainer() to clean up things done here
    HRESULT hr;

	if( m_pDxDiagRoot != NULL )
	{
		FMsg("DXDiagNVUtil::InitIDxDiagContainer already initialized!\n");
		return( S_OK );
	}
    
    // Init COM.  COM may fail if its already been inited with a different 
    // concurrency model.  And if it fails you shouldn't release it.
    hr = CoInitialize(NULL);
    m_bCleanupCOM = SUCCEEDED(hr);

    // Get an IDxDiagProvider
    hr = CoCreateInstance( CLSID_DxDiagProvider,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDxDiagProvider,
                           (LPVOID*) &m_pDxDiagProvider );
    if( SUCCEEDED(hr) )
    {
        // Fill out a DXDIAG_INIT_PARAMS struct
        DXDIAG_INIT_PARAMS dxDiagInitParam;
        ZeroMemory( &dxDiagInitParam, sizeof(DXDIAG_INIT_PARAMS) );
        dxDiagInitParam.dwSize                  = sizeof(DXDIAG_INIT_PARAMS);
        dxDiagInitParam.dwDxDiagHeaderVersion   = DXDIAG_DX9_SDK_VERSION;
        dxDiagInitParam.bAllowWHQLChecks        = false;
        dxDiagInitParam.pReserved               = NULL;

        // Init the m_m_pDxDiagProvider
        hr = m_pDxDiagProvider->Initialize( &dxDiagInitParam ); 
        if( SUCCEEDED(hr) )
        {
            // Get the DxDiag root container
            hr = m_pDxDiagProvider->GetRootContainer( & m_pDxDiagRoot );
			if( FAILED(hr) )
			{
				FMsg("Couldn't GetRootContainer from DxDiagProvider!\n");
				assert( false );
				FreeIDxDiagContainer();
				return( hr );
			}
		}
		else
		{
			FMsg("Couldn't Initialize DxDiagProvider!\n");
			assert( false );
			FreeIDxDiagContainer();
			return( hr );
		}
	}
	else
	{
		FMsg("Couldn't initialize COM!\n");
		assert( false );
		FreeIDxDiagContainer();
		return( hr );
	}

	return( hr );
}


HRESULT DXDiagNVUtil::FreeIDxDiagContainer()
{
	HRESULT hr = S_OK;
	SAFE_RELEASE( m_pDxDiagProvider );
	SAFE_RELEASE( m_pDxDiagRoot );

    if( m_bCleanupCOM )
	{
        CoUninitialize();
		m_bCleanupCOM = false;
	}

	return( hr );
}


}
