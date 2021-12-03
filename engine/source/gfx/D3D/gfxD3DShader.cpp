#include "d3dx9.h"
#include "gfxD3DShader.h"
#include "platform/platform.h"
#include "console/console.h"
#include "gfx/D3D/gfxD3DDevice.h"
#include <fstream>

//****************************************************************************
// GFX D3D Shader
//****************************************************************************
//#define ASSERT_ON_BAD_SHADER
//#define PRINT_SHADER_WARNINGS

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
GFXD3DShader::GFXD3DShader()
{
    mD3DDevice = dynamic_cast<GFXD3DDevice*>(GFX)->getDevice();
    vertShader = NULL;
    pixShader = NULL;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
GFXD3DShader::~GFXD3DShader()
{
    if (vertShader)
    {
        vertShader->Release();
        vertShader = NULL;
    }
    if (pixShader)
    {
        pixShader->Release();
        pixShader = NULL;
    }
}

//----------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------
bool GFXD3DShader::init(char* vertFile, char* pixFile, F32 pixVersion)
{

    if (pixVersion > GFX->getPixelShaderVersion())
    {
        return false;
    }

    if (pixVersion < 1.0 && pixFile && pixFile[0])
    {
        return false;
    }


    char vertTarget[32];
    char pixTarget[32];

    U32 mjVer = floor(pixVersion);
    U32 mnVer = (pixVersion - F32(mjVer)) * 10.01; // 10.01 instead of 10.0 because of floating point issues

    dSprintf(vertTarget, sizeof(vertTarget), "vs_%d_%d", mjVer, mnVer);
    dSprintf(pixTarget, sizeof(pixTarget), "ps_%d_%d", mjVer, mnVer);

    if ((pixVersion < 2.0) && (pixVersion > 1.101))
    {
        dStrcpy(vertTarget, "vs_1_1");
    }

    initVertShader(vertFile, vertTarget);
    initPixShader(pixFile, pixTarget);

    return true;
}

//--------------------------------------------------------------------------
// Init vertex shader
//--------------------------------------------------------------------------
void GFXD3DShader::initVertShader(char* vertFile, char* vertTarget)
{
    if (!vertFile || !vertFile[0]) return;

    HRESULT res;
    LPD3DXBUFFER code;
    LPD3DXBUFFER errorBuff;

    std::string newVertFile = vertFile;
    newVertFile += "x";

    goto compile;

    load:

    if (dStrstr((const char*)newVertFile.c_str(), ".hlslx"))
    {
        Con::printf("Loading Cached Vertex Shader: %s", newVertFile.c_str());

        std::fstream inFile(newVertFile.c_str(), std::ios::in | std::ios::binary);
        if (!inFile.is_open())
        {
            Con::errorf("GFXD3DShader::initVertShader - Could not open file %s", vertFile);
            return;
        }

        // Get file size
        inFile.seekg(0, std::ios::end);
        U32 fileSize = (U32)inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        // Read file into buffer
        char* fileBuffer = new char[fileSize];
        inFile.read(fileBuffer, fileSize);
        inFile.close();

        // create the shader
        res = mD3DDevice->CreateVertexShader((DWORD*)fileBuffer, &vertShader);
        D3DAssert(res, "Unable to create vertex shader");

        return;
    }

    compile:

    // compile HLSL shader
    if (dStrstr((const char*)vertFile, ".hlsl"))
    {
        res = D3DXCompileShaderFromFileA(vertFile, NULL, NULL, "main",
            vertTarget, D3DXSHADER_DEBUG, &code, &errorBuff, NULL);

        if (res != D3D_OK)
        {
            res = D3DXCompileShaderFromFileA(vertFile, NULL, NULL, "main",
                vertTarget, D3DXSHADER_DEBUG | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &code, &errorBuff, NULL);
        }

        if (errorBuff)
        {
            // remove \n at end of buffer
            U8* buffPtr = (U8*)errorBuff->GetBufferPointer();
            U32 len = dStrlen((const char*)buffPtr);
            buffPtr[len - 1] = '\0';


            if (res != D3D_OK)
            {
                Con::errorf(ConsoleLogEntry::General, (const char*)errorBuff->GetBufferPointer());
                goto load;
            }
            else
            {
#ifdef PRINT_SHADER_WARNINGS
                Con::warnf(ConsoleLogEntry::General, (const char*)errorBuff->GetBufferPointer());
#endif
            }
        }
        else if (!code)
        {
            Con::errorf("GFXD3DShader::initVertShader - no compiled code produced; possibly missing file '%s'?", vertFile);
            goto load;
        }

        if (res != D3D_OK)
        {
            Con::errorf(ConsoleLogEntry::General, "GFXD3DShader::initVertShader - unable to compile shader!");
#ifdef ASSERT_ON_BAD_SHADER
            AssertFatal(false, avar("Unable to compile vertex shader '%s'", vertFile));
#endif

            goto load;
        }
    }

    // assemble vertex shader
    else
    {
        res = D3DXAssembleShaderFromFileA(vertFile, 0, NULL, 0, &code, &errorBuff);

        if (res != D3D_OK)
        {
            res = D3DXAssembleShaderFromFileA(vertFile, 0, NULL, D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &code, &errorBuff);
        }

        if (res != D3D_OK)
        {
            Con::errorf(ConsoleLogEntry::General, (const char*)errorBuff->GetBufferPointer());

#ifdef ASSERT_ON_BAD_SHADER
            AssertFatal(false, "Unable to assemble vertex shader");
#endif

            goto load;
        }
    }

    if (code)
    {
        Con::printf("Caching Vertex Shader: %s", vertFile);
        void* data = code->GetBufferPointer();
        U32 len = code->GetBufferSize();
        std::fstream fout((std::string(vertFile) + "x").c_str(), std::ios::out | std::ios::binary);

        if (fout.is_open())
        {
            fout.write((const char*)data, len);
            fout.close();
        }

        // create the shader
        res = mD3DDevice->CreateVertexShader((DWORD*)code->GetBufferPointer(), &vertShader);
        D3DAssert(res, "Unable to create vertex shader");
    }

    if (code)
    {
        code->Release();
    }
    if (errorBuff)
    {
        errorBuff->Release();
    }

}


//--------------------------------------------------------------------------
// Init pixel shader
//--------------------------------------------------------------------------
void GFXD3DShader::initPixShader(char* pixFile, char* pixTarget)
{
    if (!pixFile || !pixFile[0]) return;

    HRESULT res;
    LPD3DXBUFFER code;
    LPD3DXBUFFER errorBuff;

    std::string newPixFile = pixFile;
    newPixFile += "x";

    goto compile;

    load:

    if (dStrstr((const char*)newPixFile.c_str(), ".hlslx"))
    {
        Con::printf("Loading Cached Pixel Shader: %s", newPixFile.c_str());

        std::fstream inFile(newPixFile.c_str(), std::ios::in | std::ios::binary);
        if (!inFile.is_open())
        {
            Con::errorf("GFXD3DShader::initPixShader - Could not open file %s", pixFile);
            return;
        }

        // Get file size
        inFile.seekg(0, std::ios::end);
        U32 fileSize = (U32)inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        // Read file into buffer
        char* fileBuffer = new char[fileSize];
        inFile.read(fileBuffer, fileSize);
        inFile.close();

        // create the shader
        res = mD3DDevice->CreatePixelShader((DWORD*)fileBuffer, &pixShader);
        D3DAssert(res, "Unable to create pixel shader");

        return;
    }

    compile:

    // compile HLSL shader
    if (dStrstr((const char*)pixFile, ".hlsl"))
    {
        res = D3DXCompileShaderFromFileA(pixFile, NULL, NULL, "main", pixTarget,
            D3DXSHADER_DEBUG, &code, &errorBuff, NULL);

        if (res != D3D_OK)
        {
            res = D3DXCompileShaderFromFileA(pixFile, NULL, NULL, "main", pixTarget,
                D3DXSHADER_DEBUG | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &code, &errorBuff, NULL);
        }

        if (errorBuff)
        {
            // remove \n at end of buffer
            U8* buffPtr = (U8*)errorBuff->GetBufferPointer();
            U32 len = dStrlen((const char*)buffPtr);
            buffPtr[len - 1] = '\0';


            if (res != D3D_OK)
            {
                Con::errorf(ConsoleLogEntry::General, (const char*)errorBuff->GetBufferPointer());
            }
            else
            {
#ifdef PRINT_SHADER_WARNINGS
                Con::warnf(ConsoleLogEntry::General, (const char*)errorBuff->GetBufferPointer());
#endif
            }
        }
        else if (!code)
        {
            Con::errorf("GFXD3DShader::initPixShader - no compiled code produced; possibly missing file '%s'?", pixFile);

            goto load;
        }

        if (res != D3D_OK)
        {
#ifdef ASSERT_ON_BAD_SHADER
            AssertFatal(false, avar("Unable to compile pixel shader '%s'", pixFile));
#endif

            goto load;
        }
    }

    // assemble pixel shader
    else
    {
        res = D3DXAssembleShaderFromFileA(pixFile, 0, NULL, D3DXSHADER_DEBUG, &code, &errorBuff);

        if (res != D3D_OK)
        {
            res = D3DXAssembleShaderFromFileA(pixFile, 0, NULL, D3DXSHADER_DEBUG | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &code, &errorBuff);
        }

        if (res != D3D_OK)
        {
            Con::errorf(ConsoleLogEntry::General, (const char*)errorBuff->GetBufferPointer());

#ifdef ASSERT_ON_BAD_SHADER
            AssertFatal(false, "Unable to assemble pixel shader");
#endif

            goto load;
        }
    }

    if (code)
    {
        Con::printf("Caching Pixel Shader: %s", pixFile);
        void* data = code->GetBufferPointer();
        U32 len = code->GetBufferSize();
        std::fstream fout((std::string(pixFile) + "x").c_str(), std::ios::out | std::ios::binary);

        if (fout.is_open())
        {
            fout.write((const char*)data, len);
            fout.close();
        }

        // create the shader
        res = mD3DDevice->CreatePixelShader((DWORD*)code->GetBufferPointer(), &pixShader);
        D3DAssert(res, "Unable to create pixel shader");
    }

    if (code)
    {
        code->Release();
    }
    if (errorBuff)
    {
        errorBuff->Release();
    }
}

//--------------------------------------------------------------------------
// Process
//--------------------------------------------------------------------------
void GFXD3DShader::process()
{
    GFX->setShader(this);
    //   mD3DDevice->SetVertexShader( vertShader );
    //   mD3DDevice->SetPixelShader( pixShader );
}
