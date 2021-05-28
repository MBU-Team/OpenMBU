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
   float4 HPOS             : POSITION;  // Transformed vertex.
   float4 TEX0             : TEXCOORD0; // Texture coordinates.
   
   float3 objRot0          : TEXCOORD1; // Object rotation matrix.
   float3 objRot1          : TEXCOORD2; //
   float3 objRot2          : TEXCOORD3; //
   
   float3 pos              : TEXCOORD4; // Position in object space of the current pixel.
   float4 lightVec         : TEXCOORD5; // Light vector in object space.
   float3 eyePos           : TEXCOORD6; // Eye position in object space.
   float3 cubemapEye       : TEXCOORD7; // Eye vector for cubemap lookups.

   float4 diffuse          : COLOR0;    // Diffuse color of light.
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
   
   //  Write out trivial things
   Out.TEX0 = In.baseTex;
   Out.diffuse = inLightDiff;
   
   // Trivial transforms.
   Out.HPOS   = mul(modelview, In.position);
   Out.pos    = In.position.xyz;
   Out.eyePos = eyePos;
   
   // Note the rotation matrix.
   Out.objRot0 = objTrans[0].xyz;
   Out.objRot1 = objTrans[1].xyz;
   Out.objRot2 = objTrans[2].xyz;
   
   float3x3 objRot;
   objRot[0] = objTrans[0].xyz;
   objRot[1] = objTrans[1].xyz;
   objRot[2] = objTrans[2].xyz;

   // Fill out the light info.
   Out.lightVec.xyz = -inLightVec;
   Out.lightVec.w = step( -0.5, dot( -inLightVec, In.normal ) );
   
   // And do the normalized eye vector for the cubemap.
   float3 pos = mul( cubeTrans, In.position ).xyz;
   float3 eye = cubeEyePos.xyz - pos;
   normalize( eye );
   Out.cubemapEye = eye;
   
   return Out;
}

