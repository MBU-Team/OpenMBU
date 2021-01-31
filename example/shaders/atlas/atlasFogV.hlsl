#define IN_HLSL
#include "../shdrConsts.h"
#include "atlas.h"

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
VertFogConnectData main( VertData IN,
                  uniform float4x4 modelView : register(C0),
                  uniform float3   eyePos    : register(VC_EYE_POS),
                  uniform float    morphT    : register(C49),
                  uniform float3   fogData   : register(C50)
                  )
{
   VertFogConnectData OUT;

   // Apply morph calculations.
   float4 realPos = IN.position + (IN.morphCoord * morphT);
   realPos.w = 1;

   // Transform and return.
   OUT.hpos = mul(modelView, realPos);
 
   // For fog, do the fog calculation.
   float eyeDist = distance( IN.position, eyePos );
   OUT.fogCoord.x = 1.0 - ( eyeDist  / fogData.z );
   OUT.fogCoord.y = (IN.position.z - fogData.x) * fogData.y;

   return OUT;
}
