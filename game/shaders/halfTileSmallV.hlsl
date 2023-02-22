//*****************************************************************************
// TSE -- HLSL procedural shader                                               
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float2 texCoord        : TEXCOORD0;
   float2 lmCoord         : TEXCOORD1;
   float3 T               : TEXCOORD2;
   float3 B               : TEXCOORD3;
//   float3 N               : TEXCOORD4;
   float3 normal          : NORMAL;
   float4 position        : POSITION;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float4 shading         : COLOR;
   float2 outTexCoord     : TEXCOORD0;
   float3 pos             : TEXCOORD1;
   float3 outEyePos       : TEXCOORD2;
   float4 outLightVec     : TEXCOORD3;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0),
                  uniform float3   inLightVec      : register(C24),
                  uniform float4   inLightColor    : register(C25),
                  uniform float3   eyePos          : register(C20),
                  uniform float3   fogData         : register(C22),
                  uniform float4x4 objTrans        : register(C12)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   OUT.outTexCoord = IN.texCoord;
   OUT.shading = inLightColor;

   float3x3 objToTangentSpace;
   objToTangentSpace[0] = IN.T;
   objToTangentSpace[1] = IN.B;
   objToTangentSpace[2] = IN.normal;

   OUT.outLightVec.xyz = -inLightVec;
   OUT.outLightVec.xyz = mul(objToTangentSpace, OUT.outLightVec);
   OUT.pos = mul(objToTangentSpace, IN.position.xyz / 100.0);;
   OUT.outEyePos.xyz = mul(objToTangentSpace, eyePos.xyz / 100.0);;
   OUT.outLightVec.w = step( 0.0, dot( -inLightVec, IN.normal ) );

   return OUT;
}
