//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float2 texCoord   : TEXCOORD0;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN, uniform sampler2D diffuseMap : register(S0) )
{
   Fragout OUT;
   
   OUT.col = tex2D( diffuseMap, IN.texCoord );
   
   return OUT;
}

