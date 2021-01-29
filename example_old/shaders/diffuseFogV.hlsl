//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float2 texCoord        : TEXCOORD0;
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
   float4 shading         : COLOR;
   float2 outTexCoord     : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0),
                  uniform float3   inLightVec      : register(C24),
                  uniform float3   eyePos          : register(C20),
                  uniform float3   fogData         : register(C22),
                  uniform float4x4 objTrans        : register(C12)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   OUT.shading = saturate( dot(-inLightVec, IN.normal) );
   OUT.outTexCoord = IN.texCoord;

   // fog setup
   float3 transPos = mul( objTrans, IN.position );
   OUT.fogCoord.x = 1.0 - ( distance( IN.position, eyePos ) / fogData.z );
   OUT.fogCoord.y = (transPos.z - fogData.x) * fogData.y;
   return OUT;
}
