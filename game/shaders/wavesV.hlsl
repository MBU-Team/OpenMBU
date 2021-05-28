#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
struct Appdata
{
	float4 position   : POSITION;
	float4 normal     : NORMAL;
	float4 baseTex    : TEXCOORD0;
	float4 lmTex      : TEXCOORD1;
	float3 T          : TEXCOORD2;
	float3 B          : TEXCOORD3;
	float3 N          : TEXCOORD4;
};


struct Conn
{
   float4 HPOS             : POSITION;
	float4 TEX0             : TEXCOORD0;
	float4 tangentToCube0   : TEXCOORD1;
	float4 tangentToCube1   : TEXCOORD2;
	float4 tangentToCube2   : TEXCOORD3;
   float4 outLightVec      : TEXCOORD4;
   float3 pos              : TEXCOORD5;
   float3 outEyePos        : TEXCOORD6;
   
};



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main( Appdata In, 
           uniform float4x4 modelview : register(VC_WORLD_PROJ),
           uniform float3x3 cubeTrans : register(VC_CUBE_TRANS),
           uniform float4   cubeEyePos : register(VC_CUBE_EYE_POS),
           uniform float3   inLightVec : register(VC_LIGHT_DIR1),
           uniform float3   eyePos     : register(VC_EYE_POS)
)
{
   Conn Out;

   Out.HPOS = mul(modelview, In.position);
   Out.TEX0 = In.baseTex;

   
	float3x3 objToTangentSpace;
	objToTangentSpace[0] = In.T;
	objToTangentSpace[1] = In.B;
	objToTangentSpace[2] = In.N;
   
   
   Out.tangentToCube0.xyz = mul( objToTangentSpace, cubeTrans[0].xyz );
   Out.tangentToCube1.xyz = mul( objToTangentSpace, cubeTrans[1].xyz );
   Out.tangentToCube2.xyz = mul( objToTangentSpace, cubeTrans[2].xyz );
   
   float3 pos = mul( cubeTrans, In.position ).xyz;
   float3 eye = cubeEyePos.xyz - pos;
   normalize( eye );

   Out.tangentToCube0.w = eye.x;
   Out.tangentToCube1.w = eye.y;
   Out.tangentToCube2.w = eye.z;

   Out.outLightVec.xyz = -inLightVec;
   Out.outLightVec.xyz = mul(objToTangentSpace, Out.outLightVec);
   Out.pos = mul(objToTangentSpace, In.position.xyz / 10.0);;
   Out.outEyePos.xyz = mul(objToTangentSpace, eyePos.xyz / 10.0);;
   Out.outLightVec.w = step( 0.0, dot( -inLightVec, In.normal ) );
   
   
   return Out;
}


