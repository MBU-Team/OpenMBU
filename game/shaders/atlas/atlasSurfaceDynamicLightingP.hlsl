//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
#define IN_HLSL
#include "../shdrConsts.h"
#include "../lightingSystem/shdrLib.h"

struct ConnectData
{
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
   float2 detailCoord     : TEXCOORD2;
   float2 detailCoord2    : TEXCOORD3;
   float3 lightingCoord   : TEXCOORD4;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D diffuseMap      : register(S0),
              uniform sampler2D fogMap          : register(S1),
              uniform sampler2D detailMap       : register(S2),
              uniform sampler2D detailMap2      : register(S3),
			  
              uniform sampler3D dlightMap       : register(S4),
              uniform float4    lightColor      : register(PC_DIFF_COLOR)
              )
{
   Fragout OUT;
   
   float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);
   float4 fogCol       = tex2D(fogMap,     IN.fogCoord);
   float4 detailCol  =  tex2D(detailMap,   IN.detailCoord);
          detailCol  *= tex2D(detailMap2,  IN.detailCoord2);
		  
   diffuseColor *= getDynamicLighting(dlightMap, IN.lightingCoord, lightColor);

   OUT.col = diffuseColor;//lerp( diffuseColor, fogCol, fogCol.a ); //* (detailCol + 0.6)
   OUT.col.w = 1.0 - fogCol.a;
   
   // need to filter lexels to save bandwidth...
   OUT.col.w *= diffuseColor.x + diffuseColor.y + diffuseColor.z;
   
   return OUT;
}
