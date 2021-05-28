//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
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
              uniform sampler2D diffuseMap      : register(S0),
              uniform sampler2D lightMap        : register(S1),
              uniform sampler2D fogTex          : register(S2)
)
{
   Fragout OUT;

   float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);
   float4 lmColor = tex2D(lightMap, IN.lmCoord);
   float4 fogColor = tex2D(fogTex, IN.fogCoord);
   
   OUT.col = lerp( diffuseColor * lmColor, fogColor, fogColor.a );

   return OUT;
}
