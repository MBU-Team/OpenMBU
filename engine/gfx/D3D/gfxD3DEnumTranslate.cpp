//------------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (c) GarageGames.Com
//------------------------------------------------------------------------------

#include "gfx/D3D/gfxD3DEnumTranslate.h"
#include "console/console.h"

//------------------------------------------------------------------------------

_D3DFORMAT GFXD3DIndexFormat[GFXIndexFormat_COUNT];
_D3DSAMPLERSTATETYPE GFXD3DSamplerState[GFXSAMP_COUNT];
_D3DFORMAT GFXD3DTextureFormat[GFXFormat_COUNT];
_D3DRENDERSTATETYPE GFXD3DRenderState[GFXRenderState_COUNT];
_D3DTEXTUREFILTERTYPE GFXD3DTextureFilter[GFXTextureFilter_COUNT];
_D3DBLEND GFXD3DBlend[GFXBlend_COUNT];
_D3DBLENDOP GFXD3DBlendOp[GFXBlendOp_COUNT];
_D3DSTENCILOP GFXD3DStencilOp[GFXStencilOp_COUNT];
_D3DCMPFUNC GFXD3DCmpFunc[GFXCmp_COUNT];
_D3DCULL GFXD3DCullMode[GFXCull_COUNT];
_D3DPRIMITIVETYPE GFXD3DPrimType[GFXPT_COUNT];
_D3DTEXTURESTAGESTATETYPE GFXD3DTextureStageState[GFXTSS_COUNT];
_D3DTEXTUREADDRESS GFXD3DTextureAddress[GFXAddress_COUNT];
_D3DTEXTUREOP GFXD3DTextureOp[GFXTOP_COUNT];

//------------------------------------------------------------------------------

#define GFXD3D_UNSUPPORTED_VAL 0xDEADBEEF
#define GFXD3D_UNINIT_VAL 0xDECAFBAD

