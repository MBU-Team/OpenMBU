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
	float3 TEX1 : TEXCOORD1;
};

struct Fragout
{
	float4 col : COLOR0;
};

float4 sgSampleShadow(sampler2D shadowmap, float4 coord, float4 world)
{
	float4 shadow = tex2D(shadowmap, coord);
	return saturate((world - shadow.x) * 10000);
};

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
Fragout main(v2f IN,
			 uniform sampler2D diffuseMap : register(S0),
			 uniform float4    stride     : register(C0),
			 uniform float4    color      : register(C3))
{
	Fragout OUT;
	
	float4 coord = IN.TEX0;
	float1 str = stride.w;
	
	
	// clamp to lexel...
	float4 intuv = (coord * stride.z);
	intuv = floor(intuv);
	intuv *= str.x;
	
	
	// sample...
	float4 uv = intuv;
	float4 s00 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv.x += str.x;
	float4 s10 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv.x += str.x;
	float4 s20 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv = intuv;
	uv.y += str.x;
	float4 s01 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv.x += str.x;
	float4 s11 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv.x += str.x;
	float4 s21 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv.x = intuv.x;
	uv.y += str.x;
	float4 s02 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv.x += str.x;
	float4 s12 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	
	uv.x += str.x;
	float4 s22 = sgSampleShadow(diffuseMap, uv, IN.TEX0.z);
	

	// filter...	
	float1 s10_11 = s10.x + s11.x;
	float1 s12_11 = s12.x + s11.x;
	
	float4 ss;
	ss.x = s00.x + s01.x + s10_11.x;
	ss.y = s20.x + s21.x + s10_11.x;
	ss.z = s01.x + s02.x + s12_11.x;
	ss.w = s21.x + s22.x + s12_11.x;

	float2 lerpuv;
	lerpuv.x = (coord.x - intuv.x) * stride.z;
	lerpuv.y = (coord.y - intuv.y) * stride.z;
	
	float3 val;
	val.x = lerp(ss.x, ss.y, lerpuv.x);
	val.y = lerp(ss.z, ss.w, lerpuv.x);
	val.z = lerp(val.x, val.y, lerpuv.y);
	

	// final setup...
	float4 shadow = color * val.z * 0.5 * 0.25;
	shadow *= IN.TEX0.w;
	OUT.col = 1.0 - shadow;
	
	OUT.col = saturate(OUT.col);
	OUT.col.w = 1.0 - OUT.col.x;
	
	return OUT;
}
