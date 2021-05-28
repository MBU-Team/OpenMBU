//This shader has the alpha channel block specular hearts xbox huge
#define IN_HLSL
#include "..\shdrConsts.h"

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS             : POSITION;
   float4 TEX0             : TEXCOORD0;
   float3 otherEyePos      : TEXCOORD1;
   float4 lightVec         : TEXCOORD4;
   float3 pixPos           : TEXCOORD5;
   float3 eyePos           : TEXCOORD6;
   float3 realPixPos       : TEXCOORD7;
   float4 diffuse    : COLOR0;
};

struct Fragout
{
   float4 color : COLOR0;
};

// Helper fresnel function.
float3 fast_fresnel(float3 I, float3 N, float3 fresnelValues)
{
    float power = fresnelValues.x;
    float  scale = fresnelValues.y;
    float  bias = fresnelValues.z;

    return bias + pow(1.0 - dot(I, N), power) * scale;
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
             uniform sampler2D    bumpMap  : register(S0),
             uniform sampler2D    diffMap  : register(S1),
             uniform samplerCUBE  cubeMap  : register(S3),
             uniform float4    specularColor   : register(PC_MAT_SPECCOLOR),
             uniform float     specularPower   : register(PC_MAT_SPECPOWER),
             uniform float3    marbVel         : register(C12)            
)
{
   Fragout OUT;

   // Get diffuse and normal.
   float4 diffuseColor = IN.diffuse;
   float4 texColor     = tex2D(diffMap, IN.TEX0);
   
   // Eye position and normal.
   float3 eye          = IN.otherEyePos;
   float3 norm         =  normalize(IN.realPixPos);

   // And figure our reflection.
   float3 reflectVec = 2.0 * dot( eye, norm ) * norm - eye * dot( norm, norm );
   
   // Also do some lighting.
   float3 eyeVec = normalize(IN.eyePos - IN.pixPos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float4 specular = dot(norm, halfAng) * IN.lightVec.w;
   specular = specularColor * pow(specular, specularPower);

   float4 diffuse = diffuseColor * (dot( norm, IN.lightVec.xyz) + 1.3) * 0.5;

   // Sample the cubemap.
   float4 reflectColor = texCUBE(cubeMap, reflectVec);
   
   // This desaturates the colors.
   //float4 avgColor = (reflectColor.r + reflectColor.g + reflectColor.b) / 3.0;
   //float4 finalReflectColor = lerp( reflectColor, avgColor, 0.1 );
   
   float4 finalReflectColor = reflectColor;

   // Modulate this stuff by the alpha in the diffmap, 1 = shiny, 0 = no
   OUT.color = lerp(texColor * diffuse * 1.2,
                    finalReflectColor * diffuse,
                    texColor.a) +
               specular * 0.5 * texColor.a;
   
   return OUT;
}
