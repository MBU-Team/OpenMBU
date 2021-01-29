//*****************************************************************************
// Glow Shader
//*****************************************************************************
//-----------------------------------------------------------------------------
// Structures                                                                  
//-----------------------------------------------------------------------------
struct ConnectData
{
   float2 tex0            : TEXCOORD0;
   float2 tex1            : TEXCOORD1;
   float2 tex2            : TEXCOORD2;
   float2 tex3            : TEXCOORD3;
};


struct Fragout
{
   half4 col : COLOR0;
};


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( ConnectData IN,
              uniform sampler2D diffuseMap0  : register(S0),
              uniform sampler2D diffuseMap1  : register(S1),
              uniform sampler2D diffuseMap2  : register(S2),
              uniform sampler2D diffuseMap3  : register(S3),
              uniform float4    kernel      : register(C0)
)
{
   Fragout OUT;

   OUT.col =  tex2D( diffuseMap0, IN.tex0 ) * kernel.x;
   OUT.col += tex2D( diffuseMap1, IN.tex1 ) * kernel.y;
   OUT.col += tex2D( diffuseMap2, IN.tex2 ) * kernel.z;
   OUT.col += tex2D( diffuseMap3, IN.tex3 ) * kernel.w;


   return OUT;
}

