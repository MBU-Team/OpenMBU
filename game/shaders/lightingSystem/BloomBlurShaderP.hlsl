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
	float4 TEX1 : TEXCOORD1;
	float4 TEX2 : TEXCOORD2;
	float4 TEX3 : TEXCOORD3;
	float4 TEX4 : TEXCOORD4;
	float4 TEX5 : TEXCOORD5;
	float4 TEX6 : TEXCOORD6;
};

struct Fragout
{
	float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
			 uniform sampler2D diffuseMap : register(S0),
			 uniform float4 stride        : register(C0),
			 uniform float4 seedamount    : register(C1))
{
	Fragout OUT;
	
	float4 multiplier = 0.9;
	//float4 tex = IN.TEX0;
	//float4 stride = samplestride;
	//float4 start = tex - (stride * 2.0);
	
	//tex.y = start.y;
	OUT.col = tex2D(diffuseMap, IN.TEX0.xy) * 0.04;
	

	//tex.x = start.x;
	//tex.y += stride.y;
	

	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX0.zw) * 0.07;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX1.xy) * 0.1;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX1.zw) * 0.07;

	
	//tex.x = start.x;
	//tex.y += stride.y;
	
	OUT.col += tex2D(diffuseMap, IN.TEX2.xy) * 0.04;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX2.zw) * 0.1;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX3.xy) * seedamount;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX3.zw) * 0.1;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX4.xy) * 0.04;
	
	//tex.x = start.x;
	//tex.y += stride.y;
	

	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX4.zw) * 0.07;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX5.xy) * 0.1;
	//tex.x += stride.x;
	OUT.col += tex2D(diffuseMap, IN.TEX5.zw) * 0.07;

	
	//tex.x = IN.TEX0.x;
	//tex.y += stride.y;
	
	OUT.col += tex2D(diffuseMap, IN.TEX6.xy) * 0.04;
	
	
	return OUT;
}
