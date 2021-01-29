//This shader has the alpha channel block specular hearts xbox huge
#define IN_HLSL
#include "..\shdrConsts.h"

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
   float3 otherEyePos      : TEXCOORD1;
   float4 outLightVec      : TEXCOORD4;
   float3 pos              : TEXCOORD5;
   float3 outEyePos        : TEXCOORD6;
   float3 outPixPos        : TEXCOORD7;
   float4 diffuse    : COLOR0;
};



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main( Appdata In, 
           uniform float4x4 modelview   : register(VC_WORLD_PROJ),
           uniform float4x4 objTrans    : register(VC_OBJ_TRANS),
           uniform float3x3 cubeTrans   : register(VC_CUBE_TRANS),
           uniform float4   cubeEyePos  : register(VC_CUBE_EYE_POS),
           uniform float3   inLightVec  : register(VC_LIGHT_DIR1),
           uniform float4   inLightDiff : register(VC_LIGHT_DIFFUSE1),
           uniform float3   inLightSpec : register(VC_LIGHT_SPEC1),
           uniform float3   eyePos      : register(VC_EYE_POS)
)
{
   Conn Out;

   Out.HPOS = mul(modelview, In.position);
   Out.pos = mul(objTrans, In.position.xyz / 100.0);;
   Out.TEX0 = In.baseTex;

   Out.diffuse = inLightDiff;

   // Compensate for object rotation...
   float3x3 objRot;
   objRot[0] = objTrans[0].xyz;
   objRot[1] = objTrans[1].xyz;
   objRot[2] = objTrans[2].xyz;
   
   Out.outPixPos = mul(objRot, In.position);
   
   float3 pos = mul( cubeTrans, In.position ).xyz;
   float3 eye = cubeEyePos.xyz - pos;
   normalize( eye );

   Out.otherEyePos = eye;

   Out.outLightVec.xyz = mul(objRot, -inLightVec);
   Out.outLightVec.w = step( -0.5, dot( -inLightVec, In.normal ) );
   
   
   Out.outEyePos.xyz = mul(objTrans, eyePos.xyz / 100.0);


   return Out;
}

