#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

struct ConnectData
{
   float2   texCoord    : TEXCOORD0;
   float4   shading     : COLOR0;
   float3   lightVec    : TEXCOORD1;
   float3   reflectVec  : TEXCOORD2;
   float3   pixPos      : TEXCOORD3;
   float3   eyePos      : TEXCOORD4;
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
              uniform samplerCUBE cubeMap    : register(S2),
              uniform float4 specularColor   : register(PC_MAT_SPECCOLOR),
              uniform float  specularPower   : register(PC_MAT_SPECPOWER)
)
{
   Fragout OUT;


   float4 bumpNormal = tex2D(bumpMap, IN.texCoord);
   float4 bumpDot = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, IN.lightVec.xyz) );
   OUT.col = bumpDot * 0.75;
   OUT.col += IN.shading;
   OUT.col *= tex2D( baseTex, IN.texCoord );
   OUT.col += texCUBE(cubeMap, IN.reflectVec) * bumpNormal.a;
   
   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec);
   float specular = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, halfAng) );
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular * bumpNormal.a * 8.0;

   return OUT;
}
