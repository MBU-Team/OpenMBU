//------------------------------------------------------------------------------
// Configuration for DXVersionChecker
//------------------------------------------------------------------------------

// Required D3DX9 DLL Version
// Depending on the SDK update you are using, there will be a different version
// of the DLL that is required. December 2005 is d3dx9_28.dll
#define D3DXDLL_VER 28

// Subdirectory off game root which contains installer
// ex: c:\tse_game\tse.exe 
// will be
// c:\tse_game\dxsetup
#define DXINSTALLER_DIR L"dxsetup"

// File name of dx installer, using the last example, this will be
// c:\tse_game\dxsetup\dxsetup.exe
#define DXINSTALLER_EXE L"dxsetup.exe"

// If this is defined, than TSE will not ask the user if they want to upgrade,
// it will simply run the dxupdate installer, if the user needs to upgrade.
// Disabled by default.
//#define DXINSTALL_NOPROMPT

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#include <windows.h>
#include <wchar.h>
#include <strsafe.h>

extern HRESULT GetDXVersion(DWORD* pdwDirectXVersion, TCHAR* strDirectXVersion, int cchDirectXVersion);

//------------------------------------------------------------------------------

// Variables to get filled by initVersionStrings
WCHAR gDXVersionString[64] = { 0 };
DWORD gD3DXDLLVersion = 0;
DWORD gDXVersion = 0;

//------------------------------------------------------------------------------

// Helper macro to help add versions
#define MAKE_DX_CASE( dwDXVersion, wcDXVersionString, dwD3DXDLLVersion ) \
   case dwD3DXDLLVersion: \
      StringCbCopyW( gDXVersionString, sizeof( gDXVersionString ) / sizeof( WCHAR ), wcDXVersionString ); \
      gD3DXDLLVersion = dwD3DXDLLVersion; \
      gDXVersion = dwDXVersion; \
      break; \

#define DX90C_VER 0x00090003

void initVersionStrings(DWORD d3dxdllver)
{
    // This is really only relevant to the version of d3dx9_x.dll you have, so
    // all of these are going to deal with DirectX 9.0c and higher. At the time
    // of writing this code, December 2005 is the earliest version of DirectX
    // that is valid, because of the XInput code. So it is going to start
    // with December 2005, and go up. You may need to input future version
    // information here as necessary.
    switch (d3dxdllver)
    {
        // December 2005 DirectX 9.0c
        // 0x00090003
        // d3dx9_28.dll
        MAKE_DX_CASE(DX90C_VER, L"December 2005 DirectX 9.0c", 28);

        // February 2005 DirectX 9.0c
        // 0x00090003
        // d3dx9_29.dll
        MAKE_DX_CASE(DX90C_VER, L"February 2005 DirectX 9.0c", 29);

        // Default case. Spit out a warning, but try to assign workable values
        // anyway.
    default:
        StringCbCopyW(gDXVersionString, sizeof(gDXVersionString) / sizeof(WCHAR), L"Unknown DirectX Version Id");
        gD3DXDLLVersion = d3dxdllver;
        gDXVersion = DX90C_VER;

        OutputDebugString(L"DirectX Version Warning! Unknown DirectX version specified. Please check code in platformWin32/dxVersionChecker.cpp\n");
        break;
    }
}

//------------------------------------------------------------------------------

// This code comes from the DirectX Dev Mailing list, courtesy of Herb Marselas
// in his post on Feb 24th 2006
bool checkDXVersion()
{
    // Init the strings and such
    initVersionStrings(D3DXDLL_VER);

    DWORD dwVersion = 0;

    // call dx version check supplied by the SDK sample
    HRESULT hr = GetDXVersion(&dwVersion, 0, 0);

    // If there is a dx version, check the version against the desired DLL 
    // version and deal with it.
    if (SUCCEEDED(hr))
    {
        WCHAR cPath[_MAX_PATH] = { 0 }, cFilename[_MAX_PATH] = { 0 };

        GetWindowsDirectory(cPath, _MAX_PATH);

        StringCbPrintfW(cFilename, _MAX_PATH, L"%s\\system32\\d3dx9_%d.dll", cPath, gD3DXDLLVersion);

        HANDLE hFile = CreateFile(cFilename, GENERIC_READ,
            FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        CloseHandle(hFile);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            OutputDebugString(L"DX Version Checker: Requested D3DX DLL version not found!\n");
            dwVersion = 0;
        }
    }

    // Check version
    if (dwVersion < gDXVersion)
    {
#ifndef DXINSTALL_NOPROMPT
        WCHAR promptMessage[512];
        StringCbPrintfW(promptMessage, sizeof(promptMessage) / sizeof(WCHAR),
            L"This application requires the %s or later.\n Press OK to install "
            L"%s, or Cancel to exit.", gDXVersionString, gDXVersionString);

        long dw = MessageBoxW(0, promptMessage, L"DirectX Version Checker", MB_OKCANCEL | MB_ICONEXCLAMATION);

        // If they cancel, just return false and let the init() deal with it
        if (dw == IDCANCEL)
            return false;
#endif

        // find out where we are
        WCHAR cDrive[_MAX_DRIVE] = { 0 };
        WCHAR cDir[_MAX_DIR] = { 0 };
        WCHAR cFilename[_MAX_FNAME] = { 0 };
        WCHAR cExt[_MAX_EXT] = { 0 };

        // parse path
        WCHAR cPath[_MAX_PATH] = { 0 };

        GetModuleFileName(0, cPath, _MAX_PATH);

        // Path split
        _wsplitpath(cPath, cDrive, cDir, cFilename, cExt);


        StringCbPrintfW(cPath, _MAX_PATH, L"%s%s%s", cDrive, cDir, DXINSTALLER_DIR);
        StringCbPrintfW(cFilename, _MAX_FNAME, L"%s\\%s", cPath, DXINSTALLER_EXE);

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));

        BOOL ret = CreateProcess(0, cFilename, 0, 0, TRUE, 0, 0, cPath, &si, &pi);

        if (!ret)
        {
            WCHAR failMessage[256];

            StringCbPrintfW(failMessage, sizeof(failMessage) / sizeof(WCHAR),
                L"Failed to launch %s setup.\nApplication may not run correctly", gDXVersionString);

            MessageBoxW(0, failMessage, L"DirectX Version Checker", MB_OKCANCEL | MB_ICONEXCLAMATION);

            return false;
        }

        // Wait until setup process exits
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Close process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return true;
}