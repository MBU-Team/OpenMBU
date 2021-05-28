#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float4 position        : POSITION;
   float2 texCoord        : TEXCOORD0;
//   float3 normal          : NORMAL;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
   float2 detailCoord     : TEXCOORD2;
   float2 detailCoord2    : TEXCOORD3;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(VC_WORLD_PROJ),
                  uniform float4x4 objTrans        : register(VC_OBJ_TRANS),
                  uniform float3   eyePos          : register(VC_EYE_POS),
                  uniform float3   fogData         : register(VC_FOGDATA),
                  uniform float4   texGen          : register(C44),
                  uniform float4   scaleOff        : register(C45),
                  uniform float4   morphInfo       : register(C46)
                  )
{
   ConnectData OUT;
   
   // Do positional transform on the heightfield data, and apply morph.
   float4  realPos;
   realPos.x = IN.position.x * scaleOff.z + scaleOff.x;
   realPos.y = IN.position.y * scaleOff.z + scaleOff.y;
   realPos.z = IN.position.z + IN.texCoord.x * morphInfo.x;
   realPos.w = IN.position.w;
   
   OUT.hpos        = mul(modelview, realPos);
   float3 worldPos = mul(objTrans,  realPos.xyz);
   float3 worldEye = mul(objTrans,  eyePos);

   /// Generate texture co-ords.
   OUT.texCoord.y = realPos.x * texGen.z + texGen.x;
   OUT.texCoord.x = realPos.y * texGen.z + texGen.y;

   // And fog texture co-ords.   
   OUT.fogCoord.x = 1.0 - ( distance( worldPos, worldEye ) / fogData.z );
   OUT.fogCoord.y = (worldPos.z - fogData.x) * fogData.y;

   // And detail texture co-ords.
   OUT.detailCoord  = realPos / 51;
   OUT.detailCoord2 = realPos / 13;

   return OUT;
}
