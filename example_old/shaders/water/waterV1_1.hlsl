//*****************************************************************************
// Lightmap / cubemap shader
//*****************************************************************************
#define IN_HLSL
#include "..\shdrConsts.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
struct Appdata
{
	float4 position   : POSITION;
};


struct Conn
{
   float4 HPOS             : POSITION;
	float2 TEX0             : TEXCOORD0;
	float4 tangentToCube0   : TEXCOORD1;
	float4 tangentToCube1   : TEXCOORD2;
	float4 tangentToCube2   : TEXCOORD3;
};



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main( Appdata In, 
           uniform float4x4 modelview : register(VC_WORLD_PROJ),
           uniform float3x3 cubeTrans : register(VC_CUBE_TRANS),
           uniform float4  cubeEyePos : register(VC_CUBE_EYE_POS)
)
{
   Conn Out;

   Out.HPOS = mul(modelview, In.position);
   Out.TEX0.xy = In.position.xy * 0.14;

   
	float3x3 objToTangentSpace;
	objToTangentSpace[0] = float3( 1.0, 0.0, 0.0 );
	objToTangentSpace[1] = float3( 0.0, 1.0, 0.0 );
	objToTangentSpace[2] = float3( 0.0, 0.0, 1.0 );
   
   
   Out.tangentToCube0.xyz = mul( objToTangentSpace, cubeTrans[0].xyz );
   Out.tangentToCube1.xyz = mul( objToTangentSpace, cubeTrans[1].xyz );
   Out.tangentToCube2.xyz = mul( objToTangentSpace, cubeTrans[2].xyz );
   
   float3 pos = mul( cubeTrans, In.position ).xyz;
   float3 eye = cubeEyePos.xyz - pos;
   normalize( eye );

   Out.tangentToCube0.w = eye.x;
   Out.tangentToCube1.w = eye.y;
   Out.tangentToCube2.w = eye.z;

   return Out;
}


