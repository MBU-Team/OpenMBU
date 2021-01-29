//*****************************************************************************
// Lightmap / cubemap shader
//*****************************************************************************

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------
struct v2f
{
   float4 HPOS       : POSITION;
	float4 TEX0       : TEXCOORD0;
	float4 TEX1       : TEXCOORD1;
	float4 reflectVec : TEXCOORD2;
   float  fresnel    : TEXCOORD3;
};

struct Fragout
{
   float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
				uniform sampler2D    baseMap  : register(S0),
				uniform sampler2D    lightMap : register(S1),
            uniform samplerCUBE  cubeMap  : register(S2)
)
{
	Fragout OUT;

   float4 reflectColor = texCUBE(cubeMap, IN.reflectVec) * 0.1 * IN.fresnel;
   OUT.col = reflectColor + tex2D(lightMap, IN.TEX1) * tex2D(baseMap, IN.TEX0);

	return OUT;
}
