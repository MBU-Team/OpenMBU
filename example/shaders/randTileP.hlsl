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
              uniform sampler2D noiseMap        : register(S2),
              uniform float4    specularColor   : register(PC_MAT_SPECCOLOR),
              uniform float     specularPower   : register(PC_MAT_SPECPOWER)
)
{
   Fragout OUT;

   const float TEX_SIZE = 64.0;
   const float TEX_SIZE_MINUS_ONE = TEX_SIZE - 1.0; 
   const float NUM_TILES = 4.0;
   
   float4 bumpNormal = tex2D(bumpMap, IN.texCoord);
   float4 bumpDot = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, IN.lightVec.xyz) );

   // grab noise color for use indexing into tile texture
   float2 noiseIndex;
   noiseIndex.x = floor(IN.texCoord.x) / TEX_SIZE_MINUS_ONE + (0.5 / TEX_SIZE);
   noiseIndex.y = floor(IN.texCoord.y) / TEX_SIZE_MINUS_ONE + (0.5 / TEX_SIZE);
   float4 noiseColor = tex2D( noiseMap, noiseIndex );   

   // setup final indexing into diffuse texture using noise color
   float baseOffset = floor( noiseColor.x * NUM_TILES );  // now is (0, 1, 2, or 3) if NUM_TILES is 4
   float fractionX = frac( IN.texCoord.x );  
   float2 finalCoords = float2( (baseOffset + fractionX) / NUM_TILES, IN.texCoord.y );
   float4 diffuse = tex2D( diffuseMap, finalCoords );

   OUT.col = IN.shading * diffuse * (bumpDot + ambient);

   // specular
   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float specular = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, halfAng) ) * IN.lightVec.w;
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular * diffuse.a;
   

   return OUT;
}




