#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS             : POSITION;
	float4 TEX0             : TEXCOORD0;
	float4 tangentToCube0   : TEXCOORD1;
	float4 tangentToCube1   : TEXCOORD2;
	float4 tangentToCube2   : TEXCOORD3;
   float4 lightVec         : TEXCOORD4;
   float3 pixPos           : TEXCOORD5;
   float3 eyePos           : TEXCOORD6;
};



struct Fragout
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
				uniform sampler2D    diffMap  : register(S0),
				uniform sampler2D    bumpMap  : register(S2),
            uniform samplerCUBE  cubeMap  : register(S1),
            uniform float4    specularColor   : register(PC_MAT_SPECCOLOR),
            uniform float     specularPower   : register(PC_MAT_SPECPOWER),
            uniform float4    ambient         : register(PC_AMBIENT_COLOR),
            uniform float accumTime   : register(PC_ACCUM_TIME)
)
{
	Fragout OUT;

   float2 texOffset;
   float sinOffset1 = sin( accumTime * 0.5 + IN.TEX0.y * 6.28319 * 3.0 ) * 0.01;
   float sinOffset2 = sin( accumTime * 1.0 + IN.TEX0.y * 6.28319 ) * 0.01;

   texOffset.x = IN.TEX0.x + sinOffset1 + sinOffset2;
   texOffset.y = IN.TEX0.y + cos( accumTime * 3.0 + IN.TEX0.x * 6.28319 * 2.0 ) * 0.01;


   float4 bumpNorm = tex2D( bumpMap, texOffset ) * 2.0 - 1.0;
   float4 diffuse = tex2D( diffMap, texOffset );

   OUT.col = diffuse * (saturate( dot( IN.lightVec, bumpNorm.xyz ) ) + ambient);
   
   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float specular = saturate( dot(bumpNorm, halfAng) ) * IN.lightVec.w;
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular;
   
	
   
   return OUT;
}

