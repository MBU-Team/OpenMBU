//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 texCoord        : TEXCOORD0;
   float2 gradiantCoord   : TEXCOORD1;
   float4 specular        : COLOR0;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D refractMap      : register(S0),
              uniform sampler2D gradiantMap     : register(S1)
)
{
   Fragout OUT;

   
   OUT.col = tex2D( refractMap, IN.texCoord );
   OUT.col += tex2D( gradiantMap, IN.gradiantCoord );
   OUT.col += IN.specular;

   return OUT;
}
