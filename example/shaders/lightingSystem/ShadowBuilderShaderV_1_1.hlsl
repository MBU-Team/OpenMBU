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
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(Appdata In,
	uniform float4x4 lightspace       : register(C0),
	uniform float4   projectioninfo   : register(C4))
{
	Conn Out;
	
	Out.HPOS = mul(lightspace, In.position);
	Out.HPOS.y = Out.HPOS.z;
	Out.HPOS.z = 0.5;
	
	return Out;
}

