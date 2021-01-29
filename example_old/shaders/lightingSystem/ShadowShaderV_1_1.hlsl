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
	float2 TEX0 : TEXCOORD0;
	float2 TEX1 : TEXCOORD1;
	float2 TEX2 : TEXCOORD2;
	float2 TEX3 : TEXCOORD3;
	// keep this float3, DX9 complains about anything less...
	float3 COL  : COLOR0;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(Appdata In,
	uniform float4x4 toscreen         : register(C0),
	uniform float4x4 tolightspace     : register(C4),
	uniform float4   projectioninfo   : register(C8),
	uniform float4   stride           : register(C9))
{
	Conn Out;
	
	float4 pos = mul(tolightspace, In.position);
	
	float1 depth = pos.y;
	Out.TEX0.x = (pos.x * projectioninfo.z * 0.5) + 0.5;
	Out.TEX0.y = (-pos.z * projectioninfo.z * 0.5) + 0.5;
	
	Out.TEX1 = Out.TEX0;
	Out.TEX1.x += stride.x;
	Out.TEX2 = Out.TEX0;
	Out.TEX2.y += stride.y;
	Out.TEX3 = Out.TEX0 + stride.xy;
	
	Out.HPOS = mul(toscreen, In.position);
	Out.HPOS.z -= 0.0005;
	
	Out.COL = 0;
	Out.COL.x = 1.0 - (((depth + projectioninfo.y) / projectioninfo.x) - 0.001);
	//Out.COL = saturate(Out.COL);
	
	return Out;
}

