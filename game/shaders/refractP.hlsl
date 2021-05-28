#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 texCoord        : TEXCOORD0;
   float2 tex2            : TEXCOORD1;
   float4 lightVec        : TEXCOORD2;
   float3 pixPos          : TEXCOORD3;
   float3 eyePos          : TEXCOORD4;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D bumpMap         : register(S0),
              uniform sampler2D refractMap      : register(S1),
              uniform sampler2D diffuseMap      : register(S2),
              uniform float4     specularColor     : register(PC_MAT_SPECCOLOR),
              uniform float      specularPower     : register(PC_MAT_SPECPOWER)
)
{
   Fragout OUT;

   float3 bumpNorm = tex2D(bumpMap, IN.tex2) * 2.0 - 1.0;
   float4 diffuse  = tex2D(diffuseMap, IN.tex2);
   
   // calc distortion, place in projected tex coords
   float distortion = 0.30;
   IN.texCoord.xy += bumpNorm.xy * distortion;
 
   // get clean refract color
   float4 refractColor = tex2Dproj( refractMap, IN.texCoord );
   OUT.col = refractColor * diffuse;

   float4 bumpDot = saturate( dot(bumpNorm, IN.lightVec.xyz) );

   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float specular = saturate( dot(bumpNorm.xyz, halfAng) ) * IN.lightVec.w;
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular * diffuse.a;
   
   
   return OUT;
}
