//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 texCoord        : TEXCOORD0;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D refractMap      : register(S0)
)
{
   Fragout OUT;

//   float2 texCoord = float2( IN.pos.x / (IN.pos.w), -IN.pos.y / (IN.pos.w) );
//   texCoord = ( (texCoord + 1.0) * 0.5 );
   
   
   OUT.col = tex2D( refractMap, IN.texCoord ) + float4( 0.1, 0.1, 0.2, 0.0 );


   return OUT;
}
