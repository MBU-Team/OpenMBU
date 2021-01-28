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
	float2 TEX0 : TEXCOORD0;
	float2 TEX1 : TEXCOORD1;
	float2 TEX2 : TEXCOORD2;
	float2 TEX3 : TEXCOORD3;
	// keep this float3, DX9 complains about anything less...
	float4 COL  : COLOR0;
};

struct Fragout
{
	float4 col : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
			 uniform sampler2D diffuseMap0: register(S0),
			 uniform sampler2D diffuseMap1: register(S1),
			 uniform sampler2D diffuseMap2: register(S2),
			 uniform sampler2D diffuseMap3: register(S3),
			 uniform float4    color      : register(C3))
{
	Fragout OUT;
	
	float4 s;
	s = tex2D(diffuseMap0, IN.TEX0);
	s += tex2D(diffuseMap1, IN.TEX1);
	s += tex2D(diffuseMap2, IN.TEX2);
	s += tex2D(diffuseMap3, IN.TEX3);
	
	float4 shadow = color * s * 0.25 * 0.5;
	shadow *= IN.COL.x;
	shadow = 1.0 - shadow;
	
	OUT.col = shadow;
	// not enough instructions for the alpha test... :(
	//OUT.col.w = 1.0 - shadow.x;
	
	return OUT;
}
