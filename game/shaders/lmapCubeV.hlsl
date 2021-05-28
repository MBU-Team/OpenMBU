//*****************************************************************************
// Lightmap / cubemap shader
//*****************************************************************************
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
   float4 HPOS : POSITION;
	float4 TEX0 : TEXCOORD0;
	float4 TEX1 : TEXCOORD1;
	float3 reflectVec : TEXCOORD2;
   float3 fresnel : TEXCOORD3;
};


//-----------------------------------------------------------------------------
// fresnel approximation
//-----------------------------------------------------------------------------
float fast_fresnel(float3 I, float3 N, float3 fresnelValues)
{
    float power = fresnelValues.x;
    float scale = fresnelValues.y;
    float bias = fresnelValues.z;

    return bias + pow(1.0 - dot(I, N), power) * scale;
}


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
   Out.TEX0 = In.baseTex;
   Out.TEX1 = In.lmTex;


   // rotate position and normal by modelview orientation
   float3 pos = mul( cubeTrans, In.position ).xyz;
   float3 newNorm = mul( cubeTrans, In.normal ).xyz;
   normalize( newNorm );

   // calc reflection vector and pass to pixel shader
   float3 eyeToVert = pos - cubeEyePos.xyz;
   normalize(eyeToVert);

   float3 vertToEye = cubeEyePos.xyz - In.position;
   normalize( vertToEye );
   
//   Out.fresnel = fast_fresnel( vertToEye, In.normal, float3( 1.0, 1.0, 0.5) ).xxx;
   float surfAng = max( 1.0 - dot( vertToEye, In.normal ), 0.0 );
   
   Out.fresnel = (surfAng * 0.25 + 0.5).xxx;
   
   Out.reflectVec = reflect(eyeToVert, newNorm.xyz);


   return Out;
}

