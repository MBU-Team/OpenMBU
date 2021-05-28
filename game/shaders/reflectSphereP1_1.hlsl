//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------

struct ConnectData
{
   float3 reflectVec      : TEXCOORD0;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform samplerCUBE cubeMap         : register(S0)
)
{
   Fragout OUT;

   OUT.col = texCUBE(cubeMap, IN.reflectVec);
   OUT.col = OUT.col + float4( 0.0, 0.0, 0.1, 0.0 );

   return OUT;
}
