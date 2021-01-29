#define IN_HLSL
#include "..\shdrConsts.h"

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
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
   float4 bumpColor    = tex2D(bumpMap, IN.TEX0);
   
   // Get the object rotation matrix.
   float3x3 objRotMatrix = float3x3(IN.objRot0, IN.objRot1, IN.objRot2);

   // Eye position and normal.
   float3 eye  = IN.eyePos;
   float3 norm = normalize(bumpColor.xyz * 2.0 - 1.0);
   float4 diffuse = diffuseColor * (dot( norm, IN.lightVec.xyz) + 1.3) * 0.5;;
   
   // Also do some lighting.
   float3 eyeVec = normalize(IN.eyePos - IN.pos);
   float3 halfAng = normalize(eyeVec + IN.lightVec.xyz);
   float4 specular = dot(norm, halfAng) * IN.lightVec.w;
   specular = specularColor * pow(specular, specularPower);


   float fresnelBias = 0.0;
   float fresnelPow  = 1.2;
   float fresnelScale = 1.0;
   float fresnelTerm = fresnelBias + fresnelScale*(1.0-fresnelBias)*pow(1.0 - max(dot(eyeVec, norm), 0.0), fresnelPow);

   // And figure our reflection.
   norm = mul(objRotMatrix, norm);
   float3 reflectVec = 2.0 * dot( IN.cubemapEye, norm ) * norm - IN.cubemapEye * dot( norm, norm );

   // Sample the cubemap.
   float4 reflectColor = texCUBE(cubeMap, reflectVec);
   float4 finalReflectColor = (reflectColor.x + reflectColor.x + reflectColor.x) / 3.0;

   // Modulate this stuff by the alpha in the diffmap, 1 = shiny, 0 = no
   OUT.color = texColor * lerp(float4(1,1,1,1), finalReflectColor, bumpColor.a) * diffuse +
               specular * bumpColor.a;

   return OUT;
}
