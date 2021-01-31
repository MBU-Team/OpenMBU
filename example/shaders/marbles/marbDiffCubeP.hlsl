//*****************************************************************************
// TSE -- HLSL procedural shader                                               
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

#define IN_HLSL
#include "../shdrConsts.h"

struct ConnectData
{
   float4 shading         : COLOR;
   float2 texCoord        : TEXCOORD0;
   float3 reflectVec      : TEXCOORD1;
   float3 normal          : TEXCOORD2;
   float3 pixPos          : TEXCOORD3;
   float3 eyePos          : TEXCOORD4;
   float4 lightVec        : TEXCOORD5;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform float4    ambient         : register(PC_AMBIENT_COLOR),
              uniform sampler2D diffuseMap      : register(S0),
              uniform samplerCUBE cubeMap         : register(S1),
              uniform float4    specularColor   : register(PC_MAT_SPECCOLOR),
              uniform float     specularPower   : register(PC_MAT_SPECPOWER),
              uniform float     visibility      : register(PC_VISIBILITY)
)
{
   Fragout OUT;

   OUT.col = IN.shading + float4( 0.4, 0.4, 0.4, 0.4 );
   float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);
   OUT.col *= diffuseColor;
   
   float4 cubeColor = texCUBE(cubeMap, IN.reflectVec);
   cubeColor.xyz = (cubeColor.x + cubeColor.y + cubeColor.z) / 3.0; 
   OUT.col *= cubeColor;

   
   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float specular = saturate( dot(IN.normal.xyz, halfAng) ) * IN.lightVec.w;
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular * diffuseColor.a;


   return OUT;
}
