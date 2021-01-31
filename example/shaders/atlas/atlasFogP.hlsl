struct ConnectData
{
   float4 hpos      : POSITION;
   float2 fogCoord  : TEXCOORD0;
};

struct Fragout
{
   float4 col : COLOR0;
};

Fragout main( ConnectData IN,
              uniform sampler2D fogMap          : register(S0)
              )
{
   Fragout OUT;
   OUT.col = tex2D(fogMap,     IN.fogCoord);
   return OUT;
}
