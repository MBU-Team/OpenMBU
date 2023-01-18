//------------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (c) GarageGames.Com
//------------------------------------------------------------------------------

#include "gfx/D3D9/gfxD3D9Shader.h"
#include "gfx/gfxEnums.h"

//------------------------------------------------------------------------------

namespace GFXD3D9EnumTranslate
{
   void init();
};

//------------------------------------------------------------------------------

extern _D3DFORMAT GFXD3D9IndexFormat[GFXIndexFormat_COUNT];
extern _D3DSAMPLERSTATETYPE GFXD3D9SamplerState[GFXSAMP_COUNT];
extern _D3DFORMAT GFXD3D9TextureFormat[GFXFormat_COUNT];
#ifdef TORQUE_OS_XENON
extern _D3DFORMAT GFXD3D9TiledTextureFormat[GFXFormat_COUNT];
extern _D3DFORMAT GFXD3D9RenderTargetFormat[GFXFormat_COUNT];
#endif
extern _D3DRENDERSTATETYPE GFXD3D9RenderState[GFXRenderState_COUNT];
extern _D3DTEXTUREFILTERTYPE GFXD3D9TextureFilter[GFXTextureFilter_COUNT];
extern _D3DBLEND GFXD3D9Blend[GFXBlend_COUNT];
extern _D3DBLENDOP GFXD3D9BlendOp[GFXBlendOp_COUNT];
extern _D3DSTENCILOP GFXD3D9StencilOp[GFXStencilOp_COUNT];
extern _D3DCMPFUNC GFXD3D9CmpFunc[GFXCmp_COUNT];
extern _D3DCULL GFXD3D9CullMode[GFXCull_COUNT];
extern _D3DPRIMITIVETYPE GFXD3D9PrimType[GFXPT_COUNT];
extern _D3DTEXTURESTAGESTATETYPE GFXD3D9TextureStageState[GFXTSS_COUNT];
extern _D3DTEXTUREADDRESS GFXD3D9TextureAddress[GFXAddress_COUNT];
extern _D3DTEXTUREOP GFXD3D9TextureOp[GFXTOP_COUNT];

#define GFXREVERSE_LOOKUP( tablearray, enumprefix, val ) \
   for( int i = enumprefix##_FIRST; i < enumprefix##_COUNT; i++ ) \
      if( (int)tablearray[i] == val ) \
      { \
         val = i; \
         break; \
      } \

