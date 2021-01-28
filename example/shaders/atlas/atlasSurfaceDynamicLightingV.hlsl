// Atlas Surface Vertex Shader

#define IN_HLSL
#include "../shdrConsts.h"
#include "../lightingSystem/shdrLib.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float4 position        : POSITION;  // Our base position.
   float2 texCoord        : TEXCOORD0; // Our texture co-ordinates.
   float3 morphCoord      : TEXCOORD1; // We have a 3d morph vector, which is scaled by 0..1 and added to position to get our current position.
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
   float2 detailCoord     : TEXCOORD2;
   float2 detailCoord2    : TEXCOORD3;
   float3 lightingCoord   : TEXCOORD4;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelView       : register(VC_WORLD_PROJ),
                  uniform float4x4 objTrans        : register(VC_OBJ_TRANS),
                  uniform float3   eyePos          : register(VC_EYE_POS),
                  uniform float3   fogData         : register(VC_FOGDATA),
                  uniform float3   texGen          : register(C44),
                  uniform float4   scaleOff        : register(C45),
				  
                  uniform float4   lightPos        : register(VC_LIGHT_POS1),
                  uniform float4x4 lightingMatrix  : register(VC_LIGHT_TRANS)
                  )
{
   ConnectData OUT;
   
   // Calculate our morph value.
   float4  morphOffset;
   
   morphOffset.xyz = IN.morphCoord * 0; //scaleOff.w;
   morphOffset.w = 0;
   
   // Calculate our position in objectspace.
   float4 realPos;
   realPos.x = IN.position.x;
   realPos.y = IN.position.y;
   realPos.z = IN.position.z;
   realPos.w = 1;
   
   // Combine morph and realPos.
   realPos += morphOffset;
   OUT.hpos = mul(modelView, realPos);
   
   // Pass the texcoord along after scaling + offsetting it.
   OUT.texCoord.x = IN.texCoord.x * texGen.z + texGen.x;
   OUT.texCoord.y = IN.texCoord.y * texGen.z + texGen.y;
   
   // Generate detail coords and shading info.
   OUT.detailCoord  = realPos / 51;
   OUT.detailCoord2 = realPos / 13;

   // Fog coordinate generation.
   float3 worldPos = mul(objTrans, realPos.xyz);
   OUT.fogCoord.x = 1.0 - ( distance( realPos.xyz, eyePos ) / fogData.z );
   OUT.fogCoord.y = (worldPos.z - fogData.x) * fogData.y;
   
   OUT.lightingCoord = getDynamicLightingCoord(IN.position, lightPos, objTrans, lightingMatrix);
   
   return OUT;
}
