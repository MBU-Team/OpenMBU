//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gfx/D3D9/gfxD3D9Effect.h"
#include "gfx/D3D9/gfxD3D9Shader.h"
#include "gfx/D3D9/gfxD3D9Device.h"
#include "gfx/D3D9/gfxD3D9TextureObject.h"
#include "util/safeRelease.h"
#include "core/fileStream.h"
#include "core/frameAllocator.h"
#include "game/version.h"

//TODO: add support for offline-compiled effects

GFXD3D9Effect::GFXD3D9Effect()
{
   mEffect = NULL;
}

GFXD3D9Effect::~GFXD3D9Effect()
{
   SAFE_RELEASE( mEffect );
}

bool GFXD3D9Effect::init( const char *file, LPD3DXEFFECTPOOL pool )
{
   if( !file || !file[ 0 ] )
      return false;

   GFXD3D9Shader::smD3DXInclude->_pShaderFileName = file;

   // Read the effect file into memory.

   FileStream* stream = new FileStream();
   if( !stream->open( file, FileStream::Read ) )
   {
      Con::errorf( "GFXD3D9Effect::init - failed to open effect file '%s'", file );
      return false;
   }

   FrameAllocatorMarker fam;
   U32 numBytes = stream->getStreamSize();
   char* buffer = ( char* ) fam.alloc( numBytes );
   stream->read( numBytes, buffer );
   stream->close();
   SAFE_DELETE( stream );

   // Create some #defines for better tool support (ripped straight from shader code).

   D3DXMACRO defines[3];
   defines[0].Name = "TORQUE";
   defines[0].Definition = "1";
   defines[1].Name = "TORQUE_VERSION";
   char versionBuf[32];
   dSprintf(versionBuf, 31, "%d", getVersionNumber());   
   defines[1].Definition = versionBuf;
   defines[2].Name = NULL;
   defines[2].Definition = NULL;

   // Compile the effect.

   LPD3DXEFFECT effect;
   LPD3DXBUFFER errors;

#ifdef TORQUE_DEBUG
   U32 flags = D3DXSHADER_DEBUG;
#else
   U32 flags = D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif

   HRESULT result = GFXD3DX.D3DXCreateEffect(
      static_cast< GFXD3D9Device* >( GFX )->getDevice(),
      buffer, numBytes, defines, GFXD3D9Shader::smD3DXInclude, flags,
      pool, &effect, &errors );

   if( errors )
   {
      // remove \n at end of buffer
      U8 *buffPtr = (U8*) errors->GetBufferPointer();
      U32 len = dStrlen( (const char*) buffPtr );
      buffPtr[len-1] = '\0';

      if( result != D3D_OK )
      {
         Con::errorf( "Error compiling effect: %s", file );
         Con::errorf( ConsoleLogEntry::General, ( const char* ) errors->GetBufferPointer() );
      }
      else
      {
         Con::warnf( "Warning when compiling effect: %s", file );
         Con::warnf( ConsoleLogEntry::General, ( const char* ) errors->GetBufferPointer() );
      }
   }
   SAFE_RELEASE( errors );

   if( result != S_OK )
   {
      Con::errorf( "GFXD3D9Effect::init - failed to create effect for '%s'", file );
      return false;
   }

   mEffect = effect;
   return true;
}

void GFXD3D9Effect::setTechnique( const char* name )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setTechnique -- no compiled effect" );
   mEffect->SetTechnique( (D3DXHANDLE)name );
}

U32 GFXD3D9Effect::begin( bool dontSaveState )
{
   AssertFatal( mEffect, "GFXD3D9Effect::begin -- no compiled effect" );
   U32 numPasses = 0;
   DWORD flags = 0;
   if( dontSaveState )
      flags |= D3DXFX_DONOTSAVESTATE;
   mEffect->Begin( &numPasses, flags );
   return numPasses;
}

void GFXD3D9Effect::end()
{
   AssertFatal( mEffect, "GFXD3D9Effect::end -- no compiled effect" );
   mEffect->End();
}

void GFXD3D9Effect::beginPass( U32 n )
{
   AssertFatal( mEffect, "GFXD3D9Effect::beginPass -- no compiled effect" );
   mEffect->BeginPass( n );
}

void GFXD3D9Effect::endPass()
{
   AssertFatal( mEffect, "GFXD3D9Effect::endPass -- no compiled effect" );
   mEffect->EndPass();
}

void GFXD3D9Effect::commit()
{
   AssertFatal( mEffect, "GFXD3D9Effect::commit -- no compiled effect" );
   mEffect->CommitChanges();
}

void GFXD3D9Effect::setBool( HANDLE parameter, bool value )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setBool -- no compiled effect" );
   mEffect->SetBool( ( D3DXHANDLE ) parameter, value );
}

void GFXD3D9Effect::setFloat( HANDLE parameter, F32 value )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setFloat -- no compiled effect" );
   mEffect->SetFloat( ( D3DXHANDLE ) parameter, value );
}

void GFXD3D9Effect::setFloatArray( HANDLE parameter, F32* values, U32 count )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setFloatArray -- no compiled effect" );
   mEffect->SetFloatArray( ( D3DXHANDLE ) parameter, values, count );
}

void GFXD3D9Effect::setInt( HANDLE parameter, S32 value )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setInt -- no compiled effect" );
   mEffect->SetInt( ( D3DXHANDLE ) parameter, value );
}

void GFXD3D9Effect::setVector( HANDLE parameter, const Point4F& vector )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setVector -- no compiled effect" );
   mEffect->SetVector( ( D3DXHANDLE ) parameter, reinterpret_cast< const D3DXVECTOR4* >( &vector ) );
}

void GFXD3D9Effect::setVectorArray( HANDLE parameter, const Point4F* vectors, U32 count )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setVectorArray -- no compiled effect" );
   mEffect->SetVectorArray( ( D3DXHANDLE ) parameter, reinterpret_cast< const D3DXVECTOR4* >( vectors ), count );
}

void GFXD3D9Effect::setTexture( HANDLE parameter, GFXTexHandle texture )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setTexture -- no compiled effect" );
   GFXTextureObject* texObject = texture;
   LPDIRECT3DBASETEXTURE9 tex = static_cast< GFXD3D9TextureObject* >( texObject )->getTex();
   mEffect->SetTexture( ( D3DXHANDLE ) parameter, tex );
}

void GFXD3D9Effect::setMatrix( HANDLE parameter, const MatrixF& matrix )
{
   AssertFatal( mEffect, "GFXD3D9Effect::setMatrix -- no compiled effect" );
   mEffect->SetMatrixTranspose( ( D3DXHANDLE ) parameter, ( const D3DXMATRIX* ) static_cast< F32* >( matrix ) );
}

GFXD3D9Effect::HANDLE GFXD3D9Effect::getParameterElement( HANDLE parameter, U32 index )
{
   AssertFatal( mEffect, "GFXD3D9Effect::getParameterElement -- no compiled effect" );
   return (GFXD3D9Effect::HANDLE)mEffect->GetParameterElement( ( D3DXHANDLE ) parameter, index );
}
