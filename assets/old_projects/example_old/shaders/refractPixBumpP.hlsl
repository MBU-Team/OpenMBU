//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 pos        : TEXCOORD0;
   float2 texCoord   : TEXCOORD1;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D refractMap   : register(S0),
              uniform sampler2D bumpMap      : register(S1)
)
{
   Fragout OUT;

   float3 bumpNorm = tex2D( bumpMap, IN.texCoord ) * 2.0 - 1.0;
   float3 refractVec = refract( float3( 0.0, 0.0, 1.0), normalize( bumpNorm ), 1.0 );
   
   float2 refractCoord = float2( ( IN.pos.x + refractVec.x) / (IN.pos.w), 
                                 (-IN.pos.y + refractVec.y) / (IN.pos.w) );
   
   refractCoord = saturate( (refractCoord + 1.0) * 0.5 );
   
   OUT.col = tex2D( refractMap, refractCoord ) + 0.25;


   return OUT;
}
