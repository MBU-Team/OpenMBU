#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float2 texCoord        : TEXCOORD0;
   float4 position        : POSITION;
   float3 normal          : NORMAL;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0),
                  uniform float3   eyePos          : register(VC_EYE_POS),
                  uniform float3   fogData         : register(VC_FOGDATA)
                  )
{
   ConnectData OUT;

   OUT.hpos       = mul(modelview, IN.position);
   OUT.texCoord   = IN.texCoord;
   OUT.fogCoord.x = 1.0 - ( distance( IN.position, eyePos ) / fogData.z );
   OUT.fogCoord.y = (IN.position.z - fogData.x) * fogData.y;

   return OUT;
}
