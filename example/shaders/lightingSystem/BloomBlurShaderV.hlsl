//-----------------------------------------------
// Synapse Gaming - Lighting System
// Copyright © Synapse Gaming 2003
// Written by John Kabus
//-----------------------------------------------
#define IN_HLSL
#include "../shdrConsts.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
struct Appdata
{
	float4 position   : POSITION;
	float4 baseTex    : TEXCOORD0;
	float4 lmTex      : TEXCOORD1;
	float3 T          : TEXCOORD2;
	float3 B          : TEXCOORD3;
	float3 N          : TEXCOORD4;
};

struct Conn
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

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(Appdata In,
		  uniform float4 stride        : register(C0))
{
	Conn Out;
	
	Out.HPOS = In.position;
	
	
	float4 tex = In.baseTex;
	float4 start = tex - (stride * 2.0);
	
	tex.y = start.y;
	Out.TEX0.xy = tex.xy;
	

	tex.x = start.x;
	tex.y += stride.y;
	
	tex.x += stride.x;
	Out.TEX0.zw = tex.xy;
	tex.x += stride.x;
	Out.TEX1.xy = tex.xy;
	tex.x += stride.x;
	Out.TEX1.zw = tex.xy;

	
	tex.x = start.x;
	tex.y += stride.y;
	
	Out.TEX2.xy = tex.xy;
	tex.x += stride.x;
	Out.TEX2.zw = tex.xy;
	tex.x += stride.x;
	Out.TEX3.xy = tex.xy;
	tex.x += stride.x;
	Out.TEX3.zw = tex.xy;
	tex.x += stride.x;
	Out.TEX4.xy = tex.xy;
	
	
	tex.x = start.x;
	tex.y += stride.y;
	
	tex.x += stride.x;
	Out.TEX4.zw = tex.xy;
	tex.x += stride.x;
	Out.TEX5.xy = tex.xy;
	tex.x += stride.x;
	Out.TEX5.zw = tex.xy;

	
	tex.x = In.baseTex.x;
	tex.y += stride.y;
	
	Out.TEX6 = tex;
	
	return Out;
}

