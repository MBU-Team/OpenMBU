//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/gfxD3D9Shader.h"
#include "gfx/D3D9/gfxD3D9Device.h"
#include "platform/platform.h"
#include "console/console.h"
#include "core/frameAllocator.h"
#include "shaderGen/shaderGenManager.h"
#include "core/fileStream.h"
#include "game/version.h"
#include "core/resManager.h"

//#define PRINT_SHADER_WARNINGS

_gfxD3DXIncludeRef GFXD3D9Shader::smD3DXInclude = NULL;

Vector<U8*> gIncludeAllocs;

HRESULT _gfxD3DXInclude::Open(THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, 
                              LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes, 
                              LPSTR pFullPath, DWORD cbFullPath)
{
   const char *psTemp = dStrrchr( _pShaderFileName, '/' );
   const char *tmpFileName = pFileName;

   FrameTemp<char> path( 512 );
   path[0] = 0;

   if( psTemp && !( dStrncmp( pFileName, "game:\\", 6 ) == 0 ) ) 
   {
      dsize_t loc = psTemp - _pShaderFileName + 1;
      dStrncpy( path, _pShaderFileName, loc );
      path[loc] = 0;  

      // Chomp off the '../'
      bool pushback = false;
      while( dStrncmp( tmpFileName, "../", 3 ) == 0 )
      {
         if( path[ dStrlen( path ) - 1 ] == '/' )
            path[ dStrlen( path ) - 1 ] = 0; // Kill the leading slash

         AssertISV( ( psTemp = dStrrchr( path, '/' ) ) != NULL, "Relative path error in shader include file" );
         path[psTemp - (char*)path] = 0;
         tmpFileName += 3;
         pushback = true;
      }
      if( pushback )
         tmpFileName--; // This is to put a slash back on it.
   }

   // Now cat what's left of the file name onto the end of what's left of the path
   dStrcat( path, tmpFileName );

   if( cbFullPath > 0 )
   {
      dStrncpy( pFullPath, path, getMin( cbFullPath, (U32)dStrlen( path ) ) );
      pFullPath[getMin( cbFullPath, (U32)dStrlen( path ) )] = 0;
   }
   else if( psTemp && pFullPath != NULL )
      pFullPath[0] = 0;

   // TODO: do we need to use makeFullPathName here?
   //char buff[256];
   //Platform::makeFullPathName( path, buff, sizeof( buff ) );

   //StringTableEntry ste = StringTable->insert( buff );
    StringTableEntry ste = StringTable->insert( path );

   //if( mIncludeFileName !=  ste )
   {
      mIncludeFileName = ste;

      Stream* fs;
      fs = ResourceManager->openStream(mIncludeFileName);
      if (fs == NULL)
      {
          // Try the default shaders/ dir
          char defPath[1024] = "shaders/";
          dStrcat(defPath, tmpFileName);
          StringTableEntry ste = StringTable->insert(defPath);
          mIncludeFileName = ste;
          fs = ResourceManager->openStream(mIncludeFileName);
      }
      if (fs == NULL)
      {
          AssertISV(false, "Something went horribly wrong with the ID3DXInclude stuff");
      }
      //FileStream fs;
      // AssertISV( fs.open( mIncludeFileName, FileStream::Read ), "Something went horribly wrong with the ID3DXInclude stuff" );

      //SAFE_DELETE_ARRAY( mIncludeData );

      mIncludeDataSize = fs->getStreamSize();
      mIncludeData = new U8[mIncludeDataSize];
      if(!fs->read( mIncludeDataSize, mIncludeData ))
      {
         // Read failed, cut our losses.
         mIncludeData[0] = 0;
      }
      ResourceManager->closeStream(fs);

      gIncludeAllocs.push_back(mIncludeData);
   }

   *pBytes = mIncludeDataSize;
   *ppData = mIncludeData;

   return S_OK;
}

