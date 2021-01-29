//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 shading         : COLOR;
   float2 texCoord        : TEXCOORD0;
   float2 lmCoord         : TEXCOORD1;
   float2 fogCoord        : TEXCOORD2;
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
              uniform sampler2D lightMap        : register(S1),
              uniform sampler2D fogMap          : register(S2)
)
{
   Fragout OUT;

   OUT.col = tex2D(diffuseMap, IN.texCoord);
   OUT.col *= tex2D(lightMap, IN.lmCoord) * IN.shading + ambient;
   float4 fogColor = tex2D(fogMap, IN.fogCoord);
   OUT.col = lerp( OUT.col, fogColor, fogColor.a );

   return OUT;
}