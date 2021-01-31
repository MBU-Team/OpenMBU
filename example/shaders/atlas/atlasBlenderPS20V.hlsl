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
   float2 tex1       : TEXCOORD2;
   float2 tex2       : TEXCOORD3;
   float2 tex3       : TEXCOORD4;
   float2 tex4       : TEXCOORD5;
};

FragOut main(FragIn input, uniform float4x4 modelview : register(C0))
{
   FragOut OUT;
   
   OUT.HPOS = mul(modelview, input.HPOS);
   OUT.tex   = input.opacityTex;
   OUT.lmTex = input.lmTex;
   OUT.tex1  = input.masterTex * 128.0;
   OUT.tex2  = input.masterTex * 128.0;
   OUT.tex3  = input.masterTex * 128.0;
   OUT.tex4  = input.masterTex * 128.0;
   
   return OUT;
}

