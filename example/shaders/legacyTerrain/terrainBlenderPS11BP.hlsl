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
	float2 tex1       : TEXCOORD1;
   float2 tex2       : TEXCOORD2;
   float2 texLM      : TEXCOORD3;
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
				uniform sampler2D    tex1     : register(S1),
            uniform sampler2D    tex2     : register(S2),
            uniform sampler2D    lightMap : register(S3)
)
{
	Fragout OUT;

   // Get colors
   float4 o  = tex2D(opacity,  IN.tex);
   float4 t1 = tex2D(tex1,     IN.tex1);
   float4 t2 = tex2D(tex2,     IN.tex2);
   float4 lm = tex2D(lightMap, IN.texLM);

   // Blend
   OUT.col  = (o.b * t1 + o.a * t2) * lm * 2.0;
   
   
	return OUT;
}
