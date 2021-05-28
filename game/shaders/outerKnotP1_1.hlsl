#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

struct ConnectData
{
   float2   texCoord    : TEXCOORD0;
   float2   bumpCoord   : TEXCOORD1;
   float4   shading     : COLOR0;
   float3   reflectVec  : TEXCOORD2;
   float3   lightVec    : TEXCOORD3;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D baseTex      : register(S0),
              uniform sampler2D bumpMap      : register(S1),
              uniform samplerCUBE cubeMap    : register(S2)
)
{
   Fragout OUT;


   float4 bumpNormal = tex2D(bumpMap, IN.bumpCoord);
   float4 bumpDot = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, IN.lightVec.xyz * 2.0 - 1.0) );
   OUT.col = bumpDot * 0.75;
   OUT.col += IN.shading;
   OUT.col *= tex2D( baseTex, IN.texCoord );
   OUT.col += texCUBE(cubeMap, IN.reflectVec) * bumpNormal.a;
   

   return OUT;
}
