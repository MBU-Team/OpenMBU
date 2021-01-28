//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct Conn
{
   float2 texCoord : TEXCOORD0;
   float4 color : COLOR0;
};

struct Frag
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Frag main( Conn In,
           uniform sampler2D diffuseMap : register(S0)
)
{
   Frag Out;

   Out.col = tex2D(diffuseMap, In.texCoord) * In.color;

   return Out;
}
