//*****************************************************************************
// TSE -- HLSL procedural shader                                               
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

#define IN_HLSL
#include "../shdrConsts.h"

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
   float4 shading         : COLOR;
   float2 outTexCoord     : TEXCOORD0;
   float3 reflectVec      : TEXCOORD1;
   float3 outNormal       : TEXCOORD2;
   float3 pos             : TEXCOORD3;
   float3 outEyePos       : TEXCOORD4;
   float4 outLightVec     : TEXCOORD5;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(VC_WORLD_PROJ),
                  uniform float3   inLightVec      : register(VC_LIGHT_DIR1),
                  uniform float3x3 cubeTrans       : register(VC_CUBE_TRANS),
                  uniform float3   cubeEyePos      : register(VC_CUBE_EYE_POS),
                  uniform float3   eyePos          : register(VC_EYE_POS)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   OUT.shading = ( dot(-inLightVec, IN.normal) + 1.0) * 0.5;
   OUT.shading.w = 1.0;
   OUT.outTexCoord = IN.texCoord;
   float3 cubeVertPos = mul(cubeTrans, IN.position).xyz;
   float3 cubeNormal = normalize( mul(cubeTrans, IN.normal).xyz );
   float3 eyeToVert = cubeVertPos - cubeEyePos;
   OUT.reflectVec = reflect(eyeToVert, cubeNormal);

   OUT.outNormal = IN.normal;
   OUT.pos = IN.position.xyz;
   OUT.outEyePos.xyz = eyePos.xyz;
   OUT.outLightVec.xyz = -inLightVec;
   OUT.outLightVec.w = step( -0.5, dot( -inLightVec, IN.normal ) );   return OUT;
}
