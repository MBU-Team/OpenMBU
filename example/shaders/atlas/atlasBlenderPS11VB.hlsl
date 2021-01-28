//**********   *******************************************************************
// Lightmap / cubemap shader
//*****************************************************************************

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct FragIn
{
   float4 HPOS       : POSITION;
   float2 opacityTex : TEXCOORD0;
   float2 lmTex      : TEXCOORD1;
   float2 masterTex  : TEXCOORD2;
};

struct FragOut
{
   float4 HPOS       : POSITION;
   float2 tex        : TEXCOORD0;
   float2 lmTex      : TEXCOORD1;
   float2 tex3       : TEXCOORD2;
   float2 tex4       : TEXCOORD3;
};

FragOut main(FragIn input, uniform float4x4 modelview : register(C0))
{
   FragOut OUT;
   
   OUT.HPOS = mul(modelview, input.HPOS);
   OUT.tex   = input.opacityTex;
   OUT.lmTex = input.lmTex;
   OUT.tex3  = input.masterTex * 128.0;
   OUT.tex4  = input.masterTex * 128.0;
   
   return OUT;
}

