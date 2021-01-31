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
	float3 normal     : NORMAL;
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
	float3 TEX1 : TEXCOORD1;
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(Appdata In,
	uniform float4x4 toscreen         : register(C0),
	uniform float4x4 tolightspace     : register(C4),
	uniform float4   projectioninfo   : register(C8),
	uniform float4   bias             : register(C10),
	uniform float4   lightvector      : register(C12))
{
	Conn Out;
	
	float4 pos = mul(tolightspace, In.position);
	
	float1 depth = pos.y;
	Out.TEX0.x = (pos.x * projectioninfo.z * 0.5) + 0.5;
	Out.TEX0.y = (-pos.z * projectioninfo.z * 0.5) + 0.5;
	Out.TEX0.z = ((depth + projectioninfo.y) / projectioninfo.x) - bias.x;
	Out.TEX0.w = 1.0 - (((depth + projectioninfo.y) / projectioninfo.x) - 0.001);
	
	Out.TEX1 = mul(tolightspace, In.normal);
	Out.TEX1.x = saturate(dot(lightvector, Out.TEX1) * 2.0);
	
	Out.TEX0.w *= Out.TEX1.x;
	
	Out.HPOS = mul(toscreen, In.position);
	Out.HPOS.z -= bias.y;
	
	return Out;
}

