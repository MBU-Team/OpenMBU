#define IN_HLSL
#include "shdrConsts.h"

//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float4 texCoord        : TEXCOORD0;
   float2 lmCoord         : TEXCOORD1;
   float3 T               : TEXCOORD2;
   float3 B               : TEXCOORD3;
   float3 N               : TEXCOORD4;
   float3 normal          : NORMAL;
   float4 position        : POSITION;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float4 texCoord        : TEXCOORD0;
   float2 tex2            : TEXCOORD1;
   float4 lightVec        : TEXCOORD2;
   float3 pos             : TEXCOORD3;
   float3 eyePos          : TEXCOORD4;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelviewProj   : register(VC_WORLD_PROJ),
                  uniform float3   eyePos          : register(VC_EYE_POS),
                  uniform float3   lightVec        : register(VC_LIGHT_DIR1)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelviewProj, IN.position);

   float4x4 texGenTest = { 0.5,  0.0,  0.0,  0.5 + 0.5/512,
                           0.0, -0.5,  0.0,  0.5 + 1.0/512,
                           0.0,  0.0,  1.0,  0.0,
                           0.0,  0.0,  0.0,  1.0 };
                           
   OUT.texCoord = mul( texGenTest, OUT.hpos );
   OUT.tex2 = IN.texCoord;

   float3x3 objToTangentSpace;
   objToTangentSpace[0] = IN.T;
   objToTangentSpace[1] = IN.B;
   objToTangentSpace[2] = IN.N;
   
   OUT.lightVec.xyz = -lightVec;
   OUT.lightVec.xyz = mul(objToTangentSpace, OUT.lightVec);
   OUT.pos = mul(objToTangentSpace, IN.position.xyz / 100.0);;
   OUT.eyePos.xyz = mul(objToTangentSpace, eyePos.xyz / 100.0);;
   OUT.lightVec.w = step( 0.0, dot( -lightVec, IN.normal ) );
   
   return OUT;
}
