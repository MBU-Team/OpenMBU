//*****************************************************************************
// TSE -- HLSL procedural shader                                               
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

#define IN_HLSL
#include "./shdrConsts.h"

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
   float3 pos             : TEXCOORD1;
   float3 outEyePos       : TEXCOORD2;
   float4 outLightVec     : TEXCOORD3;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(VC_WORLD_PROJ),
                  uniform float3   inLightVec      : register(VC_LIGHT_DIR1),
                  uniform float4   inLightColor    : register(VC_LIGHT_DIFFUSE1),
                  uniform float3   eyePos          : register(VC_EYE_POS),
                  uniform float3   fogData         : register(VC_FOGDATA),
                  uniform float4x4 objTrans        : register(VC_OBJ_TRANS)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   OUT.outTexCoord = IN.texCoord * 0.5;
   OUT.shading = inLightColor;

   float3x3 objToTangentSpace;
   objToTangentSpace[0] = IN.T;
   objToTangentSpace[1] = IN.B;
   objToTangentSpace[2] = IN.N;

   OUT.outLightVec.xyz = -inLightVec;
   OUT.outLightVec.xyz = mul(objToTangentSpace, OUT.outLightVec);
   OUT.pos = mul(objToTangentSpace, IN.position.xyz / 100.0);;
   OUT.outEyePos.xyz = mul(objToTangentSpace, eyePos.xyz / 100.0);;
   OUT.outLightVec.w = step( 0.0, dot( -inLightVec, IN.normal ) );

   return OUT;
}
