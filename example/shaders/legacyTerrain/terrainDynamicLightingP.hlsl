#define IN_HLSL
#include "../shdrConsts.h"
#include "../lightingSystem/shdrLib.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
#define IN_HLSL
#include "../shdrConsts.h"

struct ConnectData
{
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
   float3 dlightCoord     : TEXCOORD2;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D fogMap          : register(S1),
              uniform sampler2D diffuseMap      : register(S0),
			  uniform sampler3D dlightMap       : register(S2),
              uniform float4    lightColor      : register(PC_DIFF_COLOR)
              )
{
   Fragout OUT;
   
   float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);
   diffuseColor *= getDynamicLighting(dlightMap, IN.dlightCoord, lightColor) * 2.0;
   
   float4 fogColor = tex2D(fogMap,     IN.fogCoord);
   fogColor.x = 0.0;
   fogColor.y = 0.0;
   fogColor.z = 0.0;
   OUT.col = lerp( diffuseColor, fogColor, fogColor.a );
   
   return OUT;
}