HRESULT _gfxD3DXInclude::Close( THIS_ LPCVOID pData )
{
   return S_OK;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

GFXD3D9Shader::GFXD3D9Shader()
{
   mD3D9Device = dynamic_cast<GFXD3D9Device *>(GFX)->getDevice();
   vertShader = NULL;
   pixShader = NULL;

   if( smD3DXInclude == NULL )
      smD3DXInclude = new _gfxD3DXInclude;
}

//------------------------------------------------------------------------------

GFXD3D9Shader::~GFXD3D9Shader()
{
   SAFE_RELEASE( vertShader );
   SAFE_RELEASE( pixShader );
}

//----------------------------------------------------------------------------
// Init
//----------------------------------------------------------------------------
bool GFXD3D9Shader::init( const char *vertFile, const char *file, F32 pixVersion  )
{
   if( pixVersion > GFX->getPixelShaderVersion() )
      return false;

   if( pixVersion < 1.0 && file && file[0] )
      return false;

   char vertTarget[32];
   char pixTarget[32];

   U32 mjVer = (U32)mFloor( pixVersion );
   U32 mnVer = (U32)( ( pixVersion - F32( mjVer ) ) * 10.01 ); // 10.01 instead of 10.0 because of floating point issues

   dSprintf( vertTarget, sizeof(vertTarget), "vs_%d_%d", mjVer, mnVer );
   dSprintf( pixTarget, sizeof(pixTarget), "ps_%d_%d", mjVer, mnVer );

   // Adjust version for vertex shaders
   if( ( pixVersion < 2.f ) && ( pixVersion > 1.101f ) )
      dStrcpy( vertTarget, "vs_1_1" );

   initShader( vertFile, vertTarget );
   initShader( file, pixTarget );

   return true;
}

//------------------------------------------------------------------------------

void GFXD3D9Shader::initShader( const char *file, const char *target )
{
   if( !file || !file[0] ) 
      return;

   HRESULT res = D3DERR_INVALIDCALL;
   LPD3DXBUFFER code;
   LPD3DXBUFFER errorBuff;

#ifdef TORQUE_DEBUG
   U32 flags = D3DXSHADER_DEBUG;
#else
   U32 flags = 0;
#endif

#ifdef D3DXSHADER_USE_LEGACY_D3DX9_31_DLL
   if( D3DX_SDK_VERSION >= 32 )
   {
      // will need to use old compiler for 1_1 shaders - check for pixel
      // or vertex shader with appropriate version.
      if (target != NULL && (dStrnicmp(target, "vs1", 3) == 0 || dStrnicmp(target, "vs_1", 4) == 0))
         flags |= D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;

      if (target != NULL && (dStrnicmp(target, "ps1", 3) == 0 || dStrnicmp(target, "ps_1", 4) == 0))
         flags |= D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
   }
#endif

   // Create some #defines for better tool support
   D3DXMACRO defines[3];
   defines[0].Name = "TORQUE";
   defines[0].Definition = "1";
   defines[1].Name = "TORQUE_VERSION";
   char versionBuf[32];
   dSprintf(versionBuf, 31, "%d", getVersionNumber());   
   defines[1].Definition = versionBuf;
   defines[2].Name = NULL;
   defines[2].Definition = NULL;

   // compile HLSL shader
   if( dStrstr( (const char *)file, ".hlsl" ) )
   {
      Stream *s = ShaderGenManager::readShaderStream( file );

      FrameAllocatorMarker fam;
      char *buffer = NULL;

      // Set this so that the D3DXInclude::Open will have this information for
      // relative paths.
      smD3DXInclude->_pShaderFileName = file;

      if( s != NULL )
      {
         buffer = (char *)fam.alloc( s->getStreamSize() );
         s->read( s->getStreamSize(), buffer );

         res = GFXD3DX.D3DXCompileShader( buffer, s->getStreamSize(), defines, smD3DXInclude, "main", 
            target, flags, &code, &errorBuff, NULL );

         ShaderGenManager::closeShaderStream( s );
      }
      else
      {
         // Ok it's not in the shader gen manager, so ask Torque for it
         s = ResourceManager->openStream(file);
         if(!s)
         {
            AssertISV(false, avar("GFXD3D9Shader::initShader - failed to open shader '%s'.", file));
         }

         if( s != NULL )
         {
            buffer = (char *)fam.alloc( s->getStreamSize() );
            s->read( s->getStreamSize(), buffer );

            res = GFXD3DX.D3DXCompileShader( buffer, s->getStreamSize(), defines, smD3DXInclude, "main", 
               target, flags, &code, &errorBuff, NULL );

            ResourceManager->closeStream(s);
         }
         else
         {
            res = GFXD3DX.D3DXCompileShaderFromFileA( Platform::createPlatformFriendlyFilename( file ), 
               defines, smD3DXInclude, "main", target, 
               flags, &code, &errorBuff, NULL );
         }
      }
   }
   else
   {
      res = GFXD3DX.D3DXAssembleShaderFromFileA( file, 0, NULL, flags, &code, &errorBuff );
   }

   if(Con::getBoolVariable("$pref::assertOnBadShader", false))
   {
      AssertISV( res == D3D_OK, avar("GFXD3D9Shader::initShader - Unable to compile shader '%s'", file) );
   }

   // Wipe our allocations from this compile.
   while (gIncludeAllocs.size())
   {
      delete [] gIncludeAllocs.last();
      gIncludeAllocs.pop_back();
   }

   if( errorBuff )
   {
      // remove \n at end of buffer
      U8 *buffPtr = (U8*) errorBuff->GetBufferPointer();
      U32 len = dStrlen( (const char*) buffPtr );
      buffPtr[len-1] = '\0';

      if( res != D3D_OK )
      {
         Con::errorf("Error compiling shader: %s", file);
         Con::errorf( ConsoleLogEntry::General, (const char*) errorBuff->GetBufferPointer() );
      }
#ifdef PRINT_SHADER_WARNINGS
      else
      {
         Con::warnf("Warning when compiling shader: %s", file);
         Con::warnf( ConsoleLogEntry::General, (const char*) errorBuff->GetBufferPointer() );
      }
#endif
   }
   else if( code == NULL )
   {
      Con::errorf( "GFXD3D9Shader::initShader - no compiled code produced; possibly missing file '%s'?", file);
   }

   // Create the proper shader if we have code
   if( code != NULL )
   {
      if( dStrncmp( target, "ps_", 3 ) == 0 )
         res = mD3D9Device->CreatePixelShader( (DWORD*)code->GetBufferPointer(), &pixShader );
      else
         res = mD3D9Device->CreateVertexShader( (DWORD*)code->GetBufferPointer(), &vertShader );

      D3D9Assert( res, "Unable to create shader" );
   }

   SAFE_RELEASE( code );
   SAFE_RELEASE( errorBuff );
}

//------------------------------------------------------------------------------

void GFXD3D9Shader::process()
{
   GFX->setShader( this );
}

void GFXD3D9Shader::zombify()
{
   // Shaders don't need zombification
}

void GFXD3D9Shader::resurrect()
{
   // Shaders are never zombies, and therefore don't have to be brought back.
}