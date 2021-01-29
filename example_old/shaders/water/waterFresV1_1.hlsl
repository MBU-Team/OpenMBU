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
	float2 TEX0             : TEXCOORD1;
	float3 reflectVec       : TEXCOORD2;
	float2 fogCoord         : TEXCOORD3;
};



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(  Appdata In, 
            uniform float4x4 modelview    : register(VC_WORLD_PROJ),
            uniform float3   eyePos       : register(VC_EYE_POS),
            uniform float3x3 cubeTrans    : register(VC_CUBE_TRANS),
            uniform float3   cubeEyePos   : register(VC_CUBE_EYE_POS),
            uniform float4x4 objTrans     : register(VC_OBJ_TRANS),
            uniform float3   fogData      : register(VC_FOGDATA)
)
{
   Conn Out;

   Out.HPOS = mul(modelview, In.position);
   
   float3 eyeToVert = normalize( eyePos - In.position );
   Out.TEX0.x = dot( eyeToVert, float3( 0.0, 0.0, 1.0 ) );
   Out.TEX0.y = 0.0;
   
   float3 cubeVertPos = mul(cubeTrans, In.position).xyz;
   float3 cubeNormal = normalize( mul(cubeTrans, float3( 0.0, 0.0, 1.0 )).xyz );
   float3 cubeEyeToVert = cubeVertPos - cubeEyePos;
   Out.reflectVec = reflect(cubeEyeToVert, cubeNormal);

   float3 transPos = mul( objTrans, In.position );
   Out.fogCoord.x = 1.0 - ( distance( In.position, eyePos ) / fogData.z );
   Out.fogCoord.y = (transPos.z - fogData.x) * fogData.y;

   return Out;
}


