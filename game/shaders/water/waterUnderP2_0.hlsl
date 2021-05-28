//*****************************************************************************
// TSE -- water shader
//*****************************************************************************

// Underwater shader

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
#define IN_HLSL
#include "..\shdrConsts.h"


struct ConnectData
{
   float4 texCoord   : TEXCOORD0;
   float3 lightVec   : TEXCOORD1;
   float2 texCoord2  : TEXCOORD2;
   float4 texCoord3  : TEXCOORD3;
   float2 fogCoord   : TEXCOORD4;
   float3 pos        : TEXCOORD6;
   float3 eyePos     : TEXCOORD7;
};

struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler    bumpMap           : register(S0),
              uniform sampler2D  reflectMap        : register(S1),
              uniform sampler    refractBuff       : register(S2),
              uniform sampler    fogMap            : register(S3),
              uniform float4     baseColor         : register(C20)
)
{
   Fragout OUT;

                       

   float3 bumpNorm = tex2D(bumpMap, IN.texCoord.xy) * 2.0 - 1.0;
   bumpNorm += tex2D(bumpMap, IN.texCoord.zw) * 2.0 - 1.0;
   
   // This large scale texture has 1/3 the influence as the other two.
   // Its purpose is to break up the repetitive patterns of the other two textures.
   bumpNorm += (tex2D(bumpMap, IN.texCoord2) * 2.0 - 1.0) * 0.3;
   
   // calc distortion, place in projected tex coords
   float distortion = (length( IN.eyePos - IN.pos ) / 20.0) + 0.15;
   IN.texCoord3.xy += bumpNorm.xy * distortion;
   
   // get clean refract color
   float4 refractColor = tex2Dproj( refractBuff, IN.texCoord3 );
   OUT.col = refractColor;

   // fog it
   float4 fogColor = tex2D(fogMap, IN.fogCoord);
   OUT.col = lerp( OUT.col, fogColor, fogColor.a );


   return OUT;
}

