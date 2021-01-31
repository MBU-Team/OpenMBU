//*****************************************************************************
// Lightmap / cubemap shader
//*****************************************************************************

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS       : POSITION;
   float2 tex        : TEXCOORD0;
   float2 lmTex      : TEXCOORD1;
   float2 tex3       : TEXCOORD2;
   float2 tex4       : TEXCOORD3;
};

struct Fragout
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
	uniform sampler2D    opacity  : register(S0),
	uniform sampler2D    lightMap : register(S1),
	uniform sampler2D    tex3     : register(S2),
	uniform sampler2D    tex4     : register(S3)
)
{
	Fragout OUT;

   // Get colors
   float4 o  = tex2D(opacity,  IN.tex);
   float4 t3 = tex2D(tex3,     IN.tex3);
   float4 t4 = tex2D(tex4,     IN.tex4);
   float4 lm = tex2D(lightMap, IN.lmTex);

   // Blend
   OUT.col = lm * (o.b * t3 + o.a * t4);
   
   
   return OUT;
}
