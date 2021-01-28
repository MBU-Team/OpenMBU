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
				uniform sampler2D    bumpMap  : register(S0),
				uniform sampler2D    diffMap  : register(S1),
            uniform samplerCUBE  cubeMap  : register(S3),
            uniform float4    specularColor   : register(PC_MAT_SPECCOLOR),
            uniform float     specularPower   : register(PC_MAT_SPECPOWER)
            
)
{
	Fragout OUT;

   float3 bumpNorm = tex2D( bumpMap, IN.TEX0 ) * 2.0 - 1.0;
   
   float3 eye = float3( IN.tangentToCube0.w, 
                        IN.tangentToCube1.w, 
                        IN.tangentToCube2.w );

   float3x3 cubeMat = float3x3(  IN.tangentToCube0.xyz,
                                 IN.tangentToCube1.xyz,
                                 IN.tangentToCube2.xyz );

   float3 norm = mul( cubeMat, bumpNorm );

   float3 reflectVec = 2.0 * dot( eye, norm ) * norm - eye * dot( norm, norm );
                       
   
   float4 diffuseColor = tex2D( diffMap, IN.TEX0 );
   
   OUT.col = texCUBE(cubeMap, reflectVec ) * diffuseColor;

   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float specular = saturate( dot(bumpNorm, halfAng) ) * IN.lightVec.w;
   specular = pow(specular, specularPower);
   OUT.col += specularColor * specular * diffuseColor.a;
	
   
   return OUT;
}
