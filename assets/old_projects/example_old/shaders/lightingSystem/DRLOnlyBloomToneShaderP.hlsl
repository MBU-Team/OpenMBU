//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------

//-----------------------------------------------------------------------------
// Data 
//-----------------------------------------------------------------------------
struct v2f
{
	float4 HPOS : POSITION;
	float4 TEX0 : TEXCOORD0;
};

struct Fragout
{
	float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
			 //uniform sampler2D intensitymap : register(S0),
			 uniform sampler2D bloommap1    : register(S1),
			 //uniform sampler2D bloommap2    : register(S2),
			 uniform sampler2D frame        : register(S3),
			 //uniform sampler2D tonearea     : register(S4),
			 uniform sampler2D tonemap      : register(S5),
			 uniform float4 drlinfo         : register(C0))
{
	Fragout OUT;
	
	float4 framecol = tex2D(frame, IN.TEX0);
	OUT.col = framecol;
	
	framecol.w = max(max(framecol.x, framecol.y), framecol.z);
	
	float2 uv;
	uv.x = framecol.w * drlinfo.w * 0.5;
	uv.y = 0.5;
	
	OUT.col.xyz += tex2D(bloommap1, IN.TEX0).xyz * 2.0;
	OUT.col.xyz *= tex2D(tonemap, uv).xyz * 2.0;

	return OUT;
}
