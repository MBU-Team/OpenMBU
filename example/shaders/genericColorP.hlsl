//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float4 color : COLOR0;
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
   
   //float4 diffuseColor = tex2D(diffuseMap, IN.texCoord);

   OUT.col = IN.color;
   
   return OUT;
}

