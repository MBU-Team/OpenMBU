//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 shading         : COLOR;
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform float4    ambient         : register(C2),
              uniform sampler2D diffuseMap      : register(S0),
              uniform sampler2D fogMap          : register(S1)
)
{
   Fragout OUT;

   OUT.col = IN.shading + ambient;
   OUT.col *= tex2D(diffuseMap, IN.texCoord);
   float4 fogColor = tex2D(fogMap, IN.fogCoord);
   OUT.col = lerp( OUT.col, fogColor, fogColor.a );

   return OUT;
}
