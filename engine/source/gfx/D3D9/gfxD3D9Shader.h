//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _GFXD3D9SHADER_H_
#define _GFXD3D9SHADER_H_

#include "gfx/D3D9/platformD3D.h"
#include "gfx/gfxShader.h"
#include "core/refBase.h"
#include "util/safeDelete.h"
#include "gfx/gfxResource.h"

//------------------------------------------------------------------------------

class _gfxD3DXInclude : public ID3DXInclude, public RefBase
{
private:
   U8 *mIncludeData;
   dsize_t mIncludeDataSize;
   StringTableEntry mIncludeFileName;

public:
   const char *_pShaderFileName;

   _gfxD3DXInclude() : mIncludeFileName( NULL ), mIncludeData( NULL ), mIncludeDataSize( 0 ) {}
   ~_gfxD3DXInclude() { /*SAFE_DELETE_ARRAY( mIncludeData );*/ }

   STDMETHOD(Close)(THIS_ LPCVOID pData);

   // 360 
   STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes, /* OUT */ LPSTR pFullPath, DWORD cbFullPath);

   // PC
   STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
   {
      return Open( IncludeType, pFileName, pParentData, ppData, pBytes, NULL, 0 );
   }
};

typedef RefPtr<_gfxD3DXInclude> _gfxD3DXIncludeRef;

//------------------------------------------------------------------------------

class GFXD3D9Shader : public GFXShader
{
   friend class GFXD3D9Device;
   friend class GFX360Device;
   friend class GFXD3D9Effect; // let it access our include stuff

protected:
   LPDIRECT3DDEVICE9 mD3D9Device;

   IDirect3DVertexShader9 *vertShader;
   IDirect3DPixelShader9 *pixShader;

   static _gfxD3DXIncludeRef smD3DXInclude;

   virtual void initShader( const char *file, const char *target );

public:
   GFXD3D9Shader();
   virtual ~GFXD3D9Shader();
   
   virtual bool init( const char *vertFile, const char *pixFile, F32 pixVersion );
   virtual void process();

   // GFXResource interface
   virtual void zombify();
   virtual void resurrect();

};

#endif

