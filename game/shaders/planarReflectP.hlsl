//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float2 texCoord   : TEXCOORD0;
   float4 tex2       : TEXCOORD1;
};


struct Fragout
{
   float4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D texMap          : register(S0),
              uniform sampler2D refractMap      : register(S1)
)
{
   Fragout OUT;

   float4 diffuseColor = tex2D( texMap, IN.texCoord );
   float4 reflectColor = tex2Dproj( refractMap, IN.tex2 );

   OUT.col = diffuseColor + reflectColor * diffuseColor.a;

   return OUT;
}
