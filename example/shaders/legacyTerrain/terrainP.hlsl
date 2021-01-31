//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
#define IN_HLSL
#include "../shdrConsts.h"

struct ConnectData
{
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
   float2 lightMapCoord   : TEXCOORD2;
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
			  uniform sampler2D lightMap        : register(S2)
              )
{
   Fragout OUT;
   
   float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);
   diffuseColor *= tex2D(lightMap, IN.lightMapCoord) * 4.0;
   
   float4 fogColor     = tex2D(fogMap,     IN.fogCoord);

   OUT.col = lerp( diffuseColor, fogColor, fogColor.a );
   
   return OUT;
}
