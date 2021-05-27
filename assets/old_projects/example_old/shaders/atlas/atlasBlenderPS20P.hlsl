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
   float2 tex1       : TEXCOORD2;
   float2 tex2       : TEXCOORD3;
   float2 tex3       : TEXCOORD4;
   float2 tex4       : TEXCOORD5;
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
	uniform sampler2D    tex1     : register(S2),
	uniform sampler2D    tex2     : register(S3),
	uniform sampler2D    tex3     : register(S4),
	uniform sampler2D    tex4     : register(S5)
)
{
	Fragout OUT;

   // Get colors
   float4 o  = tex2D(opacity,  IN.tex);
   float4 t1 = tex2D(tex1,     IN.tex1);
   float4 t2 = tex2D(tex2,     IN.tex2);
   float4 t3 = tex2D(tex3,     IN.tex3);
   float4 t4 = tex2D(tex4,     IN.tex4);
   float4 lm = tex2D(lightMap, IN.lmTex);

   // Blend
   OUT.col = lm * (o.r * t1 + o.g * t2 + o.b * t3 + o.a * t4);
   
   
   return OUT;
}
