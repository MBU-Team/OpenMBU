//*****************************************************************************
// TSE -- HLSL procedural shader                                               
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float4 position        : POSITION;
};


struct ConnectData
{
   float4 hpos            : POSITION;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0)
)
{
   ConnectData OUT;
   
   OUT.hpos = mul(modelview, IN.position);
   OUT.hpos.z += 0.001; // Z-bias on 360 -pw
   
   return OUT;
}
