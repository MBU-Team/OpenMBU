//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float2 texCoord        : TEXCOORD0;
   float2 fogCoord        : TEXCOORD1;
   float2 detailCoord     : TEXCOORD2;
   float2 detailCoord2    : TEXCOORD3;
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
              uniform sampler2D fogMap          : register(S1),
              uniform sampler2D detailMap       : register(S2),
              uniform sampler2D detailMap2      : register(S3)
              )
{
   Fragout OUT;
   
   float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);
   float4 fogCol       = tex2D(fogMap,     IN.fogCoord);
   float4    detailCol  =  tex2D(detailMap,   IN.detailCoord);
             detailCol  *= tex2D(detailMap2,  IN.detailCoord2);

   OUT.col = lerp( diffuseColor * (detailCol + 0.6), fogCol, fogCol.a );
   
   return OUT;
}
