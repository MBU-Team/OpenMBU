//*****************************************************************************
// TSE -- HLSL procedural shader                                               
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

#define IN_HLSL
#include "./shdrConsts.h"

struct ConnectData
{
   float4 shading         : COLOR;
   float2 texCoord        : TEXCOORD0;
   float3 pixPos          : TEXCOORD1;
   float3 eyePos          : TEXCOORD2;
   float4 lightVec        : TEXCOORD3;
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
              uniform sampler2D bumpMap         : register(S1),
              uniform float4    specularColor   : register(PC_MAT_SPECCOLOR),
              uniform float     specularPower   : register(PC_MAT_SPECPOWER)
)
{
   Fragout OUT;

   
   float4 bumpNormal = tex2D(bumpMap, IN.texCoord);
   float4 bumpDot = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, IN.lightVec.xyz) );

   float4 diffuse = tex2D( diffuseMap, IN.texCoord );

   OUT.col = IN.shading * diffuse * (bumpDot + ambient);

   // specular
   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float specular = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, halfAng) ) * IN.lightVec.w;
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular * diffuse.a;
   

   return OUT;
}




