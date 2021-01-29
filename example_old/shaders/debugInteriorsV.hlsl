//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float2 texCoord        : TEXCOORD0;
   float3 normal          : NORMAL;
   float4 position        : POSITION;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 outTexCoord     : TEXCOORD0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0),
                  uniform float3   inLightVec      : register(C24)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   OUT.outTexCoord = IN.texCoord;

   return OUT;
}
