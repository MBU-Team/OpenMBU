//------------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (c) GarageGames.Com
//------------------------------------------------------------------------------
#include <d3d9.h>
#include "gfx/gfxEnums.h"

//------------------------------------------------------------------------------

namespace GFXD3DEnumTranslate
{
    void init();
};

//------------------------------------------------------------------------------

extern _D3DFORMAT GFXD3DIndexFormat[GFXIndexFormat_COUNT];
extern _D3DSAMPLERSTATETYPE GFXD3DSamplerState[GFXSAMP_COUNT];
extern _D3DFORMAT GFXD3DTextureFormat[GFXFormat_COUNT];
extern _D3DRENDERSTATETYPE GFXD3DRenderState[GFXRenderState_COUNT];
extern _D3DTEXTUREFILTERTYPE GFXD3DTextureFilter[GFXTextureFilter_COUNT];
extern _D3DBLEND GFXD3DBlend[GFXBlend_COUNT];
extern _D3DBLENDOP GFXD3DBlendOp[GFXBlendOp_COUNT];
extern _D3DSTENCILOP GFXD3DStencilOp[GFXStencilOp_COUNT];
extern _D3DCMPFUNC GFXD3DCmpFunc[GFXCmp_COUNT];
extern _D3DCULL GFXD3DCullMode[GFXCull_COUNT];
extern _D3DPRIMITIVETYPE GFXD3DPrimType[GFXPT_COUNT];
extern _D3DTEXTURESTAGESTATETYPE GFXD3DTextureStageState[GFXTSS_COUNT];
extern _D3DTEXTUREADDRESS GFXD3DTextureAddress[GFXAddress_COUNT];
extern _D3DTEXTUREOP GFXD3DTextureOp[GFXTOP_COUNT];

#define GFXREVERSE_LOOKUP( tablearray, enumprefix, val ) \
   for( int i = enumprefix##_FIRST; i < enumprefix##_COUNT; i++ ) \
      if( (int)tablearray [i] == val ) \
      { \
         val = i; \
         break; \
      } \

