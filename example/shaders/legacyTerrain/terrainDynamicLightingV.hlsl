#define IN_HLSL
#include "../shdrConsts.h"
#include "../lightingSystem/shdrLib.h"

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
   float3 dlightCoord     : TEXCOORD2;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0),
                  uniform float3   eyePos          : register(VC_EYE_POS),
                  uniform float3   fogData         : register(VC_FOGDATA),
				  
                  uniform float4   lightPos        : register(VC_LIGHT_POS1),
                  uniform float4x4 objTrans        : register(VC_OBJ_TRANS),
                  uniform float4x4 lightingMatrix  : register(VC_LIGHT_TRANS)
				  
                  )
{
   ConnectData OUT;
   
   OUT.texCoord = IN.texCoord;
   OUT.dlightCoord = getDynamicLightingCoord(IN.position, lightPos, objTrans, lightingMatrix);

   OUT.hpos       = mul(modelview, IN.position);
   OUT.fogCoord.x = 1.0 - ( distance( IN.position, eyePos ) / fogData.z );
   OUT.fogCoord.y = (IN.position.z - fogData.x) * fogData.y;

   return OUT;
}
