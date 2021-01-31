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

   float4 bumpNormal = tex2D(bumpMap, IN.texCoord);
   float4 bumpDot = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, IN.lightVec.xyz) );

   float4 diffuse = tex2D(diffuseMap, IN.texCoord);

   float2 noiseIndex;
   float4 noiseColor[4];
   float2 halfPixel = float2( 1.0 / 64.0, 1.0 / 64.0 );
   
   
   noiseIndex.x = floor( IN.texCoord.x - halfPixel.x ) / 63.0 + 0.5/64.0;
   noiseIndex.y = floor( IN.texCoord.y - halfPixel.y ) / 63.0 + 0.5/64.0;
   noiseColor[0] = tex2D( noiseMap, noiseIndex ) * 1.0 - 0.5;   

   noiseIndex.x = floor( IN.texCoord.x - halfPixel.x ) / 63.0 + 0.5/64.0;
   noiseIndex.y = floor( IN.texCoord.y + halfPixel.y ) / 63.0 + 0.5/64.0;
   noiseColor[1] = tex2D( noiseMap, noiseIndex ) * 1.0 - 0.5;   

   noiseIndex.x = floor( IN.texCoord.x + halfPixel.x ) / 63.0 + 0.5/64.0;
   noiseIndex.y = floor( IN.texCoord.y + halfPixel.y ) / 63.0 + 0.5/64.0;
   noiseColor[2] = tex2D( noiseMap, noiseIndex ) * 1.0 - 0.5;   

   noiseIndex.x = floor( IN.texCoord.x + halfPixel.x ) / 63.0 + 0.5/64.0;
   noiseIndex.y = floor( IN.texCoord.y - halfPixel.y ) / 63.0 + 0.5/64.0;
   noiseColor[3] = tex2D( noiseMap, noiseIndex ) * 1.0 - 0.5;   
   
   float4 finalNoiseCol = (noiseColor[0] + noiseColor[1] + noiseColor[2] + noiseColor[3]) / 4.0; 

   OUT.col = diffuse + finalNoiseCol * diffuse.a;
   OUT.col *= IN.shading;
   OUT.col *= bumpDot + ambient;

   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float specular = saturate( dot(bumpNormal.xyz * 2.0 - 1.0, halfAng) ) * IN.lightVec.w;
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular * diffuse.a;

   

   return OUT;
}
