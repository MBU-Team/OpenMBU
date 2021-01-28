#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float4 texCoord        : TEXCOORD0;
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
   float2 outTexCoord     : TEXCOORD0;
   float2 outBumpTexCoord : TEXCOORD1;
   float4 shading         : COLOR0;
   float3 reflectVec      : TEXCOORD2;
   float3 outLightVec     : TEXCOORD3;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(VC_WORLD_PROJ),
                  uniform float4x4 texMat          : register(VC_TEX_TRANS1),
                  uniform float3x3 cubeTrans       : register(VC_CUBE_TRANS),
                  uniform float3   cubeEyePos      : register(VC_CUBE_EYE_POS),
                  uniform float3   eyePos          : register(VC_EYE_POS)
)
{
   float3 inLightVec = normalize( float3( 0.0, -0.7, 0.3 ) );
   
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   OUT.outTexCoord = mul(texMat, IN.texCoord);
   OUT.outBumpTexCoord = OUT.outTexCoord;

   float3x3 objToTangentSpace = float3x3( IN.T, IN.B, IN.N );
   
   OUT.outLightVec = -inLightVec;
   OUT.outLightVec = mul( objToTangentSpace, OUT.outLightVec );
   OUT.outLightVec = OUT.outLightVec / 2.0 + 0.5;

   float3 cubeNormal = normalize( mul(cubeTrans, IN.normal).xyz );
   float3 cubeVertPos = mul(cubeTrans, IN.position).xyz;
   float3 eyeToVert = cubeVertPos - cubeEyePos;
   OUT.reflectVec = reflect(eyeToVert, cubeNormal);

   float3 eyeVec = normalize( eyePos - IN.position );
   float falloff = 1.0 - saturate( dot( eyeVec, IN.normal ) );
   float4 falloffColor = falloff * float4( 0.50, 0.50, 0.6, 1.0 );
   
   OUT.shading = falloffColor;


   return OUT;
}
