#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float2 texCoord        : TEXCOORD0;
   float2 lmCoord         : TEXCOORD1;
   float3 T               : TEXCOORD2;
   float3 B               : TEXCOORD3;
   float3 N               : TEXCOORD4;
   float3 normal          : NORMAL;
   float4 position        : POSITION;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 texCoord        : TEXCOORD0;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(VC_WORLD_PROJ),
                  uniform float3   eyePos          : register(VC_EYE_POS)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   
   float3 screenNorm = mul( modelview, IN.normal );
   float3 refractVec = refract( float3( 0.0, 0.0, 1.0 ), screenNorm, 0.7 );
   
   OUT.texCoord = float2( ( OUT.hpos.x + refractVec.x ) / (OUT.hpos.w), 
                          ( -OUT.hpos.y + refractVec.y ) / (OUT.hpos.w) );
   
   OUT.texCoord = saturate( (OUT.texCoord + 1.0) * 0.5 );
   

   
   return OUT;
}
