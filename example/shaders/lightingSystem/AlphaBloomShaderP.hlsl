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
			 uniform sampler2D diffuseMap : register(S0))
{
	Fragout OUT;
	
	OUT.col = tex2D(diffuseMap, IN.TEX0);
	OUT.col.w = (OUT.col.x + OUT.col.y + OUT.col.z) * 0.333;
	
	OUT.col.x = (OUT.col.x - 0.75) * 2.0;
	OUT.col.y = (OUT.col.y - 0.75) * 2.0;
	OUT.col.z = (OUT.col.z - 0.75) * 2.0;

	return OUT;
}
