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
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Conn main(Appdata In)
{
	Conn Out;
	
	Out.HPOS = In.position;
	Out.TEX0 = In.baseTex;
	
	return Out;
}