#define INIT_LOOKUPTABLE( tablearray, enumprefix, type ) \
   for( int i = enumprefix##_FIRST; i < enumprefix##_COUNT; i++ ) \
      tablearray##[i] = (##type##)GFXD3D_UNINIT_VAL;

#define VALIDATE_LOOKUPTABLE( tablearray, enumprefix ) \
   for( int i = enumprefix##_FIRST; i < enumprefix##_COUNT; i++ ) \
      if( (int)tablearray##[i] == GFXD3D_UNINIT_VAL ) \
         Con::warnf( "GFXD3DEnumTranslate: Unassigned value in " #tablearray ": %i", i ); \
      else if( (int)tablearray##[i] == GFXD3D_UNSUPPORTED_VAL ) \
         Con::warnf( "GFXD3DEnumTranslate: Unsupported value in " #tablearray ": %i", i );

//------------------------------------------------------------------------------

void GFXD3DEnumTranslate::init()
{
   INIT_LOOKUPTABLE( GFXD3DIndexFormat, GFXIndexFormat, _D3DFORMAT );
   GFXD3DIndexFormat[GFXIndexFormat16] = D3DFMT_INDEX16;
   GFXD3DIndexFormat[GFXIndexFormat32] = D3DFMT_INDEX32;
   VALIDATE_LOOKUPTABLE( GFXD3DIndexFormat, GFXIndexFormat );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DSamplerState, GFXSAMP, _D3DSAMPLERSTATETYPE );
   GFXD3DSamplerState[GFXSAMPAddressU] = D3DSAMP_ADDRESSU;
   GFXD3DSamplerState[GFXSAMPAddressV] = D3DSAMP_ADDRESSV;
   GFXD3DSamplerState[GFXSAMPAddressW] = D3DSAMP_ADDRESSW;
   GFXD3DSamplerState[GFXSAMPBorderColor] = D3DSAMP_BORDERCOLOR;
   GFXD3DSamplerState[GFXSAMPMagFilter] = D3DSAMP_MAGFILTER;
   GFXD3DSamplerState[GFXSAMPMinFilter] = D3DSAMP_MINFILTER;
   GFXD3DSamplerState[GFXSAMPMipFilter] = D3DSAMP_MIPFILTER;
   GFXD3DSamplerState[GFXSAMPMipMapLODBias] = D3DSAMP_MIPMAPLODBIAS;
   GFXD3DSamplerState[GFXSAMPMaxMipLevel] = D3DSAMP_MAXMIPLEVEL;
   GFXD3DSamplerState[GFXSAMPMaxAnisotropy] = D3DSAMP_MAXANISOTROPY;
   GFXD3DSamplerState[GFXSAMPSRGBTexture] = D3DSAMP_SRGBTEXTURE;
   GFXD3DSamplerState[GFXSAMPElementIndex] = D3DSAMP_ELEMENTINDEX;
   GFXD3DSamplerState[GFXSAMPDMapOffset] = D3DSAMP_DMAPOFFSET;
   VALIDATE_LOOKUPTABLE( GFXD3DSamplerState, GFXSAMP );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DTextureFormat, GFXFormat, _D3DFORMAT );
   GFXD3DTextureFormat[GFXFormatR8G8B8] = D3DFMT_R8G8B8;
   GFXD3DTextureFormat[GFXFormatR8G8B8A8] = D3DFMT_A8R8G8B8;
   GFXD3DTextureFormat[GFXFormatR8G8B8X8] = D3DFMT_X8R8G8B8;
   GFXD3DTextureFormat[GFXFormatR5G6B5] = D3DFMT_R5G6B5;
   GFXD3DTextureFormat[GFXFormatR5G5B5A1] = D3DFMT_A1R5G5B5;
   GFXD3DTextureFormat[GFXFormatR5G5B5X1] = D3DFMT_X1R5G5B5;
   GFXD3DTextureFormat[GFXFormatA8] = D3DFMT_A8;
   GFXD3DTextureFormat[GFXFormatP8] = D3DFMT_P8;
   GFXD3DTextureFormat[GFXFormatL8] = D3DFMT_L8;
   GFXD3DTextureFormat[GFXFormatDXT1] = D3DFMT_DXT1;
   GFXD3DTextureFormat[GFXFormatDXT2] = D3DFMT_DXT2;
   GFXD3DTextureFormat[GFXFormatDXT3] = D3DFMT_DXT3;
   GFXD3DTextureFormat[GFXFormatDXT4] = D3DFMT_DXT4;
   GFXD3DTextureFormat[GFXFormatDXT5] = D3DFMT_DXT5;
   GFXD3DTextureFormat[GFXFormatR32G32B32A32F] = D3DFMT_A32B32G32R32F;
   GFXD3DTextureFormat[GFXFormatR16G16B16A16F] = D3DFMT_A16B16G16R16F;
   GFXD3DTextureFormat[GFXFormatL16] = D3DFMT_L16;
   GFXD3DTextureFormat[GFXFormatR16G16B16A16] = D3DFMT_A16B16G16R16;
   GFXD3DTextureFormat[GFXFormatR16G16] = D3DFMT_G16R16;
   GFXD3DTextureFormat[GFXFormatR16F] = D3DFMT_R16F;
   GFXD3DTextureFormat[GFXFormatR16G16F] = D3DFMT_G16R16F;
   GFXD3DTextureFormat[GFXFormatR10G10B10A2] = D3DFMT_A2R10G10B10;
   GFXD3DTextureFormat[GFXFormatD32] = D3DFMT_D32;
   GFXD3DTextureFormat[GFXFormatD24X8] = D3DFMT_D24X8;
   GFXD3DTextureFormat[GFXFormatD16] = D3DFMT_D16;
   VALIDATE_LOOKUPTABLE( GFXD3DTextureFormat, GFXFormat);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DRenderState, GFXRenderState, _D3DRENDERSTATETYPE );
   GFXD3DRenderState[GFXRSZEnable] = D3DRS_ZENABLE;
   GFXD3DRenderState[GFXRSFillMode] = D3DRS_FILLMODE;
   GFXD3DRenderState[GFXRSZWriteEnable] = D3DRS_ZWRITEENABLE;
   GFXD3DRenderState[GFXRSAlphaTestEnable] = D3DRS_ALPHATESTENABLE;
   GFXD3DRenderState[GFXRSSrcBlend] = D3DRS_SRCBLEND;
   GFXD3DRenderState[GFXRSDestBlend] = D3DRS_DESTBLEND;
   GFXD3DRenderState[GFXRSCullMode] = D3DRS_CULLMODE;
   GFXD3DRenderState[GFXRSZFunc] = D3DRS_ZFUNC;
   GFXD3DRenderState[GFXRSAlphaRef] = D3DRS_ALPHAREF;
   GFXD3DRenderState[GFXRSAlphaFunc] = D3DRS_ALPHAFUNC;
   GFXD3DRenderState[GFXRSAlphaBlendEnable] = D3DRS_ALPHABLENDENABLE;
   GFXD3DRenderState[GFXRSStencilEnable] = D3DRS_STENCILENABLE;
   GFXD3DRenderState[GFXRSStencilFail] = D3DRS_STENCILFAIL;
   GFXD3DRenderState[GFXRSStencilZFail] = D3DRS_STENCILZFAIL;
   GFXD3DRenderState[GFXRSStencilPass] = D3DRS_STENCILPASS;
   GFXD3DRenderState[GFXRSStencilFunc] = D3DRS_STENCILFUNC;
   GFXD3DRenderState[GFXRSStencilRef] = D3DRS_STENCILREF;
   GFXD3DRenderState[GFXRSStencilMask] = D3DRS_STENCILMASK;
   GFXD3DRenderState[GFXRSStencilWriteMask] = D3DRS_STENCILWRITEMASK;
   GFXD3DRenderState[GFXRSWrap0] = D3DRS_WRAP0;
   GFXD3DRenderState[GFXRSWrap1] = D3DRS_WRAP1;
   GFXD3DRenderState[GFXRSWrap2] = D3DRS_WRAP2;
   GFXD3DRenderState[GFXRSWrap3] = D3DRS_WRAP3;
   GFXD3DRenderState[GFXRSWrap4] = D3DRS_WRAP4;
   GFXD3DRenderState[GFXRSWrap5] = D3DRS_WRAP5;
   GFXD3DRenderState[GFXRSWrap6] = D3DRS_WRAP6;
   GFXD3DRenderState[GFXRSWrap7] = D3DRS_WRAP7;
   GFXD3DRenderState[GFXRSClipPlaneEnable] = D3DRS_CLIPPLANEENABLE;
   GFXD3DRenderState[GFXRSPointSize] = D3DRS_POINTSIZE;
   GFXD3DRenderState[GFXRSPointSizeMin] = D3DRS_POINTSIZE_MIN;
   GFXD3DRenderState[GFXRSPointSize_Max] = D3DRS_POINTSIZE_MAX;
   GFXD3DRenderState[GFXRSPointSpriteEnable] = D3DRS_POINTSPRITEENABLE;
   GFXD3DRenderState[GFXRSMultiSampleantiAlias] = D3DRS_MULTISAMPLEANTIALIAS;
   GFXD3DRenderState[GFXRSMultiSampleMask] = D3DRS_MULTISAMPLEMASK;
   GFXD3DRenderState[GFXRSShadeMode] = D3DRS_SHADEMODE;
   GFXD3DRenderState[GFXRSLastPixel] = D3DRS_LASTPIXEL;
   GFXD3DRenderState[GFXRSClipping] = D3DRS_CLIPPING;
   GFXD3DRenderState[GFXRSPointScaleEnable] = D3DRS_POINTSCALEENABLE;
   GFXD3DRenderState[GFXRSPointScale_A] = D3DRS_POINTSCALE_A;
   GFXD3DRenderState[GFXRSPointScale_B] = D3DRS_POINTSCALE_B;
   GFXD3DRenderState[GFXRSPointScale_C] = D3DRS_POINTSCALE_C;
   GFXD3DRenderState[GFXRSLighting] = D3DRS_LIGHTING;
   GFXD3DRenderState[GFXRSAmbient] = D3DRS_AMBIENT;
   GFXD3DRenderState[GFXRSFogVertexMode] = D3DRS_FOGVERTEXMODE;
   GFXD3DRenderState[GFXRSColorVertex] = D3DRS_COLORVERTEX;
   GFXD3DRenderState[GFXRSLocalViewer] = D3DRS_LOCALVIEWER;
   GFXD3DRenderState[GFXRSNormalizeNormals] = D3DRS_NORMALIZENORMALS;
   GFXD3DRenderState[GFXRSDiffuseMaterialSource] = D3DRS_DIFFUSEMATERIALSOURCE;
   GFXD3DRenderState[GFXRSSpecularMaterialSource] = D3DRS_SPECULARMATERIALSOURCE;
   GFXD3DRenderState[GFXRSAmbientMaterialSource] = D3DRS_AMBIENTMATERIALSOURCE;
   GFXD3DRenderState[GFXRSEmissiveMaterialSource] = D3DRS_EMISSIVEMATERIALSOURCE;
   GFXD3DRenderState[GFXRSVertexBlend] = D3DRS_VERTEXBLEND;
   GFXD3DRenderState[GFXRSFogEnable] = D3DRS_FOGENABLE;
   GFXD3DRenderState[GFXRSSpecularEnable] = D3DRS_SPECULARENABLE;
   GFXD3DRenderState[GFXRSFogColor] = D3DRS_FOGCOLOR;
   GFXD3DRenderState[GFXRSFogTableMode] = D3DRS_FOGTABLEMODE;
   GFXD3DRenderState[GFXRSFogStart] = D3DRS_FOGSTART;
   GFXD3DRenderState[GFXRSFogEnd] = D3DRS_FOGEND;
   GFXD3DRenderState[GFXRSFogDensity] = D3DRS_FOGDENSITY;
   GFXD3DRenderState[GFXRSRangeFogEnable] = D3DRS_RANGEFOGENABLE;
   GFXD3DRenderState[GFXRSDebugMonitorToken] = D3DRS_DEBUGMONITORTOKEN;
   GFXD3DRenderState[GFXRSIndexedVertexBlendEnable] = D3DRS_INDEXEDVERTEXBLENDENABLE;
   GFXD3DRenderState[GFXRSTweenFactor] = D3DRS_TWEENFACTOR;
   GFXD3DRenderState[GFXRSTextureFactor] = D3DRS_TEXTUREFACTOR;
   GFXD3DRenderState[GFXRSPatchEdgeStyle] = D3DRS_PATCHEDGESTYLE;
   GFXD3DRenderState[GFXRSPositionDegree] = D3DRS_POSITIONDEGREE;
   GFXD3DRenderState[GFXRSNormalDegree] = D3DRS_NORMALDEGREE;
   GFXD3DRenderState[GFXRSAntiAliasedLineEnable] = D3DRS_ANTIALIASEDLINEENABLE;
   GFXD3DRenderState[GFXRSAdaptiveTess_X] = D3DRS_ADAPTIVETESS_X;
   GFXD3DRenderState[GFXRSAdaptiveTess_Y] = D3DRS_ADAPTIVETESS_Y;
   GFXD3DRenderState[GFXRSdaptiveTess_Z] = D3DRS_ADAPTIVETESS_Z;
   GFXD3DRenderState[GFXRSAdaptiveTess_W] = D3DRS_ADAPTIVETESS_W;
   GFXD3DRenderState[GFXRSEnableAdaptiveTesselation] = D3DRS_ENABLEADAPTIVETESSELLATION;
   GFXD3DRenderState[GFXRSDitherEnable] = D3DRS_DITHERENABLE;
   GFXD3DRenderState[GFXRSColorWriteEnable] = D3DRS_COLORWRITEENABLE;
   GFXD3DRenderState[GFXRSBlendOp] = D3DRS_BLENDOP;
   GFXD3DRenderState[GFXRSScissorTestEnable] = D3DRS_SCISSORTESTENABLE;
   GFXD3DRenderState[GFXRSSlopeScaleDepthBias] = D3DRS_SLOPESCALEDEPTHBIAS;
   GFXD3DRenderState[GFXRSMinTessellationLevel] = D3DRS_MINTESSELLATIONLEVEL;
   GFXD3DRenderState[GFXRSMaxTessellationLevel] = D3DRS_MAXTESSELLATIONLEVEL;
   GFXD3DRenderState[GFXRSTwoSidedStencilMode] = D3DRS_TWOSIDEDSTENCILMODE;
   GFXD3DRenderState[GFXRSCCWStencilFail] = D3DRS_CCW_STENCILFAIL;
   GFXD3DRenderState[GFXRSCCWStencilZFail] = D3DRS_CCW_STENCILZFAIL;
   GFXD3DRenderState[GFXRSCCWStencilPass] = D3DRS_CCW_STENCILPASS;
   GFXD3DRenderState[GFXRSCCWStencilFunc] = D3DRS_CCW_STENCILFUNC;
   GFXD3DRenderState[GFXRSColorWriteEnable1] = D3DRS_COLORWRITEENABLE1;
   GFXD3DRenderState[GFXRSColorWriteEnable2] = D3DRS_COLORWRITEENABLE2;
   GFXD3DRenderState[GFXRSolorWriteEnable3] = D3DRS_COLORWRITEENABLE3;
   GFXD3DRenderState[GFXRSBlendFactor] = D3DRS_BLENDFACTOR;
   GFXD3DRenderState[GFXRSSRGBWriteEnable] = D3DRS_SRGBWRITEENABLE;
   GFXD3DRenderState[GFXRSDepthBias] = D3DRS_DEPTHBIAS;
   GFXD3DRenderState[GFXRSWrap8] = D3DRS_WRAP8;
   GFXD3DRenderState[GFXRSWrap9] = D3DRS_WRAP9;
   GFXD3DRenderState[GFXRSWrap10] = D3DRS_WRAP10;
   GFXD3DRenderState[GFXRSWrap11] = D3DRS_WRAP11;
   GFXD3DRenderState[GFXRSWrap12] = D3DRS_WRAP12;
   GFXD3DRenderState[GFXRSWrap13] = D3DRS_WRAP13;
   GFXD3DRenderState[GFXRSWrap14] = D3DRS_WRAP14;
   GFXD3DRenderState[GFXRSWrap15] = D3DRS_WRAP15;
   GFXD3DRenderState[GFXRSSeparateAlphaBlendEnable] = D3DRS_SEPARATEALPHABLENDENABLE;
   GFXD3DRenderState[GFXRSSrcBlendAlpha] = D3DRS_SRCBLENDALPHA;
   GFXD3DRenderState[GFXRSDestBlendAlpha] = D3DRS_DESTBLENDALPHA;
   GFXD3DRenderState[GFXRSBlendOpAlpha] = D3DRS_BLENDOPALPHA;
   VALIDATE_LOOKUPTABLE( GFXD3DRenderState, GFXRenderState );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DTextureFilter, GFXTextureFilter, _D3DTEXTUREFILTERTYPE );
   GFXD3DTextureFilter[GFXTextureFilterNone] = D3DTEXF_NONE;
   GFXD3DTextureFilter[GFXTextureFilterPoint] = D3DTEXF_POINT;
   GFXD3DTextureFilter[GFXTextureFilterLinear] = D3DTEXF_LINEAR;
   GFXD3DTextureFilter[GFXTextureFilterAnisotropic] = D3DTEXF_ANISOTROPIC;
   GFXD3DTextureFilter[GFXTextureFilterPyramidalQuad] = D3DTEXF_PYRAMIDALQUAD;
   GFXD3DTextureFilter[GFXTextureFilterGaussianQuad] = D3DTEXF_GAUSSIANQUAD;
   VALIDATE_LOOKUPTABLE( GFXD3DTextureFilter, GFXTextureFilter );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DBlend, GFXBlend, _D3DBLEND );
   GFXD3DBlend[GFXBlendZero] = D3DBLEND_ZERO;
   GFXD3DBlend[GFXBlendOne] = D3DBLEND_ONE;
   GFXD3DBlend[GFXBlendSrcColor] = D3DBLEND_SRCCOLOR;
   GFXD3DBlend[GFXBlendInvSrcColor] = D3DBLEND_INVSRCCOLOR;
   GFXD3DBlend[GFXBlendSrcAlpha] = D3DBLEND_SRCALPHA;
   GFXD3DBlend[GFXBlendInvSrcAlpha] = D3DBLEND_INVSRCALPHA;
   GFXD3DBlend[GFXBlendDestAlpha] = D3DBLEND_DESTALPHA;
   GFXD3DBlend[GFXBlendInvDestAlpha] = D3DBLEND_INVDESTALPHA;
   GFXD3DBlend[GFXBlendDestColor] = D3DBLEND_DESTCOLOR;
   GFXD3DBlend[GFXBlendInvDestColor] = D3DBLEND_INVDESTCOLOR;
   GFXD3DBlend[GFXBlendSrcAlphaSat] = D3DBLEND_SRCALPHASAT;
   VALIDATE_LOOKUPTABLE( GFXD3DBlend, GFXBlend );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DBlendOp, GFXBlendOp, _D3DBLENDOP );
   GFXD3DBlendOp[GFXBlendOpAdd] = D3DBLENDOP_ADD;
   GFXD3DBlendOp[GFXBlendOpSubtract] = D3DBLENDOP_SUBTRACT;
   GFXD3DBlendOp[GFXBlendOpRevSubtract] = D3DBLENDOP_REVSUBTRACT;
   GFXD3DBlendOp[GFXBlendOpMin] = D3DBLENDOP_MIN;
   GFXD3DBlendOp[GFXBlendOpMax] = D3DBLENDOP_MAX;
   VALIDATE_LOOKUPTABLE( GFXD3DBlendOp, GFXBlendOp );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DStencilOp, GFXStencilOp, _D3DSTENCILOP );
   GFXD3DStencilOp[GFXStencilOpKeep] = D3DSTENCILOP_KEEP;
   GFXD3DStencilOp[GFXStencilOpZero] = D3DSTENCILOP_ZERO;
   GFXD3DStencilOp[GFXStencilOpReplace] = D3DSTENCILOP_REPLACE;
   GFXD3DStencilOp[GFXStencilOpIncrSat] = D3DSTENCILOP_INCRSAT;
   GFXD3DStencilOp[GFXStencilOpDecrSat] = D3DSTENCILOP_DECRSAT;
   GFXD3DStencilOp[GFXStencilOpInvert] = D3DSTENCILOP_INVERT;
   GFXD3DStencilOp[GFXStencilOpIncr] = D3DSTENCILOP_INCR;
   GFXD3DStencilOp[GFXStencilOpDecr] = D3DSTENCILOP_DECR;
   VALIDATE_LOOKUPTABLE( GFXD3DStencilOp, GFXStencilOp );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DCmpFunc, GFXCmp, _D3DCMPFUNC );
   GFXD3DCmpFunc[GFXCmpNever] = D3DCMP_NEVER;
   GFXD3DCmpFunc[GFXCmpLess] = D3DCMP_LESS;
   GFXD3DCmpFunc[GFXCmpEqual] = D3DCMP_EQUAL;
   GFXD3DCmpFunc[GFXCmpLessEqual] = D3DCMP_LESSEQUAL;
   GFXD3DCmpFunc[GFXCmpGreater] = D3DCMP_GREATER;
   GFXD3DCmpFunc[GFXCmpNotEqual] = D3DCMP_NOTEQUAL;
   GFXD3DCmpFunc[GFXCmpGreaterEqual] = D3DCMP_GREATEREQUAL;
   GFXD3DCmpFunc[GFXCmpAlways] = D3DCMP_ALWAYS;
   VALIDATE_LOOKUPTABLE( GFXD3DCmpFunc, GFXCmp );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DCullMode, GFXCull, _D3DCULL );
   GFXD3DCullMode[GFXCullNone] = D3DCULL_NONE;
   GFXD3DCullMode[GFXCullCW] = D3DCULL_CW;
   GFXD3DCullMode[GFXCullCCW] = D3DCULL_CCW;
   VALIDATE_LOOKUPTABLE( GFXD3DCullMode, GFXCull );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DPrimType, GFXPT, _D3DPRIMITIVETYPE );
   GFXD3DPrimType[GFXPointList] = D3DPT_POINTLIST;
   GFXD3DPrimType[GFXLineList] = D3DPT_LINELIST;
   GFXD3DPrimType[GFXLineStrip] = D3DPT_LINESTRIP;
   GFXD3DPrimType[GFXTriangleList] = D3DPT_TRIANGLELIST;
   GFXD3DPrimType[GFXTriangleStrip] = D3DPT_TRIANGLESTRIP;
   GFXD3DPrimType[GFXTriangleFan] = D3DPT_TRIANGLEFAN;
   VALIDATE_LOOKUPTABLE( GFXD3DPrimType, GFXPT );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DTextureStageState, GFXTSS, _D3DTEXTURESTAGESTATETYPE );
   GFXD3DTextureStageState[GFXTSSColorOp] = D3DTSS_COLOROP;
   GFXD3DTextureStageState[GFXTSSColorArg1] = D3DTSS_COLORARG1;
   GFXD3DTextureStageState[GFXTSSColorArg2] = D3DTSS_COLORARG2;
   GFXD3DTextureStageState[GFXTSSAlphaOp] = D3DTSS_ALPHAOP;
   GFXD3DTextureStageState[GFXTSSAlphaArg1] = D3DTSS_ALPHAARG1;
   GFXD3DTextureStageState[GFXTSSAlphaArg2] = D3DTSS_ALPHAARG2;
   GFXD3DTextureStageState[GFXTSSBumpEnvMat00] = D3DTSS_BUMPENVMAT00;
   GFXD3DTextureStageState[GFXTSSBumpEnvMat01] = D3DTSS_BUMPENVMAT01;
   GFXD3DTextureStageState[GFXTSSBumpEnvMat10] = D3DTSS_BUMPENVMAT10;
   GFXD3DTextureStageState[GFXTSSBumpEnvMat11] = D3DTSS_BUMPENVMAT11;
   GFXD3DTextureStageState[GFXTSSTexCoordIndex] = D3DTSS_TEXCOORDINDEX;
   GFXD3DTextureStageState[GFXTSSBumpEnvlScale] = D3DTSS_BUMPENVLSCALE;
   GFXD3DTextureStageState[GFXTSSBumpEnvlOffset] = D3DTSS_BUMPENVLOFFSET;
   GFXD3DTextureStageState[GFXTSSTextureTransformFlags] = D3DTSS_TEXTURETRANSFORMFLAGS;
   GFXD3DTextureStageState[GFXTSSColorArg0] = D3DTSS_COLORARG0;
   GFXD3DTextureStageState[GFXTSSAlphaArg0] = D3DTSS_ALPHAARG0;
   GFXD3DTextureStageState[GFXTSSResultArg] = D3DTSS_RESULTARG;
   GFXD3DTextureStageState[GFXTSSConstant] = D3DTSS_CONSTANT;
   VALIDATE_LOOKUPTABLE( GFXD3DTextureStageState, GFXTSS );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DTextureAddress, GFXAddress, _D3DTEXTUREADDRESS );
   GFXD3DTextureAddress[GFXAddressWrap] = D3DTADDRESS_WRAP ;
   GFXD3DTextureAddress[GFXAddressMirror] = D3DTADDRESS_MIRROR;
   GFXD3DTextureAddress[GFXAddressClamp] = D3DTADDRESS_CLAMP;
   GFXD3DTextureAddress[GFXAddressBorder] = D3DTADDRESS_BORDER;
   GFXD3DTextureAddress[GFXAddressMirrorOnce] = D3DTADDRESS_MIRRORONCE;
   VALIDATE_LOOKUPTABLE(GFXD3DTextureAddress, GFXAddress );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
   INIT_LOOKUPTABLE( GFXD3DTextureOp, GFXTOP, _D3DTEXTUREOP );
   GFXD3DTextureOp[GFXTOPDisable] = D3DTOP_DISABLE;
   GFXD3DTextureOp[GFXTOPSelectARG1] = D3DTOP_SELECTARG1;
   GFXD3DTextureOp[GFXTOPSelectARG2] = D3DTOP_SELECTARG2;
   GFXD3DTextureOp[GFXTOPModulate] = D3DTOP_MODULATE;
   GFXD3DTextureOp[GFXTOPModulate2X] = D3DTOP_MODULATE2X;
   GFXD3DTextureOp[GFXTOPModulate4X] = D3DTOP_MODULATE4X;
   GFXD3DTextureOp[GFXTOPAdd] = D3DTOP_ADD;
   GFXD3DTextureOp[GFXTOPAddSigned] = D3DTOP_ADDSIGNED;
   GFXD3DTextureOp[GFXTOPAddSigned2X] = D3DTOP_ADDSIGNED2X;
   GFXD3DTextureOp[GFXTOPSubtract] = D3DTOP_SUBTRACT;
   GFXD3DTextureOp[GFXTOPAddSmooth] = D3DTOP_ADDSMOOTH;
   GFXD3DTextureOp[GFXTOPBlendDiffuseAlpha] = D3DTOP_BLENDDIFFUSEALPHA;
   GFXD3DTextureOp[GFXTOPBlendTextureAlpha] = D3DTOP_BLENDTEXTUREALPHA;
   GFXD3DTextureOp[GFXTOPBlendFactorAlpha] = D3DTOP_BLENDFACTORALPHA;
   GFXD3DTextureOp[GFXTOPBlendTextureAlphaPM] = D3DTOP_BLENDTEXTUREALPHAPM;
   GFXD3DTextureOp[GFXTOPBlendCURRENTALPHA] = D3DTOP_BLENDCURRENTALPHA;
   GFXD3DTextureOp[GFXTOPPreModulate] = D3DTOP_PREMODULATE;
   GFXD3DTextureOp[GFXTOPModulateAlphaAddColor] = D3DTOP_MODULATEALPHA_ADDCOLOR;
   GFXD3DTextureOp[GFXTOPModulateColorAddAlpha] = D3DTOP_MODULATECOLOR_ADDALPHA;
   GFXD3DTextureOp[GFXTOPModulateInvAlphaAddColor] = D3DTOP_MODULATEINVALPHA_ADDCOLOR;
   GFXD3DTextureOp[GFXTOPModulateInvColorAddAlpha] = D3DTOP_MODULATEINVCOLOR_ADDALPHA;
   GFXD3DTextureOp[GFXTOPBumpEnvMap] = D3DTOP_BUMPENVMAP;
   GFXD3DTextureOp[GFXTOPBumpEnvMapLuminance] = D3DTOP_BUMPENVMAPLUMINANCE;
   GFXD3DTextureOp[GFXTOPDotProduct3] = D3DTOP_DOTPRODUCT3;
   VALIDATE_LOOKUPTABLE( GFXD3DTextureOp, GFXTOP );
}

