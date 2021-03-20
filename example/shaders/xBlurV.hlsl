//*****************************************************************************
// Glow shader
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct VertData
{
   float2 texCoord        : TEXCOORD0;
   float4 position        : POSITION;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 tex0            : TEXCOORD0;
   float2 tex1            : TEXCOORD1;
   float2 tex2            : TEXCOORD2;
   float2 tex3            : TEXCOORD3;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0),
                  uniform float2   offset0         : register(C4),
                  uniform float2   offset1         : register(C5),
                  uniform float2   offset2         : register(C6),
                  uniform float2   offset3         : register(C7)
)
{
   ConnectData OUT;

   OUT.hpos = mul(modelview, IN.position);
   
   OUT.tex0 = IN.texCoord + offset0;
   OUT.tex1 = IN.texCoord + offset1;
   OUT.tex2 = IN.texCoord + offset2;
   OUT.tex3 = IN.texCoord + offset3;

   return OUT;
}
