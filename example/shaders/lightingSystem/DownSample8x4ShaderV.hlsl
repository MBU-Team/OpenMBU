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
	float4 TEX7 : TEXCOORD7;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(Appdata In,
		  uniform float4 samplestride  : register(C0))
{
	Conn Out;
	
	Out.HPOS = In.position;

	float4 uv;
	uv.xy = In.baseTex.xy;
	uv.zw = In.baseTex.xy;
	
	float4 stridey;
	stridey.x = 0;
	stridey.y = samplestride.y;
	stridey.z = 0;
	stridey.w = samplestride.y;
	
	Out.TEX0 = uv;
	Out.TEX0.z += samplestride.x;
	Out.TEX1 = uv;
	Out.TEX1.x += samplestride.x * 2.0;
	Out.TEX1.z += samplestride.x * 3.0;
	
	uv += stridey;
	Out.TEX2 = uv;
	Out.TEX2.z += samplestride.x;
	Out.TEX3 = uv;
	Out.TEX3.x += samplestride.x * 2.0;
	Out.TEX3.z += samplestride.x * 3.0;
	
	uv += stridey;
	Out.TEX4 = uv;
	Out.TEX4.z += samplestride.x;
	Out.TEX5 = uv;
	Out.TEX5.x += samplestride.x * 2.0;
	Out.TEX5.z += samplestride.x * 3.0;
	
	uv += stridey;
	Out.TEX6 = uv;
	Out.TEX6.z += samplestride.x;
	Out.TEX7 = uv;
	Out.TEX7.x += samplestride.x * 2.0;
	Out.TEX7.z += samplestride.x * 3.0;
	
	return Out;
}

