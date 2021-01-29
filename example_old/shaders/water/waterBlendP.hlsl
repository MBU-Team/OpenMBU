//*****************************************************************************
// Lightmap / cubemap shader
//*****************************************************************************

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS             : POSITION;
	float2 TEX0             : TEXCOORD0;
	float2 TEX1             : TEXCOORD1;
};



struct Fragout
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
				uniform sampler2D    texMap      : register(S0),
				uniform sampler2D    texMap2     : register(S1)
)
{
	Fragout OUT;

   float4 tex0 = tex2D( texMap, IN.TEX0 );
   float4 tex1 = tex2D( texMap2, IN.TEX1 );

   float4 color = (tex0 + tex1) / 2.0;
   OUT.col = color;

	return OUT;
}

